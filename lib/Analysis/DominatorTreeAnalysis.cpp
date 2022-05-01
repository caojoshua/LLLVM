// Compute the dominator tree using the 'simple' algorithm from `Advanced
// Compiler Design and Implementation`. We should change this to use the faster,
// more complex algorithm.

#include "Analysis/DominatorTreeAnalysis.h"
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

namespace lllvm {

llvm::AnalysisKey DominatorTreeAnalysis::Key;

void DominatorTreeAnalysis::setUnion(DenseSet<BasicBlock *> &LHS,
                                     const DenseSet<BasicBlock *> &RHS) {
  for (BasicBlock *BB : RHS) {
    LHS.insert(BB);
  }
}

void DominatorTreeAnalysis::setIntersection(DenseSet<BasicBlock *> &LHS,
                                            const DenseSet<BasicBlock *> &RHS) {
  auto BB = LHS.begin();
  while (BB != LHS.end()) {
    if (!RHS.contains(*BB)) {
      auto tmp = BB;
      ++BB;
      LHS.erase(tmp);
    } else {
      ++BB;
    }
  }
}

DenseMap<BasicBlock *, DenseSet<BasicBlock *>>
DominatorTreeAnalysis::getDominators(Function &F) {
  DenseSet<BasicBlock *> BBs;
  DenseSet<BasicBlock *> nonEntryBBs;
  DenseMap<BasicBlock *, DenseSet<BasicBlock *>> dominators;
  BasicBlock *entryBlock = &F.getEntryBlock();

  for (BasicBlock &BB : F) {
    BBs.insert(&BB);
    if (&BB != entryBlock) {
      nonEntryBBs.insert(&BB);
    }
  }

  dominators[entryBlock].insert(entryBlock);
  for (BasicBlock *BB : nonEntryBBs) {
    if (BB != entryBlock) {
      dominators[BB] = DenseSet<BasicBlock *>(BBs);
    }
  }

  while (true) {
    bool change = false;
    for (BasicBlock *BB : nonEntryBBs) {
      DenseSet<BasicBlock *> newDominators = DenseSet<BasicBlock *>(BBs);
      for (BasicBlock *pred : predecessors(BB)) {
        setIntersection(newDominators, dominators[pred]);
      }

      newDominators.insert(BB);
      if (newDominators != dominators[BB]) {
        dominators[BB] = newDominators;
        change = true;
      }
    }

    if (!change) {
      break;
    }
  }

  return dominators;
}

DenseMap<BasicBlock *, DenseSet<BasicBlock *>>
DominatorTreeAnalysis::getImmediateDominators(Function &F,
                                              DominatorsT dominators) {
  DominatorsT immediateDominators;
  for (BasicBlock &BB : F) {
    immediateDominators[&BB] = dominators[&BB];
    immediateDominators[&BB].erase(&BB);
  }

  for (BasicBlock &BB : F) {
    if (&F.getEntryBlock() != &BB) {
      for (BasicBlock *i : immediateDominators[&BB]) {
        for (BasicBlock *j : immediateDominators[&BB]) {
          if (i != j && dominators[i].contains(j)) {
            immediateDominators[&BB].erase(j);
          }
        }
      }
    }
  }

  return immediateDominators;
}

void DominatorTreeAnalysis::addDominatorTreeChildren(
    Function &F, DominatorTree *DT, DominatorsT &immediateDominators) {
  for (BasicBlock &BB : F) {
    if (immediateDominators[&BB].contains(DT->getBasicBlock())) {
      addDominatorTreeChildren(F, DT->add_child(&BB), immediateDominators);
    }
  }
}

DominatorTreeAnalysis::Result
DominatorTreeAnalysis::buildDominatorTree(Function &F,
                                          DominatorsT &immediateDominators) {
  DominatorTree DT(&F.getEntryBlock());
  addDominatorTreeChildren(F, &DT, immediateDominators);
  return DT;
}

DominatorTreeAnalysis::Result
DominatorTreeAnalysis::run(llvm::Function &F,
                           llvm::FunctionAnalysisManager &FAM) {

  auto dominators = getDominators(F);
  auto immediateDominators = getImmediateDominators(F, dominators);
  return buildDominatorTree(F, immediateDominators);
}

PreservedAnalyses DominatorTreePrinterPass::run(Function &F,
                                                FunctionAnalysisManager &FAM) {
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  llvm::outs() << "DominatorTree for function: " << F.getName() << "\n";
  printNode(DT);
  return PreservedAnalyses::all();
}

void DominatorTreePrinterPass::printNode(DominatorTree &DT, int depth) {
  for (int i = 0; i < depth; ++i) {
    llvm::outs() << "  ";
  }
  llvm::outs() << "[" << depth << "] %" << DT.getBasicBlock()->getName()
               << "\n";

  for (DominatorTree child : DT) {
    printNode(child, depth + 1);
  }
}

} // namespace lllvm
