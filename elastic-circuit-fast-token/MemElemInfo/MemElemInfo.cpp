/*
 * Author: Atri Bhattacharyya
 */

#include "MemElemInfo/MemElemInfo.h"
#include "MemElemInfo/IndexAnalysis.h"
#include "MemElemInfo/MemUtils.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "MEI"

void MemElemInfo::finalize() {
    /* Make list of non-conflicting loop instructions */
    for (auto Inst : loopInstrSet)
        if (instToLSQ.find(Inst) == instToLSQ.end())
            otherInsts.push_back(Inst);

    /* Create mapping from BB to relevant LSQs */
    for (auto it : instToLSQ) {
        auto BB  = it.first->getParent();
        auto LSQ = it.second;

        /* std::map's [] operator will create an empty set
         * if BBtoLSQ doesn't already contain BB */
        auto& set = BBtoLSQs[BB];
        set.insert(LSQ);
    }
}

bool MemElemInfoPass::runOnFunction(Function& F) {
    IA       = &getAnalysis<IndexAnalysisPass>().getInfo();
    AA       = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    /* Process loops according to AA */
    for (auto L : LI) {
        /* Currently, we shall analyze only top-level loops */
        // TODO: Properly handle multi-level loops
        if (L->getLoopDepth() > 1)
            continue;

        processLoop(L);
    }
    for (auto& LM : loopMeta)
        createSets(LM);

    /// Determine whether memory accessing instructions outside any loop must be
    /// connected to an LSQ.
    /// For instructions outside loops, they use LSQ connection if:
    ///     1. LSQ already exists due to other loop instructions
    ///     2. More than one access to the array outside loops
    ///
    std::multimap<const Value*, const Instruction*> instsByBase;
    std::set<const Value*> bases;
    for (auto& BB : F) {
        /* Ignore BBs within loops */
        if (LI.getLoopDepth(&BB) != 0)
            continue;
        for (auto& I : BB) {
            /* Ignore non-memory instructions */
            if (!I.mayReadOrWriteMemory())
                continue;
            // TODO: Hack: Ignore call instructions - How do I handle this?
            if (isa<CallInst>(I))
                continue;

            auto base  = findBase(&I);
            auto LSQit = MEI.baseToLSQ.find(base);

            // If an LSQ has already been emitted for the given address base,
            // add this instruction to the instructions connected to the LSQ
            if (LSQit != MEI.baseToLSQ.end()) {
                LSQit->second->insts.insert(&I);
                MEI.instToLSQ[&I] = LSQit->second;
            } else {
                /* Only accesses to arrays without LSQs already */
                instsByBase.insert({base, &I});
                bases.insert(base);
            }
        }
    }

    /// LSQ emmission
    for (auto base : bases) {
        if (instsByBase.count(base) > 1) {
            auto LSQ   = new LSQset(base);
            auto range = instsByBase.equal_range(base);

            for (auto i = range.first; i != range.second; ++i) {
                auto* I = i->second;
                LSQ->insts.insert(I);
                MEI.instToLSQ[I] = LSQ;
            }
            MEI.LSQList.insert(LSQ);
        }
    }

    MEI.finalize();

    return false;
}

void MemElemInfoPass::print(llvm::raw_ostream& OS, const Module* M) const {
    DEBUG(dbgs() << __func__ << " BEGIN\n");
    OS << "Instructions in some loop: \n";
    for (auto inst : MEI.loopInstrSet)
        OS << *inst << "\n";

    OS << "LSQ sets: \n";
    int count    = 0;
    auto setList = MEI.getLSQList();
    for (auto set : setList) {
        OS.indent(4) << "Set " << count++ << ": " << set << "\n";
        for (auto inst : *set)
            OS.indent(8) << *inst << "\n";
    }

    OS << "BB to LSQ sets : \n";
    for (auto it : MEI.BBtoLSQs) {
        OS << it.first->getName() << "\t{";
        for (auto set : it.second)
            OS << set << ", ";
        OS << "}\n";
    }

    OS << "Other instructions: \n";
    for (auto Inst : MEI.otherInsts)
        OS << *Inst << "\n";
    DEBUG(dbgs() << __func__ << " END\n");
}

void MemElemInfoPass::releaseMemory() {
    DEBUG(dbgs() << __func__ << " BEGIN\n");
    loopMeta.clear();

    /***** Clear structures in MEI *********/
    for (auto set : MEI.LSQList)
        delete set;
    MEI.LSQList.clear();
    MEI.instToLSQ.clear();
    MEI.BBtoLSQs.clear();
    MEI.otherInsts.clear();
    MEI.loopInstrSet.clear();
    /***************************************/

    DEBUG(dbgs() << __func__ << " END\n");
}

void MemElemInfoPass::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<IndexAnalysisPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<AAResultsWrapperPass>();
    AU.setPreservesAll();
}

MemElemInfo& MemElemInfoPass::getInfo() { return MEI; }

