#ifndef LLLVM_CSE
#define LLLVM_CSE

#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

struct CommonSubexpressionEliminationPass : PassInfoMixin<CommonSubexpressionEliminationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);

private:
  DenseMap<Instruction *, Instruction *> commonSubexpressions;
};

#endif // LLLVM_CSE
