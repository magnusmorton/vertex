#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <vector>

#include "GraphSan.h"

using namespace llvm;

FunctionCallee create_func, check_func;

PreservedAnalyses GraphPass::run(Function &F, FunctionAnalysisManager &M) {
  IRBuilder<> builder(F.getContext());
  visit(F);
  return PreservedAnalyses::all();
};

PreservedAnalyses GraphPass::run(Module &M, ModuleAnalysisManager &MAM) {

  LLVMContext &ctx = M.getContext();
  /*
    dfsan mangles function symbols on instrumented CUs. Our pass is run after
    this happens. I can't find a way of running it before with the new pass
    manager, so I do it here. 
  */
  create_func = M.getOrInsertFunction(
    "dfs$_create_label",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx),
    Type::getInt8PtrTy(ctx),
    Type::getInt64Ty(ctx)
    );
  
  check_func = M.getOrInsertFunction(
    "dfs$_check_ptr",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx)
    );
  
  visit(M);
  return PreservedAnalyses::none();
};

void GraphPass::visitCallInst(CallInst &callinst) {
  Function *func = callinst.getCalledFunction();
  StringRef name = "none";
  if (func)
    name = func->getName();

  if (name == "malloc") {
    IRBuilder<> builder(callinst.getNextNode());
    std::vector<Value*> args;
    Value *strptr = builder.CreateGlobalStringPtr("labbbellll");
    args.push_back(strptr);
    args.push_back(&callinst);
    for (auto& arg : callinst.args()) {
      args.push_back(arg.get());
    }
    builder.CreateCall(create_func, args);
  }
}

void GraphPass::visitAllocaInst(AllocaInst &allInst) {
  outs() << allInst << "\n";
}

void GraphPass::visitLoadInst(LoadInst &loadInst) {
  IRBuilder<> builder(loadInst.getNextNode());
  std::vector<Value*> args;
  args.push_back(loadInst.getPointerOperand());
  builder.CreateCall(check_func, args);
}

void GraphPass::visitStoreInst(StoreInst &storeInst) {
  // TODO: implement
}

void GraphPass::visitAtomicRMWInst(AtomicRMWInst &inst) {
  // bet this never gets called
  assert(false);
}

void GraphPass::visitAtomicCmpXchgInst(AtomicCmpXchgInst &inst) {
  // bet this never gets called
  assert(false);

}

void GraphPass::visitMemIntrinsic(MemIntrinsic &intr) {
  //not sure what todo here
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GraphPass", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &PM) { PM.addPass(GraphPass()); });
          }};
}
