#pragma once
#include "Head.h"

// Aya: 14/02/2022
#include <set>
#include <bitset>
#include <string>

/*
 * Authors: Radhika Ghosal
 * 			Ayatallah Elakhras
 */


//------------- Forward Declarations ---------------------//

class OptimizeBitwidth;

//--------------------------------------------------------//


class CircuitGenerator {

private:
// Aya: This should hold the post-dominator tree of the BBs inside the Function object
    PostDomTreeBase<BasicBlock>* my_post_dom_tree;
 

public:
    std::vector<ENode*>* enode_dag;
    std::vector<BBNode*>* bbnode_dag;

    // Temporary for debugging!!!
    std::ofstream dbg_file;

    // Maps LLVM BasicBlocks to BBNodes, since 1-1 correspondence
    std::unordered_map<BasicBlock*, BBNode*>* BBMap;

    OptimizeBitwidth* OB;
    MemElemInfo& MEI;
	//ControlDependencyGraph* CDG; 
    //ControlDependencyGraphPass* CDGP;

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

	static int inj_id ;  // AYA: 25/02/2022: the new component responsible for token regeneration

    //--------------------------------------------------------//

    CircuitGenerator(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag,
                     OptimizeBitwidth* OB, MemElemInfo& MEI)//, ControlDependencyGraphPass* CDGP)
        : enode_dag(enode_dag), bbnode_dag(bbnode_dag), OB(OB), MEI(MEI) { //, CDGP(CDGP) {

        BBMap = new std::unordered_map<BasicBlock*, BBNode*>;

	// Aya: allocating memory for the Post-dominator tree object
        my_post_dom_tree = new PostDominatorTree;
    }

	// AYA: 15/11/2021: function to trigger all constants except those feeding an unconditional branch!!
	void addSourceForConstants();

	// AYA: 09/11/2021: this function is needed to store the execution frequency of each BB.. This is needed later in adding buffers!!
	void setFreqs(const std::string& function_name);

    // Implementation in `dbgPrints.cpp`
    void printPostDomTree(Function& F);
    void PrintBridgesIndices(BBNode* producer_BB, BBNode* consumer_BB, std::vector<int>* bridges_indices);
    void dbgEnode_dag(std::string file_name);	
	void PrintLoopHeaders(Function& F, LoopInfo& LI, bool cntrl_path_flag);

    // Implementation in `AddBridges.cpp`
    // To add branches in the data path
	int returnPosRemoveNode(std::vector<ENode*>* vec, ENode* elem);
	void ConnectBrPreds(llvm::BranchInst* BI, std::vector<ENode*>* branches, 
			int j, ENode* enode, bool cntrl_path_flag);
	void ConnectBrSuccs(llvm::BranchInst* BI, std::vector<ENode*>* branches, int j, bool cntrl_path_flag, bool is_special_phi, BasicBlock *llvm_predBB);
    void AddBridges(ENode* producer, ENode* consumer, std::vector<int>* bridges_indices, bool cntrl_path_flag, std::vector<ENode*>* branches, bool is_special_phi, BasicBlock *llvm_predBB, std::ofstream& h_file);

    void FindBridges(BBNode* consumer_BB, BBNode* producer_BB, std::vector<int> path, int start, bool recur_flag, std::vector<int>* bridges_indices);

	// AYA: 13/02/2022: I changed the type of the parameter bridges_indices to account for multiple reachability points between a producer and a consumer..

    void FindBridgesHelper(BBNode* consumer_BB, BBNode* producer_BB, std::vector<std::vector<int>>* final_paths, std::vector<int>* bridges_indices, ENode* consumer, ENode* producer);

	void FindBridges_special_loops(BBNode* consumer_BB, std::vector<int>* bridges_indices, ENode* consumer, ENode* producer);

    void enumerate_paths_dfs(BBNode* current_BB, BBNode* producer_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths);

	void enum_paths_to_self(BBNode* current_BB, BBNode* producer_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths, bool first, std::ofstream& t_file);

