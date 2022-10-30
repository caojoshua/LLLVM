#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct LoopUnswitchingPass : PassInfoMixin<LoopUnswitchingPass> {

public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
  typedef SmallPtrSet<Value *, 8> ValueSet;

  bool isInvariant(Loop &L, Value *V, ValueSet &loopDefinedValues);
  bool isInvariant(Loop &L, Value *V, ValueSet &loopDefinedValues, DenseMap<Value *, bool> &invariant);
  void unswitchConditionalBranch(Loop &L, BranchInst *br);

  SmallPtrSet<Instruction *, 4> getInvariants(Loop &L);

};
