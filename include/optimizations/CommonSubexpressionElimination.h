#ifndef LLLVM_CSE
#define LLLVM_CSE

#include "support/DominatorTree.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

struct CommonSubexpressionEliminationPass
    : PassInfoMixin<CommonSubexpressionEliminationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  void processBlock(
      lllvm::DominatorTree &DT,
      ScopedHashTable<Instruction *, Instruction *> &commonSubexpressions);
};

#endif // LLLVM_CSE
