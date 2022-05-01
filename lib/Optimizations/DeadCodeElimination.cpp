// Dead code elimination pass. Elimates all instructions that are unused.

#include "Optimizations/DeadCodeElimination.h"

#include "llvm/IR/IRBuilder.h"

bool isEssentialInstruction(Instruction &I) {
  // Is this right?
  return I.isTerminator() || I.mayHaveSideEffects();
}

llvm::PreservedAnalyses
DeadCodeEliminationPass::run(Function &F, FunctionAnalysisManager &) {
  // Add all essential instructions to `worklist`
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (I.isTerminator()) {
        worklist.insert(&I);
      }
    }
  }

  // Keep adding the operands of values in the worklist until the worklist is
  // empty
  while (!worklist.empty()) {
    Value *V = *worklist.begin();
    worklist.erase(worklist.begin());
    used.insert(V);

    User *U = dyn_cast<User>(V);
    if (U) {
      for (auto operand = U->op_begin(); operand != U->op_end(); ++operand) {
        worklist.insert(operand->get());
      }
    }
  }

  // Remove unused instructions
  auto BB = F.begin();
  while (BB != F.end()) {
    auto I = BB->begin();
    while (I != BB->end()) {
      if (used.find(&*I) == used.end()) {
        I = I->eraseFromParent();
      } else {
        ++I;
      }
    }
    ++BB;
  }

  return PreservedAnalyses::none();
}
