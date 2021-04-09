#include <iostream>
#include <vector>

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include "GraphSan.h"

using namespace llvm;

FunctionCallee create_func, check_func;

Type* int64ty;
Type* charstar;

PreservedAnalyses GraphPass::run(Function &F, FunctionAnalysisManager &M) {
  IRBuilder<> builder(F.getContext());
  visit(F);
  return PreservedAnalyses::all();
};

PreservedAnalyses GraphPass::run(Module &M, ModuleAnalysisManager &MAM) {

  LLVMContext &ctx = M.getContext();
  int64ty = Type::getInt64Ty(ctx);
  charstar = Type::getInt8PtrTy(ctx);
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
    int64ty,
    Type::getInt8PtrTy(ctx),
    int64ty
    );
  
  check_func = M.getOrInsertFunction(
    "dfs$_check_ptr",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx),
    charstar,
    int64ty
    );
  
  visit(M);
  return PreservedAnalyses::none();
};

std::pair<unsigned, Value*> getMetaData(IRBuilder<> &builder, DILocation *loc) {
  unsigned line;
  Value *file;
  if (loc) {
    line = loc->getLine();
    file = builder.CreateGlobalStringPtr(loc->getFilename());
  } else {
    line = 0;
    file = builder.CreateGlobalStringPtr("No debug");
  }
  return std::make_pair(line, file);
}

void GraphPass::visitCallInst(CallInst &callinst) {
  Function *func = callinst.getCalledFunction();
  StringRef name = "none";
  if (func)
    name = func->getName();

  if (name == "malloc") {
    DILocation *loc = callinst.getDebugLoc();
    IRBuilder<> builder(callinst.getNextNode());
    std::vector<Value*> args;
    Value *strptr = builder.CreateGlobalStringPtr("labbbellll");
    auto[line, file] = getMetaData(builder, loc);
    
    args.push_back(strptr);
    args.push_back(&callinst);
    for (auto& arg : callinst.args()) {
      args.push_back(arg.get());
    }
    args.push_back(file);
    args.push_back(ConstantInt::get(int64ty, line));
    builder.CreateCall(create_func, args);
  }
}

void GraphPass::visitAllocaInst(AllocaInst &allInst) {
  outs() << allInst << "\n";
}

void GraphPass::visitLoadInst(LoadInst &loadInst) {
  DILocation *loc = loadInst.getDebugLoc();
  IRBuilder<> builder(loadInst.getNextNode());
  auto[line, file] = getMetaData(builder, loc);
  std::vector<Value*> args = {loadInst.getPointerOperand(), file, ConstantInt::get(int64ty, line)};
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