    void ConnectProds_Cons(bool cntrl_path_flag, bool extra_cst_check);

	void ConnectProds_Cons_wrong(bool cntrl_path_flag, bool extra_cst_check);

	// 08/02/2022: this function should properly connect producers to consumers of type Merge by doing the trick of adding virtualBB and running the analysis on it instead of on the original consumer Merge!!
	void ConnectProds_Cons_Merges();
	void ConnectProds_Cons_Merges_cntrl();

	void enumerate_paths_dfs_start_to_end(BBNode* start_BB, BBNode* end_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths);
	bool my_is_post_dom(BBNode* firstBB, BBNode* secondBB, BBNode* endBB);
	void insertVirtualBB(ENode* enode, BBNode* virtualBB, BasicBlock *llvm_predBB);
	void removeVirtualBB(ENode* enode, BBNode* virtualBB, BasicBlock *llvm_predBB);

	// the following struct is used inside the new_setMuxes function in the construction of f0 and f1 
	struct pierCondition{
		int pierBB_index;
		bool is_negated;

		bool operator ==(const pierCondition& rhs) const {
			return(rhs.pierBB_index == pierBB_index && rhs.is_negated == is_negated);
		}

		bool operator ==(const int& rhs_int) const {
			return(rhs_int == pierBB_index);
		}

		bool operator !=(const pierCondition& rhs) const {
			return(rhs.pierBB_index != pierBB_index || rhs.is_negated != is_negated);
		}
	};

	// Aya: 10/08/2022: added the following type to distinguish the network that the algorithm will operate on
	enum networkType {
		data,
		constCntrl,
		memDeps,
	};

	// returns true if the hacky Merge for the initialization token is already present for another MUX!!
	ENode* MergeExists(bool&merge_exists, BasicBlock* target_bb);

	bool are_equal_paths(std::vector<std::vector<pierCondition>>* sum_of_products_ptr, int i, int j);
	void delete_repeated_paths(std::vector<std::vector<pierCondition>>* sum_of_products_ptr);

	void new_setMuxes_OLD();  // should eventually delete this function!!
	// helper functions inside new_setMuxes()
	void find_path_piers(std::vector<int> one_path, std::vector<int>* pierBBs, BBNode* virtualBB, BBNode* endBB);
	bool is_negated_cond(int pierBB_index, int next_pierBB_index, BasicBlock* phi_BB, std::ofstream& dbg_file);
	std::vector<std::string> get_binary_string_function (std::vector<std::vector<pierCondition>> funct, std::vector<int> condition_variables);

	int get_binary_sop(std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions, std::vector<std::string>* minterms_only_in_binary, std::vector<std::string>* minterms_dont_cares_in_binary, std::vector<int>* condition_variables, std::ofstream& dbg_file);

	ENode* returnSELCond(std::vector<std::string> essential_sel_always, std::vector<int>* condition_variables, bool& is_negated, std::ofstream& dbg_file);

	std::vector<std::vector<pierCondition>> essential_impl_to_pierCondition(std::vector<std::string> essential_sel_always);

	void printShannons_mux(std::ofstream& dbg_file, Shannon_Expansion::MUXrep sel_always_mux);

	std::vector<std::string> get_sel_no_token(std::vector<std::string>* minterms_only_in_binary, std::vector<std::string>* minterms_plus_dont_cares_in_binary);

	ENode* addOneNewMux(Shannon_Expansion::MUXrep sel_always_mux, BasicBlock* target_bb);
	ENode* identifyNewMuxinput(Shannon_Expansion::MUXrep sel_always_mux, int in_index, BasicBlock* target_bb, bool& is_negated_input);
	ENode* addNewMuxes_wrapper(Shannon_Expansion::MUXrep sel_always_mux, BasicBlock* target_bb);

	// 01/03/2022: same for connectProdCons but for identifying the piers between Start_ and cst_node passed at its parameter
	void FindPiers_for_constant(ENode* cst_node);

