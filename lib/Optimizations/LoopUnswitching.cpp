// Loop Unswitching optimisation. If `cond` is loop invariant, this optmization
// transforms:
//
// for (int i = 0; i < N; ++i) {
//  if (cond) {
//    foo();
//  } else {
//    bar();
//  }
// }
//
// into:
// if (cond) {
//  for (int i = 0; i < N; ++i) {
//    foo();
//  }
// } else {
//  for (int i = 0; i < N; ++i) {
//    bar();
//  }
// }
//
// Algorithm overview:
// 1. Discover loop invariant branches
// 2. Move branch instruction and it's dependencies within the loop outside the
// loops
// 3. The loop's preheader points to the unswitch block.
// 4. Clone the loop
// 5. The unswitch block's original predecessors in the original loop
// unconditionally branch to the true block, and those in the cloned loop
// unconditionally branch to the false block.
// 6. (TODO) Reapply loop unswitching to newly created blocks. Can we rely on
// the LoopPassManager to do this? Its what LLVM does.
//
// Caveats:
// 1. Not downsafe. Like loop-invariant code motion, this introduces more
// expensive paths when not entering the loop. We can ensure downsafety by
// transforming the loop into a do-while in a previous pass.
// 2. Increases code size. The transformation duplicates the loop. LLVM only
// applies the transformation if the amount of copying is under some threshold.
// In our implementation, we always apply loop unswitching when possible.
//
// Implementation limitations:
// 1. LLVM considers unswitching without an else:
// for (...) {
//  if (...) {
//    return; // any instruction that exits the loop
//  } else {
//    ...
//  }
// }
// to be `trivial` unswitches. It does not require cloning, since the if can
// just be moved outside the for. We duplicate the loop in this implementaiton.
// 2. We don't need to clone blocks dominated by the unswitched block's true and
// false blocks. The original loop can point to the true block, and the cloned
// block can point to the false block.
// 3. Probably so much more. LLVM does a lot of CFG transformations that are too
// much of a pain to implement.

#include "Optimizations/LoopUnswitching.h"

#include "llvm/Transforms/Utils/Cloning.h"

SmallPtrSet<Instruction *, 4> LoopUnswitchingPass::getInvariants(Loop &L) {
  SmallPtrSet<Instruction *, 4> invariants;

  return invariants;
}

bool LoopUnswitchingPass::isInvariant(Loop &L, Value *V,
                                      ValueSet &loopDefinedValues) {
  DenseMap<Value *, bool> invariant;
  return isInvariant(L, V, loopDefinedValues, invariant);
}

bool LoopUnswitchingPass::isInvariant(Loop &L, Value *V,
                                      ValueSet &loopDefinedValues,
                                      DenseMap<Value *, bool> &invariant) {
  if (invariant.find(V) != invariant.end()) {
    return invariant[V];
  }
  // Set to false initially. If two instructions in a loop depend on each other,
  // we will get an infinite loop since we will keep on checking its
  // dependencies.
  bool isValueInvariant = false;
  invariant[V] = false;

  if (Instruction *I = dyn_cast<Instruction>(V)) {
    isValueInvariant = true;
    for (Value *op : I->operands()) {
      if (!isInvariant(L, op, loopDefinedValues, invariant)) {
        isValueInvariant = false;
        break;
      }
    }
  } else if (dyn_cast<Constant>(V) || !loopDefinedValues.contains(V)) {
    isValueInvariant = true;
  }
  invariant[V] = isValueInvariant;
  return isValueInvariant;
}

