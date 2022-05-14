// Loop invariant code motion pass. Moves computations that are loop invariant
// to the loop preheader.
//
// An instruction in a loop is invariant if for each of its operands, it is:
// 1. a constant
// 2. defined outside of the loop
// 3. loop-invariant

#include "Optimizations/LICM.h"

#include "llvm/IR/IRBuilder.h"
#include <cassert>

// It turns out that somewhere, the code is already passed through the
// LoopSimplify pass and a preheader is added. Not sure how to turn the
// LoopSimplify pass off and instead use this function. Leaving this function
// around in case I want to implement my own LoopSimplify pass.
void LICMPass::add_preheader(Loop *L) {
  if (L->getLoopPreheader()) {
    return;
  }

  BasicBlock *header = L->getHeader();
  assert(header->canSplitPredecessors() &&
         "Cannot split header. Is it ill-formed?");

  SmallPtrSet<BasicBlock *, 4> outsideBlocks;
  for (auto predecessor : predecessors(header)) {
    if (!L->contains(predecessor)) {
      outsideBlocks.insert(predecessor);
    }
  }

  BasicBlock *preheader =
      BasicBlock::Create(header->getContext(), "", header->getParent(), header);

  auto phis = header->phis();
  auto headerPhi = phis.begin();
  while (headerPhi != phis.end()) {
    // We subtract one because the preheader phi will not have any back edges
    // from the loop
    PHINode *preheaderPhi =
        PHINode::Create(headerPhi->getType(),
                        headerPhi->getNumIncomingValues() - 1, "", preheader);
    for (BasicBlock *outsideBlock : outsideBlocks) {
      preheaderPhi->addIncoming(
          headerPhi->getIncomingValueForBlock(outsideBlock), outsideBlock);
      headerPhi->removeIncomingValue(outsideBlock);
    }
    headerPhi->addIncoming(preheaderPhi, preheader);
    ++headerPhi;
  }
}

llvm::PreservedAnalyses LICMPass::run(Loop &L, LoopAnalysisManager &AM,
                                      LoopStandardAnalysisResults &AR,
                                      LPMUpdater &U) {
  // First record all values defined in the loop
  for (auto iter = L.block_begin(); iter != L.block_end(); ++iter) {
    for (Instruction &I : **iter) {
      loopDefinedValues.insert(&I);
      if (!I.mayHaveSideEffects() && !I.isTerminator()) {
        worklist.insert(&I);
      }
    }
  }

  // Mark instructions as invariant. If an instruction is determined
  // invariant, add it's users to the worklist. Iterate until the worklist is
  // empty.
  while (!worklist.empty()) {
    Instruction *I = *worklist.begin();
    worklist.erase(worklist.begin());

    bool isInvariant = true;
    for (auto operandIter = I->op_begin(); operandIter != I->op_end();
         ++operandIter) {
      Constant *constant = dyn_cast<Constant>(*operandIter);
      if (!constant &&
          loopDefinedValues.find(*operandIter) != loopDefinedValues.end() &&
          invariantValues.find(*operandIter) == invariantValues.end()) {
        isInvariant = false;
        break;
      }
    }

    if (isInvariant) {
      invariantValues.insert(I);
      invariantSequence.push_back(I);
      for (auto userIter = I->user_begin(); userIter != I->user_end();
           ++userIter) {
        Instruction *user = dyn_cast<Instruction>(*userIter);
        worklist.insert(user);
      }
    }
  }

  // Move invariant instructions to the preheader
  if (invariantValues.size() > 0) {
    BasicBlock *preheader = L.getLoopPreheader();
    assert(preheader && "Loop header should have a single preheader");

    for (Value *V : invariantSequence) {
      Instruction *I = dyn_cast<Instruction>(V);
      if (I) {
        I->moveBefore(preheader->getTerminator());
      }
    }
  }

  return PreservedAnalyses::none();
}
