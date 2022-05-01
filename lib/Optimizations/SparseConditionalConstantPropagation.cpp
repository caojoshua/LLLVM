// Sparse Conditional Constant Propagation pass. Performs basic constant folding
// and propagating those constants to uses. Also removes unexecutable basic
// blocks.
//
// LLVM's SCCP will track a lot more information from values. For example, it
// tracks constant ranges and undefined values. The specifics can be found in
// the `ValueLatticeElement` enum.

#include "Optimizations/SparseConditionalConstantPropagation.h"

#include "llvm/IR/IRBuilder.h"

llvm::PreservedAnalyses
SparseConditionalConstantPropagationPass::run(Function &F,
                                              FunctionAnalysisManager &) {
  IRBuilder<> Builder(F.getContext());
  BBWorkList.push_back(&F.getEntryBlock());

  while (!BBWorkList.empty()) {
    BasicBlock *BB = BBWorkList.pop_back_val();
    BBExecutable.insert(BB);

    // Constant folding. Only support binary addition between ints because I'm
    // too lazy for complex constant folding. Consider just using LLVM libraries
    // for constant folding.
    auto I = BB->begin();
    while (I != BB->end()) {
      if (I->isBinaryOp()) {
        Value *left = I->getOperand(0);
        Value *right = I->getOperand(1);
        ConstantInt *leftCI = dyn_cast<ConstantInt>(I->getOperand(0));
        ConstantInt *rightCI = dyn_cast<ConstantInt>(I->getOperand(1));
        if (leftCI && rightCI) {
          switch (I->getOpcode()) {
          case Instruction::Add:
            auto constant =
                Builder.getInt(leftCI->getValue() + rightCI->getValue());
            I->replaceAllUsesWith(constant);
            I = I->eraseFromParent();
            continue;
          }
        }
      }
      ++I;
    }

    // Compute which successor basic blocks to add to the work list. If the
    // current basic block terminator instruction is a branch with a constant
    // conditional, we can determine that only one of two basic blocks are
    // executable.
    BranchInst *terminator = dyn_cast<BranchInst>(BB->getTerminator());
    bool addAllSuccessorsToWorkList = true;
    if (terminator && terminator->isConditional()) {
      ConstantInt *constCond =
          dyn_cast<ConstantInt>(terminator->getCondition());
      if (constCond) {
        addAllSuccessorsToWorkList = false;
        int successor = constCond->getValue().getBoolValue();
        BBWorkList.push_back(terminator->getSuccessor(successor));
        BranchInst::Create(terminator->getSuccessor(successor), BB);
        terminator->eraseFromParent();
      }
    }
    if (addAllSuccessorsToWorkList) {
      for (BasicBlock *successor : successors(BB->getTerminator())) {
        if (!BBExecutable.contains(successor)) {
          BBWorkList.push_back(successor);
        }
      }
    }
  }

  // Delete constant instructions. They have already been replaced by constant
  // values.
  for (auto constant : constants) {
    Instruction *ins = dyn_cast<Instruction>(constant.first);
    if (ins && ins->isSafeToRemove()) {
      ins->eraseFromParent();
    }
  }

  // Delete unused basic blocks. For each successor phi, remove the incoming
  // value from BB. If a phi only has one incoming value, replace all usages of
  // the phi with its one incoming value.
  auto BB = F.begin();
  while (BB != F.end()) {
    if (BBExecutable.contains(&*BB)) {
      ++BB;
    } else {
      for (auto successor : successors(BB->getTerminator())) {
        auto phis = successor->phis();
        auto phi = successor->phis().begin();
        while (phi != phis.end()) {
          phi->removeIncomingValue(phi->getBasicBlockIndex(&*BB));
          if (phi->getNumIncomingValues() == 1) {
            phi->replaceAllUsesWith(phi->getIncomingValue(0));
            auto old_phi = phi;
            // This returns an instruction iterator, which we can't set to
            // `phi`. Instead iterate using a temporary `old_phi`.
            ++phi;
            old_phi->eraseFromParent();
          } else {
            ++phi;
          }
        }
      }
      BB = BB->eraseFromParent();
    }
  }

  return PreservedAnalyses::none();
}
