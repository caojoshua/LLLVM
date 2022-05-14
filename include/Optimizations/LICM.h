#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct LICMPass : PassInfoMixin<LICMPass> {

public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
  static void add_preheader(Loop *L);

  // Holds the values defined in the loop. Values not defined in the loop are
  // loop invariant.
  DenseSet<Value *> loopDefinedValues;

  // Holds the instructions we are interested in checking if they are invariant.
  DenseSet<Instruction *> worklist;

  // Holds invariant values in a hashed data structure.
  SmallPtrSet<Value *, 4> invariantValues;

  // Holds the order in which values are determined invariant. We move loop
  // invariant values in this order, to make sure invariant instructions
  // dominate all its uses, which may also be loop invariant and need to be
  // moved.
  SmallVector<Value *> invariantSequence;
};
