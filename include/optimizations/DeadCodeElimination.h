#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct DeadCodeEliminationPass : PassInfoMixin<DeadCodeEliminationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);

private:
  DenseSet<Value *> used;
  DenseSet<Value *> worklist;
};
