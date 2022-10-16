// Mostly following Value-Based Partial Redundancy Elimination
// https://www.cs.purdue.edu/homes/hosking/papers/cc04.pdf
//
// This optimization subsumes common subexpression elimination, global value
// numbering, partial redundancy elimination, and loop invariant code motion. We
// explain the intuition behind the algorithm.
//
// 1. Remove critical edges.
// A critical edge is an edge where the predecessor has multiple successors and
// the successor has multiple predecessors. This function removes critical edges
// by inserting a basic block in between the predecessor and successor.
//
// Intuitively, lets say we have:
//      A
//    /   \
//  B       C
//    \   /   \
//      D       E
//
// The edge C -> D is a critical edge. Lets removed edge C -> D and insert a
// BasicBlock `C.5` and edges C -> C.5 and C.5 ->D. Expression X is computed in
// blocks B and D, which means it is partially redundant along path A -> B -> D.
// We would like to place a copy of X in C, but X is not anticipated in C since
// the path A -> C -> E does not compute X. Instead, we copy X in the newly
// created C.5. This does not affect path A -> C -> E, and A -> C -> C.5 -> D
// has the same number of computations.
//
// 2. Compute available expressions.
// The set of available expressions at a program point P is the set of
// expressions that have been computed at P. We compute available expressions
// for each basic block following a single pass along the Dominator Tree. We do
// not perform fixed point iteration. Note that this means that for a Basic
// Block B, if Expression E is computed in B and all of B's non-dominating
// predecessors, E is not considered available on entry to B. This is fine
// because the Elimination step will eliminate the the computation of E in B.
// For every program point, we map an expression to a leader Instruction, which
// is the first computation of that expression along the program path. The
// leader is later used to determine which instruction to replace redundant
// expressions with.
//
// 3. Compute anticipated expressions.
// The set of anticipated expressions at a program point P is the set of
// expressions that is computed along any path from P to program exit. We
// perform post dominator traversal fixed point iteration to compute anticipated
// expression. Intuitively, we only place instructions at program points where
// there are anticipated, otherwise we would introduce a useless computation
// along some program points.
//
// 4. Insertion.
// For each basic block B and anticipated expression E, we copy E into
// each predecessor P that do not have E available on exit. E is added to the
// P's available on exit set. E must be propagated to all basic blocks that are
// dominated by P. Intuitively, we place all expressions in program points where
// there are anticipated, but not available yet. This introduces many duplicated
// expressions that will be handled in the next step.
//
// 5. Elimination.
// For each instruction, if it is not a leader, replace all it's uses with the
// leader. Since we have determined that the leader is available on that program
// point, it is safe to replace and remove the instruction.
//
// TODO: figure out what is the advantage of numbering, rather than just using
// hash_combine. LLVM does a combination of both. It maps each value to a
// number, and then does hash_combine on the numbers. Not sure yet why this is
// benenficial versus our approach of only using hash_combine.

#include "Optimizations/GVNPRE.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/Value.h>
#include <queue>

unsigned GVNPREPass::numCriticalEdgesRemoved = 0;

