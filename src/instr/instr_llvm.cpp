/**
 * @file instr/instr_llvm.cpp
 *
 * @brief LLVM plugin to instrument memory writes
 *
 * This is the LLVM plugin which instruments memory allocations so as to enable
 * transparent rollbacks of application code state.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <instr/instr_llvm.hpp>

extern "C"
{
	#include <log/log.h>
}

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#ifndef ROOTSIM_VERSION
#define ROOTSIM_VERSION "debugging_version"
#endif

#define LLVM_USE_NEW_PASSMANAGER 0

using namespace llvm;

namespace {
	enum instrumentation_stats {
		TRACED_STORE,
		TRACED_MEMSET,
		TRACED_MEMCPY,
		TRACED_CALL,
		TRACED_ATOMIC,
		TRACED_UNKNOWN,
		INSTRUMENTATION_STATS_COUNT
	};

	static bool nullTermArrayContains(const char *const *arr,
					  const char *val, size_t val_len)
	{
		while (*arr) {
			if (!strncmp(*arr, val, val_len) && !(*arr)[val_len])
				return true;
			++arr;
		}
		return false;
	}

	static bool isToSubstitute(StringRef s)
	{
		return nullTermArrayContains(instr_cfg.to_substitute,
					     s.data(), s.size());
	}

	static bool isToIgnore(StringRef s)
	{
		return nullTermArrayContains(instr_cfg.to_ignore,
					     s.data(), s.size());
	}

	static Function *CloneFunctionStub(Function &F, const char *suffix)
	{
		std::string NewFName = F.getName().str() + suffix;
		Function *NewF = Function::Create(
			F.getFunctionType(),
			F.getLinkage(),
			F.getAddressSpace(),
			NewFName,
			F.getParent()
		);
		NewF->copyAttributesFrom(&F);
		return NewF;
	}

	static void CloneFunctionIntoAndMap(Function *NewF, const Function &F,
		ValueToValueMapTy &VMap, const char *suffix)
	{
		Function::arg_iterator DestI = NewF->arg_begin();
		for (const Argument &I : F.args()) {
			DestI->setName(I.getName());
			VMap[&I] = DestI++;
		}

		SmallVector<ReturnInst *, 8> Returns;
#if LLVM_VERSION_MAJOR >= 13
		CloneFunctionInto(NewF, &F, VMap,
				CloneFunctionChangeType::LocalChangesOnly,
				Returns, suffix);
#else
		CloneFunctionInto(NewF, &F, VMap, true, Returns, suffix);
#endif

		 for (const Argument &I : F.args())
			 VMap.erase(&I);
	}

#if LLVM_USE_NEW_PASSMANAGER
class RootsimPass: public PassInfoMixin<RootsimPass> {
	FunctionAnalysisManager *fa_manager = nullptr;
public:
	PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
		fa_manager = &AM.getResult<FunctionAnalysisManagerModuleProxy>(M)
				.getManager();
		ProcessModule(M);
		return PreservedAnalyses::none();
	}
#else
class RootsimPass: public ModulePass {
	TargetLibraryInfoWrapperPass *lib_info_pass = nullptr;
public:
	RootsimPass() : ModulePass(ID) {}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const
	{
		AU.addRequired<TargetLibraryInfoWrapperPass>();
	}

	bool runOnModule(Module &M)
	{
		lib_info_pass = &getAnalysis<TargetLibraryInfoWrapperPass>();
		ProcessModule(M);
		return true;
	}

#endif
private:
	static char ID;
	unsigned stats[INSTRUMENTATION_STATS_COUNT] = {0};

	void ProcessModule(Module &M)
	{
#if LOG_LEVEL <= LOG_DEBUG
		errs() << "Instrumenting module " << raw_ostream::CYAN <<
				M.getName() << "\n";
		errs().resetColor();
#endif

		ValueToValueMapTy VMap;
		std::vector<Function *> F_vec;
		for (Function &F : M) {
			if ((isSystemSide(F) && !isToSubstitute(F.getName()))
					|| isToIgnore(F.getName()))
				continue;

			F_vec.push_back(&F);
		}

		for (Function *F : F_vec) {
			if (isToSubstitute(F->getName()))
				VMap[F] = CloneFunctionStub(*F, instr_cfg.sub_suffix);
			else
				VMap[F] = CloneFunctionStub(*F, instr_cfg.proc_suffix);
		}

#ifdef ROOTSIM_INCREMENTAL
		FunctionCallee wfnc = InitMemtraceFunction(M, "__write_mem");
		std::vector<Instruction *> I_instr_vec;
#endif

		for (Function *F : F_vec) {
			if (F->isDeclaration() || isToSubstitute(F->getName()))
				continue;

			Function *Cloned = cast<Function>(VMap[F]);
			if (Cloned == nullptr)
				continue;
#if LOG_LEVEL <= LOG_DEBUG
			errs() << "Processing " << Cloned->getName() << "()\n";
#endif
			CloneFunctionIntoAndMap(Cloned, *F, VMap,
						instr_cfg.proc_suffix);
#ifdef ROOTSIM_INCREMENTAL
			for (inst_iterator I = inst_begin(Cloned), E = inst_end(Cloned); I != E; ++I)
					if (I->mayWriteToMemory())
						I_instr_vec.push_back(&*I);
#endif
		}

#ifdef ROOTSIM_INCREMENTAL
		for (Instruction *I : I_instr_vec)
			InstrumentWriteInstruction(M, I, wfnc);

		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_STORE] << " stores\n";
		errs().resetColor();
		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_MEMSET] << " memsets\n";
		errs().resetColor();
		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_MEMCPY] << " memcpys\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::CYAN << stats[TRACED_CALL] << " calls\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::RED << stats[TRACED_ATOMIC] << " atomics\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::RED << stats[TRACED_UNKNOWN] << " unknown instructions\n";
		errs().resetColor();
#endif
	}

	bool isSystemSide(Function &F)
	{

		if (F.getIntrinsicID() || F.doesNotReturn())
			return true;

		enum llvm::LibFunc LLF;
#if LLVM_USE_NEW_PASSMANAGER
		return fa_manager->getResult<TargetLibraryAnalysis>(F).getLibFunc(F, LLF);
#elif LLVM_VERSION_MAJOR >= 10
		return lib_info_pass->getTLI(F).getLibFunc(F, LLF);
#else
		return lib_info_pass->getTLI().getLibFunc(F, LLF);
#endif
	}

	FunctionCallee InitMemtraceFunction(Module &M, const char *memtrace_name)
	{
		LLVMContext &ctx = M.getContext();

		Type *MemtraceArgs[] = {
			Type::getInt8PtrTy(ctx),
			IntegerType::get(M.getContext(), sizeof(size_t) * CHAR_BIT)
		};

		FunctionType *Fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			MemtraceArgs,
			false
		);
#if LOG_LEVEL <= LOG_DEBUG
		AttributeList al = AttributeList();
		al = al.addAttribute(ctx, 0, Attribute::NoInline);
		return M.getOrInsertFunction(memtrace_name, Fty, al);
#else
		return M.getOrInsertFunction(memtrace_name, Fty);
#endif
	}

	void InstrumentWriteInstruction(Module &M, Instruction *TI,
					FunctionCallee &memtrace_fnc)
	{
		Value *args[2];

		if (StoreInst *SI = dyn_cast<StoreInst>(TI)) {
			Value *V = SI->getPointerOperand();
			PointerType *pType = cast<PointerType>(V->getType());
			uint64_t storeSize = M.getDataLayout().getTypeStoreSize(
					pType->getElementType());

			args[0] = CastInst::CreatePointerBitCastOrAddrSpaceCast(
				V, memtrace_fnc.getFunctionType()->getParamType(0), "", TI);
			args[1] = ConstantInt::get(IntegerType::get(M.getContext(),
				sizeof(size_t) * CHAR_BIT), storeSize);
			++stats[TRACED_STORE];
		} else if (MemSetInst *MSI = dyn_cast<MemSetInst>(TI)) {
			args[0] = MSI->getRawDest();
			args[1] = MSI->getLength();
			++stats[TRACED_MEMSET];
		} else if (MemCpyInst *MCI = dyn_cast<MemCpyInst>(TI)) {
			args[0] = MCI->getRawDest();
			args[1] = MCI->getLength();
			++stats[TRACED_MEMCPY];
		} else {
			 if (isa<CallBase>(TI)) {
				 ++stats[TRACED_CALL];
			} else if (TI->isAtomic()) {
				errs() << "Encountered an atomic non-store instruction in function "
					<< TI->getParent()->getParent()->getName() << "\n";
				++stats[TRACED_ATOMIC];
			} else {
				errs() << "Encountered an unknown memory writing instruction in function "
					<< TI->getParent()->getParent()->getName() << "\n";
				++stats[TRACED_UNKNOWN];
			}
			return;
		}

		CallInst *c = CallInst::Create(memtrace_fnc, args, "", TI);
		c->setDebugLoc(TI->getDebugLoc());
	}
};
}

#if LLVM_USE_NEW_PASSMANAGER

static void newPassManagerLoadPass(ModulePassManager &MPM,
                PassBuilder::OptimizationLevel Level) {
	MPM.addPass(RootsimPass());
}

void rootsimPluginRegister(PassBuilder &PB)
{
	PB.registerOptimizerLastEPCallback(newPassManagerLoadPass);
}

llvm::PassPluginLibraryInfo rootsimPluginInfoGet(void)
{
	return {LLVM_PLUGIN_API_VERSION, "ROOT-Sim plugin", ROOTSIM_VERSION,
		rootsimPluginRegister};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
	return rootsimPluginInfoGet();
}

#else

char RootsimPass::ID = 0;

static void loadPass(const PassManagerBuilder &Builder,
		llvm::legacy::PassManagerBase &PM)
{
	(void)Builder;
	PM.add(new RootsimPass());
}

static RegisterStandardPasses clangtoolLoader_Ox(
	PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

static RegisterStandardPasses clangtoolLoader_O0(
	PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);

#endif
