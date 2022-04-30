#include "optimizations/CommonSubexpressionElimination.h"
#include "optimizations/DeadCodeElimination.h"
#include "optimizations/SparseConditionalConstantPropagation.h"
#include "llvm/Passes/PassBuilder.h"
#include <llvm/Passes/PassPlugin.h>

using namespace llvm;

PassPluginLibraryInfo getPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "lllvm-cse") {
                    FPM.addPass(CommonSubexpressionEliminationPass());
                    return true;
                  } else if (Name == "lllvm-dce") {
                    FPM.addPass(DeadCodeEliminationPass());
                    return true;
                  } else if (Name == "lllvm-sccp") {
                    FPM.addPass(SparseConditionalConstantPropagationPass());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getPluginInfo();
}
