// Copyright 2021 University of Edinburgh. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// A note on names: When I started this, I spent a long time looking at the
// DataFlow sanitizer, which has things called labels. I never got round to
// removing the word label from this
 
#include <iostream>
#include <vector>

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include "MemPass.h"

using namespace llvm;
// LLVM specific delclarations
FunctionCallee create_func, check_func, store_func, finish_func;

// Flag if we are in debug mode
bool debug;

// LLVM specific delclarations
Type* int64ty;
Type* charstar;
const DataLayout *dataLayout;

// This is the top-level function called when running the pass
PreservedAnalyses MemPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::cout << "mempass running.....\n";
  dataLayout = &M.getDataLayout();
  LLVMContext &ctx = M.getContext();
  int64ty = Type::getInt64Ty(ctx);
  charstar = Type::getInt8PtrTy(ctx);

  finish_func = M.getOrInsertFunction(
    "finish_san",
    Type::getVoidTy(ctx)
    );

  create_func = M.getOrInsertFunction(
    "mark_root",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx),
    Type::getInt8PtrTy(ctx),
    int64ty,
    Type::getInt8PtrTy(ctx),
    int64ty
    );
  
  check_func = M.getOrInsertFunction(
    "check_ptr",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx),
    charstar,
    int64ty
    );

  store_func = M.getOrInsertFunction(
    "handle_store",
    Type::getVoidTy(ctx),
    Type::getInt8PtrTy(ctx),
    Type::getInt8PtrTy(ctx)
    );
  

  // are we debug?
  debug = M.getNamedMetadata("llvm.dbg.cu") != nullptr;

  // I have no idea of what the priority value means, but it doesn't matter
  appendToGlobalDtors(M, cast<Function>(finish_func.getCallee()), 99, nullptr);
  
  visit(M);
  return PreservedAnalyses::none();
};

// Get line number and filename of declaration
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

// Build IR for call to allocation handling function
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

void MemPass::visitCallInst(CallInst &callinst) {
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
  Optional<TypeSize> alloc_size = allInst.getAllocationSizeInBits(*dataLayout);
  assert(alloc_size.hasValue());
  size_t size = alloc_size.getValue() / 8;

  // don't care about anything smaller. Maybe should use a larger number?
  if (size > 8) {
    buildLabelCall(allInst, ConstantInt::get(int64ty, alloc_size.getValue() / 8), "stack", loc);
  }
}

void MemPass::visitAllocaInst(AllocaInst &allInst) {
  errs() << "visiting ALLOCA....\n";
  handleAlloca(allInst, allInst.getDebugLoc());
    
}

void MemPass::visitDbgDeclareInst(DbgDeclareInst &dbgInst) {
  errs() << "visiting DBG DECLARE....\n";
  if ( AllocaInst *AI = dyn_cast<AllocaInst>(dbgInst.getAddress())) {
    handleAlloca(*AI, dbgInst.getDebugLoc());
  }
}

void MemPass::visitLoadInst(LoadInst &loadInst) {
  DILocation *loc = loadInst.getDebugLoc();
  IRBuilder<> builder(loadInst.getNextNode());
  auto[line, file] = getMetaData(builder, loc);
  std::vector<Value*> args = {loadInst.getPointerOperand(), file, ConstantInt::get(int64ty, line)};
  builder.CreateCall(check_func, args);
}

void MemPass::visitStoreInst(StoreInst &storeInst) {
  Value *ptr = storeInst.getPointerOperand();
  Value *source = storeInst.getValueOperand();
  IRBuilder<> builder(storeInst.getNextNode());
  std::vector<Value*> args = {ptr, source};
  builder.CreateCall(store_func, args);
}

void MemPass::visitAtomicRMWInst(AtomicRMWInst &inst) {
  // bet this never gets called
  assert(false);
}

void MemPass::visitAtomicCmpXchgInst(AtomicCmpXchgInst &inst) {
  // bet this never gets called
  assert(false);

}

void MemPass::visitMemIntrinsic(MemIntrinsic &intr) {
  // not sure what to do here
}

// This is where we register the the pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MemPass", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &PM, llvm::PassBuilder::OptimizationLevel opt) { PM.addPass(MemPass()); });
          }};
}
