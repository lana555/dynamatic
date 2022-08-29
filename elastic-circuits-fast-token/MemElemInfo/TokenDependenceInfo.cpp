/*
 * Author: Atri Bhattacharyya
 */

#include "MemElemInfo/TokenDependenceInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "TDI"

namespace {

typedef struct Path {
    std::vector<const BasicBlock*> Blocks;
    std::map<const BasicBlock*, std::set<const Value*>> Vals;
} Path;

bool inLoopLatches(const BasicBlock* BB, const std::set<Loop*>& LS) {
    for (auto L : LS)
        if (L->getLoopLatch() == BB) {
            return true;
        }
    return false;
}

bool tokenDepends(const Path& P, const Instruction* I_A, const std::set<Loop*>& LS) {
    int len                           = P.Blocks.size();
    const BasicBlock* CurBB           = P.Blocks.back();
    std::set<const Value*> ActiveVals = P.Vals.at(CurBB);
    std::map<const BasicBlock*, std::set<const Value*>> phiDepends;

    DEBUG(dbgs().indent(len * 4) << CurBB->getName() << "\n");

    /// Determine active values of the current basic block
    /// For each instruction in the current basic block, determine whether it
    /// has been marked as an active dependence in a previous call to
    /// tokenDepends (or if path P was initialized with an active dependence).
    /// when an active dependence is found, add all of its own arguments are
    /// themselves added as active dependences.
    for (auto rit = CurBB->rbegin(); rit != CurBB->rend(); ++rit) {
        auto* I = &*rit;
        if (isa<BranchInst>(I) || isa<DbgInfoIntrinsic>(I))
            continue;

        /* If this instruction is an not active dependence, ignore it */
        if (ActiveVals.find(I) == ActiveVals.end())
            continue;
        /* Else, its operands are active dependences too */
        if (auto PI = dyn_cast<PHINode>(I)) {
            /* For Phi nodes, the active dependent values may be different
                in the different predecessors BB, so we store them in this map
                for now. We add it to the Path.Val set before recursive calls */
            for (const auto& PredBB : PI->blocks()) {
                auto* V = PI->getIncomingValueForBlock(PredBB);
                if (!(isa<Argument>(V) || isa<Constant>(V)))
                    phiDepends[PredBB].insert(V);
            }
        } else {
            for (auto op : I->operand_values())
                if (!(isa<Argument>(op) || isa<Constant>(op)))
                    ActiveVals.insert(op);
        }
    }

    bool depends = true;
    if (I_A->getParent() == CurBB) {
        // If through successive tokenDepends calls we have reached the basic
        // block containing I_A, check whether I_A has been added to the list of
        // active dependences. If so, dependency is met.
        depends = ActiveVals.find(I_A) != ActiveVals.end();
    } else {
        std::vector<const BasicBlock*> validPreds;
        for (auto PredBB : predecessors(CurBB)) {
            /* This depends on having a canonical loop structure. Loops will
             * have a single latch with a single successor: the loop header.
             * Continuing across an edge from a latch to header for any loop in
             * LS is not allowed. */
            if (!(inLoopLatches(PredBB, LS) ||
                  (P.Vals.count(PredBB) && P.Vals.at(PredBB) == ActiveVals))) {
                validPreds.push_back(PredBB);
            }
        }

        // if we have reached a point where active values has been determined,
        // but there are no valid predecessors to produce these values, then
        // dependency is not met.
        depends = validPreds.size() > 0;

        // Else, for each predecessor block, propagate the active values from
        // this basic block (plus any potential active values from phi-nodes
        // with incoming values for the given predecessor block) into a
        // successive call to tokenDepends.
        for (auto PredBB : validPreds) {
            if (!depends)
                break;

            Path predBBPath = P;
            predBBPath.Blocks.push_back(PredBB);
            predBBPath.Vals[PredBB] = ActiveVals;
            auto it                 = phiDepends.find(PredBB);
            if (it != phiDepends.end())
                for (const auto& val : it->second)
                    predBBPath.Vals[PredBB].insert(val);

            depends &= tokenDepends(predBBPath, I_A, LS);
        }
    }

    DEBUG(dbgs().indent(len * 4) << depends << "\n");
    return depends;
}

static bool tokenRevDepends(Path P, const Instruction* I_A, const std::set<Loop*>& LS) {
    const int len            = P.Blocks.size();
    const BasicBlock* CurBB  = P.Blocks.back();
    const BasicBlock* PredBB = (len > 1) ? P.Blocks[len - 2] : nullptr;
    auto ActiveVals          = P.Vals[CurBB];

    DEBUG(dbgs().indent(len * 4) << CurBB->getName() << "\n");
    /// Determine active values of the current basic block
    /// For each instruction in the current basic block, its operands are
    /// checked to see whether they are present in the current set of active
    /// dependences.
    /// If so, the instruction which has the operand is itself reverse
    /// dependent.
    for (const auto& I : *CurBB) {
        if (isa<BranchInst>(&I) || isa<DbgInfoIntrinsic>(&I))
            continue;

        std::vector<const Value*> operands;
        if (auto PI = dyn_cast<PHINode>(&I)) {
            /* For a PHI node, the only relevant operand is decided by the
             * prev BB */
            if (PredBB != nullptr)
                operands.push_back(PI->getIncomingValueForBlock(PredBB));
        } else {
            for (auto op : I.operand_values())
                operands.push_back(op);
        }
        /* If any of the operands has revdep on LI, this value does too */
        for (auto op : operands)
            if (ActiveVals.find(op) != ActiveVals.end())
                ActiveVals.insert(&I);
    }

    bool depends = true;
    if (I_A->getParent() == CurBB) {
        depends = ActiveVals.find(I_A) != ActiveVals.end();
    } else if (inLoopLatches(CurBB, LS)) {
        /* This depends on having a canonical loop structure. Loops will
         * have a single latch with a single successor: the loop header.
         * Continuing across an edge from a latch to header for any loop in
         * LS is not allowed. */
    } else {
        const unsigned numSucc = CurBB->getTerminator()->getNumSuccessors();
        depends                = (numSucc > 0);

        for (auto succBB : successors(CurBB)) {
            if (!depends)
                break;

            /* Skip successor BB if no active values have been added in this
             * call to TokenRevDepends */
            if (std::find(P.Blocks.begin(), P.Blocks.end(), succBB) != P.Blocks.end()) {
                if (P.Vals[succBB] == ActiveVals)
                    continue;
            }

            /* If next BB has not been sufficiently explored, explore again */
            Path succBBPath = P;
            succBBPath.Blocks.push_back(succBB);
            succBBPath.Vals[succBB] = ActiveVals;

            depends &= tokenRevDepends(succBBPath, I_A, LS);
        }
    }
    DEBUG(dbgs().indent(len * 4) << depends << "\n");
    return depends;
}

} // namespace