GVNPREPass::ExpressionHash getHashValue(const Instruction *Ins) {
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

bool isExpression(const Instruction *Ins) {
  // can support more instruction types if we want
  return Ins->isBinaryOp() || Ins->isUnaryOp() || dyn_cast<PHINode>(Ins);
}

void GVNPREPass::removeCriticalEdges(Function &F) {
  for (BasicBlock &BB : F) {
    if (pred_size(&BB) <= 1) {
      continue;
    }
    for (BasicBlock *pred : predecessors(&BB)) {
      if (succ_size(pred) <= 1) {
        continue;
      }

      char name[64];
      snprintf(name, sizeof(name), "gvnpre.inbetween.%d",
               numCriticalEdgesRemoved);
      BasicBlock *inBetween = BasicBlock::Create(F.getContext(), name, &F);
      BranchInst::Create(&BB, inBetween);
      BB.replacePhiUsesWith(pred, inBetween);
      ++numCriticalEdgesRemoved;

      if (BranchInst *terminator =
              dyn_cast<BranchInst>(pred->getTerminator())) {
        for (int i = 0; i < terminator->getNumSuccessors(); ++i) {
          if (terminator->getSuccessor(i) == &BB) {
            terminator->setSuccessor(i, inBetween);
          }
        }
      }
    }
  }
}

void GVNPREPass::computeAvailable(ExpressionFlowSets &available,
                                  lllvm::DominatorTree &DT,
                                  BasicBlock *domPred) {
  BasicBlock *BB = DT.getBasicBlock();
  auto in = &available.in[BB];
  auto out = &available.out[BB];
  for (auto pair : available.out[domPred]) {
    in->insert(pair);
    out->insert(pair);
  }

  for (Instruction &I : *BB) {
    unsigned hash = getHashValue(&I);
    if (out->find(hash) == out->end()) {
      out->insert(std::pair<ExpressionHash, Instruction *>(hash, &I));
    }
  }

  for (lllvm::DominatorTree child : DT) {
    computeAvailable(available, child, BB);
  }
}

template <class T>
T *GVNPREPass::phiTranslate(T *I, BasicBlock *pred, BasicBlock *succ) {
  if (PHINode *phi = dyn_cast<PHINode>(I)) {
    Value *incomingValue = phi->getIncomingValueForBlock(pred);
    if (Instruction *expr = dyn_cast<Instruction>(incomingValue)) {
      return expr;
    }
  }
  return I;
}

// TODO: write own PostDominatorTree
// TODO: only need to return anticipated.in. anticipated.out is only used to
// compute available.in.
GVNPREPass::ExpressionFlowSets
GVNPREPass::getAnticipated(PostDominatorTree &PDT) {
  ExpressionFlowSets anticipated;

  SmallPtrSet<BasicBlock *, 8> visited;
  bool change = true;
  while (change) {
    change = false;
    visited.clear();
    std::queue<BasicBlock *> worklist;
    for (BasicBlock *root : PDT.roots()) {
      worklist.push(root);
    }

    while (!worklist.empty()) {
      BasicBlock *BB = worklist.front();
      worklist.pop();
      visited.insert(BB);

      unsigned succSize = succ_size(BB);
      if (succSize == 1) {
        BasicBlock *succ = BB->getSingleSuccessor();
        for (auto expr : anticipated.out[BB]) {
          Instruction *translatedExpr = phiTranslate(expr.second, BB, succ);
          anticipated.out[BB][getHashValue(translatedExpr)] = translatedExpr;
        }
      } else if (succSize > 1) {
        // anticipated.out is the intersection of successor anticipated.in
        auto succ_iter = succ_begin(BB);
        anticipated.out[BB] =
            DenseMap<ExpressionHash, Instruction *>(anticipated.in[*succ_iter]);
        ++succ_iter;

        while (succ_iter != succ_end(BB)) {
          for (auto expr : anticipated.out[BB]) {
            auto succ_in = &anticipated.in[*succ_iter];
            if (succ_in->find(expr.first) == succ_in->end()) {
              anticipated.out[BB].erase(expr.first);
            }
          }
          ++succ_iter;
        }
      }

      auto in = &anticipated.in[BB];
      for (auto expr : anticipated.out[BB]) {
        in->insert(expr);
      }

      for (Instruction &I : *BB) {
        if (isExpression(&I)) {
          unsigned hash = getHashValue(&I);
          if (in->find(hash) == in->end()) {
            (*in)[hash] = &I;
            change = true;
          }
        }
      }

      // Append children nodes to worklist
      SmallVector<BasicBlock *> descendants;
      PDT.getDescendants(BB, descendants);
      for (BasicBlock *descendant : descendants) {
        if (descendant != BB && !visited.contains(descendant)) {
          worklist.push(descendant);
        }
      }
    }
  }

  return anticipated;
}

// Helper struct to help with insert function
struct PredecessorInstruction {
  BasicBlock *BB;
  Instruction *I;
  bool needsInsertion;
  PredecessorInstruction(BasicBlock *BB, Instruction *I, bool needsInsertion)
      : BB(BB), I(I), needsInsertion(needsInsertion) {}
};

// One thing we do not do that is done in Value-Based Partial Redundancy
// Elimination paper is that for every instruction I inserted in basic block B,
// I is inserted into a set of new leaders. For every Basic Block B2 dominated
// by B, I is added to B2's available.in set.
//
// I don't see why this is necessary. If a block has one predecessor, we don't
// insert any instructions. If a block B has multiple predecessors, then
// instructions might be inserted into the predecessors. However, the
// predecessors will only have B as its successor due to critical edge removal.
// Therefore, it is useless to propagate new leaders to successor, we already
// know the leader is available on entry to its only successor B.
bool GVNPREPass::insert(lllvm::DominatorTree &DT, ExpressionFlowSets &available,
                        ExpressionFlowSets &anticipated,
                        SmallPtrSet<Instruction *, 8> &determinedRedundant) {
  bool change = false;
  BasicBlock *BB = DT.getBasicBlock();

  unsigned num_preds = pred_size(BB);
  if (num_preds > 1) {
    for (auto expr : anticipated.in[BB]) {
      // Keep track of instructions that are already determined to be redundant.
      // This is required to prevent duplicate insertions for the same
      // instruction across different iterations.
      if (determinedRedundant.contains(expr.second)) {
        continue;
      }

      SmallVector<PredecessorInstruction> predecessorInstructions;
      unsigned numInserts = 0;
      for (BasicBlock *pred : predecessors(BB)) {
        Instruction *clone = expr.second->clone();
        for (auto operand = clone->op_begin(); operand != clone->op_end();
             ++operand) {
          operand->set(phiTranslate(operand->get(), pred, BB));
        }
        auto leader = available.out[pred].find(getHashValue(clone));
        if (leader == available.out[pred].end()) {
          predecessorInstructions.push_back(
              PredecessorInstruction(pred, clone, true));
          ++numInserts;
        } else {
          predecessorInstructions.push_back(
              PredecessorInstruction(pred, leader->second, false));
          clone->deleteValue();
        }
      }

      // If expr does not need to be inserted in all predecessors, then it is
      // not fully nor partially redundant, and no insertions are required.
      llvm::outs() << numInserts << "\n";
      if (numInserts < pred_size(BB)) {
        PHINode *phi = PHINode::Create(expr.second->getType(), num_preds, "",
                                       &BB->front());
        determinedRedundant.insert(expr.second);
        change = true;
        for (auto predInstruction : predecessorInstructions) {
          if (predInstruction.needsInsertion) {
            predInstruction.I->insertBefore(
                predInstruction.BB->getTerminator());
            unsigned hash = getHashValue(predInstruction.I);
            available.out[predInstruction.BB][hash] = predInstruction.I;
          }
          phi->addIncoming(predInstruction.I, predInstruction.BB);
          // Although the phi is defined in the block, it is safe to assume its
          // available in. It is required to make the phi a leader for the
          // elimination step.
          available.in[BB][expr.first] = phi;
          available.out[BB][expr.first] = phi;
        }
      } else {
        for (auto predInstruction : predecessorInstructions) {
          predInstruction.I->deleteValue();
        }
      }
    }
  }

  for (lllvm::DominatorTree child : DT) {
    if (insert(child, available, anticipated, determinedRedundant)) {
      change = true;
    }
  }
  return change;
}

// TODO: Need to understand why multiple passes at insert are needed. The
// example in the paper does not ready easy. Should add a testcase for this.
void GVNPREPass::insert(lllvm::DominatorTree &DT, ExpressionFlowSets &available,
                        ExpressionFlowSets &anticipated) {
  SmallPtrSet<Instruction *, 8> determinedRedundant;
  while (insert(DT, available, anticipated, determinedRedundant)) {
  }
}

BasicBlock::iterator GVNPREPass::replaceWithLeader(BasicBlock::iterator I,
                                                   Instruction *leader) {
  BasicBlock::iterator tmp = I;
  ++tmp;
  I->replaceAllUsesWith(leader);
  I->eraseFromParent();
  return tmp;
}

void GVNPREPass::eliminate(Function &F, ExpressionFlowSets &available) {
  for (BasicBlock &BB : F) {
    // We eliminate local expressions here. Without this, we only eliminate
    // instructions that are vailable on entry to the basic block.
    DenseMap<ExpressionHash, Instruction *> localExpressions;

    LeaderMap in = available.in[&BB];
    auto I = BB.begin();
    while (I != BB.end()) {
      unsigned hash = getHashValue(&*I);
      if (isExpression(&*I) && in.find(hash) != in.end()) {
        // There is a leader in a predecessor. We can eliminate this
        // instruction.
        I = replaceWithLeader(I, in[hash]);
      } else if (localExpressions.find(hash) != localExpressions.end()) {
        I = replaceWithLeader(I, localExpressions[hash]);
      } else {
        localExpressions[hash] = &*I;
        ++I;
      }
    }
  }
}

llvm::PreservedAnalyses GVNPREPass::run(Function &F,
                                        FunctionAnalysisManager &AM) {
  removeCriticalEdges(F);

  lllvm::DominatorTree &DT = AM.getResult<lllvm::DominatorTreeAnalysis>(F);
  PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);

  ExpressionFlowSets available;
  computeAvailable(available, DT, nullptr);

  ExpressionFlowSets anticipatable = getAnticipated(PDT);

  insert(DT, available, anticipatable);
  /* llvm::outs() << "\n******after insert******\n\n"; */
  /* F.print(llvm::outs()); */
  eliminate(F, available);
  /* llvm::outs() << "\n******after eliminate******\n\n"; */
  /* F.print(llvm::outs()); */

  return PreservedAnalyses::none();
}
