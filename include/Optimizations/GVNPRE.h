#include "Analysis/DominatorTreeAnalysis.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// New PM implementation
struct GVNPREPass : PassInfoMixin<GVNPREPass> {

public:
  typedef unsigned ExpressionHash;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  static unsigned numCriticalEdgesRemoved;
  typedef DenseMap<ExpressionHash, Instruction *> LeaderMap;
  typedef DenseMap<BasicBlock *, LeaderMap> ExpressionFlowSet;
  struct ExpressionFlowSets {
    ExpressionFlowSet in;
    ExpressionFlowSet out;
  };

  void removeCriticalEdges(Function &F);

  void computeAvailable(ExpressionFlowSets &available, lllvm::DominatorTree &DT,
                        BasicBlock *domPred);

  ExpressionFlowSets getAnticipated(PostDominatorTree &PDT);

  void insert(lllvm::DominatorTree &DT, ExpressionFlowSets &available,
              ExpressionFlowSets &anticipated);
  bool insert(lllvm::DominatorTree &DT, ExpressionFlowSets &available,
              ExpressionFlowSets &anticipated,
              SmallPtrSet<Instruction *, 8> &determinedRedundant);

  // same phiTranslate that is mentioned `Value-Based Partial Redudancy
  // Elimination` defined as a template to handle both Value* and Instruction*
  template <class T> T *phiTranslate(T *I, BasicBlock *pred, BasicBlock *succ);

  BasicBlock::iterator replaceWithLeader(BasicBlock::iterator,
                                         Instruction *leader);
  void eliminate(Function &F, ExpressionFlowSets &available);
};
