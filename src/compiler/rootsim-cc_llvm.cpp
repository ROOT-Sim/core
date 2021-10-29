/**
 * @file instr/instr_llvm.cpp
 *
 * @brief LLVM compiler plugin support for rootsim-cc
 *
 * This is the header of the LLVM plugin used to manipulate model's code.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <compiler/rootsim-cc_llvm.hpp>

extern "C" {
#include <log/log.h>
#include <lib/config/reflect.h>
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
		// XXX: solves an LLVM bug but removes debug info from clones
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

		bool doFinalization(Module &M) override
		{
			generateStructMetadata();
			return false;
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override
		{
			AU.addRequired<TargetLibraryInfoWrapperPass>();
		}

		bool runOnModule(Module &M) override
		{
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

		// TODO: probably this function might be significantly refactored
		void getMemberMetadata(Type *Ty)
		{
			struct autoconf_type_map member = {nullptr, 0, AUTOCONF_INVALID, nullptr, 0};

			switch(Ty->getTypeID()) {
				case Type::DoubleTyID:
					member.type = AUTOCONF_DOUBLE;
					break;

				case Type::IntegerTyID: {
					unsigned width = cast<IntegerType>(Ty)->getBitWidth();
					if(width == 8) {
						member.type = AUTOCONF_BOOL;
					}
					if(width == 32) {
						member.type = AUTOCONF_UNSIGNED;
					}
					break;
				}

				case Type::PointerTyID: {
					auto *PTy = cast<PointerType>(Ty);
					Type *dereferenced = PTy->getElementType();
					switch(dereferenced->getTypeID()) {
						case Type::IntegerTyID:
							if(cast<IntegerType>(dereferenced)->getBitWidth() == 8) {
								member.type = AUTOCONF_STRING;
							}
							if(cast<IntegerType>(dereferenced)->getBitWidth() == 32) {
								member.type = AUTOCONF_ARRAY_UNSIGNED;
							}
							break;

						case Type::StructTyID: {
							member.type = AUTOCONF_OBJECT;
//							auto *STy = cast<StructType>(Ty); // TODO: use this to fill other members of the metadata struct
							break;
						}

						case Type::DoubleTyID:
							member.type = AUTOCONF_ARRAY_DOUBLE;
							break;

						case Type::PointerTyID: {
							auto *PTy2 = cast<PointerType>(dereferenced);
							Type *dereferenced2 = PTy2->getElementType();

							switch(dereferenced2->getTypeID()) {
								case Type::IntegerTyID:
									if(cast<IntegerType>(dereferenced)->getBitWidth() == 8) {
										member.type = AUTOCONF_ARRAY_STRING;
									}
									break;

								case Type::StructTyID: {
									member.type = AUTOCONF_ARRAY_OBJECT;
//									auto *STy = cast<StructType>(Ty); // TODO: use this to fill other members of the metadata struct
									break;
								}

								default:
									break;
							}
							break;
						}

						default:
							break;
					}
					break;
				}

				case Type::FloatTyID: // TODO: we can easily support also floats
				default:
					break;
			}

			if(member.type == AUTOCONF_INVALID) {
				report_fatal_error("Found an unsupported type while analyzing structs for reflection.",false);
			}

			// TODO: do something with the struct which we just populated
		}

/*
		AUTOCONF_ARRAY_STRING, ///< The corresponding struct member is char **
		AUTOCONF_ARRAY_OBJECT ///< The corresponding struct member is struct **
*/
		void generateStructMetadata(StructType *STy)
		{
			struct autoconf_name_map str = {nullptr /*getStructName(STy)*/, nullptr};

			if (STy->getNumElements() > 0) {
				((Type *)STy)->print(errs());

				StructType::element_iterator I = STy->element_begin();
				getMemberMetadata(*I++);
				for (StructType::element_iterator E = STy->element_end(); I != E; ++I) {
					getMemberMetadata(*I); // TODO: pack into an array!
				}
				// TODO: add final element into array
				// TODO: reference array into str.members
			}
		}

		void generateStructMetadata()
		{
#if LOG_LEVEL <= LOG_DEBUG
			errs() << "Analysing all structs involved in autoconfiguration\n";
#endif

			for(StructType *str: annotStructs) {
#if LOG_LEVEL <= LOG_DEBUG
				errs() << raw_ostream::CYAN << getStructName(str) << "\n";
				errs().resetColor();
#endif

				generateStructMetadata(str);
			}
		}

		void findReferencedStructs(Type *Ty)
		{
			if(Ty->getTypeID() == Type::StructTyID) {
				auto *STy = cast<StructType>(Ty);
				annotStructs.insert(STy);

				// Look for nested structs
				if (STy->getNumElements() > 0) {
					StructType::element_iterator I = STy->element_begin();
					findReferencedStructs(*I++);
					for (StructType::element_iterator E = STy->element_end(); I != E; ++I) {
						findReferencedStructs(*I);
					}
				}
			}

			// Check for pointers to structs
			if(Ty->getTypeID() == Type::PointerTyID) {
				auto *PTy = cast<PointerType>(Ty);
				findReferencedStructs(PTy->getElementType());
			}
		}

		std::string getStructName(StructType *STy)
		{
			std::string name = STy->getName().str();
			std::replace(name.begin(), name.end(), '.', ' ');
			return name;
		}

		void getAnnotatedStructs(Module *M)
		{
			for(Module::global_iterator I = M->global_begin(), E = M->global_end(); I != E; ++I) {
				if(I->getName() == "llvm.global.annotations") {
					auto *CA = dyn_cast<ConstantArray>(I->getOperand(0));
					for(auto OI = CA->op_begin(); OI != CA->op_end(); ++OI) {
						auto *CS = dyn_cast<ConstantStruct>(OI->get());
						auto *value = CS->getOperand(0)->getOperand(0);
						auto *AnnotationGL = dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));
						StringRef annotation = dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())->getAsCString();
						if(annotation.compare("reflect") == 0) {
							const GlobalValue *GV = dyn_cast<GlobalValue>(value);
							Type *Ty = GV->getValueType();
							if(Ty->getTypeID() != Type::StructTyID) {
								report_fatal_error("Found an _autoconf annotation in something not a struct.", false);
							}
							findReferencedStructs(Ty);
#if LOG_LEVEL <= LOG_DEBUG
							auto *STy = cast<StructType>(Ty);
							errs() << "Found annotated struct " << raw_ostream::CYAN << getStructName(STy) << "\n";
							errs().resetColor();
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

		// TODO: this code has been refactored and improved but it is currently unused
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
