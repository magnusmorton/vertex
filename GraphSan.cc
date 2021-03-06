#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

#include "GraphSan.h"

using namespace llvm;

PreservedAnalyses GraphPass::run(Function &F, FunctionAnalysisManager &M) {
  visit(F);
  return PreservedAnalyses::all();
};

PreservedAnalyses GraphPass::run(Module &F, ModuleAnalysisManager &M) {
  visit(F);
  return PreservedAnalyses::none();
};

void GraphPass::visitCallInst(CallInst &callinst) {
  Function *func = callinst.getCalledFunction();
  StringRef name = "none";
  if (func)
    name = func->getName();
  outs() << name << "\n";
}

void GraphPass::visitAllocaInst(AllocaInst &allInst) {
  std::cout << "foooooooooooooooooooooooooooooo"
            << "\n";
  outs() << allInst << "\n";
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GraphPass", "v0.1", [](PassBuilder &PB) {
            std::cout << "gjasdlk;jaskld;fjklsdajkl;fjaslkd;f"
                      << "\n";

            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &PM) { PM.addPass(GraphPass()); });
          }};
}
