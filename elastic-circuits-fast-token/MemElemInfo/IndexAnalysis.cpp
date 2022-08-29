/*
 * Author: Atri Bhattacharyya
 */

#include "MemElemInfo/IndexAnalysis.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "IDA"

/********** IndexAnalysisPass functions *************/
bool IndexAnalysisPass::runOnFunction(Function& F) {
    DEBUG(dbgs() << "IndexAnalysisPass:" << __func__ << "START\n");
    auto RI = &getAnalysis<RegionInfoPass>().getRegionInfo();
    SI      = getAnalysis<ScopInfoWrapperPass>().getSI();
    LI      = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    std::deque<Region*> RQ;
    getAllRegions(*RI->getTopLevelRegion(), RQ);

    Scop* S;
    for (Region* R : RQ) {
        if ((S = SI->getScop(R)))
            processScop(*S);
    }
    return false;
}

void IndexAnalysisPass::print(llvm::raw_ostream& OS, const Module* M) const {
    OS << "List of memory instructions: \n";
    for (auto itScop : ScopMetas)
        itScop->print(OS);

    OS << "List of possible RAW dependences: \n";
    for (auto it : IA.instRAWlist)
        OS << *it.first << " " << *it.second << "\n";

    OS << "List of possible WAW dependences: \n";
    for (auto it : IA.instWAWlist)
        OS << *it.first << " " << *it.second << "\n";
}

void IndexAnalysisPass::releaseMemory() {
    DEBUG(dbgs() << __func__ << " START\n");
    for (auto it : ScopMetas)
        delete it;
    ScopMetas.clear();
    DEBUG(dbgs() << __func__ << " END\n");
}

void ScopMeta::print(llvm::raw_ostream& OS) {
    for (auto I : MemInsts)
        OS << *I << "\n";
}

void IndexAnalysisPass::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<RegionInfoPass>();
    AU.addRequired<ScopInfoWrapperPass>();
    AU.setPreservesAll();
}

IndexAnalysis& IndexAnalysisPass::getInfo() { return IA; }

void IndexAnalysisPass::getAllRegions(Region& R, std::deque<Region*>& RQ) {
    RQ.push_back(&R);
    for (const auto& E : R)
        getAllRegions(*E, RQ);
}

bool hasMemoryReadOrWrite(ScopStmt& Stmt) {
    bool hasRdWr = false;
    for (auto Inst : Stmt.getInstructions()) {
        hasRdWr |= Inst->mayReadOrWriteMemory();
    }
    return hasRdWr;
}

void IndexAnalysisPass::processScop(Scop& S) {
    DEBUG(dbgs() << "IndexAnalysisPass:" << __func__ << "START\n");

    auto meta = new ScopMeta(S);
    ScopMetas.push_back(meta);

    for (auto& Stmt : S) {
        auto BB = Stmt.getBasicBlock();
        IA.BBlist.insert(BB);
        IA.BBToScopMap[BB] = ScopMetas.size();

        if (!hasMemoryReadOrWrite(Stmt))
            continue;

        meta->addScopStmt(Stmt);
    }

    meta->computeIntersections();
    auto intersectList = meta->getIntersectionList();

    for (auto it : meta->getInstsToBase()) {
        auto I           = it.first;
        auto V           = it.second;
        IA.instToBase[I] = V;
    }

    /* The convention used in ScopMeta class is that the first element
     * in an instPair is a store instruction. Thus, checking the type
     * of the second instruction tells us whther it is a RAW/WAW dependency */
    for (auto pair : intersectList) {
        if (pair.second->mayWriteToMemory())
            IA.instWAWlist.insert(pair);
        else
            IA.instRAWlist.insert(pair);
    }
    DEBUG(dbgs() << "IndexAnalysisPass:" << __func__ << "END\n");
}

/**************  ScopMeta functions **************/

/* Returns the maximum common loop depth between 2 instructions*/
int ScopMeta::getMaxCommonDepth(const Instruction* I0, const Instruction* I1) {
    DEBUG(dbgs() << *I0 << " and " << *I1 << " \n");
    auto BB0   = I0->getParent();
    auto BB1   = I1->getParent();
    int depth0 = LI->getLoopDepth(BB0);
    int depth1 = LI->getLoopDepth(BB1);
    Loop* L0   = LI->getLoopFor(BB0);
    Loop* L1   = LI->getLoopFor(BB1);

    while (depth0 > depth1) {
        L0 = L0->getParentLoop();
        depth0--;
    }
    while (depth1 > depth0) {
        L1 = L1->getParentLoop();
        depth1--;
    }

    /* Keep reducing loop depths until they match,
     * or we reach outside all loops */
    while (L1 != L0 && depth0-- > 0) {
        L0 = L0->getParentLoop();
        L1 = L1->getParentLoop();
    }

    return depth0;
}

