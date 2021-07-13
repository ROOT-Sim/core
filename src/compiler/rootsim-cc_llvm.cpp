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
#include <compiler/rootsim-cc_llvm.hpp>

extern "C" {
#include <log/log.h>
}

#include <set>

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/ModuleSlotTracker.h"

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

	bool nullTermArrayContains(const char *const *arr, const char *val, size_t val_len)
	{
		while(*arr) {
			if(!strncmp(*arr, val, val_len) && !(*arr)[val_len]) {
				return true;
			}
			++arr;
		}
		return false;
	}

	bool toReplace(StringRef s)
	{
		return nullTermArrayContains(instr_cfg.to_substitute, s.data(), s.size());
	}

	bool toIgnore(StringRef s)
	{
		return nullTermArrayContains(instr_cfg.to_ignore, s.data(), s.size());
	}

	Function *CloneFunctionStub(Function &F, const char *suffix)
	{
		std::string NewFName = F.getName().str() + suffix;
		Function *NewF = Function::Create(cast<FunctionType>(F.getValueType()), F.getLinkage(),
						  F.getAddressSpace(), NewFName, F.getParent());
		NewF->copyAttributesFrom(&F);
		return NewF;
	}

	void CloneFunctionIntoAndMap(Function *NewF, const Function &F, ValueToValueMapTy &VMap, const char *suffix)
	{
		Function::arg_iterator DestI = NewF->arg_begin();
		for(const Argument &I : F.args()) {
			DestI->setName(I.getName());
			VMap[&I] = DestI++;
		}

		SmallVector<ReturnInst *, 8> Returns;
		CloneFunctionInto(NewF, &F, VMap, true, Returns, suffix);

		for(const Argument &I : F.args()) {
			VMap.erase(&I);
		}
		// XXX: solves a LLVM bug but removes debug info from clones
		NewF->setSubprogram(nullptr);
	}

	class RootsimCC : public ModulePass {
	public:
		RootsimCC() : ModulePass(ID) {}

		bool doInitialization(Module &M) override
		{
			getAnnotatedStructs(&M);
			return false;
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override
		{
			AU.addRequired<TargetLibraryInfoWrapperPass>();
		}

		bool runOnModule(Module &M) override
		{
//#if LOG_LEVEL <= LOG_DEBUG
//			errs() << "Analyzing structs in module " << raw_ostream::CYAN << M.getName() << "\n";
//			errs().resetColor();
//#endif
//			std::vector<StructType *> structs = M.getIdentifiedStructTypes();
//			for(const auto &S: structs) {
//#if LOG_LEVEL <= LOG_DEBUG
//				if(S->hasName()) {
//					errs() << "Identified struct " << S->getName() << "\n";
//				}
//#endif
//
//				for(unsigned i = 0; i < S->getNumElements(); i++) {
//
//				}
//
//			}

#if LOG_LEVEL <= LOG_DEBUG
			errs() << "Instrumenting module " << raw_ostream::CYAN << M.getName() << "\n";
			errs().resetColor();
#endif

			ValueToValueMapTy VMap;
			std::vector<Function *> F_vec;
			for(Function &F : M) {
				if((belongsToRuntime(F) && !toReplace(F.getName())) || toIgnore(F.getName())) {
//#if LOG_LEVEL <= LOG_DEBUG
//					errs() << "Ignoring function " << F.getName() << "\n";
//#endif
				} else {
//#if LOG_LEVEL <= LOG_DEBUG
//					errs() << "Found function " << F.getName() << "\n";
//#endif
					F_vec.push_back(&F);
				}
			}

			for(Function *F : F_vec) {
				if(toReplace(F->getName())) {
					VMap[F] = CloneFunctionStub(*F, instr_cfg.sub_suffix);
				} else {
					VMap[F] = CloneFunctionStub(*F, instr_cfg.proc_suffix);
				}
			}

			for(Function *F : F_vec) {
				if(F->isDeclaration() || toReplace(F->getName())) {
					continue;
				}

				auto *Cloned = cast<Function>(VMap[F]);
				if(Cloned == nullptr) {
					continue;
				}
//#if LOG_LEVEL <= LOG_DEBUG
//				errs() << "Processing " << Cloned->getName() << "\n";
//#endif
				CloneFunctionIntoAndMap(Cloned, *F, VMap, instr_cfg.proc_suffix);

				for(BasicBlock &B : *Cloned) {
					for(Instruction &I : B) {
						// TODO: incremental instrumentation
						(void)I;
					}
				}
			}

			return true;
		}

	private:
		static char ID;
		std::set<StructType *> annotStructs;
		unsigned stats[INSTRUMENTATION_STATS_COUNT] = {0};

		static void printStructBody(StructType *STy) {
			if (STy->getNumElements() == 0) {
				errs() << "{}";
			} else {
				StructType::element_iterator I = STy->element_begin();
				errs() << "{ ";
				TypePrint(*I++);
				for (StructType::element_iterator E = STy->element_end(); I != E; ++I) {
					errs() << ", ";
					TypePrint(*I);
				}

				errs() << " }";
			}
		}

		static void TypePrint(Type *Ty) {
			switch (Ty->getTypeID()) {
				case Type::VoidTyID:      errs() << "void"; return;
				case Type::HalfTyID:      errs() << "half"; return;
				case Type::BFloatTyID:    errs() << "bfloat"; return;
				case Type::FloatTyID:     errs() << "float"; return;
				case Type::DoubleTyID:    errs() << "double"; return;
				case Type::X86_FP80TyID:  errs() << "x86_fp80"; return;
				case Type::FP128TyID:     errs() << "fp128"; return;
				case Type::PPC_FP128TyID: errs() << "ppc_fp128"; return;
				case Type::LabelTyID:     errs() << "label"; return;
				case Type::MetadataTyID:  errs() << "metadata"; return;
				case Type::X86_MMXTyID:   errs() << "x86_mmx"; return;
				case Type::TokenTyID:     errs() << "token"; return;
				case Type::IntegerTyID:
					errs() << 'i' << cast<IntegerType>(Ty)->getBitWidth();
					return;

				case Type::StructTyID: {
					auto *STy = cast<StructType>(Ty);
					return printStructBody(STy);
				}

				case Type::PointerTyID: {
					auto *PTy = cast<PointerType>(Ty);
					TypePrint(PTy->getElementType());
					errs() << '*';
					return;
				}
				default:
					abort();
			}
			llvm_unreachable("Invalid TypeID");
		}

		static void analyzeStructMembers(Value *V)
		{
			const GlobalValue *GV = dyn_cast<GlobalValue>(V);
			ModuleSlotTracker MST(GV->getParent(), false);
			TypePrint(GV->getValueType());
		}

		static void getAnnotatedStructs(Module *M)
		{
			for(Module::global_iterator I = M->global_begin(), E = M->global_end(); I != E; ++I) {
				if(I->getName() == "llvm.global.annotations") {
					auto *CA = dyn_cast<ConstantArray>(I->getOperand(0));
					for(auto OI = CA->op_begin(); OI != CA->op_end(); ++OI) {
						auto *CS = dyn_cast<ConstantStruct>(OI->get());
						auto *value = CS->getOperand(0)->getOperand(0);
//						auto *valueType = CS->getOperand(0)->getOperand(0)->getType();
						auto *AnnotationGL = dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));
						StringRef annotation = dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())->getAsCString();
						if(annotation.compare("reflect") == 0) {
#if LOG_LEVEL <= LOG_DEBUG
							errs() << "Found annotated struct\n";
							analyzeStructMembers(value);
							errs() << "\n";
#endif
						}
					}
				}
			}
		}

		bool belongsToRuntime(Function &F)
		{
			enum llvm::LibFunc LLF;
			return F.getIntrinsicID() || F.doesNotReturn() || getAnalysis<TargetLibraryInfoWrapperPass>()
#if LLVM_VERSION_MAJOR >= 10
				.getTLI(F.getFunction()).getLibFunc(F.getFunction(), LLF);
#else
			.getTLI().getLibFunc(F.getFunction(), LLF);
#endif
		}

		// TODO: this code has been refactored and improved but it is unused
		static FunctionCallee InitMemtraceFunction(Module &M, const char *memtrace_name)
		{
			Type *MemtraceArgs[] = {PointerType::getUnqual(Type::getVoidTy(M.getContext())),
						IntegerType::get(M.getContext(), sizeof(size_t) * CHAR_BIT)};

			FunctionType *Fty = FunctionType::get(Type::getVoidTy(M.getContext()), MemtraceArgs, false);

			return M.getOrInsertFunction(memtrace_name, Fty);
		}

		// TODO: this code has been refactored and improved but it is unused
		void InstrumentWriteInstruction(Module &M, Instruction *TI, FunctionCallee memtrace_fnc)
		{
			if(!TI->mayWriteToMemory()) {
				return;
			}

			Value *args[2];

			if(auto *SI = dyn_cast<StoreInst>(TI)) {
				Value *V = SI->getPointerOperand();
				auto *pointerType = cast<PointerType>(V->getType());
				uint64_t storeSize = M.getDataLayout().getTypeStoreSize(
					pointerType->getPointerElementType());
				args[0] = V;
				args[1] = ConstantInt::get(IntegerType::get(M.getContext(), sizeof(size_t) * CHAR_BIT),
							   storeSize);
				++stats[TRACED_STORE];
			} else if(auto *MSI = dyn_cast<MemSetInst>(TI)) {
				args[0] = MSI->getRawDest();
				args[1] = MSI->getLength();
				++stats[TRACED_MEMSET];
			} else if(auto *MCI = dyn_cast<MemCpyInst>(TI)) {
				args[0] = MCI->getRawDest();
				args[1] = MCI->getLength();
				++stats[TRACED_STORE];
			} else {
				if(isa<CallBase>(TI)) {
					++stats[TRACED_CALL];
				} else if(TI->isAtomic()) {
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

			CallInst::Create(memtrace_fnc, args, "", TI);
		}
	};
}

char RootsimCC::ID = 0;

static void loadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM)
{
	(void) Builder;
	PM.add(new RootsimCC());
}

static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