bool TokenDependenceInfo::hasTokenDependence(const Instruction* I_B, const Instruction* I_A) {

    Path P;
    auto BB = I_B->getParent();
    P.Blocks.push_back(BB);
    P.Vals[BB] = {I_B};

    auto LS = std::set<Loop*>();
    for (Loop* L = LI.getLoopFor(I_B->getParent()); L != nullptr; L = L->getParentLoop())
        LS.insert(L);

    DEBUG(dbgs() << "I_B = " << *I_B << " depends " << *I_A << " ? \n");
    return tokenDepends(P, I_A, LS);
}

bool TokenDependenceInfo::hasRevTokenDependence(const Instruction* I_A, const Instruction* I_B) {
    Path P;
    auto BB = I_A->getParent();
    P.Blocks.push_back(BB);
    P.Vals[BB] = {I_A};

    auto LS = std::set<Loop*>();
    for (Loop* L = LI.getLoopFor(I_B->getParent()); L != nullptr; L = L->getParentLoop())
        LS.insert(L);

    DEBUG(dbgs() << "I_A = " << *I_A << " revdeps " << *I_B << " ? \n");
    return tokenRevDepends(P, I_B, LS);
}

bool TokenDependenceInfo::hasControlDependence(const Instruction* I_A, const Instruction* I_B) {
    return false;
}
