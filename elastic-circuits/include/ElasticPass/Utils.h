#pragma once
#include <algorithm>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Nodes.h"

//----------------------------------------------------------------------------

template <typename T> typename std::vector<T*>::iterator removeNode(std::vector<T*>* vec, T* elem) {
    auto pos = std::find(vec->begin(), vec->end(), elem);
    if (pos != vec->end()) {
        return vec->erase(pos);
    }
    return vec->end();
}

template <typename T> bool contains(std::vector<T*>* vec, const T* elem) {
    if (vec == NULL) {
        return false;
    }

    if (std::find(vec->begin(), vec->end(), elem) != vec->end()) {
        return true;
    }
    return false;
}

template <typename T> int indexOf(std::vector<T*>* vec, const T* elem) {
    ptrdiff_t pos = std::find(vec->begin(), vec->end(), elem) - vec->begin();
    if (pos >= ptrdiff_t(vec->size())) {
        return -1;
    } else {
        return pos;
    }
}

template <typename T> std::vector<T*>* cpyList(std::vector<T*>* node_list) {
    std::vector<T*>* cpy = new std::vector<T*>;

    for (auto& node : *node_list) {
        cpy->push_back(node);
    }
    return cpy;
}

char* getNodeDotName(ENode* enode);

void printDotCFG(std::vector<BBNode*>* bbnode_dag, std::string name);
std::string getIntCmpType(ENode* enode);
std::string getFloatCmpType(ENode* enode);

int getOperandIndex(ENode* enode, ENode* enode_succ);
bool skipNodePrint(ENode* enode);
char* getNodeDotType(ENode* enode);
char* getNodeOpType(ENode* enode);
int getInPortSize(ENode* enode, int index);
int getOutPortSize(ENode* enode, int index);

bool containsAll(ENode* pred_node, ENode* enode);
unsigned int getConstantValue(ENode* enode);
bool isLSQport(ENode* enode);

void printDotDFG(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string name,
                 bool full, std::string serial_number);
void printDotNodes(std::vector<ENode*>* enode_dag, bool full);
std::string getNodeDotNameNew(ENode* enode);
std::string getNodeDotTypeNew(ENode* enode);
std::string getNodeDotbbID(ENode* enode);
std::string getNodeDotInputs(ENode* enode);
std::string getNodeDotOutputs(ENode* enode);
void setBBIndex(ENode* enode, BBNode* bbnode);

//----------------------------------------------------------------------------
// Aurelien (OptimizeBitwidth) ; implementation in bitwidth.cpp

// returns the idx st. node == pred.successors[idx]
//Indexing used in sizes_succs array : 
// - idx in [0, CntrlSuccs->size()[                                             => CntrlSuccessor 
// - idx in [CntrlSuccs->size(), CntrlSuccs->size()+JustCntrlSuccs->size()[   => JustCntrlSuccessor
int getSuccIdxInPred(const ENode* node, const ENode* pred);

// returns the idx st. node == succ.predecessors[idx]
//Indexing used in sizes_preds array : 
// - idx in [0, CntrlPreds->size()[                                           => CntrlPredecessor 
// - idx in [CntrlPreds->size(), CntrlPreds->size()+JustCntrlPreds->size()[   => JustCntrlPredecessor
int getPredIdxInSucc(const ENode* node, const ENode* succ);


// returns true if the edge (from, branch) is the edge giving the condition of the branch
//assumes that from is a predecessor of branch and that branch is a Branch_n/Branch_c node
bool isBranchConditionEdge(const ENode* from, const ENode* branch);
// returns true if the edge (from, mux) is the edge giving the condition of the mux
//assumes that from is a predecessor of mux and that mux is a Phi_/Phi_n node with the isMux flag
bool isMuxConditionEdge(const ENode* from, const ENode* mux);
// returns true if the edge (cmerge, to) is the edge carrying the condition of the CMerge
//assumes that to is a successor of cmerge, and that cmerge is a Phi_c node with the isCtrlMg flag
bool isCMergeConditionEdge(const ENode* cmerge, const ENode* to);
