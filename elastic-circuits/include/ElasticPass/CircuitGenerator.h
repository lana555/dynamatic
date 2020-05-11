#pragma once
#include "Head.h"

/*
 * Author: Radhika Ghosal
 */

//------------- Forward Declarations ---------------------//

class OptimizeBitwidth;

//--------------------------------------------------------//

class CircuitGenerator {

public:
    std::vector<ENode*>* enode_dag;
    std::vector<BBNode*>* bbnode_dag;

    // Maps LLVM BasicBlocks to BBNodes, since 1-1 correspondence
    std::unordered_map<BasicBlock*, BBNode*>* BBMap;

    OptimizeBitwidth* OB;
    MemElemInfo& MEI;

    // TODO: fix naming hack; this is quite terrible
    // will be declaring these in the pass source file, ie. `MyCFGPass/MyCFGPass.cpp`
    static int ssa;
    static int fork_id;
    static int phi_id;
    static int buff_id;
    static int branch_id;
    static int store_id;
    static int memcont_id;
    static int start_id;
    static int fifo_id;
    static int idt;
    static int ret_id;
    static int call_id;
    static int switch_id;
    static int select_id;
    static int cst_id;
    static int op_id;
    static int sink_id;
    static int source_id;

    //--------------------------------------------------------//

    CircuitGenerator(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag,
                     OptimizeBitwidth* OB, MemElemInfo& MEI)
        : enode_dag(enode_dag), bbnode_dag(bbnode_dag), OB(OB), MEI(MEI) {

        BBMap = new std::unordered_map<BasicBlock*, BBNode*>;
    }

    // Implementation in `AddComp.cpp`
    void buildDagEnodes(Function& F);
    void InsertNode(ENode* enode, ENode* after);
    void InsertBranch(ENode* enode, ENode* after, BBNode* bbnode);
    void InsertBranchNew(ENode* enode, BBNode* bbnode);
    void addForkAfter(ENode* enode, BBNode* bbnode);
    void addFork();
    void addBranch();
    void formLiveBBnodes();
    void check();
    void fixBuildDagEnodes();
    void addSink();
    void addSource();
    ENode* findPhiC(BBNode* bbnode);
    void addControl();
    void removeRedundantBeforeElastic(std::vector<BBNode*>* bbnode_dag,
                                      std::vector<ENode*>* enode_dag);
    void removeRedundantAfterElastic(std::vector<ENode*>* enode_dag);
    void addBuffersSimple();
    void setMuxes();
    void setBBIds();
    void setFreqs(const std::string& function_name);

    // Implementation in `AddPhi.cpp`
    void addPhi();
    //void makePhiDag(std::vector<BBNode*>* path, ENode* li);
    //ENode* find_phi(BBNode* bbnode, ENode* li);
    //void addPhiLoops(std::vector<BBNode*>* path, ENode* li);
    //void findAllPaths(BBNode* to, BBNode* from, std::vector<BBNode*>* visited,
     //                 std::vector<std::vector<BBNode*>*>* paths, BBNode* currNode);
    //void printPath(std::vector<BBNode*>* path, FILE* f);
    //void printPaths(std::vector<std::vector<BBNode*>*>* paths, FILE* f);
   // void fixLLVMPhiLiveOuts();
    void setPhiLiveOuts();

    // Implementation in `Bitwidth.cpp`
    void setSizes();
    void createSizes(ENode* e, std::string inst_type = "");

    // Implementation in `SanityChecker.cpp`
    void phiSanityCheck(std::vector<ENode*>* enode_dag);
    void forkSanityCheck(std::vector<ENode*>* enode_dag);
    void branchSanityCheck(std::vector<ENode*>* enode_dag);
    void instructionSanityCheck(std::vector<ENode*>* enode_dag);
    void sanityCheckVanilla(std::vector<ENode*>* enode_dag);

    // Implementation in Memory.cpp
    void addMemoryInterfaces(const bool useLSQ);
    void constructLSQNodes(std::map<const Value*, ENode*>& LSQnodes);
    ENode* getOrCreateMCEnode(const Value* base, std::map<const Value*, ENode*>& baseToMCEnode);
    void addMCForEnode(ENode* enode, std::map<const Value*, ENode*>& baseToMCEnode);
    void updateMCConstant(ENode* enode, ENode* mcEnode);

    void setGetelementPtrConsts(std::vector<ENode*>* enode_dag);

    //--------------------------------------------------------//

    // returns the BBnode from bbnode_dag corresponding to the enode
    template <typename T> BBNode* findBB(T* tnode) {
        auto BBMapIt = BBMap->find(tnode->BB);
        if (BBMapIt == BBMap->end()) {
            return NULL;
        } else {
            return BBMapIt->second;
        }
    }

    template <typename T> void resetDFS(std::vector<T*>* node_list) {
        for (auto& node : *node_list) {
            node->counter = 0;
        }
    }

    std::vector<ENode*> getNodesOfType(const node_type t) {
        std::vector<ENode*> set;
        std::copy_if(enode_dag->begin(), enode_dag->end(), std::back_inserter(set),
                     [t](const ENode* node) { return node->type == t; });
        return set;
    }
};
