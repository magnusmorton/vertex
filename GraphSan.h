#ifndef LLVM_TRANSFORMS_VERTEX_ONE
#define LLVM_TRANSFORMS_VERTEX_ONE

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"

namespace llvm {
	class GraphPass : public PassInfoMixin<GraphPass> {
	public:
		static char ID;
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
	};
}

#endif