	// 14/03/2022: added this function to be used inside AddMerges that are used to mimic the idea of initialization token.. Returns true ONLY if there exists a Const_ in the same BB as the passed enode and this COnst_ doesn't have any PHI_c in its predecessors  
	bool phi_free_const_exists(ENode* enode);

	ENode* addMergeLoopSel(ENode* enode);
	ENode* addMergeLoopSel_for_injector(ENode* enode);

	// AYA: 26/03/2022: added the following to aid in converting Phi_c to Muxes!!
	ENode* addMergeLoopSel_ctrl(ENode* enode);

	void new_setMuxes();

	void my_swapVectors(std::vector<ENode*>* vec_1, std::vector<ENode*>* vec_2);
	void FixBranchesCond();

	//void addInj_hack_wrapper(bool cntrl_path_flag, std::ofstream& dbg_file);
	void addInj(bool cntrl_path_flag, std::ofstream& dbg_file);
	
	void debug_get_binary_sop (std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions, std::ofstream& dbg_file);

	std::vector<int> get_boolean_function_variables(   
		std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions);

	std::string intToBinString(int size, int val);
	void merge_paths(std::vector<std::vector<pierCondition>> sum_of_products, int i, int j);
	bool should_merge_paths(std::vector<pierCondition> path_i, std::vector<pierCondition> path_j);

	// new function for adding extra branches in the case where phis have predecessors that are not guaranteed to always come, yet the predecessors are post-dominating eachother!
	void ConnectProds_Cons_extraPhis();

	void deleteLLVM_Br();

    // To insert forks in the data and control paths
	void fixBranchesSuccs(ENode* enode, bool cntrl_br_flag);
    void aya_InsertNode(ENode* enode, ENode* after);
    void addFork();

	// 16/11/2021: replaced it with a new one :(
	void addFork_redunCntrl(); // we call this inside addFork to account for the new redundant control network
	void new_addFork_redunCntrl();

	// called inside addFork()!!
	void addFork_Mem_loads();

	// Temporary approach for adding buffers
    void addBuffersSimple_OLD();

	// called inside addFork to connect the successors of branches and add forks where needed
	void addBrSuccs(ENode* branch, networkType network_flag);

	// temporarily for debugging the placement of trueSuccs and falseSuccs of a branch!!!
	void printDbgBranches(std::vector<ENode*>* branches, bool cntrl_br_flag);	

