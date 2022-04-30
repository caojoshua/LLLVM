// Dead code elimination pass. Elimates all instructions that are unused.

#include "optimizations/CommonSubexpressionElimination.h"

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
  if (Ins->isBinaryOp()) {
    Value *LHS = Ins->getOperand(0);
    Value *RHS = Ins->getOperand(1);
    if (Ins->isCommutative() && RHS > LHS) {
      std::swap(LHS, RHS);
    }
    return hash_combine(Ins->getOpcode(), LHS, RHS);
  }
  return (unsigned long)Ins;
}

bool DenseMapInfo<Instruction *>::isEqual(const Instruction *LHS,
                                          const Instruction *RHS) {
  if (isSentinel(LHS) || isSentinel(RHS)) {
    return LHS == RHS;
  }

  return LHS && RHS && LHS->isBinaryOp() && RHS->isBinaryOp() &&
         LHS->getOpcode() == RHS->getOpcode();
}

llvm::PreservedAnalyses
CommonSubexpressionEliminationPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  for (BasicBlock &BB : F) {
    auto I = BB.begin();
    while (I != BB.end()) {
      if (Instruction *foo = commonSubexpressions.lookup(&*I)) {
        I->replaceAllUsesWith(foo);
        I = I->eraseFromParent();
      } else {
        commonSubexpressions.insert(std::make_pair<>(&*I, &*I));
        ++I;
      }
    }
  }

  return PreservedAnalyses::none();
}
