#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct CommonSubexpressionEliminationPass : PassInfoMixin<CommonSubexpressionEliminationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);

private:
  DenseMap<Instruction *, Instruction *> commonSubexpressions;
};