#ifndef LLLVM_TRE
#define LLLVM_TRE

#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

struct TailRecursionEliminationPass
    : PassInfoMixin<TailRecursionEliminationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  void markTailCall(CallInst *I);
  void elimTailCall(CallInst *I);
};

#endif // LLLVM_TRE
