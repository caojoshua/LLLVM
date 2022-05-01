#ifndef LLLVM_DOM_TREE_ANALYSIS
#define LLLVM_DOM_TREE_ANALYSIS

#include "Support/DominatorTree.h"
#include "llvm/IR/PassManager.h"

namespace lllvm {

struct DominatorTreeAnalysis
    : public llvm::AnalysisInfoMixin<DominatorTreeAnalysis> {
private:
  // If `dom` is of type `DominatorsT`, then dom[BB] contains a set of
  // basic blocks that dominate BB.
  using DominatorsT = DenseMap<BasicBlock *, DenseSet<BasicBlock *>>;

  static void setUnion(DenseSet<BasicBlock *> &LHS,
                       const DenseSet<BasicBlock *> &RHS);
  static void setIntersection(DenseSet<BasicBlock *> &LHS,
                              const DenseSet<BasicBlock *> &RHS);

  DominatorsT getDominators(Function &F);
  DominatorsT getImmediateDominators(Function &F, DominatorsT dominators);
  void addDominatorTreeChildren(Function &F, DominatorTree *DT, DominatorsT &immediateDominators);

public:
  using Result = DominatorTree;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &);

private:
  Result buildDominatorTree(Function &F, DominatorsT &dominators);

  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<DominatorTreeAnalysis>;
};

struct DominatorTreePrinterPass
    : public llvm::PassInfoMixin<DominatorTreePrinterPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);

private:
  static void printNode(DominatorTree &DT, int depth = 1);
};

} // namespace lllvm

#endif // LLLVM_DOM_TREE_ANALYSIS
