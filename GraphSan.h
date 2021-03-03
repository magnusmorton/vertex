#ifndef LLVM_TRANSFORMS_VERTEX_ONE
#define LLVM_TRANSFORMS_VERTEX_ONE

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstVisitor.h"


namespace llvm {
	class GraphPass : public PassInfoMixin<GraphPass>, public InstVisitor<GraphPass> {
	public:
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

		void visitCallInst(CallInst &callinst);
		void visitAllocaInst(AllocaInst &allInst);
	};
}

#endif