isl::map ScopMeta::getMap(const Instruction* Inst, const unsigned int depthToKeep,
                          const bool getFuture) {

    const auto currentMap = InstToCurrentMap[Inst];

    const unsigned int in_dims = currentMap.dim(isl::dim::in);
    assert(in_dims >= depthToKeep);

    isl::map retMap = currentMap.project_out(isl::dim::in, depthToKeep, in_dims - depthToKeep);
    if (getFuture && depthToKeep > 0) {
        retMap = make_future_map(retMap);
    }
    return removeMapMeta(retMap);
}

isl::map ScopMeta::removeMapMeta(isl::map map) {
    auto emptyID = isl::id::alloc(*ctx, "", nullptr);

    map = map.set_tuple_id(isl::dim::in, emptyID);
    map = map.set_tuple_id(isl::dim::out, emptyID);

    return map;
}

isl::map ScopMeta::make_future_map(isl::map Map) {
    isl::map FMap, tmpMap;

    const unsigned int nIns = Map.dim(isl::dim::in);

    /* Add input vars */
    tmpMap = Map.add_dims(isl::dim::in, nIns);
    /* Add future constraints on new input variables */
    for (int i = 1; i <= nIns; i++) {
        isl::map constrMapN = add_future_condition(tmpMap, i);
        if (i == 1)
            FMap = constrMapN;
        else
            FMap = FMap.unite(constrMapN);
    }
    /* Project out old input vars */
    FMap = FMap.project_out(isl::dim::in, 0, nIns);
    assert(FMap.get() != nullptr);
    return FMap;
}

/* Add constraints on the 'n' most significant dimensions */
isl::map ScopMeta::add_future_condition(isl::map Map, int n) {
    int nIns           = Map.dim(isl::dim::in) / 2;
    isl::map constrMap = Map;

    isl_local_space* lsp = isl_local_space_from_space(Map.get_space().release());
    isl::local_space ls  = isl::manage(lsp);

    /* Add equality constraints on the first 'n - 1' dims,
        Inequality on the last dim
    */
    for (int i = 0; i < n; i++) {
        isl::constraint c;
        if (i == n - 1) {
            c = isl::constraint::alloc_inequality(ls);
            c = c.set_constant_si(-1);
        } else
            c = isl::constraint::alloc_equality(ls);
        c         = c.set_coefficient_si(isl::dim::in, i, 1);
        c         = c.set_coefficient_si(isl::dim::in, i + nIns, -1);
        constrMap = constrMap.add_constraint(c);
    }

    return isl::map(constrMap);
}

isl::map ScopMeta::copyMapMeta(isl::map map, isl::map templateMap) {

    isl::id inTupleID  = templateMap.get_tuple_id(isl::dim::in);
    isl::id outTupleID = templateMap.get_tuple_id(isl::dim::out);

    map = map.set_tuple_id(isl::dim::in, inTupleID);
    map = map.set_tuple_id(isl::dim::out, outTupleID);

    return map;
}

ScopMeta::ScopMeta(Scop& S) : S(S), TDI(*S.getLI()) {
    ctx = new isl::ctx(isl_ctx_alloc());
    DT  = S.getDT();
    LI  = S.getLI();

    /* Calculate scopMinDepth based on first scopStmt */
    auto BB      = S.begin()->getBasicBlock();
    auto L       = LI->getLoopFor(BB);
    scopMinDepth = LI->getLoopDepth(BB) - S.getRelativeLoopDepth(L);
    assert(scopMinDepth > 0);

    DEBUG(if (scopMinDepth > 1) dbgs() << "WARNING: may get strange results for "
                                       << "scop inside non-scop loop\n"
                                       << "Will be conservative\n");
}

ScopMeta::~ScopMeta() {
    InstToCurrentMap.clear();
    isl_ctx_free(ctx->release());
}

