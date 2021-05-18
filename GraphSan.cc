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

bool debug;

Type* int64ty;
Type* charstar;
const DataLayout *dataLayout;

std::set<AllocaInst*> ais;

PreservedAnalyses GraphPass::run(Function &F, FunctionAnalysisManager &M) {
  IRBuilder<> builder(F.getContext());
  visit(F);
  return PreservedAnalyses::all();
};

PreservedAnalyses GraphPass::run(Module &M, ModuleAnalysisManager &MAM) {
  dataLayout = &M.getDataLayout();
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

  // are we debug?
  debug = M.getNamedMetadata("llvm.dbg.cu") != NULL;
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

void buildLabelCall(Instruction &inst, Value* size, std::string label, DILocation *loc) {
  IRBuilder<> builder(inst.getNextNode());
  std::vector<Value*> args;
  Value *strptr = builder.CreateGlobalStringPtr(label);
  auto[line, file] = getMetaData(builder, loc);
  
  args.push_back(strptr);
  args.push_back(&inst);
  args.push_back(size);
  args.push_back(file);
  args.push_back(ConstantInt::get(int64ty, line));
  builder.CreateCall(create_func, args);
}

void GraphPass::visitCallInst(CallInst &callinst) {
  Function *func = callinst.getCalledFunction();
  StringRef name = "none";
  if (func)
    name = func->getName();

  if (name == "malloc") {
    Value *size;
    for (auto& arg : callinst.args()) {
      size = arg.get();
    }
    buildLabelCall(callinst, size, "heap", callinst.getDebugLoc());
  }
}

void handleAlloca(AllocaInst &allInst, DILocation *loc) {
  Optional<uint64_t> alloc_size = allInst.getAllocationSizeInBits(*dataLayout);
  assert(alloc_size.hasValue());
  size_t size = alloc_size.getValue() / 8;

  // don't care about anything smaller
  if (size > 8) {
    ais.insert(&allInst);
    buildLabelCall(allInst, ConstantInt::get(int64ty, alloc_size.getValue() / 8), "stack", loc);
  }
}

void GraphPass::visitAllocaInst(AllocaInst &allInst) {
  if (!debug) {
    outs() << "No debug\n";
    handleAlloca(allInst, allInst.getDebugLoc());
  }
  
}

void GraphPass::visitDbgDeclareInst(DbgDeclareInst &dbgInst) {
  outs() << "DEBUG\n";
  if ( AllocaInst *AI = dyn_cast<AllocaInst>(dbgInst.getAddress())) {
    outs() << "ALLOCA DEBUG\n";
    handleAlloca(*AI, dbgInst.getDebugLoc());
  }
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
