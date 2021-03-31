#ifndef LLVM_TRANSFORMS_VERTEX_ONE
#define LLVM_TRANSFORMS_VERTEX_ONE

#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {
class GraphPass : public PassInfoMixin<GraphPass>,
                  public InstVisitor<GraphPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

  void visitCallInst(CallInst &callinst);
  void visitAllocaInst(AllocaInst &allInst);
  void visitLoadInst(LoadInst &loadInst);
  void visitStoreInst(StoreInst &storeInst);
  void visitAtomicRMWInst(AtomicRMWInst &inst);
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &inst);
  void visitMemIntrinsic(MemIntrinsic &intr);
};
} // namespace llvm

#endif