	// Implementation in `RemoveRedunBranches.cpp`
    // To remove the redundant branches 
	void removeExtraBranchesWrapper(networkType network_flag);
	void deleteExtraBranches(networkType network_flag);
	void removeExtraBranches(networkType network_flag);
	void cutBranchPreds(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, ENode* redun_br);
	void cut_keepBranch_trueSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, int idx_of_br_to_keep);
	void cut_keepBranch_falseSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, int idx_of_br_to_keep);

	// AYA: 27/02/2022: Added the following removeRedun functions inside "new_RemoveRedunCom.cpp" 
	void FixMergeSuccs(ENode* redun_merge, ENode* merge_to_keep, std::ofstream& dbg_file);
	void cutMergePreds(ENode* redun_merge, std::ofstream& dbg_file);
	void removeRedunLoopSelMerges(std::vector<int>* redun_merge_indices, std::ofstream& dbg_file);

	void cleanRedunMerges(std::vector<int>* redun_merges_indices);
	void cleanConstants();
	void removeRedunLoopSelMerges_wrapper(std::ofstream& dbg_file);

	// AYA: 19/03/2022: functions inside AddBuffers_naive.cpp
	void addBuffersSimple();

	// AYA: 20/03/2022: 
	void deleteReturn();  // delete the return enode if the function is void
	// AYA: 20/03/2022: 
	void triggerReturn(); // trigger the return enode from START_ if the function is void

	void newAddSourceForConstants();
	
	// temporarily for debugging
	void printBrSuccs();
	///////////////////////////////////////////

    // Implementation in `AddCtrl.cpp`
    void addStartC();
    void addExitC();
    // To insert forks in the control path
    void InsertCntrlNode(ENode* enode, ENode* after, bool has_data_flag);
    void addForkC();

	//////////////// New functions for creating the redundant control network
	void buildRedunCntrlNet();
	void connectBrSuccs_RedunCntrlNet(); // called inside buildRedunCntrlNet
	bool connectReturnToVoid(BasicBlock* last_BB); // called inside buildRedunCntrlNet to trigger return through the redunCntrl network in case of void functions 
	void Fix_Redun_PhiCPreds(); // AYA: 01/10/2021: added this function to assure that the predecessors of the redun_cntrl_phis are compatible with the sequential order of the llvm CFG!! I call it at the end of buildRedunCntrlNet() after connectBrSuccs_RedunCntrlNet() 

	// AYA: 01/10/2021: I call it in MyCFGPass in the beginning right after building the enode_dag, and its implementation is in addcomp!!
	void Fix_LLVM_PhiPreds();

	// AYA: 06/10/2021: I call it in MyCFGPass after adding phis then removing redundant phis, and its implementation is in addPhi!!
	void Fix_my_PhiPreds(LoopInfo& LI, networkType network_flag);

	// AYA: 31/10/2021: This function is a wrapper that calls the above functions of adding branches and adding phis but with a special condition, only to check if the extra constants added for serving as a condition for unconditional branches are triggered correctly!!!
	void triggerBrConstsCheck(Function& F, LoopInfo& LI);

	void saveEnodeDag(std::vector<ENode*>* enode_dag_cpy_with_llvm_br);
	void constructOrderCntrlNet(std::vector<ENode*>* enode_dag_cpy_with_llvm_br);
	void connectBrSuccsPhiPreds_CntrlNet(std::vector<ENode*>* enode_dag_cpy_with_llvm_br);
	void addFork_CntrlNet();

	// My new implementation of converting Merges to muxes
	void setMuxes();

    // Implementation in `AddComp.cpp`
    void buildDagEnodes(Function& F);
    void fixBuildDagEnodes();	
    void removeRedundantBeforeElastic(std::vector<BBNode*>* bbnode_dag,
                                      std::vector<ENode*>* enode_dag);
    void setGetelementPtrConsts(std::vector<ENode*>* enode_dag);

	void removeRedundantAfterElastic(std::vector<ENode*>* enode_dag);

////////////////////////////////////////////////////////
	/*
	void cutBranchConnections(bool cntrl_path_flag, ENode* branch_to_delete); 
	void mergeRedundantBranches(bool cntrl_path_flag, std::vector<ENode*>* redundant_branches);
	void removeRedundantBranches(bool cntrl_path_flag); 
	void removeRedunBranches(bool cntrl_path_flag);
	*/
/////////////////////////////////////////////////

// for adding Phis
	void checkLoops(Function& F, LoopInfo& LI, networkType network_flag);
	
	void checkTailBranch(Loop* prod_loop, BasicBlock* producer_BB, BasicBlock* consumer_BB,  ENode* orig_producer_enode, ENode* orig_consumer_enode, std::vector<ENode*> branches, int count);

	void checkPhiModify(Loop* cons_loop, BasicBlock* producer_BB, BasicBlock* consumer_BB, ENode* orig_producer_enode, ENode* orig_consumer_enode, networkType network_flag, std::ofstream& h_file, std::vector<ENode*>* new_phis);

	void removeExtraPhisWrapper(networkType network_flag);
	void removeExtraPhis(networkType network_flag);
	void deleteExtraPhis(networkType network_flag);

	void cutPhiPreds(networkType network_flag, bool constCntrl_flag, int idx_of_redun_phi, std::vector<ENode*> *redun_phi_preds);

	void FixPhiSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_phi, int idx_of_phi_to_keep, std::vector<ENode*> *redun_phi_succs);


/////////////////////
	void FindLoopDetails(LoopInfo& LI);

	void identifyBackwardEdges(bool cntrl_path_flag);
