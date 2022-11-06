// A REALLY basic form of loop strength reduction. Strenght reduction transforms
// instructions with equivalent cheaper instructions. In this implementation, we
// only strength reduce the following:
//
// ```
// for (int i = 0; i < N; ++i) {
//  foo = i * 7;
//  print(foo);
// }
// ```
//
// with:
//
// ```
// int foo = 0;
// for (int i = 0; i < N; ++i) {
//  foo += 7;
//  print(foo);
// }
// ```
//
// This replaces a multiply instruction in the loop with a cheaper addition
// instruction.
//
// Strength reduction is an application of finite differences. We only consider
// the above example because it is simple and common, but there are many more
// cases. For example:
//
// ```
// for (int i = 0; i < N; ++i) {
//  foo = i^2;
// }
// ```
//
// Is a series of powers 0, 1, 4, 9, 16, 25, ..., which has finites differences
// 1, 3, 5, 7, 9... We can strength reduce this with:
//
// ```
// if (N <= 0) {
//  foo = 0;
//  return;
// }
// foo = 1;
// for (int i = 1; i < N; ++i) {
//  foo = foo + 2 * i + 1
// }
// ```
//
// This implementation makes a lot of assumptions i.e. there is a preheader
// block, and that constants of binary instructions are always the right
// operand. We also only consider induction variables that can only be
// incremented one way in the loop, such as if it has two different increments
// based on an if-else.

#include "Optimizations/LoopStrengthReduction.h"

SmallPtrSet<PHINode *, 2>
LoopStrengthReductionPass::getInductionVariables(Loop &L) {
  SmallPtrSet<PHINode *, 2> inductionVariables;

  for (PHINode &phi : L.getHeader()->phis()) {
    // Some induction variables may have multiple incoming values from different
    // blocks in the loop. We do not consider those cases in this
    // implementation.
    if (phi.getNumIncomingValues() == 2 &&
        phi.getIncomingBlock(0) == L.getLoopPreheader() &&
        L.contains(phi.getIncomingBlock(1))) {
      inductionVariables.insert(&phi);
    }
  }

  return inductionVariables;
}

PreservedAnalyses
LoopStrengthReductionPass::run(Loop &L, LoopAnalysisManager &AM,
                               LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  BasicBlock *preheader = L.getLoopPreheader();
  BasicBlock *header = L.getHeader();
  if (!preheader || !header) {
    return PreservedAnalyses::none();
  }

  SmallPtrSet<PHINode *, 2> inductionVariables = getInductionVariables(L);
  for (PHINode *inductionVar : inductionVariables) {
    ConstantInt *initialValue =
        dyn_cast<ConstantInt>(inductionVar->getIncomingValue(0));
    Instruction *incrementIns =
        dyn_cast<Instruction>(inductionVar->getIncomingValue(1));
    BasicBlock *backBlock = inductionVar->getIncomingBlock(
        1); // Block in loop that returns to the header

    if (initialValue && incrementIns &&
        incrementIns->getOpcode() == Instruction::Add &&
        incrementIns->getOperand(0) == inductionVar) {

      if (ConstantInt *increment =
              dyn_cast<ConstantInt>(incrementIns->getOperand(1))) {
        auto user = inductionVar->user_begin();
        while (user != inductionVar->user_end()) {
          // Look for uses of inductionVar that are in the form `inductionVar *
          // C` where `C` is a constant
          Instruction *I = dyn_cast<Instruction>(*user);
          if (I && L.contains(I->getParent()) &&
              I->getOpcode() == Instruction::Mul) {
            if (ConstantInt *multiplier =
                    dyn_cast<ConstantInt>(I->getOperand(1))) {
              // We can strength reduce I. Create a new phi on entry to the loop
              // that will be incremented by the multiplier on each iteration.
              PHINode *newPhi =
                  PHINode::Create(I->getType(), 2, "", inductionVar);
              BinaryOperator *addIns =
                  BinaryOperator::CreateAdd(newPhi, multiplier, "", I);

              newPhi->addIncoming(
                  ConstantInt::get(I->getType(), initialValue->getValue() *
                                                     multiplier->getValue()),
                  preheader);
              newPhi->addIncoming(addIns, backBlock);

              ++user;
              I->replaceAllUsesWith(addIns);
              I->eraseFromParent();
              continue;
            }
          }
          ++user;
        }
      }
    }
  }

  return PreservedAnalyses::none();
}