void MemElemInfoPass::processLoop(Loop* L) {
    loopMeta.emplace_back();
    auto& LM = loopMeta.back();

    for (auto BB : L->getBlocks()) {
        int scopId    = IA->getScopID(BB);
        bool isInScop = IA->isInScop(BB);

        for (auto& I : *BB) {
            if (!I.mayReadOrWriteMemory())
                continue;
            if (isa<CallInst>(&I))
                continue;

            MEI.loopInstrSet.push_back(&I);

            if (I.mayReadFromMemory())
                LM.rdInsts.insert(&I);
            if (I.mayWriteToMemory())
                LM.wrInsts.insert(&I);

            if (isInScop)
                LM.instToScop[&I] = scopId;
        }
    }
}

void MemElemInfoPass::createSets(struct TLLMeta& LM) {
    std::list<instPairT> intersectList;
    auto rdInstrSet = LM.rdInsts;
    auto wrInstrSet = LM.wrInsts;

    for (auto wrInst : wrInstrSet) {
        /* Find RAW dependencies */
        for (auto rdInst : rdInstrSet) {
            auto pair = instPairT(wrInst, rdInst);

            /* Each base array is emitted as a separate RAM in the design. Two
             * instructions targetting differing base arrays can never depend */
            if (!equalBase(wrInst, rdInst))
                continue;

            /*  If both instructions are in the same scop,
                use the result from IndexAnalysis */
            auto rdIt = LM.instToScop.find(rdInst);
            auto wrIt = LM.instToScop.find(wrInst);
            if (rdIt != LM.instToScop.end() && wrIt != LM.instToScop.end() &&
                rdIt->second == wrIt->second) {
                if (IA->getRAWlist().find(pair) != IA->getRAWlist().end())
                    intersectList.push_back(pair);
                continue;
            }

            /* Otherwise, use results from AA */
            auto* LI = dyn_cast<LoadInst>(rdInst);
            auto* SI = dyn_cast<StoreInst>(wrInst);

            if (LI == nullptr || SI == nullptr) {
                llvm_unreachable("Expecting only Read-Write pairs of "
                                 "instructions when locating RAW dependencies");
            }

            if (AA->alias(MemoryLocation::get(LI), MemoryLocation::get(SI)) != NoAlias)
                intersectList.push_back(pair);
        }
        /* Find WAW dependencies */
        for (auto wrInst1 : wrInstrSet) {
            if (wrInst1 == wrInst)
                continue;

            /* Each base array is emitted as a separate RAM in the design. Two
             * instructions targetting differing base arrays can never depend */
            if (!equalBase(wrInst, wrInst1))
                continue;

            auto pair    = instPairT(wrInst1, wrInst);
            auto pairRev = instPairT(wrInst, wrInst1);
            /*  If both instructions are in the same scop,
                use the result rom IndexAnalysis */
            auto wr1It = LM.instToScop.find(wrInst1);
            auto wrIt  = LM.instToScop.find(wrInst);
            if (wr1It != LM.instToScop.end() && wrIt != LM.instToScop.end() &&
                wr1It->second == wrIt->second) {
                if (IA->getWAWlist().find(pair) != IA->getWAWlist().end())
                    intersectList.push_back(pair);
                else if (IA->getWAWlist().find(pairRev) != IA->getWAWlist().end())
                    intersectList.push_back(pairRev);
                continue;
            }

            /* Otherwise, use results from AA */
            auto* SI  = dyn_cast<StoreInst>(wrInst);
            auto* SI1 = dyn_cast<StoreInst>(wrInst1);

            if (SI == nullptr || SI1 == nullptr) {
                llvm_unreachable("Expecting only Write-Write pairs of "
                                 "instructions when locating WAW dependencies");
            }

            if (AA->alias(MemoryLocation::get(SI), MemoryLocation::get(SI1)) != NoAlias)
                intersectList.push_back(pair);
        }
    }

    /* Create sets from pairs of intersecting accesses such that
     * both instructions of every pair end up in the same set */
    for (auto instPair : intersectList) {
        auto lInst = instPair.first;
        auto rInst = instPair.second;
        DEBUG(dbgs() << __func__ << " Intersecting : " << *lInst << "," << *rInst << " \n");

        /* Find base array */
        const Value* base = findBase(lInst);
        if (!equalBase(lInst, rInst)) {
            llvm_unreachable("Must only emit LSQs for memory accesses "
                             "targeting the same base arrays");
        }

        auto LSQit = MEI.baseToLSQ.find(base);
        LSQset* LSQ;
        if (LSQit == MEI.baseToLSQ.end()) {
            /* Create new LSQ */
            LSQ = new LSQset(base, lInst, rInst);
            MEI.LSQList.insert(LSQ);
            MEI.baseToLSQ[base] = LSQ;
            assert(MEI.LSQList.size() != 0);
        } else {
            /* Add instructions to existing LSQ */
            LSQ = LSQit->second;
            /* Not checking for existence since this allows for a single access
             * only */
            LSQ->insts.insert(lInst);
            LSQ->insts.insert(rInst);
        }
        MEI.instToLSQ[lInst] = MEI.instToLSQ[rInst] = LSQ;
    }
}

bool MemElemInfoPass::equalBase(const Instruction* A, const Instruction* B) const {
    return findBase(A) == findBase(B);
}

char MemElemInfoPass::ID = 0;
static RegisterPass<MemElemInfoPass>
    X("memelem", "Determine type of memory element required for instruction", false, true);
