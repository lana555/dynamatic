/*
 * Author: Atri Bhattacharyya
 */

#pragma once

#include "MemElemInfo/TokenDependenceInfo.h"
#include "polly/ScopInfo.h"
#include <stdexcept>

using namespace polly;

typedef std::pair<const Instruction*, const Instruction*> instPairT;
class ScopMeta {
    Scop& S;
    DominatorTree* DT;
    LoopInfo* LI;
    TokenDependenceInfo TDI;

    int scopMinDepth;
    std::vector<const Instruction*> MemInsts;
    std::map<const Instruction*, isl::map> InstToCurrentMap;
    std::map<const Instruction*, int> InstToLoopDepth;
    std::set<instPairT> Intersections;
    std::set<instPairT> nonIntersections;
    std::map<const Instruction*, const Value*> InstToBase;
    /* Each Minimized Scop has a separate context. This ensures that
     * trying to intersect maps for instructions from separate Scops
     * will raise an error */
    isl::ctx* ctx;
    /* Used by the dependsInternal() function */
    std::map<instPairT, bool> dependsCache;
    std::set<instPairT> outstandingDependsQueries;

    int getMaxCommonDepth(const Instruction*, const Instruction*);
    isl::map getMap(const Instruction*, const unsigned int depthToKeep, const bool getFuture);
    /* Functions for modifying isl::map to future forms */
    isl::map make_future_map(isl::map map);
    isl::map add_future_condition(isl::map map, int n);
    isl::map copyMapMeta(isl::map map, isl::map templateMap);
    isl::map removeMapMeta(isl::map);

public:
    ScopMeta(Scop& S);
    ~ScopMeta();
    /* Use addScopStmt() to add all ScopStmt's in a Scop. Then,
     * computeIntersections() and finally getIntersectionList() */
    void addScopStmt(ScopStmt& Stmt);
    void computeIntersections();
    std::set<instPairT>& getIntersectionList();
    std::map<const Instruction*, const Value*>& getInstsToBase();
    void print(llvm::raw_ostream& OS);

    using iterator = std::vector<const Instruction*>::iterator;
    iterator begin() { return MemInsts.begin(); }
    iterator end() { return MemInsts.end(); }
};

class IndexAnalysis {
    friend class IndexAnalysisPass;

public:
    IndexAnalysis() : otherInsts() {}
    ~IndexAnalysis() = default;

    /// Returns all memory instructions in SCoPs which do not require an LSQ
    /// connection
    std::vector<const Instruction*>& getOtherInsts() { return otherInsts; }

    /// Query whether any SCoP contains BB
    bool isInScop(const BasicBlock* BB) { return BBlist.find(BB) != BBlist.end(); }

    /// Returns an integer uniquely identifying the SCoP which contains BB
    int getScopID(const BasicBlock* BB) { return (isInScop(BB)) ? BBToScopMap[BB] : -1; }

    /// Returns a set of instruction pairs which exist in some SCoP and have RAW
    /// dependencies between them
    const std::set<instPairT>& getRAWlist() { return instRAWlist; }

    /// Returns a set of instruction pairs which exist in some SCoP and have
    /// WAW dependencies between them.
    const std::set<instPairT>& getWAWlist() { return instWAWlist; }

    /// Returns the base of the array which instruction I accesses
    const Value* getBase(const Instruction* I) const {
        try {
            return instToBase.at(I);
        } catch (std::out_of_range) {
            return nullptr;
        }
    }

private:
    // std::vector<std::set<Instruction *>> instSets;
    std::vector<const Instruction*> otherInsts;
    std::set<instPairT> instRAWlist;
    std::set<instPairT> instWAWlist;
    std::set<const BasicBlock*> BBlist;
    std::map<const BasicBlock*, int> BBToScopMap;
    std::map<const Instruction*, const Value*> instToBase;
};

class IndexAnalysisPass : public FunctionPass {
public:
    static char ID;
    IndexAnalysisPass() : FunctionPass(ID), IA() {}
    ~IndexAnalysisPass() override = default;

    bool runOnFunction(Function& F) override;
    void print(llvm::raw_ostream& OS, const Module* M) const override;
    void releaseMemory() override;
    void getAnalysisUsage(AnalysisUsage& AU) const override;

    IndexAnalysis& getInfo();

private:
    ScopInfo* SI;
    LoopInfo* LI;
    IndexAnalysis IA;

    std::vector<ScopMeta*> ScopMetas;

    void processScop(Scop& S);
    void findIntersections();
    void getAllRegions(Region& R, std::deque<Region*>& RQ);
};