/////////////////////////////////////////////////
	// Aya: this is needed to prevent adding redundant branches in case multiple consumers in a specific BB have the same producer
	void fixBranchesSuccsHelper(std::vector<ENode*>*succ,
	std::vector<ENode*>*branch_CntrlSuccs, bool cntrl_br_flag, ENode* branch);

	//void fillDoneBridgesFlags();
	//void findTwinConsumers(bool cntrl_path_flag);

    void addSink();

	// needed to maintain the correct order of preds and succs during adding fork 
	void forkToSucc(std::vector<ENode*>* preds_of_orig_succs, ENode* orig, ENode* fork);

	void buildPostDomTree(Function& F);

    /*void InsertNode(ENode* enode, ENode* after);
    void InsertBranch(ENode* enode, ENode* after, BBNode* bbnode);
    void InsertBranchNew(ENode* enode, BBNode* bbnode);
    void addForkAfter(ENode* enode, BBNode* bbnode);
    void addFork();
    void addBranch();

    void addSink();
    void addSource();
    ENode* findPhiC(BBNode* bbnode);
  
    void removeRedundantAfterElastic(std::vector<ENode*>* enode_dag);
    void addBuffersSimple();
    void setMuxes();
    void setBBIds();
    */

    // Implementation in `AddPhi.cpp`

    // Implementation in `Bitwidth.cpp`
    void setSizes();
    void createSizes(ENode* e, std::string inst_type = "");

    // Implementation in `SanityChecker.cpp`
    void phiSanityCheck(std::vector<ENode*>* enode_dag);
    void forkSanityCheck(std::vector<ENode*>* enode_dag);
    void branchSanityCheck(std::vector<ENode*>* enode_dag);
    void instructionSanityCheck(std::vector<ENode*>* enode_dag);
    void sanityCheckVanilla(std::vector<ENode*>* enode_dag);

    // Implementation in `Memory.cpp`
    void addMemoryInterfaces(const bool useLSQ);
    void constructLSQNodes(std::map<const Value*, ENode*>& LSQnodes);
    ENode* getOrCreateMCEnode(const Value* base, std::map<const Value*, ENode*>& baseToMCEnode);
    void addMCForEnode(ENode* enode, std::map<const Value*, ENode*>& baseToMCEnode);
    void updateMCConstant(ENode* enode, ENode* mcEnode);

    // Aya: added the following function to debug the special case of having all the memory deps in 1 BB
    void FixSingleBBLSQ_special();

	// AYa: added this to send control signals to the LSQ in the correct order
	// Some datastructures and types needed in the functions used to interface with the LSQ
	enum lsq_pred_type {
	    load = 0,
	    store = 1,
	    none = 2,
	};

	struct prod_cons_mem_dep {
	    int prod_idx;
	    int cons_idx;
	    bool is_backward = false;
	};
	void addControlForLSQ();
	void identifyMemDeps(ENode* lsq_enode, std::vector<prod_cons_mem_dep>& all_mem_deps); // called inside addControlForLSQ

	/* Aya: 23/05/2022: Functions for Applying FPL's algorithm 
	 */
	// functions in AddSuppress.cpp
	void identifyBBsForwardControlDependency();
	void enumerateBBpaths_dfs(BBNode* start_BB, BBNode* end_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths, bool self_loop_flag);
	void addSuppress(bool const_trigger_network_flag);

	void eliminateCommonControlAncestors(std::ofstream& dbg_file, BBNode* consumer_bb, BBNode* producer_bb, std::vector<int>& modif_consBB_deps, std::vector<int>& modif_prodBB_deps);
	void printCDFG(); // prints for debugging
	void constructSOP(std::ofstream& dbg_file, BBNode* startBB, BBNode* bbnode, const std::vector<int>& modif_BB_deps, std::vector<std::vector<pierCondition>>& f_sop);

	void remove_MUX_SEL_Negation();  // implemented in AddCtrl: it swaps the inputs of a MUX if the condition is negated!!

	// 15/06/2022
	void convertToMux(networkType network_flag, ENode* phi, ENode* supp_cond, bool is_negated_cond);
	ENode* new_MergeExists(bool&merge_exists, BasicBlock* target_bb, ENode* cond);

	void NegateApplydeMorgans(std::ofstream& dbg_file, const std::vector<std::vector<pierCondition>>& f, std::vector<std::vector<pierCondition>>& negated_f);
	std::vector<std::vector<pierCondition>> NegateProduct(const std::vector<pierCondition>& one_product);
	void myPushBack(std::vector<pierCondition>& product_to_push_to, const std::vector<pierCondition>& product_to_push);


	void new_get_boolean_function_variables(const std::vector<std::vector<pierCondition>>& f, std::vector<int>& condition_variables);
	void new_get_binary_string_minterms(std::ofstream& dbg_file, const std::vector<std::vector<pierCondition>>& funct, const std::vector<int>& condition_variables, std::vector<std::string>& minterms_only_in_binary, bool print_flag = true);
	void Simplify_Quine_McCluskey(std::ofstream& dbg_file, const std::vector<std::vector<pierCondition>>& f, std::vector<std::vector<pierCondition>>& f_simplified, bool print_flag = true);

	void new_get_binary_string_minterms_Shannons(std::ofstream& dbg_file, const std::vector<std::vector<pierCondition>>& funct, const std::vector<int>& condition_variables, std::vector<std::string>& minterms_only_in_binary, bool print_flag = true);


	bool calculate_F_Supp(std::ofstream& dbg_file, const std::vector<std::vector<pierCondition>>& f_prod, const std::vector<std::vector<pierCondition>>& f_cons, std::vector<std::vector<pierCondition>>& f_supp_simplified);
	bool simplify_if_repeated_conds(const std::vector<pierCondition>& one_product);
	void new_binary_string_to_pierCondition_vec(const std::vector<std::string>& f_string, const std::vector<int>& condition_variables, std::vector<std::vector<pierCondition>>& f_vec);
	
	ENode* applyShannon(std::ofstream& dbg_file, std::ofstream& dbg_file_5, BasicBlock* target_bb, const std::vector<std::vector<pierCondition>>& f, bool& is_condition_negated, bool print_flag = true);
	ENode* returnSingleCondENode(const std::vector<std::vector<pierCondition>>& f, bool& is_negated);
	ENode* insertBranch(networkType network_flag, ENode* branch_condition, ENode* producer, ENode* consumer, bool is_negated_cond);

	void findVirtBBDeps(std::ofstream& dbg_file, BBNode* virtualBB);
	void printControlDeps(std::ofstream& dbg_file, BBNode* virtBB);

	void remove_SUPP_COND_Negation();

	void addSuppress_with_loops(networkType network_flag);
	void OLDaddSuppress_with_loops(bool const_trigger_network_flag);

	// functions inside ConvertGSA.cpp
	void calc_MUX_SEL(std::ofstream& dbg_file, ENode* enode, std::vector<std::vector<pierCondition>>& sel_always);
	void getSOP(std::ofstream& dbg_file, const std::vector<std::vector<std::vector<pierCondition>>>& merged_paths_functions, std::vector<std::vector<pierCondition>>& sel_always);

	// functions inside SuppressLoopProd.cpp
	void addSuppLoopExit(bool const_trigger_network_flag);
	void FindLoopExits(Loop* loop, std::vector<BBNode*>& loop_exits);
	void FindLoopsInNest(Loop* loop, std::vector<Loop*>& loops_in_nest);

	void addMergeRegeneration(std::ofstream& dbg_file, bool const_trigger_network_flag);


	void addLoopsExitstoDeps_ProdInConsOut(std::ofstream& dbg_file, ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps);
	void addLoopsExitstoDeps_ProdInConsIn_BackEdge(ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps);
	
	void addLoopsExitstoDeps_Phi_regeneration(ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps);

	void FindLoopExits_ProdInConsOut(std::ofstream& dbg_file, ENode* producer, ENode* consumer, std::vector<BBNode*>& loop_exits);

	void debugLoopsOutputs(std::ofstream& dbg_file);


	void setMuxes_nonLoop();

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
