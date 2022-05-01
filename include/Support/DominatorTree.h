// This is just a regular tree I guess? C++ stdlib does not have trees.
//
// If parent Node `P` has child node `C`, then `P` dominates `C`.

#ifndef LLLVM_DOMINATOR_TREE_H
#define LLLVM_DOMINATOR_TREE_H

#include "llvm/IR/BasicBlock.h"

using namespace llvm;

namespace lllvm {

class DominatorTree {
private:
  BasicBlock *BB;
  std::vector<DominatorTree> children;

public:
  using iterator = std::vector<DominatorTree>::iterator;

  DominatorTree(llvm::BasicBlock *BB) : BB(BB) {}

  llvm::BasicBlock *getBasicBlock() { return BB; }

  DominatorTree *add_child(BasicBlock *BB) {
    children.push_back(DominatorTree(BB));
    return &children[children.size() - 1];
  }

  unsigned getNumChildren() { return children.size(); }
  iterator begin() { return children.begin(); }
  iterator end() { return children.end(); }
};

} // namespace lllvm

#endif // LLLVM_DOMINATOR_TREE_H