void LoopUnswitchingPass::unswitchConditionalBranch(Loop &L, BranchInst *br) {
  assert(br);
  assert(br->isConditional());
  assert(br->getNumSuccessors() == 2);

  BasicBlock *preheader = L.getLoopPreheader();
  if (!preheader) {
    return;
  }

  Function *F = preheader->getParent();
  SmallVector<BasicBlock *, 4> newBlocks;
  newBlocks.reserve(L.getNumBlocks());
  ValueToValueMapTy valueMap;

  BasicBlock *trueBlock = br->getSuccessor(0);
  BasicBlock *falseBlock = br->getSuccessor(1);

  // Split the unswitch block. The branch instruction and all its dependencies
  // will be moved outside of the loop as the `ifElseHeader`. The rest of the
  // basic block will remain in the loop and be cloned. The original version
  // will point to the true block, and the clone will point to the false block
  BasicBlock *unswitchBlock = br->getParent();
  BasicBlock *ifElseHeader =
      unswitchBlock->splitBasicBlock(br, unswitchBlock->getName() + ".split");

  std::function<void(Instruction &)> moveDependencies =
      [&L, &ifElseHeader, &moveDependencies](Instruction &I) {
        auto op = I.op_begin();
        while (op != I.op_end()) {
          Instruction *insOp = dyn_cast<Instruction>(op->get());
          ++op;
          if (insOp && L.contains(insOp)) {
            insOp->moveBefore(&ifElseHeader->front());
            moveDependencies(*insOp);
          }
        }
      };

  // Move all depedencies of the branch instruction within the loop to
  // ifElseHeader. This is safe, because all of the branch instructions
  // dependencies are loop invariant.
  moveDependencies(*br);

  // The basic block that contains the branch instruction we are unswitching
  // will hold the if-else before the loop. We are calling it the
  // `ifElseHeader`.
  // TODO: also need to move if-else phis to somewhere in the loop.
  auto preheaderI = preheader->begin();
  while (preheaderI != preheader->end()) {
    /* for (auto I = preheader->begin(); I != preheader->end(); ++I) { */
    auto next = preheaderI;
    ++next;
    if (PHINode *phi = dyn_cast<PHINode>(&*preheaderI)) {
      preheaderI->moveBefore(&ifElseHeader->front());
    }
    preheaderI = next;
  }

  // Modify preheader's predecessors to point to ifElseHeader
  auto pred = pred_begin(preheader);
  while (pred != pred_end(preheader)) {
    /* for (auto pred : predecessors(preheader)) { */
    auto pred_next = pred;
    ++pred_next;
    if (BranchInst *predBr = dyn_cast<BranchInst>((*pred)->getTerminator())) {
      for (int i = 0; i < predBr->getNumSuccessors(); ++i) {
        if (predBr->getSuccessor(i) == preheader) {
          predBr->setSuccessor(i, ifElseHeader);
        }
      }
    }
    pred = pred_next;
  }

  // First clone the preheader. It's important to keep preheaders for further
  // loop optimizations.
  BasicBlock *preheaderClone =
      CloneBasicBlock(preheader, valueMap, ".clone", F);
  valueMap[preheader] = preheaderClone;
  newBlocks.push_back(preheaderClone);

  // Clone each block in the loop.
  for (BasicBlock *BB : L.blocks()) {
    // TODO: don't need to clone blocks dominated by the if and else blocks. The
    // original loop can point to the original if block, and the cloned false
    // block can point to the original else block. The else blocks will need to
    // point to the right non-dominating successors.
    BasicBlock *clone = CloneBasicBlock(BB, valueMap, ".clone", F);
    valueMap[BB] = clone;
    newBlocks.push_back(clone);
  }

  // the unswitch branch points to preheaders for the original and cloned loops
  br->setSuccessor(0, preheader);
  br->setSuccessor(1, preheaderClone);

  // Original and cloned unswitch blocks point to true and false blocks
  // respectively.
  Instruction *trueOldTerminator = unswitchBlock->getTerminator();
  Instruction *falseOldTerminator =
      dyn_cast<Instruction>(&*valueMap[trueOldTerminator]);
  BranchInst::Create(trueBlock, trueOldTerminator);
  trueOldTerminator->eraseFromParent();
  BranchInst::Create(falseBlock, falseOldTerminator);
  falseOldTerminator->eraseFromParent();

  // remap cloned blocks' values
  for (BasicBlock *BB : newBlocks) {
    for (Instruction &I : *BB) {
      RemapInstruction(&I, valueMap);
    }
  }
}

llvm::PreservedAnalyses
LoopUnswitchingPass::run(Loop &L, LoopAnalysisManager &AM,
                         LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  ValueSet loopDefinedValues;
  SmallVector<BranchInst *> branches;

  for (BasicBlock *BB : L.blocks()) {
    for (Instruction &I : *BB) {
      loopDefinedValues.insert(&I);
      if (BranchInst *br = dyn_cast<BranchInst>(&I)) {
        branches.push_back(br);
      }
    }
  }

  for (BranchInst *branch : branches) {
    if (AR.LI.getLoopFor(branch->getParent()) != &L) {
    }
    // TODO: consider how to handle unconditional branch
    if (branch->isConditional()) {
      if (isInvariant(L, branch, loopDefinedValues)) {
        unswitchConditionalBranch(L, branch);
        break;
      }
    }
  }

  return PreservedAnalyses::none();
}
