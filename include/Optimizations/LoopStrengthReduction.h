#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

struct LoopStrengthReductionPass : PassInfoMixin<LoopStrengthReductionPass> {

public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
  SmallPtrSet<PHINode *, 2> getInductionVariables(Loop &L);

};
