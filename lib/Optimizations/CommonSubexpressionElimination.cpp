// Common subexpression elimination pass. Replace expressions with duplicate
// available expressions. An expression `E` is available at point `P` if `E` has
// been computed along all possible paths to `P`. In other words, we can compute
// the meet of all predecessor basic block available expressions in the
// dominator tree.
//
// We make use of LLVM's ScopedHashTable, which allows us to easily push and pop
// expressions from a block. We also define DenseMapInfo<Instruction *>, which
// makes ScopedHashTable use our custom hashing functions to check for equality
// of expressions. Our currently hashing logic is very incomplete right now, and
// we currently only support checking binary operator Instructions.

#include "Optimizations/CommonSubexpressionElimination.h"
#include "Analysis/DominatorTreeAnalysis.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/IR/IRBuilder.h"

template <> struct llvm::DenseMapInfo<Instruction *> {
  static inline Instruction *getEmptyKey() {
    return (Instruction *)DenseMapInfo<void *>::getEmptyKey();
  }
  static inline Instruction *getTombstoneKey() {
    return (Instruction *)DenseMapInfo<void *>::getTombstoneKey();
  }

  static unsigned getHashValue(const Instruction *Val);
  static bool isEqual(const Instruction *LHS, const Instruction *RHS);

  static inline bool isSentinel(const Instruction *Val) {
    return Val == getEmptyKey() || Val == getTombstoneKey();
  }
};

unsigned DenseMapInfo<Instruction *>::getHashValue(const Instruction *Ins) {
  // So much more we need to do here!
  if (Ins && Ins->isBinaryOp()) {
    Value *LHS = Ins->getOperand(0);
    Value *RHS = Ins->getOperand(1);
    if (Ins->isCommutative() && RHS > LHS) {
      std::swap(LHS, RHS);
    }
    return hash_combine(Ins->getOpcode(), LHS, RHS);
  }
  return hash_combine(Ins);
}

bool DenseMapInfo<Instruction *>::isEqual(const Instruction *LHS,
                                          const Instruction *RHS) {
  // Without this case, LHS == RHS == nullptr would return false
  if (LHS == RHS) {
    return true;
  }

  if (isSentinel(LHS) || isSentinel(RHS)) {
    return LHS == RHS;
  }

  // So much more we need to do here!
  return LHS && RHS && LHS->isBinaryOp() && RHS->isBinaryOp() &&
         LHS->getOpcode() == RHS->getOpcode();
}

void foo() {
  ScopedHashTable<Instruction *, Instruction *> HT;
  {
    ScopedHashTableScope<Instruction *, Instruction *> Scope1(HT);
    HT.insert(0, 0);
    HT.insert(0, nullptr);
    {
      ScopedHashTableScope<Instruction *, Instruction *> Scope2(HT);
      HT.insert(0, nullptr);
    }
  }
}

void test(ScopedHashTable<Instruction *, Instruction *> &foo) {
  ScopedHashTableScope<Instruction *, Instruction *> scope(foo);
  foo.insert(nullptr, nullptr);
}

llvm::PreservedAnalyses
CommonSubexpressionEliminationPass::run(Function &F,
                                        FunctionAnalysisManager &AM) {
  lllvm::DominatorTree &DT = AM.getResult<lllvm::DominatorTreeAnalysis>(F);
  ScopedHashTable<Instruction *, Instruction *> commonSubexpressions;
  processBlock(DT, commonSubexpressions);
  return PreservedAnalyses::none();
}

void CommonSubexpressionEliminationPass::processBlock(
    lllvm::DominatorTree &DT,
    ScopedHashTable<Instruction *, Instruction *> &commonSubexpressions) {
  ScopedHashTableScope<Instruction *, Instruction *> newScope(
      commonSubexpressions);

  BasicBlock *BB = DT.getBasicBlock();
  auto I = BB->begin();
  while (I != BB->end()) {
    if (Instruction *commonSubexpression = commonSubexpressions.lookup(&*I)) {
      I->replaceAllUsesWith(commonSubexpression);
      I = I->eraseFromParent();
    } else {
      commonSubexpressions.insert(&*I, &*I);
      ++I;
    }
  }

  for (lllvm::DominatorTree child : DT) {
    processBlock(child, commonSubexpressions);
  }
}
