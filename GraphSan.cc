#include <iostream>
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "GraphSan.h"


using namespace llvm;

PreservedAnalyses GraphPass::run(Function &F, FunctionAnalysisManager &M) {
	std::cout << F.getName().str() << "\n";
	return PreservedAnalyses::all();
};


extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "GraphPass", "v0.1",
    [](PassBuilder &PB) {
	std::cout << "gjasdlk;jaskld;fjklsdajkl;fjaslkd;f" << "\n";

	PB.registerVectorizerStartEPCallback(
		[](
			llvm::FunctionPassManager  &PM,
			llvm::PassBuilder::OptimizationLevel Level) {
			PM.addPass(GraphPass());
		});

    }
  };
}
