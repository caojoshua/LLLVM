// Tail recursion elimination pass. Transforms tail recursive function calls
// into for loops by replacing recursive calls with a branch to the beginning of
// the function and reusing the stack. The pass also marks all tail calls.
// Non-recursive tail calls cannot be implemented in LLVM IR, and is delegated
// to the code gen phases. The current implementation has the following
// limitations:
//
// 1. It only detects tail calls that are immediately followed by return
// statements. For example, cases where there are dead statements or a branch
// statement before a return are not detected. LLVM solves this by trying to
// move as many instructions after the call instruction to before it.
// 2. It does not detect calls with pointer types. Tail call elimination cannot
// be done if the call passes in a pointer to a variable on the caller's stack
// frame. LLVM implements logic that checks the uses of alloca's that computes
// whether the stack allocations exit the function. We do not implement this
// logic here right now.
// 3. It only detects tail calls that returns the recursive call value or void.
// There are other cases that are eligible for tail call elimination.
// 4. LLVM implements "accumulating" tail recursion elimination. This transforms
// naive factorial into tail call factorial with an accumulating variable. We do
// not implement this here.

#include "Optimizations/TailRecursionElimination.h"
#include "llvm/IR/IRBuilder.h"

llvm::PreservedAnalyses
TailRecursionEliminationPass::run(Function &F, FunctionAnalysisManager &AM) {
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      CallInst *callInst = dyn_cast<CallInst>(&I);

      // An CallInst is a tail call if it does not execute any computations with
      // side effects after the call.
      if (callInst) {
        markTailCall(callInst);

        if (callInst->isTailCall()) {
          elimTailCall(callInst);
          return PreservedAnalyses::none();
        }
      }
    }
  }
  return PreservedAnalyses::none();
}

void TailRecursionEliminationPass::markTailCall(CallInst *callInst) {
  Instruction *next = callInst->getNextNode();
  ReturnInst *returnInst = dyn_cast<ReturnInst>(next);

  // We can traverse CFG further. For example, if there is a branch that leads
  // directly into a return, or any dead code before a return, the call can
  // still be marked a tail call. LLVM tries to move instructions to before the
  // call.
  if (returnInst) {
    Value *returnValue = returnInst->getOperand(0);

    // We ignore calls with pointer arguments for now. See notes at the top of
    // this file.
    for (auto arg = callInst->arg_begin(); arg != callInst->arg_end(); ++arg) {
      if (arg->get()->getType()->isPointerTy()) {
        return;
      }
    }

    // There are other possible return values that makes the call eligible for
    // tail call elimination, but they are more complicated. See LLVM's
    // TailRecursionElimination.cpp for more info.
    if (returnValue == callInst || returnValue->getType()->isVoidTy()) {
      callInst->setTailCall();
    }
  }
}

void TailRecursionEliminationPass::elimTailCall(CallInst *callInst) {
  Function *F = callInst->getFunction();
  if (callInst->getCaller() != F) {
    return;
  }

  BasicBlock *oldEntry = &F->getEntryBlock();
  BasicBlock *newEntry = BasicBlock::Create(F->getContext(), "", F, oldEntry);

  // Move allocas over to the header
  auto I = oldEntry->begin();
  while (I != oldEntry->end()) {
    AllocaInst *allocaInst = dyn_cast<AllocaInst>(I);
    if (allocaInst) {
      ++I;
      allocaInst->removeFromParent();
      newEntry->getInstList().push_back(allocaInst);
    }
    ++I;
  }
  Instruction *oldEntryFirst = &oldEntry->front();

  BranchInst *entryBranchInst = BranchInst::Create(oldEntry, newEntry);
  BranchInst *tailCallBranchInst = BranchInst::Create(oldEntry, callInst);

  // setup phis in the old entry block
  auto param = F->arg_begin();
  for (auto arg = callInst->arg_begin(); arg != callInst->arg_end(); ++arg) {
    if (arg->get() == param) {
      // no need to create phi if the value is the same
      continue;
    }

    PHINode *phi = PHINode::Create(param->getType(), 2, "", oldEntryFirst);
    param->replaceAllUsesWith(phi);
    phi->addIncoming(param, newEntry);
    phi->addIncoming(arg->get(), tailCallBranchInst->getParent());
    ++param;
  }

  // remove dead instructions after the tail call branch
  Instruction *next = callInst;
  while (next) {
    Instruction *newNext = next->getNextNode();
    next->eraseFromParent();
    next = newNext;
  }
}