void ScopMeta::addScopStmt(ScopStmt& Stmt) {
    int depth = LI->getLoopDepth(Stmt.getBasicBlock());

    for (auto Inst : Stmt.getInstructions())
        if (Inst->mayReadOrWriteMemory()) {
            auto& MA = Stmt.getArrayAccessFor(Inst);

            auto currentMapStr  = MA.getLatestAccessRelation().to_str();
            isl::map currentMap = isl::map(*ctx, currentMapStr);

            auto domainStr  = isl::map::from_domain(Stmt.getDomain()).to_str();
            isl::map domain = isl::map(*ctx, domainStr);
            domain          = domain.add_dims(isl::dim::out, currentMap.dim(isl::dim::out));
            domain          = copyMapMeta(domain, currentMap);

            InstToCurrentMap.emplace(Inst, currentMap.intersect(domain));

            InstToLoopDepth[Inst] = depth;
            InstToBase[Inst]      = MA.getOriginalBaseAddr();
            MemInsts.push_back(Inst);
        }
}

void ScopMeta::computeIntersections() {
    DEBUG(dbgs() << "ScopMeta:" << __func__ << "START\n");
    DEBUG(for (auto Inst : MemInsts) dbgs() << *Inst << InstToCurrentMap[Inst].to_str() << "\n");

    /* Checking for RAW and WAW conflicts */
    for (auto WrInst : MemInsts) {
        if (!WrInst->mayWriteToMemory())
            continue;

        // clang-format off
        /* The following describes nodes corresponding to LoadInst and StoreInst 
        * in an elastic circuit.  
        *
        * For ... -> SI -> LI -> ... , SI may affect LI in this and future iterations
        *      ↱---------------↵
        * intersect store-set with current and future load-set 
        * 
        * For ... -> LI -> SI -> ... , SI may affect LI only in future iterations
        *      ↱---------------↵
        * intersect store-set with future load-set
        * 
        * For ... -> SI -> ......     , SI and LI iterations are independent 
        *      ↱  -> LI ->     |
        *      |---------------↵
        * intersect entire store-set with entire load-set 
        * 
        * Foreach write access, compare with relevant sets of read accesses 
        * 
        * Similarly, two stores are checked for possible WAW conflicts
        * */
        // clang-format on

        for (auto Inst : MemInsts) {
            /* Skip checking with self */
            if (Inst == WrInst)
                continue;
            /* No need to check between different arrays */
            if (InstToBase[Inst] != InstToBase[WrInst]) {
                DEBUG(dbgs() << "Skipping:Diff bases " << *Inst << " and " << *WrInst << "\n");
                continue;
            }

            const int CommonDepth = getMaxCommonDepth(Inst, WrInst);

            auto pair    = instPairT(WrInst, Inst);
            auto* RdInst = dyn_cast_or_null<LoadInst>(Inst);

            isl::map InstMap, WrInstMap;

            bool depends = TDI.hasTokenDependence(WrInst, Inst) ||
                           TDI.hasRevTokenDependence(Inst, WrInst) ||
                           TDI.hasControlDependence(WrInst, Inst);

            /* Only WrInst may only depend on Inst if Inst is a load */
            if (RdInst != nullptr && depends) {
                /* Consecutive top-level loops will finish the load before any
                 * store, since there is an operand dependency */
                if (CommonDepth == 0 && scopMinDepth == 1)
                    continue;

                const int depthToKeep = CommonDepth - scopMinDepth + 1;
                if (depthToKeep < 0) {
                    llvm_unreachable("Cannot keep negative depth!");
                }
                InstMap   = getMap(Inst, static_cast<unsigned int>(depthToKeep), true);
                WrInstMap = getMap(WrInst, static_cast<unsigned int>(depthToKeep), false);
            } else {
                /* Generic case: we cannot put any restrictions on the indices
                 * being processed by the instructions, if there are no token
                 * flow that can be established between them. Therefore, we
                 * intersect the sets of all possible indices ever accessed */
                WrInstMap = getMap(WrInst, 0, false);
                InstMap   = getMap(Inst, 0, false);
            }
            DEBUG(dbgs() << "Trying intersection of \n"
                         << *WrInst << " : " << WrInstMap.to_str() << "\nwith \n"
                         << *Inst << " : " << InstMap.to_str() << "\n");

            isl::map intersect = InstMap.intersect(WrInstMap);
            if (intersect.is_empty().is_false()) {
                DEBUG(dbgs() << *WrInst << "\t intersects \t" << *Inst << "\n");
                DEBUG(dbgs() << "Intersection is: " << intersect.to_str() << "\n");
                Intersections.insert(pair);
            }
        }
    }
}

std::set<instPairT>& ScopMeta::getIntersectionList() { return Intersections; }

std::map<const Instruction*, const Value*>& ScopMeta::getInstsToBase() { return InstToBase; }

char IndexAnalysisPass::ID = 0;

static RegisterPass<IndexAnalysisPass> X1("indexA", "Analyze array indices for intersecting sets",
                                          false, true);
