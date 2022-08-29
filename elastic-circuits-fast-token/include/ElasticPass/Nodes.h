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
    dummy_ =22,

	Inj_n=23,   // AYA: 25/02/2022: the new component responsible for token regeneration
};

class BBNode; 

class ENode {

public:
	// AYA: 23/02/2022:
	// I only set this input for a subset of branches and Muxes that are added inside newSetMuxes
		// and for those special nodes, I check this is_negated in printDot
	std::vector<bool>* is_negated_input; 	
	bool is_advanced_component;  // initialized to false and set to true for a branch or mux inserted inside newSetMuxes

	// AYA: 15/03/2022: the following field is set to true for enodes of type Const_ that are added to mimic the intialization token that is needed to break deadlocks in loops
	bool is_init_token_const; 

	// AYA: 19/03/2022: the following field is set to true for enodes of type Phi_n that are added in the process of mimicing the intialization token that is needed to break deadlocks in loops
	bool is_init_token_merge;

    int capacity;
    char* compType               = nullptr;
    ENode* branchFalseOutputSucc = nullptr;
    ENode* branchTrueOutputSucc  = nullptr;
    std::vector<ENode*>* parameterNodesReferences[3];
    int cstValue   = 0;       // Leonardo, inserted line, used only with Cst_ type of nodes
    BBNode* bbNode = nullptr; // Leonardo, inserted line // Aya: 23/05/2022: most likely the field is not written anywhere!
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

	// Aya: Introducing a new network to assure correct order!!
	std::vector<ENode*>* CntrlOrderPreds; // predecessors that only carry control signals specifically to assure order!!
    std::vector<ENode*>* CntrlOrderSuccs; // successors that only carry control signals specifically to assure order!!

	////// Aya:to keep track of backward edges
	std::vector<bool>* isBackwardPreds_Cntrl;
	std::vector<bool>* isBackwardPreds;

	/////// Aya: the following variable identifies nodes that are part of the new control path
	bool is_redunCntrlNet = false;

	////////////////// AYA: 16/11/2021: ADDED A FIELD TO DISTINGUISH THE ENODES OF TYPE CONSTANT THAT ARE FEEDING THE CONDITION OF A BRANCH!! AND ANOTHER ONE TO DENOTE THAT THE CONSTANT IS FEEDING A MC OR LSQ
	bool is_const_br_condition = false;
	bool is_const_feed_memory = false;
	

	// Aya: added the following arrays to keep track of a branch's potentially many true and/or false sucessors
	// for the data path 
	std::vector<ENode*>* true_branch_succs;
	std::vector<ENode*>* false_branch_succs;
	// for the control path responsible for constant triggering 
	std::vector<ENode*>* true_branch_succs_Ctrl;
	std::vector<ENode*>* false_branch_succs_Ctrl;
    
    //Aya: 19/08/2022: for the control path responsible for interfacing with the LSQ
    std::vector<ENode*>* true_branch_succs_Ctrl_lsq;
    std::vector<ENode*>* false_branch_succs_Ctrl_lsq;

	ENode* producer_to_branch;  // branch in a bridge BB carries data from which producer
  
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

    bool isMux = false;
    bool isCntrlMg = false;
    std::string argName;
    int lsqMCLoadId = 0; 
    int lsqMCStoreId = 0; 

    // Aya: 15/06/2022: added the following field that is true only for enodes of type Phi_n that are added in Shannon's expansion to generate a cond out of a boolean expression 
    bool is_mux_for_cond = false;
    bool is_const_for_cond = false; // true for constants that are added as inputs to Muxes or to Merges used to implement INIT 
    bool is_merge_init = false;   // true for Merges used to mimic the INIT

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

    // 23/06/2022: added the following flag to distinguish a virtualBB that is added temporarily for the analysis of a Phi consumer 
    bool is_virtBB = false;

    /*
    * Aya: 23/05/2022: the following vector holds the indices of BBs that the BBNode object is 
    * control dependent on          
    */
    std::vector<int>* BB_deps; 

	/////////////// New added fields for keeping loop information
	bool is_loop_latch, is_loop_exiting, is_loop_header;
	BasicBlock* loop_header_bb;
	Loop* loop;

	/////////////////////////////////////

    std::map<std::string, double> succ_freqs;
    int counter;

    BBNode(BasicBlock* bb, int idx,
           const char* name = NULL); 

    bool isImportedFromDotFile();
    void set_succ_freq(std::string succ_name, double freq);
    double get_succ_freq(std::string succ_name);
    bool valid_successor(std::string succ_name);
};
