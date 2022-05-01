#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct SparseConditionalConstantPropagationPass
    : PassInfoMixin<SparseConditionalConstantPropagationPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);

private:
  DenseMap<Value *, Constant *> constants;
  SmallPtrSet<BasicBlock *, 8> BBExecutable;
  SmallVector<BasicBlock *, 64> BBWorkList;
};
