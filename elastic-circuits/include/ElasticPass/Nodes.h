#pragma once
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

//-------------------------------------------------------//

using namespace llvm;

enum node_type {

    Inst_     = 0,  // instruction node
    Argument_ = 2,  // argument node
    Phi_      = 3,  // original LLVM Phi
    Branch_   = 4,  // branch condition
    Buffera_  = 5,  // argument buffer
    Bufferi_  = 6,  // instruction buffer
    Phi_n     = 7,  // added phi node
    Fork_     = 8,  // fork node
    Branch_n  = 9,  // branch node
    Branch_c  = 10, // control branch node
    Phi_c     = 11, // control phi node
    LSQ_      = 12, // memory controller
    Cst_      = 13, // node for constant inputs
    Start_    = 14,
    End_      = 15,
    Fork_c    = 16,
    Fifoa_    = 17, // argument buffer
    Fifoi_    = 18, // instruction buffer
    Sink_   = 19,
    Source_ = 20,
    MC_     = 21, 
    dummy_ =22
};

class BBNode; 

class ENode {

public:
    int capacity;
    char* compType               = nullptr;
    ENode* branchFalseOutputSucc = nullptr;
    ENode* branchTrueOutputSucc  = nullptr;
    std::vector<ENode*>* parameterNodesReferences[3];
    int cstValue   = 0;       // Leonardo, inserted line, used only with Cst_ type of nodes
    BBNode* bbNode = nullptr; // Leonardo, inserted line
    node_type type;
    std::string Name;
    Instruction* Instr = nullptr;
    ENode* Mem         = nullptr; // For memory instr, store relevant LSQ/MC enode
    BasicBlock* BB     = nullptr;
    Argument* A        = nullptr;
    ConstantInt* CI    = nullptr;
    ConstantFP* CF     = nullptr;
    std::vector<ENode*>* CntrlPreds;     // naming isnt very good, this is a normal predecessor that
                                         // carries both data and control signals
    std::vector<ENode*>* CntrlSuccs;     // naming isnt very good, this is a normal successor that
                                         // carries both data and control signals
    std::vector<ENode*>* JustCntrlPreds; // predecessors that only carry control signals
    std::vector<ENode*>* JustCntrlSuccs; // successors that only carry control signals
    
    std::vector<unsigned> sizes_preds; // sizes of the signals to predecessors
    std::vector<unsigned> sizes_succs; // sizes of the signals to successors

    bool is_live_in  = false;
    bool is_live_out = false;
    bool visited     = false;
    bool inthelist   = false;
    int id           = -1;
    int counter      = 0;
    bool controlNode; // true if this ENode is in the Cntrl_nodes list of a BBNode

    ENode(node_type nd, const char* name, Instruction* inst, BasicBlock* bb);
    ENode(node_type nd, BasicBlock* bb);
    ENode(node_type nd, const char* name, Argument* a, BasicBlock* bb);
    ENode(node_type nd, const char* name, BasicBlock* bb);
    ENode(node_type nd, const char* name);

    bool isLoadOrStore() const;

    int bbId;
    int memPortId;
    bool memPort = false; // LD or ST port connected to MC or LSQ
    int bbOffset;
    bool lsqToMC = false;

    bool isMux;
    bool isCntrlMg;
    std::string argName;
    int lsqMCLoadId = 0; 
    int lsqMCStoreId = 0; 

private:
    void commonInit(const node_type nd, const char* name, Instruction* inst, Argument* a,
                    BasicBlock* bb);
};

class BBNode {

public:
    const char* name; 
    BasicBlock* BB;
    int Idx;
    std::vector<BBNode*>* CntrlPreds;
    std::vector<BBNode*>* CntrlSuccs;
    std::vector<ENode*>* Live_in;
    std::vector<ENode*>* Live_out;
    std::vector<ENode*>* Cntrl_nodes;

    std::map<std::string, double> succ_freqs;
    int counter;

    BBNode(BasicBlock* bb, int idx,
           const char* name = NULL); 

    bool isImportedFromDotFile();
    void set_succ_freq(std::string succ_name, double freq);
    double get_succ_freq(std::string succ_name);
    bool valid_successor(std::string succ_name);
};
