#include "Analysis/DominatorTreeAnalysis.h"
#include "Optimizations/CommonSubexpressionElimination.h"
#include "Optimizations/DeadCodeElimination.h"
#include "Optimizations/GVNPRE.h"
#include "Optimizations/LICM.h"
#include "Optimizations/SparseConditionalConstantPropagation.h"
#include "Optimizations/TailRecursionElimination.h"
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
                  } else if (Name == "lllvm-gvnpre") {
                    FPM.addPass(GVNPREPass());
                    return true;
                  } else if (Name == "lllvm-sccp") {
                    FPM.addPass(SparseConditionalConstantPropagationPass());
                    return true;
                  } else if (Name == "lllvm-tre") {
                    FPM.addPass(TailRecursionEliminationPass());
                    return true;
                  } else if (Name == "lllvm-print-dom-tree") {
                    FPM.addPass(lllvm::DominatorTreePrinterPass());
                    return true;
                  }
                  return false;
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "lllvm-licm") {
                    LPM.addPass(LICMPass());
                    return true;
                  }
                  return false;
                });
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass(
                      [&] { return lllvm::DominatorTreeAnalysis(); });
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
