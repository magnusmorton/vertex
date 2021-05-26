#ifndef LLVM_TRANSFORMS_VERTEX_ONE
#define LLVM_TRANSFORMS_VERTEX_ONE

#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {
class MemPass : public PassInfoMixin<MemPass>,
                  public InstVisitor<MemPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

  void visitCallInst(CallInst &callinst);
  void visitAllocaInst(AllocaInst &allInst);
  void visitDbgDeclareInst(DbgDeclareInst &dinst);
  void visitLoadInst(LoadInst &loadInst);
  void visitStoreInst(StoreInst &storeInst);
  void visitAtomicRMWInst(AtomicRMWInst &inst);
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &inst);
  void visitMemIntrinsic(MemIntrinsic &intr);
};
} // namespace llvm

#endif
