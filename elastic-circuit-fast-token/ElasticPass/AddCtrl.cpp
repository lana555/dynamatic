#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

#include <math.h>

// AYA: 26/05/2022: The purpose of this function is to to swap the inputs
	// of a MUX if the SEL of the MUX is inverted, then remove the inversion!
void CircuitGenerator::remove_MUX_SEL_Negation(){
	for(auto& enode: *enode_dag) {
		if(enode->isMux == true && enode->is_advanced_component) {
			// check if the first input is negated!
			if(enode->is_negated_input->at(0)) {
				// switch the second input with the third input and set the is_negated_input to false!
				ENode* temp = enode->CntrlPreds->at(2);
				enode->CntrlPreds->at(2) = enode->CntrlPreds->at(1);
				enode->CntrlPreds->at(1) = temp;

				enode->is_negated_input->at(0) = false;
			}
		}
	}
}

// this function creates the necessary enodes to deliver SEL to enode, but it doesn't connect to enode internally.. handle the connections of the preds of the return of the function and the succs of the passed enode outside the loop!!
ENode* CircuitGenerator::addMergeLoopSel_for_injector(ENode* enode) {

	// Search for the loop exit BB
	BBNode* loop_exit_bb = nullptr;
	for(auto& bbnode_ : *bbnode_dag) {
		if(bbnode_->is_loop_exiting && bbnode_->loop == BBMap->at(enode->BB)->loop) {
			loop_exit_bb = bbnode_;
			break;
		}
	}
	assert(loop_exit_bb != nullptr);

	// search for llvm's branch inside the loop exit BB	
	llvm::BranchInst* BI;
	ENode* llvm_branch;
	for(auto& enode_ : *enode_dag) {
		if(enode_->type == Branch_ && enode_->BB == loop_exit_bb->BB) { 
			BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
			llvm_branch = enode_;
			break; // no need to continue looping, 1 llvm branch is enough
		}
	}

	assert(!BI->isUnconditional());
	
	// the SEL of a MUX or a token_regenerator/injector should be the output of a Merge to mimic the idea of an initialization token
	ENode* merge_for_sel = new ENode(Phi_n, "phi", enode->BB);  
    merge_for_sel->id = phi_id++;
	enode_dag->push_back(merge_for_sel);

				
	// one of the inputs of the Merge should be a constant and the other should be the condition of the exitBB

	// check the successors of the exitBB condition, one successor should be inside the loop and the other should be outside!
	BasicBlock *falseBB, *trueBB;
	trueBB = BI->getSuccessor(0);
	falseBB = BI->getSuccessor(1);			

	int cstValue;
	std::string cst_name;
	bool should_negate_cond = false;

	// is the condition = 1 when we are still iterating?
	if(BBMap->at(trueBB)->loop == BBMap->at(enode->BB)->loop) {
		// this means it continues looping when cond = 1, 
		// then we shouldn't negate the condition
			should_negate_cond = false;
			// intialization token is 0, (in the very beginning take the input that is outside the loop!)
	} else {
		// this means it continues looping when cond = 0, 
		assert(BBMap->at(falseBB)->loop == BBMap->at(enode->BB)->loop);

		// then we should negate the condition to make sure when I'm still iterating, the value returned is 1
		should_negate_cond = true;

	} 

	// I always fix the intialization token to 0, since I enforce that the condition to be 1 if we are still iterating and 0 when we are done!!(in the very beginning take the input that is outside the loop!)
	cstValue = 0;
	cst_name = std::to_string(0);

	// string constant name and the cstValue should be a variable!!!!
		// the value of the constant should be 0 if the loop header is at the true side of the branch of the loop exit, and should be 1 if it's at the false side!
	ENode* cst_condition = new ENode(Cst_, cst_name.c_str(), enode->BB);
	// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
	cst_condition->cstValue = cstValue;
	enode_dag->push_back(cst_condition);
	cst_condition->id = cst_id++;

	// AYA: 16/11/2021
	//cst_condition->is_const_br_condition = true;

	merge_for_sel->CntrlPreds->push_back(cst_condition);
	cst_condition->CntrlSuccs->push_back(merge_for_sel);

	// trigger the constant from START
	for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
		if(cnt_node->type == Start_) {
			cst_condition->JustCntrlPreds->push_back(cnt_node);
			cnt_node->JustCntrlSuccs->push_back(cst_condition);
			break; // found the start node, no need to continue looping
		}
	} 

	// feed the Merge with the loop_exitBB condition
	merge_for_sel->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
	llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(merge_for_sel);
	// there are chances now that the SEL input of the merge_for_sel is negated
		// so mark it to be checked in the DOT file!
	merge_for_sel->is_advanced_component = true;
	// the first input is the constant and we don't invert need!
	merge_for_sel->is_negated_input->push_back(false);
	// the second input is LLVM's branch condition of the loop exit BB and its negation status is dependent on 
	merge_for_sel->is_negated_input->push_back(should_negate_cond);
	assert(merge_for_sel->CntrlPreds->size() == merge_for_sel->is_negated_input->size());

	return merge_for_sel;


} 

// check if the hacky Merge for the intiialization token is already present in the BB
ENode* CircuitGenerator::MergeExists(bool&merge_exists, BasicBlock* target_bb) {
	for(auto& enode:*enode_dag) {
		if(enode->type == Phi_n && !enode->isMux && enode->BB == target_bb) {
			// it needs to be a having the first predecessor a MUX and a successor as another Phi_n or Phi_!
			if( enode->CntrlPreds->at(0)->type == Cst_ && (enode->CntrlSuccs->at(0)->type == Phi_ || enode->CntrlSuccs->at(0)->type == Phi_n || enode->CntrlSuccs->at(0)->type == Phi_c ) ) {
				merge_exists = true;
				return enode;
			}

		}
	} 
	merge_exists = false;
	return nullptr;

}

/*
	// loop on all predecessors until you reach for a Branch_c or START_
	bool found_desired_pred_complic = false;
	ENode* current_pred = e->JustCntrlPreds->at(0);
	const_pred = e->JustCntrlPreds->at(0);
	while(current_pred->type != Start_) {
		current_pred = current_pred->JustCntrlPreds->at(0);
		if(current_pred->type != Phi_c) {
			// before deciding to take it, check that none of its predecessors are PhiC!!! If you found any PhiC in the path, then add I should replicate all branches starting from the constant until the PhiC and instead of including the PhiC, connect from Start_ 
			ENode* temp_pred = current_pred;
			std::vector<ENode*> temp_preds_set;
			bool found_phi_in_path = false;
			while(temp_pred->type != Start_) {
				temp_pred = current_pred->JustCntrlPreds->at(0);
				if(temp_pred->type == Phi_c) {
					found_phi_in_path = true;
					break;
				} else {
			// I keep a copy of the nodes in the path just in case I stop at a Phi_c and in which case I should replicate all the nodes between the const e and the Phi_C, and trigger them with Start_ instead of Phi_C
					temp_preds_set.push_back(temp_pred);
				}
			}

			if(!found_phi_in_path) {
				desired_pred = current_pred;
				found_desired_pred_complic = true;
				break;
			} else {
				// loop over the temp_preds_set and create new enodes identical to them
				ENode* start_node = nullptr;
				for(auto& s_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(s_node->type == Start_) {
						start_node = s_node;
						break; // found the start node, no need to continue looping
					}
				} 
				assert(start_node!=nullptr);
				new_replica.push_back(start_node);
				for(int m = 0; m < temp_preds_set.size(); m++) {
					ENode* new_replica = new ENode(temp_preds_set.at(m)->type, temp_preds_set.at(m)->Name.c_str(), temp_preds_set.at(m)->BB);
					new_replica->id = branch_id++;

					enode_dag->push_back(new_replica);

					// predecessors and successors
					
					
				}

			}
			
		} 
		const_pred = current_pred;
	}

	if(!found_desired_pred_complic) {
		// then desired_pred should be the START node
		for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
			if(cnt_node->type == Start_) {
				desired_pred = cnt_node;
				break; // found the start node, no need to continue looping
			}
		} 

	}
*/

// AYA: 01/03/2022
void CircuitGenerator::FindPiers_for_constant(ENode* cst_node) {
	std::ofstream h_file;
    h_file.open("add_bridges_dbg_constants.txt");

	// variables needed inside enumerate_paths
	int cfg_size = bbnode_dag->size();
	bool *visited = new bool[cfg_size];

	int *path = new int[cfg_size];
	int path_index = 0; 

	// the array of branches (can be a single branch) added to the enode_dag in AddBridges()
	std::vector<ENode*>* branches = new std::vector<ENode*>;

	// Data structures that will hold the results of different intermediate analysis
	// 1) vector of all the possible paths between the producer and consumer
	std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;

	// 2) vector of indices of bbs in the bbnode_dag that are bridges in any of the paths
	std::vector<int>* bridges_indices = new std::vector<int>;

	for(int i = 0; i < cfg_size; i++){
		visited[i] = false;
	}
	path_index = 0; 

	final_paths->clear();
	bridges_indices->clear();

	branches->clear();

	enumerate_paths_dfs(BBMap->at(cst_node->BB), BBMap->at(cst_node->JustCntrlPreds->at(0)->BB), visited, path, path_index, final_paths);

	assert(final_paths->size() > 0);

	FindBridgesHelper(BBMap->at(cst_node->BB), BBMap->at(cst_node->JustCntrlPreds->at(0)->BB), final_paths, bridges_indices, cst_node, cst_node->JustCntrlPreds->at(0));

	AddBridges(cst_node->JustCntrlPreds->at(0), cst_node, bridges_indices, true, branches, false, nullptr, h_file);

	delete[] visited;
	delete[] path;

	delete bridges_indices;

	delete branches;

}

bool CircuitGenerator::phi_free_const_exists(ENode* enode) {
	bool found_const = false;
	bool found_phi_pred_const = false;
	for(auto& e : *enode_dag) {
		found_phi_pred_const = false;
		if(e->type == Cst_ && e->BB == enode->BB && e->JustCntrlPreds->at(0)->type != Source_) {
			// loop on all predecessors until you reach for a Branch_c or START_
			ENode* current_pred = e->JustCntrlPreds->at(0);
			while(current_pred->type != Start_) {	
				if(current_pred->type == Phi_c) {
					found_phi_pred_const = true;
				}
				current_pred = current_pred->JustCntrlPreds->at(0);
			}
			
			found_const = true;
			break;  // stop searching for a constant because you found one!		
		} 
	}

	// this function should return true if I found a constant AND this constant is PHI_c free!!
	if(found_const) {
		// return true only if this const is PHI_c free!!
		if(found_phi_pred_const) {
			return false;
		} else {
			return true;
		} 
	} else {
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////
ENode* CircuitGenerator::addMergeLoopSel_ctrl(ENode* enode) {

	// Search for the loop exit BB
	BBNode* loop_exit_bb = nullptr;
	for(auto& bbnode_ : *bbnode_dag) {
		if(bbnode_->is_loop_exiting && bbnode_->loop == BBMap->at(enode->BB)->loop) {
			loop_exit_bb = bbnode_;
			break;
		}
	}
	assert(loop_exit_bb != nullptr);

	// search for llvm's branch inside the loop exit BB	
	llvm::BranchInst* BI;
	ENode* llvm_branch;
	for(auto& enode_ : *enode_dag) {
		if(enode_->type == Branch_ && enode_->BB == loop_exit_bb->BB) { 
			BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
			llvm_branch = enode_;
			break; // no need to continue looping, 1 llvm branch is enough
		}
	}

	assert(!BI->isUnconditional());

	ENode* merge_for_sel = nullptr;

	// before adding a new Merge and constant, search for an existing Merge and connect to it!!
	bool merge_exists_flag;
	ENode* temp = MergeExists(merge_exists_flag, enode->BB);
	if(merge_exists_flag) {
		// the function should return the existing Merge!
		assert(temp != nullptr);
		merge_for_sel = temp;

	} else {
		// the SEL of a MUX or a token_regenerator/injector should be the output of a Merge to mimic the idea of an initialization token
		merge_for_sel = new ENode(Phi_n, "phi", enode->BB);  
		merge_for_sel->id = phi_id++;

		// AYA: 19/03/2022: mark this as a special Merge node added for initialization..
		merge_for_sel->is_init_token_merge = true;

		enode_dag->push_back(merge_for_sel);
					
		// one of the inputs of the Merge should be a constant and the other should be the condition of the exitBB

		// check the successors of the exitBB condition, one successor should be inside the loop and the other should be outside!
		BasicBlock *falseBB, *trueBB;
		trueBB = BI->getSuccessor(0);
		falseBB = BI->getSuccessor(1);			

		int cstValue;
		std::string cst_name;
		bool should_negate_cond = false;

		if(BBMap->at(trueBB)->loop == BBMap->at(enode->BB)->loop) {
			// this means it continues looping when cond = 1, 

			// to decide if the condition of the branch loop exit should be negated or not, we need to check if the enode->CntrlPreds->at(1) is inside the loop or not!
			if(BBMap->at(enode->JustCntrlPreds->at(1)->BB)->loop == BBMap->at(enode->BB)->loop) {
				// then we shouldn't negate the condition
				should_negate_cond = false;
				// intialization token is 0, (in the very beginning take the input that is outside the loop!)
				cstValue = 0;
				cst_name = std::to_string(0);
			} else {
				assert(BBMap->at(enode->JustCntrlPreds->at(0)->BB)->loop == BBMap->at(enode->BB)->loop);
				// we should negate the condition
				should_negate_cond = true;
				// intialization token is 1, (in the very beginning take the input that is outside the loop which is the .at(1) input!)
				cstValue = 1;
				cst_name = std::to_string(1);
			} 

		} else {
			assert(BBMap->at(falseBB)->loop == BBMap->at(enode->BB)->loop);
			// to decide if the condition of the branch loop exit should be negated or not, we need to check if the enode->CntrlPreds->at(0) is inside the loop or not!
			if(BBMap->at(enode->JustCntrlPreds->at(0)->BB)->loop == BBMap->at(enode->BB)->loop) 				{
				// then we shouldn't negate the condition
				should_negate_cond = false;
				// intialization token is 1, (in the very beginning take the input that is outside the loop!)
				cstValue = 1;
				cst_name = std::to_string(1);
			} else {
				assert(BBMap->at(enode->JustCntrlPreds->at(1)->BB)->loop == BBMap->at(enode->BB)->loop);
				// we should negate the condition
				should_negate_cond = true;
				// intialization token is 0, (in the very beginning take the input that is outside the loop which is the .at(0) input!)
				cstValue = 0;
				cst_name = std::to_string(0);
			} 

		}

		// AYA: 15/03/2022: It is IMP to note that for the idea of reusing an existing constant for all Merges for Muxes at loop headers to work, I'm assuming that all the conditions of all loops are following a fixed convention for loop exiting (i.e., ALL exit the loop when the condition is true or when the condition is false!)

		// AYA: 15/03/2022: added logic to check if a constant exists, connect to it instead of adding a new constant!! And, if there is no constants at all, add one in BB0 and it should be connected to all Merges later on! 
		bool found_init_const = false;
		ENode* existing_init_const = nullptr;
		for(auto& ee : *enode_dag) {
			if(ee->type == Cst_ && ee->is_init_token_const) {
				found_init_const = true;
				existing_init_const = ee;
				break;
			}
		} 

		if(found_init_const) {
			assert(existing_init_const != nullptr);
			merge_for_sel->CntrlPreds->push_back(existing_init_const);
			existing_init_const->CntrlSuccs->push_back(merge_for_sel);
		} else {
			// create a new constant, put it in BB0 and raise its is_init_token_const to true
					// string constant name and the cstValue should be a variable!!!!
			// the value of the constant should be 0 if the loop header is at the true side of the branch of the loop exit, and should be 1 if it's at the false side!
			ENode* cst_condition = new ENode(Cst_, cst_name.c_str(), bbnode_dag->at(0)->BB);
			
			//AYA: 15/03/2022
			cst_condition->is_init_token_const = true;

			// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
			cst_condition->cstValue = cstValue;
			enode_dag->push_back(cst_condition);
			cst_condition->id = cst_id++;

			// AYA: 16/11/2021
			//cst_condition->is_const_br_condition = true;

			merge_for_sel->CntrlPreds->push_back(cst_condition);
			cst_condition->CntrlSuccs->push_back(merge_for_sel);

			// trigger the constant from START
			// first search for another const (not triggered by Source) in the same BB and connect to its same predecessor
			bool found_similar_const = false;
			ENode* branch_succ = nullptr;

			// first check if there is a similar constant existing that has NO Phi_c at all in its path from STart_!
			//if(phi_free_const_exists(enode)) {
				// 14/03/2022: Updatee!!! I replaced the logic below with a direct connection to START since the initialization token should arrive to the mux at the loop header regardless of any control structure!!!
				for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(cnt_node->type == Start_) {
						cst_condition->JustCntrlPreds->push_back(cnt_node);
						cnt_node->JustCntrlSuccs->push_back(cst_condition);
						break; // found the start node, no need to continue looping
					}
				} 

		} 

		// feed the Merge with the loop_exitBB condition
		merge_for_sel->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
		llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(merge_for_sel);
		// there are chances now that the SEL input of the merge_for_sel is negated
			// so mark it to be checked in the DOT file!
		merge_for_sel->is_advanced_component = true;
		// the first input is the constant and we don't invert need!
		merge_for_sel->is_negated_input->push_back(false);
		// the second input is LLVM's branch condition of the loop exit BB and its negation status is dependent on 
		merge_for_sel->is_negated_input->push_back(should_negate_cond);
		assert(merge_for_sel->CntrlPreds->size() == merge_for_sel->is_negated_input->size());

	}
	
	assert(merge_for_sel != nullptr);
	return merge_for_sel;

}
////////////////////////////////////////////////////////////////////////////////////

ENode* CircuitGenerator::addMergeLoopSel(ENode* enode) {

	// Search for the loop exit BB
	BBNode* loop_exit_bb = nullptr;
	for(auto& bbnode_ : *bbnode_dag) {
		if(bbnode_->is_loop_exiting && bbnode_->loop == BBMap->at(enode->BB)->loop) {
			loop_exit_bb = bbnode_;
			break;
		}
	}
	assert(loop_exit_bb != nullptr);

	// TODO need to add support for multiple loop exits here!!
	// search for llvm's branch inside the loop exit BB	
	llvm::BranchInst* BI;
	ENode* llvm_branch;
	for(auto& enode_ : *enode_dag) {
		if(enode_->type == Branch_ && enode_->BB == loop_exit_bb->BB) { 
			BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
			llvm_branch = enode_;
			break; // no need to continue looping, 1 llvm branch is enough
		}
	}

	assert(!BI->isUnconditional());

	ENode* merge_for_sel = nullptr;

	// before adding a new Merge and constant, search for an existing Merge and connect to it!!
	bool merge_exists_flag;
	ENode* temp = MergeExists(merge_exists_flag, enode->BB);
	if(merge_exists_flag) {
		// the function should return the existing Merge!
		assert(temp != nullptr);
		merge_for_sel = temp;

	} else {
		// the SEL of a MUX or a token_regenerator/injector should be the output of a Merge to mimic the idea of an initialization token
		merge_for_sel = new ENode(Phi_n, "phi", enode->BB);  
		merge_for_sel->id = phi_id++;

		// AYA: 19/03/2022: mark this as a special Merge node added for initialization..
		merge_for_sel->is_init_token_merge = true;

		enode_dag->push_back(merge_for_sel);
					
		// one of the inputs of the Merge should be a constant and the other should be the condition of the exitBB

		// check the successors of the exitBB condition, one successor should be inside the loop and the other should be outside!
		BasicBlock *falseBB, *trueBB;
		trueBB = BI->getSuccessor(0);
		falseBB = BI->getSuccessor(1);			

		int cstValue;
		std::string cst_name;
		bool should_negate_cond = false;

		if(BBMap->at(trueBB)->loop == BBMap->at(enode->BB)->loop) {
			// this means it continues looping when cond = 1, 

			// to decide if the condition of the branch loop exit should be negated or not, we need to check if the enode->CntrlPreds->at(1) is inside the loop or not!
			if(BBMap->at(enode->CntrlPreds->at(1)->BB)->loop == BBMap->at(enode->BB)->loop) 				{
				// then we shouldn't negate the condition
				should_negate_cond = false;
				// intialization token is 0, (in the very beginning take the input that is outside the loop!)
				cstValue = 0;
				cst_name = std::to_string(0);
			} else {
				assert(BBMap->at(enode->CntrlPreds->at(0)->BB)->loop == BBMap->at(enode->BB)->loop);
				// we should negate the condition
				should_negate_cond = true;
				// intialization token is 1, (in the very beginning take the input that is outside the loop which is the .at(1) input!)
				cstValue = 1;
				cst_name = std::to_string(1);
			} 

		} else {
			assert(BBMap->at(falseBB)->loop == BBMap->at(enode->BB)->loop);
			// to decide if the condition of the branch loop exit should be negated or not, we need to check if the enode->CntrlPreds->at(0) is inside the loop or not!
			if(BBMap->at(enode->CntrlPreds->at(0)->BB)->loop == BBMap->at(enode->BB)->loop) 				{
				// then we shouldn't negate the condition
				should_negate_cond = false;
				// intialization token is 1, (in the very beginning take the input that is outside the loop!)
				cstValue = 1;
				cst_name = std::to_string(1);
			} else {
				assert(BBMap->at(enode->CntrlPreds->at(1)->BB)->loop == BBMap->at(enode->BB)->loop);
				// we should negate the condition
				should_negate_cond = true;
				// intialization token is 0, (in the very beginning take the input that is outside the loop which is the .at(0) input!)
				cstValue = 0;
				cst_name = std::to_string(0);
			} 

		}

		// AYA: 15/03/2022: It is IMP to note that for the idea of reusing an existing constant for all Merges for Muxes at loop headers to work, I'm assuming that all the conditions of all loops are following a fixed convention for loop exiting (i.e., ALL exit the loop when the condition is true or when the condition is false!)

		// AYA: 15/03/2022: added logic to check if a constant exists, connect to it instead of adding a new constant!! And, if there is no constants at all, add one in BB0 and it should be connected to all Merges later on! 
		bool found_init_const = false;
		ENode* existing_init_const = nullptr;
		for(auto& ee : *enode_dag) {
			if(ee->type == Cst_ && ee->is_init_token_const) {
				found_init_const = true;
				existing_init_const = ee;
				break;
			}
		} 

		if(found_init_const) {
			assert(existing_init_const != nullptr);
			merge_for_sel->CntrlPreds->push_back(existing_init_const);
			existing_init_const->CntrlSuccs->push_back(merge_for_sel);
		} else {
			// create a new constant, put it in BB0 and raise its is_init_token_const to true
					// string constant name and the cstValue should be a variable!!!!
			// the value of the constant should be 0 if the loop header is at the true side of the branch of the loop exit, and should be 1 if it's at the false side!
			ENode* cst_condition = new ENode(Cst_, cst_name.c_str(), bbnode_dag->at(0)->BB);
			
			//AYA: 15/03/2022
			cst_condition->is_init_token_const = true;

			// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
			cst_condition->cstValue = cstValue;
			enode_dag->push_back(cst_condition);
			cst_condition->id = cst_id++;

			// AYA: 16/11/2021
			//cst_condition->is_const_br_condition = true;

			merge_for_sel->CntrlPreds->push_back(cst_condition);
			cst_condition->CntrlSuccs->push_back(merge_for_sel);

			// trigger the constant from START
			// first search for another const (not triggered by Source) in the same BB and connect to its same predecessor
			bool found_similar_const = false;
			ENode* branch_succ = nullptr;

			// first check if there is a similar constant existing that has NO Phi_c at all in its path from STart_!
			//if(phi_free_const_exists(enode)) {
				// 14/03/2022: Updatee!!! I replaced the logic below with a direct connection to START since the initialization token should arrive to the mux at the loop header regardless of any control structure!!!
				for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(cnt_node->type == Start_) {
						cst_condition->JustCntrlPreds->push_back(cnt_node);
						cnt_node->JustCntrlSuccs->push_back(cst_condition);
						break; // found the start node, no need to continue looping
					}
				} 

				// 14/03/2022: Updatee!!! I commented out the following logic since I need to deliver the initializtion token regardless of any control!!!
				/* for(auto& e : *enode_dag) {
					if(e->type == Cst_ && e->BB == enode->BB && e->JustCntrlPreds->at(0)->type != Source_) {
						ENode* desired_pred = nullptr;

						// check the predecessor.at(0) of this constant
						if(e->JustCntrlPreds->at(0)->type != Phi_c) {
							desired_pred = e->JustCntrlPreds->at(0);
							// if the pred is not of type Phi_c, then it could be a Branch_c or START_ 
							branch_succ = e;
						} else {
							// loop on all predecessors until you reach for a Branch_c or START_
							ENode* current_pred = e->JustCntrlPreds->at(0);
							while(current_pred->type != Start_) {
								if(current_pred->type != Phi_c) {
									// 1st check that this current_pred doesn't have any Phi_c in its predecessors, otherwise, create replicas of the Branch_c preds and trigger them with Start_ instead of with Phi_c 
									desired_pred = current_pred;
									break;
								} 
								// if you reach here then the current_pred is a Phi_c
								branch_succ = current_pred;
								current_pred = current_pred->JustCntrlPreds->at(0);
							}

							// if you completed the loop because you arrived at a Start_
							if(current_pred->type == Start_) {
								desired_pred = current_pred;
							}
						}
						assert(desired_pred != nullptr);
						
						if(desired_pred->type == Branch_c) {
							assert(branch_succ != nullptr);
							// I should put my constant at either its trueSuccs or falseSuccs array
								// search for e (the current constant enode) in each of the true and false Succs!!
							auto pos = std::find(desired_pred->true_branch_succs_Ctrl->begin(), desired_pred->true_branch_succs_Ctrl->end(), branch_succ);
							if(pos != desired_pred->true_branch_succs_Ctrl->end()) {
								// if it was found in the trueSuccs
								desired_pred->true_branch_succs_Ctrl->push_back(cst_condition);
							} else {
								// then it must be in the falseSuccs
								pos = std::find(desired_pred->false_branch_succs_Ctrl->begin(), desired_pred->false_branch_succs_Ctrl->end(), branch_succ);
								assert(pos != desired_pred->false_branch_succs_Ctrl->end());
								desired_pred->false_branch_succs_Ctrl->push_back(cst_condition);
							} 
							
						} else {
					// it's not a Branch, so you can connect your constant to its successor directly!!
							desired_pred->JustCntrlSuccs->push_back(cst_condition);
						} 
						cst_condition->JustCntrlPreds->push_back(desired_pred);
						found_similar_const = true;
						break;
					} 
				} */

			//} 
			/*else {
				// trigger from START, then run the findPiers algorithm to add any necessary branches
				for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(cnt_node->type == Start_) {
						cst_condition->JustCntrlPreds->push_back(cnt_node);
						cnt_node->JustCntrlSuccs->push_back(cst_condition);
						break; // found the start node, no need to continue looping
					}
				} 


				// then will run our bridges algorithm to identify bridges, if any between Start_C and that particular Constant
				// 14/03/2022: Updatee!!! I commented out this function since I need to deliver the initializtion token regardless of any control!!!
				//FindPiers_for_constant(cst_condition);

			}*/



			/* if(!found_similar_const) {
				// trigger from START
				for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(cnt_node->type == Start_) {
						cst_condition->JustCntrlPreds->push_back(cnt_node);
						cnt_node->JustCntrlSuccs->push_back(cst_condition);
						break; // found the start node, no need to continue looping
					}
				} 

				// then will run our bridges algorithm to identify bridges, if any between Start_C and that particular Constant
				FindPiers_for_constant(cst_condition);
			}  */
			
			////////////////////////////////////////////////////////////////////////////
		} 

		// feed the Merge with the loop_exitBB condition
		merge_for_sel->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
		llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(merge_for_sel);
		// there are chances now that the SEL input of the merge_for_sel is negated
			// so mark it to be checked in the DOT file!
		merge_for_sel->is_advanced_component = true;
		// the first input is the constant and we don't invert need!
		merge_for_sel->is_negated_input->push_back(false);
		// the second input is LLVM's branch condition of the loop exit BB and its negation status is dependent on 
		merge_for_sel->is_negated_input->push_back(should_negate_cond);
		assert(merge_for_sel->CntrlPreds->size() == merge_for_sel->is_negated_input->size());

	}
	
	assert(merge_for_sel != nullptr);
	return merge_for_sel;

}

void CircuitGenerator::saveEnodeDag(std::vector<ENode*>* enode_dag_cpy_with_llvm_br) {
	for(auto& enode : *enode_dag) {
		enode_dag_cpy_with_llvm_br->push_back(enode);
	}

}

void CircuitGenerator::buildRedunCntrlNet() {
	for (auto& bbnode : *bbnode_dag) {
		ENode* phiC_node = nullptr;
		llvm::BranchInst* BI = nullptr;
		ENode* llvm_branch_enode = nullptr; 

		if (bbnode == bbnode_dag->front()) {
		// No need to create a Phi here because there is a start node
		// Search for the start node in this BB 
			for(auto& cnt_node : *bbnode->Cntrl_nodes) {
				if(cnt_node->type == Start_) {
					phiC_node = cnt_node;
					break; // found the start node, no need to continue looping
				}
			}  

			assert(phiC_node != nullptr); 

    	} else {
	        // create new `phi_node` node
	        phiC_node            = new ENode(Phi_c, "phiC", bbnode->BB);
	        phiC_node->id        = phi_id++;
	        phiC_node->isCntrlMg = false;
			//bbnode->Cntrl_nodes->push_back(phiC_node);
			enode_dag->push_back(phiC_node);

			phiC_node->is_redunCntrlNet = true;
    	}

		// Creating branch at the end of the same BB
		if (bbnode->CntrlSuccs->size() > 0) { // BB branches/jumps to some other BB
        	ENode* branchC_node = new ENode(Branch_c, "branchC", bbnode->BB);
        	branchC_node->id    = branch_id++;
	        //bbnode->Cntrl_nodes->push_back(branchC_node);
			setBBIndex(branchC_node, bbnode);
	        enode_dag->push_back(branchC_node);

			branchC_node->is_redunCntrlNet = true;

			// Convention of Branch Predecessors is token at pred(0) and condition at pred(1)
			branchC_node->CntrlOrderPreds->push_back(phiC_node);
			phiC_node->CntrlOrderSuccs->push_back(branchC_node);
			
			// Connecting the condition predecessor at pred(1)
			// search for LLVM branch in the same BB to extract the condition from it
			for(auto& orig_enode : *enode_dag) {
				if(orig_enode->type == Branch_ && orig_enode->BB == bbnode->BB) { 
					BI = dyn_cast<llvm::BranchInst>(orig_enode->Instr);
					llvm_branch_enode = orig_enode;
					break; // no need to continue looping 1 llvm branch is enough
				}
			}

			// make sure that a branch instruction was actually returned or else assert!!!!
			assert(BI != nullptr && llvm_branch_enode != nullptr);

			if(BI->isUnconditional()) {
				// so llvm's branch has no predecessors, so I must create an enode of type constant and treat it as the condition
				ENode* cst_condition = new ENode(Cst_, std::to_string(1).c_str(), bbnode->BB);
				// this is very crucial to specify the value of the constant!!!
				cst_condition->cstValue = 1;

				cst_condition->id = cst_id++;
				enode_dag->push_back(cst_condition);

				cst_condition->is_redunCntrlNet = true;
			
				// AYA: 16/11/2021
				cst_condition->is_const_br_condition = true;

				// 02/11/2021: changed the network to CNtrlPreds 
				// 04/11/2021: changed them back to CntrlOrder network!!!
				branchC_node->CntrlOrderPreds->push_back(cst_condition);
				cst_condition->CntrlOrderSuccs->push_back(branchC_node);

				// note that this constant must have the phiC node as its predecessor
				cst_condition->CntrlOrderPreds->push_back(phiC_node);
				phiC_node->CntrlOrderSuccs->push_back(cst_condition);

			} else {
		
			// the condition is the only predecessor of llvm's branch
				assert(llvm_branch_enode->CntrlPreds->size() > 0);
				ENode* llvm_br_condition = llvm_branch_enode->CntrlPreds->at(0);
				
				// connect this llvm_br_condition to our newly created branch_c over the redundant network and addFork will make sure that forks are added properly connecting whatever is there across the 3 networks we have!! :)

				// AYA: 02/11/2021: changed the following to CntrlSuccs and CntrlPreds instead of CntrlOrderSuccs and CntrlOrderPreds!!
				// 04/11/2021: changed them back to CntrlOrder network!!!
				llvm_br_condition->CntrlOrderSuccs->push_back(branchC_node);
				branchC_node->CntrlOrderPreds->push_back(llvm_br_condition);
				
			} 

		// Will call another function in the end of this function to take care of connecting all added branchC to all added Phi_c of the next BB!!

		} else {
			// added the following function to connect the return to the redun phi_c in the last BB in case the function is return to void!!
			bool is_void = connectReturnToVoid(bbnode->BB);

			// if the BBnode is the last one in the CFG, no need to create a branch, instead create a sink to act as the successor of the BB's phi_c
			if(bbnode_dag->size() > 1) {
				// only let the phi feed the sink if the function doesn't return to void!!
				if(!is_void){
					ENode* sink_node = new ENode(Sink_, "sink");
					sink_node->id    = sink_id++;
					enode_dag->push_back(sink_node);

					sink_node->is_redunCntrlNet = true;

					sink_node->CntrlOrderPreds->push_back(phiC_node);
					phiC_node->CntrlOrderSuccs->push_back(sink_node);
				} 

			}

		} 

	}

	connectBrSuccs_RedunCntrlNet();
	Fix_Redun_PhiCPreds();
}


void CircuitGenerator::connectBrSuccs_RedunCntrlNet() {
	
	// loop over the bbnodes
	for(int i = 0; i < bbnode_dag->size()-1; i++) {
		llvm::BranchInst* BI = nullptr;
		BasicBlock* trueBB = nullptr;
		BasicBlock* falseBB = nullptr;
		

		// first: identify the llvm branch instruction of this bbnode
		for(auto& enode : *enode_dag) {
			if(enode->type == Branch_ && enode->BB == bbnode_dag->at(i)->BB) { 
				BI = dyn_cast<llvm::BranchInst>(enode->Instr);
				break; // no need to continue looping 1 llvm branch is enough
			}	
		}
		
		assert(BI != nullptr);

		// Not all llvm branches have a true and a false successor, so need to check the number of successors first
		int llvm_br_num_of_succs = BI->getNumSuccessors();
		// sanity check: branch must have at least a single successor and no more than 2 succsessors
		assert(llvm_br_num_of_succs > 0 && llvm_br_num_of_succs < 3);

		if(llvm_br_num_of_succs == 1) {
			trueBB = BI->getSuccessor(0);
			falseBB = nullptr;
		} else {
			// then the number of successors must be 2!!
			assert(llvm_br_num_of_succs == 2);
			trueBB = BI->getSuccessor(0);
			falseBB = BI->getSuccessor(1);
		}

		assert(trueBB != nullptr);

		// search for the branchC in the current bbnode and connect it to the PhiC in the trueBB and falseBB if the falseBB is not a nullptr!
		for(auto& enode : *enode_dag) {
			if(enode->BB == bbnode_dag->at(i)->BB && enode->type == Branch_c && enode->is_redunCntrlNet) {

				ENode* true_phiC = nullptr;
				ENode* false_phiC = nullptr;

				// search for the phiC of this bbnode
				for(auto& tmp_enode : *enode_dag) {
					if(tmp_enode->BB == trueBB && tmp_enode->type == Phi_c && tmp_enode->is_redunCntrlNet) {
						true_phiC = tmp_enode;
						break;
				    }
				}
			
				assert(true_phiC != nullptr);

				true_phiC->CntrlOrderPreds->push_back(enode);
				enode->CntrlOrderSuccs->push_back(true_phiC);

				if(falseBB != nullptr) {
						// search for the phiC of this bbnode
					for(auto& tmp_enode : *enode_dag) {
						if(tmp_enode->BB == falseBB && tmp_enode->type == Phi_c && tmp_enode->is_redunCntrlNet) {
							false_phiC = tmp_enode;
							break;
						}
					}
					false_phiC->CntrlOrderPreds->push_back(enode);
					enode->CntrlOrderSuccs->push_back(false_phiC);
				} else{
					// no false successor so connect to a sink
					ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
					sink_node->id    = sink_id++;
					enode_dag->push_back(sink_node);

					sink_node->is_redunCntrlNet = true;

					sink_node->CntrlOrderPreds->push_back(enode);
					enode->CntrlOrderSuccs->push_back(sink_node);

				} 
				
				// CHECK???? CAN A BB HAVE MORE THAN ONE BRANCH_C??? SHOULD I ACCOUNT FOR THIS CASE HERE AND ABOVE DURING THE CREATION OF THOSE REDUNDANT CONTROL BRANCH???
				break; // found the branch of this bbnode, no need to continue looping
			} 
		}

	}

}


///////////// AYA: 01/10/2021: Added the following function to make sure the predecessors of any redundant PhiC are in a compatible order with LLVM's order and the CFG

void CircuitGenerator::Fix_Redun_PhiCPreds() {
// IMP Assumption here: All LLVM's Phis in 1 BB are having the same getIncomingBlock(0) and getIncomingBlock(1)!!!!
	
	for(auto& bbnode : *bbnode_dag) {
		BasicBlock* predBB_0 = nullptr;
		BasicBlock* predBB_1 = nullptr;
		for(auto& enode : *enode_dag) {
			if(enode->type == Phi_ && enode->BB == bbnode->BB) {
				BasicBlock* new_predBB_0 = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(0);
				BasicBlock* new_predBB_1 = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(1);

				if(predBB_0 != nullptr)
					assert(new_predBB_0 == predBB_0);
				
				if(predBB_1 != nullptr)
					assert(new_predBB_1 == predBB_1);

				predBB_0 = new_predBB_0;
				predBB_1 = new_predBB_1;
			} 

		} 

	}
 	///////////////////////////// End of sanity check validating the assumption! 
	for(auto& enode : *enode_dag) {
		if(enode->type == Phi_c && enode->is_redunCntrlNet) {
			ENode* redun_cntrl_phi = enode;
			// search for an LLVM PHI in the same BB
			for(auto& enode_2 : *enode_dag) {
				if(enode_2->type == Phi_ && enode_2->BB == enode->BB) {
					ENode* llvm_phi = enode_2;
					BasicBlock* llvm_predBB_0 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(0);
					BasicBlock* llvm_predBB_1 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(1);

					// compare predBBs and adjust accordingly!!
					if(redun_cntrl_phi->CntrlOrderPreds->at(0)->BB != llvm_predBB_0) {
						assert(redun_cntrl_phi->CntrlOrderPreds->at(1)->BB == llvm_predBB_0 && redun_cntrl_phi->CntrlOrderPreds->at(0)->BB == llvm_predBB_1);
					
						// swap the 2 predecessors of the redun_cntrl_phi
						ENode* temp = redun_cntrl_phi->CntrlOrderPreds->at(0);
						redun_cntrl_phi->CntrlOrderPreds->at(0) = redun_cntrl_phi->CntrlOrderPreds->at(1);
						redun_cntrl_phi->CntrlOrderPreds->at(1) = temp;
					}
					// sanity check
					assert(redun_cntrl_phi->CntrlOrderPreds->at(0)->BB == llvm_predBB_0 && redun_cntrl_phi->CntrlOrderPreds->at(1)->BB == llvm_predBB_1);
					break;   // found the llvm phi, stop looping!!
				} 
			}
		}
	}
}


bool CircuitGenerator::connectReturnToVoid(BasicBlock* last_BB) {
	ENode* last_redun_phi; 
	bool is_void = false;

	// first we need to identify the last redundant phi_c object
	for(auto& enode : *enode_dag) {
		if(enode->type == Phi_c && enode->is_redunCntrlNet && enode->BB == last_BB) {
			last_redun_phi = enode;
			break; // found it! Stop looping!
		} 
	}  

	for(auto& enode : *enode_dag) {
		if(enode->type == Inst_ && enode->Name.compare("ret") == 0 && enode->CntrlPreds->size() == 0) {
			is_void = true;
			last_redun_phi->CntrlOrderSuccs->push_back(enode);
			enode->CntrlOrderPreds->push_back(last_redun_phi);
		}
	} 

	return is_void;

} 

void CircuitGenerator::merge_paths(std::vector<std::vector<pierCondition>> sum_of_products, int i, int j) {
	// I assume that when you call this function, the paths at indices i and j have the same exact pierBBs with only a single pierBB having opposite negation in the 2 paths
	std::vector<pierCondition> path_i = sum_of_products.at(i);
	std::vector<pierCondition> path_j = sum_of_products.at(j);
	// they should have the same pierBBs so should have the same size!!
	assert(path_i.size() == path_j.size());
	int size = path_i.size();
	bool found_pierBB_opposite_negation = false;

	// search for the pierBBs with only a single pierBB having opposite negation in the 2 paths
	for(int ii = 0; ii < size; ii++) {
		int pierBB_index = path_i.at(ii).pierBB_index;
		bool pierBB_is_negated = path_i.at(ii).is_negated;
	
		for(int jj = 0; jj < size; jj++) {
			if(path_j.at(jj).pierBB_index == pierBB_index) {
				if(path_j.at(jj).is_negated != pierBB_is_negated) {
					found_pierBB_opposite_negation = true;
					break;
				}
			}
		}

		if(found_pierBB_opposite_negation) {
			// delete this pierBB entry from path_i		
			auto pos = std::find(path_i.begin(), path_i.end(), path_i.at(ii));
			assert(pos != path_i.end());
			path_i.erase(pos);

			break;
		}

	}
	
	// delete path j
	assert(path_i.size() < path_j.size());
	auto pos_3 = std::find(sum_of_products.begin(), sum_of_products.end(), path_j);
	assert(pos_3 != sum_of_products.end());

}

bool CircuitGenerator::should_merge_paths(std::vector<pierCondition> path_i, std::vector<pierCondition> path_j) {
	if(path_i.size() != path_j.size())
		return false;

	// Otherwise, the 2 paths are of the same size, check if they have the same conditions
	int size = path_i.size();
	int diff_negation_count = 0;

	for(int i = 0; i < size; i++) {
		int pierBB_index = path_i.at(i).pierBB_index;
		bool pierBB_is_negated = path_i.at(i).is_negated;
		// search for this BB in the path_j
		bool found_pierBB = false;
		for(int j = 0; j < size; j++) {
			if(path_j.at(j).pierBB_index == pierBB_index) {
				found_pierBB = true;
				// check the negation flag of both, if it's different, increment the counter
					// paths can be merged only if there is at most 1 pierBB with opposite negation 
				if(path_j.at(j).is_negated != pierBB_is_negated){
					diff_negation_count++;
				} 

				break;
			}
		}
		if(!found_pierBB || diff_negation_count > 1)
			return false;
	} 

	return true;
}


std::string CircuitGenerator::intToBinString(int size, int val) {
	std::string bin;
	// We assume an upper bound of 100 on the number of bits contained in a binary string
	bin = std::bitset<100>(val).to_string();
	bin = bin.substr(100-size);
	return bin;
}


std::vector<int>CircuitGenerator::get_boolean_function_variables(   
	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions) {

	std::vector<int> condition_variables;
	
	for(int i = 0; i < merged_paths_functions.size(); i++) {
		for(int j = 0; j < merged_paths_functions.at(i).size(); j++) {
			for(int k = 0; k < merged_paths_functions.at(i).at(j).size(); k++) {
				int pierBB_index = merged_paths_functions.at(i).at(j).at(k).pierBB_index;
				auto pos = std::find(condition_variables.begin(), condition_variables.end(), pierBB_index);
				// push to condition_variables only if you didn't push this value before!!
				if(pos == condition_variables.end()) {
					condition_variables.push_back(pierBB_index);
				}

			}

		}

	}

	// sort the condition_variables to eas your life in calculating the decimal_f0_products 
	std::sort(condition_variables.begin(), condition_variables.end());

	return condition_variables;

}

void CircuitGenerator::debug_get_binary_sop
		(std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions, std::ofstream& dbg_file) {

	/// EVERYTHING ENCLOSED HERE IS JUST FOR DEBUGGING!!!!!
	dbg_file << "\n---------------------------------------------------\n";
	dbg_file << "\n---------------------------------------------------\n";

	std::vector<int> condition_variables =
		get_boolean_function_variables(merged_paths_functions);

	dbg_file << "Printing the condition variables\n";
	for(int k = 0; k < condition_variables.size(); k++) {
		dbg_file << "C" << condition_variables.at(k) + 1 << " ";
	}
	dbg_file << "\n";

	int number_of_bits = condition_variables.size();
	int number_of_minterms = pow(2, number_of_bits);

	dbg_file << "Number of bits needed is " << number_of_bits << " and the number of truth table rows is " << number_of_minterms << "\n";

	std::vector<int> decimal_minterms;
	for(int i = 0 ; i < number_of_minterms; i++) {
		decimal_minterms.push_back(i);
	}

	dbg_file << "\nPrinting all the possible truth table entries\n";
	for(int y = 0; y < decimal_minterms.size(); y++) {
		dbg_file << decimal_minterms.at(y) << "\n";
	} 
	dbg_file << "Done printing the decimal truth table\n";

	std::vector<std::string> binary_f0_products;
	// loop over the different products constituting f0
	for(int i = 0; i < merged_paths_functions.at(0).size(); i++) {
		// for each product, check which variables are present and which are not
		std::string one_binary_f0_product = "";
		for(int j = 0; j < condition_variables.size(); j++) {
			// search for each of these conditions in the current product

			// to make this find work, I overloaded a == operator that compares the struct object with an integer
			auto pos_ = std::find(merged_paths_functions.at(0).at(i).begin(), merged_paths_functions.at(0).at(i).end(), condition_variables.at(j));

			if(pos_!= merged_paths_functions.at(0).at(i).end()) {
				auto index = pos_ - merged_paths_functions.at(0).at(i).begin();
				if(merged_paths_functions.at(0).at(i).at(index).is_negated) {
					one_binary_f0_product.push_back('0');
				} else {
					one_binary_f0_product.push_back('1');
				} 

			} else {
				// if this condition_variable is not found, then it is a don't-care so mark it as x to create two entries out of it in the end!
				one_binary_f0_product.push_back('x');
			}
		}
		binary_f0_products.push_back(one_binary_f0_product);
	}


	dbg_file << "\nPrinting the binary_f0_sum_of_products possibly with don't-cares\n";
	for(int y = 0; y < binary_f0_products.size(); y++) {
		dbg_file << binary_f0_products.at(y) << "\n";
	} 
	dbg_file << "Done printing the binary_f0_sum_of_products possibly with don't-cares\n";

	std::vector<int> indices_to_replicate;  // replicate the don't-care variables
	for(int i = 0; i < binary_f0_products.size(); i++) {
		indices_to_replicate.clear();
		// get the dont-care variables for that product
		for(int j = 0; j < binary_f0_products.at(i).length(); j++) {
			if(binary_f0_products.at(i).at(j) == 'x') {
				indices_to_replicate.push_back(j);
			}
		}

		for(int k = 0; k < indices_to_replicate.size(); k++) {
			// replace this don't-care with "0" and make another copy of it with "1"
			binary_f0_products.at(i).at(indices_to_replicate.at(k)) = '1';
			binary_f0_products.push_back(binary_f0_products.at(i));
			binary_f0_products.at(i).at(indices_to_replicate.at(k)) = '0';
		}
	}

	dbg_file << "\nPrinting the binary_f0_sum_of_products without don't-cares\n";
	for(int y = 0; y < binary_f0_products.size(); y++) {
		dbg_file << binary_f0_products.at(y) << "\n";
	} 
	dbg_file << "Done printing the binary_f0_sum_of_products without don't-cares\n";


	std::vector<std::string> binary_minterms;
	for(int i = 0 ; i < decimal_minterms.size(); i++) {
		binary_minterms.push_back(intToBinString(number_of_bits, decimal_minterms.at(i)));
	}

	dbg_file << "\nPrinting the binary_minterms\n";
	for(int y = 0; y < binary_minterms.size(); y++) {
		dbg_file << binary_minterms.at(y) << "\n";
	} 
	dbg_file << "Done printing the binary_minterms\n";


	for(int i = 0; i < binary_f0_products.size(); i++) {
		// searh for the elements of binary_f0_products in binary_minterms to erase them
		auto pos_ = std::find(binary_minterms.begin(), binary_minterms.end(), binary_f0_products.at(i));
	
		assert(pos_ != binary_minterms.end());
		binary_minterms.erase(pos_);
	}

	dbg_file << "\nPrinting the binary_minterms after deleting the entries of f0!\n";
	for(int y = 0; y < binary_minterms.size(); y++) {
		dbg_file << binary_minterms.at(y) << "\n";
	} 
	dbg_file << "Done printing the binary_minterms after deleting the entries of f0!\n";

	dbg_file << "\n---------------------------------------------------\n";
	dbg_file << "\n---------------------------------------------------\n";
			
}

std::vector<std::string> CircuitGenerator::get_binary_string_function
 	(std::vector<std::vector<pierCondition>> funct, std::vector<int> condition_variables) {

	std::vector<std::string> binary_f_products;
	// loop over the different products constituting f0
	for(int i = 0; i < funct.size(); i++) {
		// for each product, check which variables are present and which are not
		std::string one_binary_f_product = "";
		for(int j = 0; j < condition_variables.size(); j++) {
			// search for each of these conditions in the current product

			// to make this find work, I overloaded a == operator that compares the struct object with an integer
			auto pos_ = std::find(funct.at(i).begin(), funct.at(i).end(), condition_variables.at(j));

			if(pos_!= funct.at(i).end()) {
				auto index = pos_ - funct.at(i).begin();
				if(funct.at(i).at(index).is_negated) {
					one_binary_f_product.push_back('0');
				} else {
					one_binary_f_product.push_back('1');
				} 

			} else {
				// if this condition_variable is not found, then it is a don't-care so mark it as x to create two entries out of it in the end!
				one_binary_f_product.push_back('x');
			}
		}
		binary_f_products.push_back(one_binary_f_product);
	}

	std::vector<int> indices_to_replicate;  // replicate the don't-care variables

	for(int i = 0; i < binary_f_products.size(); i++) {
		indices_to_replicate.clear();
		// get the dont-care variables for that product
		for(int j = 0; j < binary_f_products.at(i).length(); j++) {
			if(binary_f_products.at(i).at(j) == 'x') {
				indices_to_replicate.push_back(j);
			}
		}

		for(int k = 0; k < indices_to_replicate.size(); k++) {
			// replace this don't-care with "0" and make another copy of it with "1"
			binary_f_products.at(i).at(indices_to_replicate.at(k)) = '1';
			binary_f_products.push_back(binary_f_products.at(i));
			binary_f_products.at(i).at(indices_to_replicate.at(k)) = '0';
		}
	}

	return binary_f_products;
}


int CircuitGenerator::get_binary_sop (
	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions, std::vector<std::string>* minterms_only_in_binary, std::vector<std::string>* minterms_plus_dont_cares_in_binary, std::vector<int>* condition_variables, std::ofstream& dbg_file) {
	
	std::vector<int> temp_condition_variables =
				get_boolean_function_variables(merged_paths_functions);  // returns the different conditions constituting the 2 functions f0 and f1

	// filling to the pointer to condition_variables to use it outside the function!!
	for(int j = 0; j < temp_condition_variables.size(); j++) {
		condition_variables->push_back(temp_condition_variables.at(j));
	} 
	
	dbg_file << "\nHii from inside get_binary_sop!\n";
	int number_of_bits = temp_condition_variables.size();
	int number_of_truth_table_rows = pow(2, number_of_bits);

	dbg_file << "\nNumber of bits is = " << number_of_bits << " and Number of truth table rows is = " << number_of_truth_table_rows << "\n";

	std::vector<int> decimal_minterms;
	for(int i = 0 ; i < number_of_truth_table_rows; i++) {
		decimal_minterms.push_back(i);
	}

	std::vector<std::string> binary_f0_products = get_binary_string_function
 	(merged_paths_functions.at(0), temp_condition_variables);

	for(int i = 0 ; i < decimal_minterms.size(); i++) {
		minterms_plus_dont_cares_in_binary->push_back(intToBinString(number_of_bits, decimal_minterms.at(i)));
	}
	// erase the entries of f0 from minterms_plus_dont_cares_in_binary!
	for(int i = 0; i < binary_f0_products.size(); i++) {
		// searh for the elements of binary_f0_products in binary_minterms to erase them
		auto pos_ = std::find(minterms_plus_dont_cares_in_binary->begin(), minterms_plus_dont_cares_in_binary->end(), binary_f0_products.at(i));
		assert(pos_ != minterms_plus_dont_cares_in_binary->end());
		minterms_plus_dont_cares_in_binary->erase(pos_);
	}
	
	std::vector<std::string> binary_f1_products = get_binary_string_function(merged_paths_functions.at(1), temp_condition_variables);

	///////////// Aya: 22/08/2022: do an extra check by removing an potential replicate entries in binary_f1_products
	auto end = binary_f1_products.end();
    for (auto it = binary_f1_products.begin(); it != end; ++it) {
        end = std::remove(it + 1, end, *it);
    }
    binary_f1_products.erase(end, binary_f1_products.end());
	//////////////////////////////////////////////////////////////

	for(int i = 0; i < binary_f1_products.size(); i++) {
		minterms_only_in_binary->push_back(binary_f1_products.at(i));
	}  

	int num_of_dont_cares = number_of_truth_table_rows - binary_f1_products.size() - binary_f0_products.size();

	dbg_file << "\nNumber of dont-care entries is = " << num_of_dont_cares << "\n";

	// some sanity checks
	assert(num_of_dont_cares == minterms_plus_dont_cares_in_binary->size() -  minterms_only_in_binary->size());
	assert(number_of_truth_table_rows == minterms_plus_dont_cares_in_binary->size() + binary_f0_products.size());

	return num_of_dont_cares;
}

// returns true if you arrive to the consumer from START through the falseBBpred of the branch inside the passed pierBB 
bool CircuitGenerator::is_negated_cond(int pierBB_index, int next_pierBB_index, BasicBlock* phi_BB, std::ofstream& dbg_file) {
	
	bool is_negated;
	llvm::BranchInst* BI;
	ENode* llvm_branch = nullptr;
	for(auto& e : *enode_dag) {
		if(e->type == Branch_ && e->BB == (bbnode_dag->at(pierBB_index))->BB) { 
			BI = dyn_cast<llvm::BranchInst>(e->Instr);
			llvm_branch = e;
			break; // no need to continue looping 1 llvm branch is enough
		}
	}

	assert(llvm_branch!=nullptr);
	// connecting the branch successors
	BasicBlock *falseBB, *trueBB;
	trueBB = BI->getSuccessor(0);
	falseBB = BI->getSuccessor(1);

	dbg_file << "\n The trueSuccBB of this pier BB is " << aya_getNodeDotbbID(BBMap->at(trueBB)) << " and the falseSuccBB of this pier BB is " << aya_getNodeDotbbID(BBMap->at(falseBB)) << "\n";

	if(next_pierBB_index == pierBB_index) {
		// happens only if the last pier is also the LLVM's "coming from" of the Phi BB
		// the consumerBB must be = to trueBB or falseBB!
		if(phi_BB == falseBB) {
			is_negated = true; 
		} else {
			assert(phi_BB == trueBB);
			is_negated = false; 
		}
	} else {
		// check to see if the next pierBB is post-dom the true or the false BB..
		if(my_post_dom_tree->properlyDominates(bbnode_dag->at(next_pierBB_index)->BB, falseBB) || bbnode_dag->at(next_pierBB_index)->BB == falseBB) {

			// sanity check: it can't be post-dominating also the trueBB!!
			assert(!my_post_dom_tree->properlyDominates(bbnode_dag->at(next_pierBB_index)->BB, trueBB) || bbnode_dag->at(next_pierBB_index)->BB == trueBB);

			is_negated = true; 
		} else {
			// sanity check: it must be post-dominating also the trueBB!!
			assert(my_post_dom_tree->properlyDominates(bbnode_dag->at(next_pierBB_index)->BB, trueBB) || bbnode_dag->at(next_pierBB_index)->BB == trueBB);
			is_negated = false; 

		} 
	}

	return is_negated;
}

void CircuitGenerator::find_path_piers(std::vector<int> one_path, std::vector<int>* pierBBs, BBNode* virtualBB, BBNode* endBB) {
	for(int hh = 0; hh < one_path.size(); hh++) {
		if(bbnode_dag->at(one_path.at(hh))->CntrlSuccs->size() >= 2) {
			if(!my_is_post_dom(bbnode_dag->at(one_path.at(hh)), virtualBB, endBB)) {
				pierBBs->push_back(one_path.at(hh));
			} // end of the post-dominance pier condition
		} // end of the pierBB condition
	} // end of loop on a single path
}

std::vector<std::vector<CircuitGenerator::pierCondition>>CircuitGenerator::essential_impl_to_pierCondition(std::vector<std::string> essential_sel_always) {

	// note that the essential_sel_always is a vector of strings serving as a sum of products 
		// one string (product) has the length = number of conditions_variables, but some of them might be dashes, indicating that this variable is not part of the essential prime implicant

	std::vector<std::vector<pierCondition>> sel_always;
	std::vector<pierCondition> one_essential_product;

	for(int i = 0; i < essential_sel_always.size(); i++) {
		one_essential_product.clear();
		for(int j = 0; j < essential_sel_always.at(i).length(); j++) {
			// skip if there is a '-' in place of that condition!
			if(essential_sel_always.at(i).at(j) == '-') {
				continue;
			} else {
				pierCondition cond;
				cond.pierBB_index = j;
				if(essential_sel_always.at(i).at(j) == '0') {
					cond.is_negated = true;
				} else {
					cond.is_negated = false;
				}

				one_essential_product.push_back(cond);
			}
		}
		sel_always.push_back(one_essential_product);
	}  

	return sel_always;
}

void CircuitGenerator::printShannons_mux(std::ofstream& dbg_file, Shannon_Expansion::MUXrep sel_always_mux) {
	dbg_file << "\n\n SEL_ALWAYS = \n";

	dbg_file << "Printing in0 info.. The number of prodcuts in it is: " << sel_always_mux.in0.size() << " and its sum of products is: ";
	for(int i = 0; i < sel_always_mux.in0.size(); i++) {
		for(int j = 0; j < sel_always_mux.in0.at(i).size(); j++) {
			if(sel_always_mux.in0.at(i).at(j).is_const) {
				dbg_file << sel_always_mux.in0.at(i).at(j).const_val << " & ";
			} else {
				if(sel_always_mux.in0.at(i).at(j).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << sel_always_mux.in0.at(i).at(j).var_index +1 << " & ";
			}

		} 
		dbg_file << " + "; 
	} 


	dbg_file << "Printing in1 info.. The number of prodcuts in it is: " << sel_always_mux.in1.size() << " and its sum of products is: ";
	for(int i = 0; i < sel_always_mux.in1.size(); i++) {
		for(int j = 0; j < sel_always_mux.in1.at(i).size(); j++) {
			if(sel_always_mux.in1.at(i).at(j).is_const) {
				dbg_file << sel_always_mux.in1.at(i).at(j).const_val << " & ";
			} else {
				if(sel_always_mux.in1.at(i).at(j).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << sel_always_mux.in1.at(i).at(j).var_index +1 << " & ";
			}

		} 
		dbg_file << " + "; 
	} 
	/*if(sel_always_mux.in0.size() == 0) {
		assert(sel_always_mux.in0.size() == 1);
		std::vector<std::vector<pierCondition>> sel_always = essential_impl_to_pierCondition(essential_sel_always);
		for(int u = 0; u < sel_always.size(); u++) {
			for(int v = 0; v < sel_always.at(u).size(); v++) {
				if(sel_always.at(u).at(v).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << sel_always.at(u).at(v).pierBB_index +1 << " & ";
			}
			dbg_file << " + "; 
		}    
	} else {
		dbg_file << "MUX(" << sel_always_mux.cond_var_index << ", ";
		////////////////////////////// mux's in0
		if(sel_always_mux.in0_preds_muxes.size() > 0) {
			assert(sel_always_mux.in0_preds_muxes.size() == 1);
			dbg_file << "MUX(" << sel_always_mux.in0_preds_muxes.at(0).cond_var_index << ", ";
			// in0
			assert(sel_always_mux.in0_preds_muxes.at(0).in0.size() == 1 && sel_always_mux.in0_preds_muxes.at(0).in0.at(0).size() == 1);
			if(sel_always_mux.in0_preds_muxes.at(0).in0.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in0_preds_muxes.at(0).in0.at(0).at(0).const_val << ", ";
			} else {							assert(sel_always_mux.in0_preds_muxes.at(0).in0.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in0_preds_muxes.at(0).in0.at(0).at(0).var_index + 1 << ", ";
			}

			// in1
			assert(sel_always_mux.in0_preds_muxes.at(0).in1.size() == 1 && sel_always_mux.in0_preds_muxes.at(0).in1.at(0).size() == 1);
			if(sel_always_mux.in0_preds_muxes.at(0).in1.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in0_preds_muxes.at(0).in1.at(0).at(0).const_val << ", ";
			} else {	
assert(sel_always_mux.in0_preds_muxes.at(0).in1.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in0_preds_muxes.at(0).in1.at(0).at(0).var_index + 1 << ", ";
			}

			dbg_file << "), ";

		} else {
			assert(sel_always_mux.in0.size() == 1 && sel_always_mux.in0.at(0).size() == 1);
			if(sel_always_mux.in0.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in0.at(0).at(0).const_val << ", ";
			} else {		
				assert(sel_always_mux.in0.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in0.at(0).at(0).var_index + 1 << ", ";
			}
		}
		////////////////////////////// end of mux's in0

		/////////////////////////////// mux's in1
		if(sel_always_mux.in1_preds_muxes.size() > 0) {
			assert(sel_always_mux.in1_preds_muxes.size() == 1);
			dbg_file << "MUX(" << sel_always_mux.in1_preds_muxes.at(0).cond_var_index << ", ";
			// in0
			assert(sel_always_mux.in1_preds_muxes.at(0).in0.size() == 1 && sel_always_mux.in1_preds_muxes.at(0).in0.at(0).size() == 1);
			if(sel_always_mux.in1_preds_muxes.at(0).in0.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in1_preds_muxes.at(0).in0.at(0).at(0).const_val << ", ";
			} else {							assert(sel_always_mux.in1_preds_muxes.at(0).in0.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in1_preds_muxes.at(0).in0.at(0).at(0).var_index + 1 << ", ";
			}

			// in1
			assert(sel_always_mux.in1_preds_muxes.at(0).in1.size() == 1 && sel_always_mux.in1_preds_muxes.at(0).in1.at(0).size() == 1);
			if(sel_always_mux.in1_preds_muxes.at(0).in1.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in1_preds_muxes.at(0).in1.at(0).at(0).const_val << ", ";
			} else {	
assert(sel_always_mux.in1_preds_muxes.at(0).in1.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in1_preds_muxes.at(0).in1.at(0).at(0).var_index + 1 << ", ";
			}

			dbg_file << "), ";

		} else {
			assert(sel_always_mux.in1.size() == 1 && sel_always_mux.in1.at(0).size() == 1);
			if(sel_always_mux.in1.at(0).at(0).is_const) {
				dbg_file << sel_always_mux.in1.at(0).at(0).const_val << ", ";
			} else {		
				assert(sel_always_mux.in1.at(0).at(0).var_index != -1);
				dbg_file << "C" << sel_always_mux.in1.at(0).at(0).var_index + 1;
			}
		}
		////////////////////////////// end of mux's in1
		dbg_file << ")";
	} 
	dbg_file << "\n\n";
	*/
}

std::vector<std::string> CircuitGenerator::get_sel_no_token(std::vector<std::string>* minterms_only_in_binary, std::vector<std::string>* minterms_plus_dont_cares_in_binary) {
	// extra sanity check, 
		// the minterms_only and minterms_plus_dont_cares can be = if there is no dont_cares in the expression.. this happens when the BB containing the sel currently under study is guaranteed to execute!! 
	assert(minterms_plus_dont_cares_in_binary->size() >= minterms_only_in_binary->size());
	std::vector<std::string> sel_no_token;

	// sel_no_token is the don't-care, so it's the difference between the two passed vectors!
	for(int i = 0; i < minterms_plus_dont_cares_in_binary->size(); i++) {
		auto pos = std::find(minterms_only_in_binary->begin(), minterms_only_in_binary->end(), minterms_plus_dont_cares_in_binary->at(i));
		// if the product is not found in minterm_only_in_binary, push it to sel_no_token!
		if(pos == minterms_only_in_binary->end()) {
			sel_no_token.push_back(minterms_plus_dont_cares_in_binary->at(i));
		} 
	} 

	// extra sanity check!!
	assert(sel_no_token.size() + minterms_only_in_binary->size() == minterms_plus_dont_cares_in_binary->size());

	return sel_no_token;
} 

ENode* CircuitGenerator::returnSELCond(std::vector<std::string> essential_sel_always, std::vector<int>* condition_variables, bool& is_negated, std::ofstream& dbg_file) {
	// this function is called after Shannon's in case there were no Muxes inserted by Shannon's, which means that the expression is made up of just a single condition variable!
	// Before, Shannon's, we sometimes run Quine_McCluskey on the expression (if we are generating SEL_ALWAYS to simplify it), or not (if we are generating SEL_NOTOKEN which doesn't need any further simplification!) If coming after_quine, you must have 1 entry and the rest are dashes!

	// extract the single condition and the NOT (if any) that should feed SEL
	dbg_file << "\n\n------------------------------------------\n\n";
	dbg_file << "I just entered returnSELCond!!\n";

	// sanity checks
	assert(essential_sel_always.size() == 1);
	assert(essential_sel_always.at(0).length() == condition_variables->size());

	// if your expression is coming out of QuineMcCluskey so could have the risk of having '-'
	int index = -1;	
	// you should have dashes at all entries and a 0 or 1 at just a single entry!!!
	// sanity check to verify this statement! I will do this by counting the number of dashes, they should be = condition_variables->size() - 1!
	int dashes_count = 0;
	for(int j = 0; j < essential_sel_always.at(0).length(); j++) {
		if(essential_sel_always.at(0).at(j) == '-') {
			dashes_count++;
		} else {
			index = j;
		}
	} 
	assert(dashes_count == condition_variables->size() - 1);

	assert(index != -1);

	// now identify the single non-dash field and find the BB index it represents
	int BB_index = condition_variables->at(index);

	dbg_file << "BBIndex is " << BB_index << "\n";

	// better identify the BB from its Idx!
	BBNode* pier_BB = nullptr;
	for(auto& bbnode : *bbnode_dag) {
		if(bbnode->Idx == BB_index) {
			pier_BB = bbnode;
			break;
		} 
	} 	
	assert(pier_BB != nullptr);

	dbg_file << "BB number is " << aya_getNodeDotbbID(pier_BB) << "\n";

	//search for LLVM's branch in this BB to get the icmp from it
	llvm::BranchInst* BI = nullptr;
	ENode* llvm_branch = nullptr;
	for(auto& enode_ : *enode_dag) {
		if(enode_->type == Branch_ && enode_->BB == pier_BB->BB) { 
			BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
			llvm_branch = enode_;
			break; // no need to continue looping, 1 llvm branch is enough
		}
	}

	dbg_file << "The LLVM branch node of this BB is called: " << getNodeDotNameNew(llvm_branch) << " of type " <<  getNodeDotTypeNew(llvm_branch) << " in BasicBlock number " << aya_getNodeDotbbID(pier_BB) << " has " << llvm_branch->CntrlPreds->size() << " predecessors\n";

	assert(BI != nullptr);
	assert(llvm_branch != nullptr);
	assert(!BI->isUnconditional());

	dbg_file << "The icmp node that will provide the condition for SEL_ALWAYS is: " << getNodeDotNameNew(llvm_branch->CntrlPreds->at(0)) << " of type " <<  getNodeDotTypeNew(llvm_branch->CntrlPreds->at(0)) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(llvm_branch->CntrlPreds->at(0)->BB)) << "\n";

	if(essential_sel_always.at(0).at(index) == '0') {
		is_negated = true;
	} else {
		is_negated = false;
	}
	
	dbg_file << "I'm leaving returnSELCond!!\n\n";
	//return is_negated;
	return llvm_branch->CntrlPreds->at(0);
}

ENode* CircuitGenerator::addOneNewMux(Shannon_Expansion::MUXrep sel_always_mux, BasicBlock* target_bb) {
	// this function creates a new ENode Phi_n node and makes it a MUX and connects it to its SEL.
		// However, it doesn't connect the 2 inputs of the MUX! Just the SEL input, that's it!
	ENode* new_mux = new ENode(Phi_n, "phi", target_bb);
	new_mux->id = phi_id++;
	enode_dag->push_back(new_mux);
	new_mux->isMux = true;

	// Aya: 15/06/2022: added the following flag so that we mark every new MUX (created during Shannon's expansion)
	new_mux->is_mux_for_cond = true;

	/*
		The convention of Muxes is SEL should be at .(0), in0 at .(1) and in2 at .(2)
	*/

	// check the Mux's SEL, cond_var_index field inside the MUX object holds the correct index of the BB that we should use its condition!!
	// better identify the BB from its Idx!
	BBNode* new_mux_sel_BB = nullptr;
	for(auto& bbnode : *bbnode_dag) {
		if(bbnode->Idx == sel_always_mux.cond_var_index) {
			new_mux_sel_BB = bbnode;
			break;
		} 
	} 	
	assert(new_mux_sel_BB != nullptr);

	// we need the condition of the BB, so search for icmp
	llvm::BranchInst* BI = nullptr;
	ENode* llvm_branch = nullptr;
	for(auto& enode_ : *enode_dag) {
		if(enode_->type == Branch_ && enode_->BB == new_mux_sel_BB->BB) { 
			BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
			llvm_branch = enode_;
			break; // no need to continue looping, 1 llvm branch is enough
		}
	}
	assert(BI != nullptr);
	assert(llvm_branch != nullptr);
	assert(!BI->isUnconditional());

	// push the select of the new MUX
	new_mux->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
	llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(new_mux);

	return new_mux;
}

ENode* CircuitGenerator::identifyNewMuxinput(Shannon_Expansion::MUXrep sel_always_mux, int in_index, BasicBlock* target_bb, bool& is_negated_input) {
	// second identify the input at the passed index of that new_mux 
	std::vector<std::vector<Shannon_Expansion::boolVariable>> in;
	std::vector<Shannon_Expansion::MUXrep> in_preds_muxes;

	ENode* in_node = nullptr;
	 
	if(in_index == 0) {
		in = sel_always_mux.in0;
		in_preds_muxes = sel_always_mux.in0_preds_muxes;
	} else {
		assert(in_index == 1); 
		in = sel_always_mux.in1;
		in_preds_muxes = sel_always_mux.in1_preds_muxes;
	}

	// first check the possibility that it could be another new MUX
	if(in_preds_muxes.size() > 0) {
		// this means it is a MUX, so we should create a new ENode for it!
		in_node = addOneNewMux(in_preds_muxes.at(0), target_bb);

		// since I have just created a new MUX, I need to check if the inputs of this mux are muxes too or are just a single condition or a constant! SO, recursively call this function again!
		bool is_in_negated_in0, is_in_negated_in1;
		ENode* in_node_mux_in0 = identifyNewMuxinput(in_preds_muxes.at(0), 0, target_bb, is_in_negated_in0);	
		ENode* in_node_mux_in1 = identifyNewMuxinput(in_preds_muxes.at(0), 1, target_bb, is_in_negated_in1);	

		assert(is_in_negated_in0 == false || is_in_negated_in0 == true);
		assert(is_in_negated_in1 == false || is_in_negated_in1 == true);

		in_node->CntrlPreds->push_back(in_node_mux_in0);
		in_node_mux_in0->CntrlSuccs->push_back(in_node);

		in_node->CntrlPreds->push_back(in_node_mux_in1);
		in_node_mux_in1->CntrlSuccs->push_back(in_node);

		// since there are chances that the inputs of the new MUX are negated, we need to label it as an advanced_component and fill its is_negated_input vector
		in_node->is_advanced_component = true; 

		// the condition of the MUX
		in_node->is_negated_input->push_back(false);
		in_node->is_negated_input->push_back(is_in_negated_in0);
		in_node->is_negated_input->push_back(is_in_negated_in1);

		assert(in_node->CntrlPreds->size() == in_node->is_negated_input->size());

		/////////////////////////////////////////////////////////////
		
		// the input is an output of another mux, and as of now, I never need to negate the output of another MUX!!
		is_negated_input = false; 

	} else {
		// this input isn't a mux! Specifically, it should be a single product made up of a single condition or a single constant!! Check in vector!

		// sanity checks
		assert(in.size() == 1);
		assert(in.at(0).size() == 1);

		Shannon_Expansion::boolVariable single_cond = in.at(0).at(0);
		// 1st check if it is a constant
		if(single_cond.is_const) {
			assert(single_cond.const_val != -1);
			// create a constant node
			std::string cst_name;
			if(single_cond.const_val == 0) {
				cst_name = std::to_string(0);
			} else {
				cst_name = std::to_string(1);
			} 
			
			in_node = new ENode(Cst_, cst_name.c_str(), target_bb);
			// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
			in_node->cstValue = single_cond.const_val;
			enode_dag->push_back(in_node);
			in_node->id = cst_id++;

			in_node->is_const_for_cond = true;

			// this new constant needs to be triggered with SOURCE!!
			ENode* source_node = new ENode(Source_, "source", in_node->BB);
			source_node->id    = source_id++;
			in_node->JustCntrlPreds->push_back(source_node);
			source_node->JustCntrlSuccs->push_back(in_node);
			enode_dag->push_back(source_node);

			
			/*for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
				if(cnt_node->type == Start_) {
					in_node->JustCntrlPreds->push_back(cnt_node);
					cnt_node->JustCntrlSuccs->push_back(in_node);
					break; // found the start node, no need to continue looping
				}
			}*/

			// the input is a constant, and as of now, I never need to negate the output of a constant!!
			is_negated_input = false; 

		} else {
			assert(single_cond.var_index != -1);
			BBNode* in_mux_BB = nullptr;
			for(auto& bbnode : *bbnode_dag) {
				if(bbnode->Idx == single_cond.var_index) {
					in_mux_BB = bbnode;
					break;
				} 
			} 	
			assert(in_mux_BB != nullptr);

			// we need the condition of the BB, so search for icmp
			llvm::BranchInst* BI = nullptr;
			ENode* llvm_branch = nullptr;
			for(auto& enode_ : *enode_dag) {
				if(enode_->type == Branch_ && enode_->BB == in_mux_BB->BB) { 
					BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
					llvm_branch = enode_;
					break; // no need to continue looping, 1 llvm branch is enough
				}
			}
			assert(BI != nullptr);
			assert(llvm_branch != nullptr);
			assert(!BI->isUnconditional());

			// the input is an LLVM branch condition, which could be negated so I need to check its field!!
			is_negated_input = single_cond.is_negated; 

			in_node = llvm_branch->CntrlPreds->at(0);
		}
	}
	
	assert(in_node != nullptr);

	assert(is_negated_input == true || is_negated_input == false); 

	return in_node;
}

ENode* CircuitGenerator::addNewMuxes_wrapper(Shannon_Expansion::MUXrep sel_always_mux, BasicBlock* target_bb) {
	// this function creates a new ENode Phi_n node and makes it a MUX!, Any of its 2 inputs may or may not be also other MUXes

	// keep in mind that my Shannon's model currently supports up to 2 layers of MUXes ONLY (i.e., a MUX feeding another MUX ONLY)!!

	// creates a new ENode of type Mux, connects its SEL and returns a reference to it
	ENode* new_mux = addOneNewMux(sel_always_mux, target_bb);

	// the second parameter of the following function indicates the index which mux input we need to calculate now
	bool is_negated_in0, is_negated_in1;
	ENode* new_mux_in0 = identifyNewMuxinput(sel_always_mux, 0, target_bb, is_negated_in0);	
	ENode* new_mux_in1 = identifyNewMuxinput(sel_always_mux, 1, target_bb, is_negated_in1);

	assert(is_negated_in0 == true || is_negated_in0 == false);	
	assert(is_negated_in1 == true || is_negated_in1 == false);	
	// AYA: 24/02/2022: new_mux_in0 and new_mux_in1 could be one of the following:
	// 1) another MUX
	// 2) a constant
	// 3) an icmp node which outputs the condition of an llvm branch
	// Regardless of what they are, there are chances that they should be negated before connecting them to the MUX.. In this case, is_negated_in0 and is_negated_in1 hold the info of whether they should be negated or not! And we should mark the new_mux enode as an is_advanced_component to check the possibility of negating its inputs in printDot

	new_mux->CntrlPreds->push_back(new_mux_in0);
	new_mux_in0->CntrlSuccs->push_back(new_mux);

	new_mux->CntrlPreds->push_back(new_mux_in1);
	new_mux_in1->CntrlSuccs->push_back(new_mux);

	new_mux->is_advanced_component = true;
	// the condition of the MUX is never negated, as of now
	new_mux->is_negated_input->push_back(false);

	// the other 2 inputs of the MUX are dependent on what the function identifyNewMuxinput returned
	new_mux->is_negated_input->push_back(is_negated_in0);
	new_mux->is_negated_input->push_back(is_negated_in1);

	assert(new_mux->CntrlPreds->size() == new_mux->is_negated_input->size());

	// we currently have nothing as a Successor of this MUX, we will manage this outside the function!

	return new_mux;
} 

bool CircuitGenerator::are_equal_paths(std::vector<std::vector<pierCondition>>* sum_of_products_ptr, int i, int j) {
	if(sum_of_products_ptr->at(i).size() != sum_of_products_ptr->at(j).size())
		return false;
	assert(sum_of_products_ptr->at(i).size() == sum_of_products_ptr->at(j).size());
	
	for(int k = 0; k < sum_of_products_ptr->at(i).size(); k++) {
		if(sum_of_products_ptr->at(i).at(k) != sum_of_products_ptr->at(j).at(k)) {
			return false;
		} 

	} 

	return true;

}

void CircuitGenerator::delete_repeated_paths(std::vector<std::vector<pierCondition>>* sum_of_products_ptr) {
	for(int i = 0; i < sum_of_products_ptr->size(); i++) {
		// compare 1 path to all other paths 
		for(int j = i+1; j < sum_of_products_ptr->size(); j++) {
			if(are_equal_paths(sum_of_products_ptr, i, j)) {
				// delete the jth path
				auto pos = sum_of_products_ptr->begin() + j;
				assert(pos != sum_of_products_ptr->end());
				sum_of_products_ptr->erase(pos);
			} 

		}
	}
	
}

// this is the one we call in our main mycfgpass.cpp
void CircuitGenerator::setMuxes_nonLoop() {
	std::ofstream dbg_file;
    dbg_file.open("f0_f1_Merge_check.txt");

	std::ofstream dbg_file_2;
    dbg_file_2.open("shannon's_debug.txt");

	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions;
	std::vector<std::vector<pierCondition>> sum_of_products;
	std::vector<pierCondition> one_product;

	BBNode* startBB = bbnode_dag->at(0);
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(auto & enode : *enode_dag) {
		merged_paths_functions.clear();
		if((enode->type == Phi_ || enode->type == Phi_n) && (!enode->isMux ) && !enode->is_merge_init ) {
			std::vector<ENode*>* enode_Preds;
			if(enode->type == Phi_ || enode->type == Phi_n) {
				enode_Preds = enode->CntrlPreds;
			} 

			// Aya: 15/06/2022: removed the logic of converting the Phis used for loop regeneration or loop-carried dependencies to Muxes
				// since it is now done inside addSuppress!!

			// Merges that are at a loop header and fed through a backward edge.
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode_Preds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode_Preds->at(1)->BB)->Idx) ) {	
				continue;
			} else {

				dbg_file << "\n\n---------------------------------------------------\n";
				dbg_file << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlPreds->size() << " predecessors\n";

				dbg_file_2 << "\n\n---------------------------------------------------\n";
				dbg_file_2 << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlPreds->size() << " predecessors\n";

				// loop over the producers of this Merge component
				for(int i = 0; i < enode->CntrlPreds->size(); i++) {
					dbg_file << "\nConsidering producer node name " << getNodeDotNameNew(enode->CntrlPreds->at(i)) << " of type " <<  getNodeDotTypeNew(enode->CntrlPreds->at(i)) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";

					BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
					bbnode_dag->push_back(virtualBB);
					BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);
					// insert a virtualBB at the edge corresponding to this producer
					insertVirtualBB(enode, virtualBB, llvm_predBB);

					// find f_i which is the conditions of pierBBs in the paths between the START and the virtualBB we just inserted, 
					////////////////////////// clearing the datastructures 
					sum_of_products.clear();

					// find all the paths between the startBB and the virtualBB
					/////////////////// variables needed inside enumerate_paths
					bool *visited = new bool[bbnode_dag->size()];
					int *path = new int[bbnode_dag->size()];
					int path_index = 0; 
					// needed to hold the enumerated paths 
					std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
					// 2nd enumerate all the paths between the current producer and this
					// reseting the data structures for a new prod_con pair
					for(int i = 0; i < bbnode_dag->size(); i++){
						visited[i] = false;
					}
					final_paths->clear();
					/////////////////////////// end of clearing the datastructures

					enumerate_paths_dfs_start_to_end(startBB, virtualBB, visited, path, path_index, final_paths);

					dbg_file << "Found " << final_paths->size() << " paths\n";
					// loop over the final_paths and identify the pierBBs with their conditions

					for(int kk = 0; kk < final_paths->size(); kk++) {
						dbg_file << "Printing path number " << kk << " details\n";
						for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
							dbg_file << final_paths->at(kk).at(hh) + 1 << " -> ";
						}
						dbg_file << "\n";

						// identifying the pierBBs in 1 path
						std::vector<int>* pierBBs = new std::vector<int>;
						find_path_piers(final_paths->at(kk), pierBBs, virtualBB, endBB);

						///// Debugging to check the pierBBs 
						dbg_file << "\n Printing all the piers of that path!\n";
						for(int u = 0; u < pierBBs->size(); u++) {
							dbg_file << pierBBs->at(u) + 1 << ", ";
						}	
						dbg_file << "\n\n";
						/////////////////////////////////////
						// loop on piers of that path to construct a product out of them, then push this product to the sum_of_products before moving to a new path
						one_product.clear();
						for(int u = 0; u < pierBBs->size(); u++) {
							pierCondition cond;
							cond.pierBB_index = pierBBs->at(u);

							int next_pierBB_index;
							if(u == pierBBs->size()-1) {
								next_pierBB_index = BBMap->at(llvm_predBB)->Idx;
							} else {
								next_pierBB_index = pierBBs->at(u+1);
							} 
							if(is_negated_cond(pierBBs->at(u), next_pierBB_index, enode->BB ,dbg_file)) {
								cond.is_negated = true;
							} else {
								cond.is_negated = false;
							}
							one_product.push_back(cond);
						}
						sum_of_products.push_back(one_product);

					}  // end of loop on all paths between START and the virtualBB

					// we should have 1 product per path!
					assert(sum_of_products.size() == final_paths->size());

					/// PRINTING the sum_of_products of all paths to that virtualBB
					dbg_file << "\nPrinting the sum of products from START to that virtualBB!\n";
					for(int u = 0; u < sum_of_products.size(); u++) {
						for(int v = 0; v < sum_of_products.at(u).size(); v++) {
							if(sum_of_products.at(u).at(v).is_negated)
								dbg_file << "not C";
							else
								dbg_file << "C";

							dbg_file << sum_of_products.at(u).at(v).pierBB_index +1 << " & ";
						}
						dbg_file << " + ";
					}
					dbg_file << "\n\n";
					
					///////////WHAT IS ENCLOSED HERE IS WRONG! LEAVE IT COMMENTED!!
					///// eliminating any redundant products in the sum_of_products, if any
					/*int ii = 0;
					int jj = 0;
					while(ii < sum_of_products.size()) {
						jj = ii+1;
						while(jj < sum_of_products.size()) {
							if(should_merge_paths(sum_of_products.at(ii), sum_of_products.at(jj))) 		{
								merge_paths(sum_of_products, ii, jj);
							} else {
								// if paths are merged, we get a new path at jj, so don't increment ot to compare with that new path!!
								jj++;
							} 
						}
						ii++;
					} */
					///////////////////////////////////////////////////
					std::vector<std::vector<pierCondition>>* sum_of_products_ptr = &sum_of_products;
					delete_repeated_paths(sum_of_products_ptr);


					////////////////////////////////////////////////////
					removeVirtualBB(enode, virtualBB, llvm_predBB); 	
					merged_paths_functions.push_back(sum_of_products);
				}  // end of loop on predecessors

				// DEBUGGING.. check the 2 functions (f0 and f1) of this Merge component!
				dbg_file << "The current Merge node the following functions: \n\t";
				for(int iter = 0; iter < merged_paths_functions.size(); iter++) {
					dbg_file << "f" << iter << " = ";
					for(int y = 0; y < merged_paths_functions.at(iter).size(); y++) {
						// take the conditions from a product by product
						for(int z = 0; z <  merged_paths_functions.at(iter).at(y).size(); z++) {
							if(merged_paths_functions.at(iter).at(y).at(z).is_negated)
								dbg_file << "not C";
							else
								dbg_file << "C";

							dbg_file << merged_paths_functions.at(iter).at(y).at(z).pierBB_index +1 << " & ";
						}
						dbg_file << " + ";
					}
					dbg_file << "\n\t";
				}	
				/////////////////////// end of debugging f0 and f1 of that phi enode

				// Aya: 22/08/2022: added the following to support the case of converting a 3 input Merge into a MUX where 2 of the inputs are one same Branch
				if(merged_paths_functions.size() > 2) {
					// for now, I'm testing it assuming a maximum of 3 inputs
					assert(merged_paths_functions.size() == 3);
					assert(enode->CntrlPreds->size() == 3);
					// for now, I'm supporting only the case where one enode is serving as two predecessors of the Merge!
					int idx_to_delete = -1;
					int idx_to_keep = -1;
					if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(1)) {
						idx_to_delete = 1;
						idx_to_keep = 0;
					} else {
						if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(2)) {
							// push the products of merged_paths_functions.at(2) to merged_paths_functions.at(0) and erase merged_paths_functions.at(2)
							idx_to_delete = 2;
							idx_to_keep = 0;
						} else {
							assert(enode->CntrlPreds->at(1) == enode->CntrlPreds->at(2));
							// push the products of merged_paths_functions.at(2) to merged_paths_functions.at(1) and erase merged_paths_functions.at(2)
							idx_to_delete = 2;
							idx_to_keep = 1;
						}
					}

					assert(idx_to_delete != -1 && idx_to_keep != -1);

					// do the necessary modifications
					// push the products of merged_paths_functions.at(idx_to_delete) to merged_paths_functions.at(idx_to_keep) and erase merged_paths_functions.at(idx_to_delete)
					for(int kk = 0; kk < merged_paths_functions.at(idx_to_delete).size(); kk++) {
						std::vector<pierCondition> temp_one_product;
						// loop over the conditions constituting one product
						for(int ll = 0; ll < merged_paths_functions.at(idx_to_delete).at(kk).size(); ll++) {
							temp_one_product.push_back(merged_paths_functions.at(1).at(kk).at(ll));
						}
						merged_paths_functions.at(idx_to_keep).push_back(temp_one_product);
					}
					// erase the merged_paths_functions.at(idx_to_delete) from the merged_paths_functions vector
					auto elem_to_remove = merged_paths_functions.begin() + idx_to_delete;  // we want to erase the element at index 1
					assert(elem_to_remove != merged_paths_functions.end());
			        merged_paths_functions.erase(elem_to_remove);		

			        // sanity check
			        assert(merged_paths_functions.size() == 2);
				}
				
				// checking what we have done by printing to file
				// DEBUGGING.. check the 2 functions (f0 and f1) of this Merge component!
				dbg_file << "\n\tPrinting the details of the Merge after modifying the functions to be the correct f0 and f1 onlyy!!\n";
				for(int iter = 0; iter < merged_paths_functions.size(); iter++) {
					dbg_file << "f" << iter << " = ";
					for(int y = 0; y < merged_paths_functions.at(iter).size(); y++) {
						// take the conditions from a product by product
						for(int z = 0; z <  merged_paths_functions.at(iter).at(y).size(); z++) {
							if(merged_paths_functions.at(iter).at(y).at(z).is_negated)
								dbg_file << "not C";
							else
								dbg_file << "C";

							dbg_file << merged_paths_functions.at(iter).at(y).at(z).pierBB_index +1 << " & ";
						}
						dbg_file << " + ";
					}
					dbg_file << "\n\t";
				}	
				/////////////////////// end of debugging f0 and f1 of that phi enode

				////////////////////////////// End of stuff added on 22/08/2022	
				
				// extract the sum of minterms (i.e., f1 + don't-cares) in a binary string form
				std::vector<std::string>* minterms_only_in_binary = new std::vector<std::string>;
				std::vector<std::string>* minterms_plus_dont_cares_in_binary = new std::vector<std::string>;
				std::vector<int>* condition_variables = new std::vector<int>;

 				int number_of_dont_cares = get_binary_sop(merged_paths_functions, minterms_only_in_binary, minterms_plus_dont_cares_in_binary, condition_variables, dbg_file);

 				// Aya: 22/08/2022:Printing for debugging!!!
 				dbg_file << "\n\n**************************************************\n\n";
 				dbg_file << "\n\t Printing the minterms_only_in_binary expression \n";
 				for(int mm = 0; mm < minterms_only_in_binary->size(); mm++) {
 					dbg_file << minterms_only_in_binary->at(mm) << "\n";
 				}
 				dbg_file << "\n\t Printing the minterms_plus_dont_cares_in_binary expression \n";
 				for(int mm = 0; mm < minterms_plus_dont_cares_in_binary->size(); mm++) {
 					dbg_file << minterms_plus_dont_cares_in_binary->at(mm) << "\n";
 				}
 				dbg_file << "\n\n**************************************************\n\n";
 				/////////////////////////// End of printing for debugging!

				// we should have at least a single minterm!
				assert(minterms_plus_dont_cares_in_binary->size() > 0);
				// debug_get_binary_sop(merged_paths_functions, dbg_file);

				// call the initialization object of Quine-McCLuskey
				Quine_McCluskey boolean_simplifier;
				int number_of_bits = minterms_plus_dont_cares_in_binary->at(0).length();
				int number_of_minterms = minterms_plus_dont_cares_in_binary->size() - number_of_dont_cares;
				boolean_simplifier.initialize(number_of_bits, number_of_minterms, number_of_dont_cares, minterms_only_in_binary, minterms_plus_dont_cares_in_binary);

				boolean_simplifier.solve(dbg_file);
				std::string bool_expression_name = "SEL_ALWAYS";
				boolean_simplifier.displayFunctions(dbg_file, bool_expression_name);
				std::vector<std::string> essential_sel_always =
						boolean_simplifier.getEssentialPrimeImpl();
				

				std::vector<std::vector<pierCondition>> sel_always = essential_impl_to_pierCondition(essential_sel_always);

				///////// debugging to check the sel_always expression
				dbg_file << "\n\nSEL_ALWAYS = ";
				for(int u = 0; u < sel_always.size(); u++) {
					for(int v = 0; v < sel_always.at(u).size(); v++) {
						if(sel_always.at(u).at(v).is_negated) 
							dbg_file << "not C";
						else
							dbg_file << "C";

						dbg_file << sel_always.at(u).at(v).pierBB_index +1 << " & ";
					}
					dbg_file << " + "; 
				} 
				dbg_file << "\n\n";
				
				///////////////////////////////////////////////////////

				Shannon_Expansion shannon;
				// apply Shannon's on the essential_sel_always!
				Shannon_Expansion::MUXrep sel_always_mux;
				sel_always_mux = shannon.apply_shannon(essential_sel_always, condition_variables, dbg_file_2);
				// corner case is to have the in0 and in1 of the resulting_mux of size 0 indicating that you don't need a MUX, and I'm handling it in the above function!

				////// debugging output
				// printShannons_mux(dbg_file, sel_always_mux);
				ENode* sel_always_enode = nullptr; // can be a MUX node that I create or an already existing BB condition
				bool is_sel_always_negated = false; // indicates if we should negate the SEL_always signal before connecting it
				if(sel_always_mux.is_valid) {
					// then SEL_ALWAYS is calculated by one or more MUXes
					sel_always_enode = addNewMuxes_wrapper(sel_always_mux, enode->BB);
					assert(sel_always_enode != nullptr);
					is_sel_always_negated = false;
				} else {
					// Shannon's expnasion didn't return any MUX, then SEL_ALWAYS must be just a single condition of an existing BB. It may or may not be an inverter..
					bool is_negated;
					sel_always_enode = returnSELCond(essential_sel_always, condition_variables, is_negated, dbg_file_2);
					///// the follwoing is for debugging purposes only!!
					assert(sel_always_enode != nullptr);
					assert(is_negated == true || is_negated == false);
					dbg_file_2 << "\n\n--------------------------------\n";
					dbg_file_2 << "It turns out that SEL_ALWAYS is composed of just a single condition coming from the enode called: " << getNodeDotNameNew(sel_always_enode) << " of type " <<  getNodeDotTypeNew(sel_always_enode)  << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(sel_always_enode->BB));
					if(is_negated) {
						dbg_file_2 << " and it should be inverted!!\n";
					} else {
						dbg_file_2 << " and it should NOT be inverted!!\n";
					}

					is_sel_always_negated = is_negated;
				}
					
				// Then, this sel_always_enode should be connected to the SEL input of the current PHI either directly or with a Branch in the middle.. The decision is based on whether SEL_NO_TOKEN is empty or not!!

				// To generate sel_no_token, I make use of the minterms and dont_cares I got above!
				dbg_file_2 << "\n\nI just calculated the SEL_NO_TOKEN and will pass it to Shannon's!\n";
				std::vector<std::string> sel_no_token = get_sel_no_token(minterms_only_in_binary, minterms_plus_dont_cares_in_binary);
				
				/////////////////// Run Quine_McCluskey for SEL_NOTOKEN
				// call the initialization object of Quine-McCLuskey
				Quine_McCluskey boolean_simplifier_2;
				int number_of_bits_2 = minterms_plus_dont_cares_in_binary->at(0).length();
				int number_of_minterms_2 = sel_no_token.size();
				int number_of_dont_cares_2 = 0;
				boolean_simplifier_2.initialize(number_of_bits_2, number_of_minterms_2, number_of_dont_cares_2, &sel_no_token, &sel_no_token);
				boolean_simplifier_2.solve(dbg_file);
				std::string bool_expression_name_2 = "SEL_NOTOKEN";
				boolean_simplifier_2.displayFunctions(dbg_file, bool_expression_name_2);
				std::vector<std::string> essential_sel_notoken =
						boolean_simplifier_2.getEssentialPrimeImpl();
				////////////////////////////////////////////////

				Shannon_Expansion::MUXrep sel_no_token_mux;
				sel_no_token_mux = shannon.apply_shannon(essential_sel_notoken, condition_variables, dbg_file_2);

				// We have 3 possibilities for sel_notoken
				// 1) Essential_sel_notoken is empty, in this case we don't need a branch!
				// 2) Shannon didn't add any MUX meaning there just a single condition in Essential_sel_notoken
				// 3) Shannon returned a normal MUX
				if(essential_sel_notoken.size() == 0) {
					// don't need a branch, connet sel_always_enode directly to the Phi under study (enode)
					enode->isMux = true;

					// TODO: check that the way I'm deleting the extra replicated predecessor is still compatible with in0 when SEL = 0 and in1 when SEL = 1
					////// Aya: 22/08/2022: added the following to tackle a very special case of the Merge having one enode twice in its successors
					if(enode->CntrlPreds->size() == 3) {
						int idx_to_delete;
						// for now, I'm only supporting the case when two of the successors are the same identical enode!
						if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(1)) {
							// erase 1
							idx_to_delete = 1;
						} else {
							if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(2)) {
								// erase 2
								idx_to_delete = 2;
							} else {
								assert(enode->CntrlPreds->at(1) == enode->CntrlPreds->at(2));
								// erase 2
								idx_to_delete = 2;
							}
						}

				        // 1st search for the enode in the successors of the repeated predecessor node to delete it from there
				        if(enode->CntrlPreds->at(idx_to_delete)->type == Branch_n) {
				        	// find out if the enode is in its true or false successors
				        	auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->begin(), enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->end(), enode);
							if(pos_ != enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->end()) {
								enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->erase(pos_);
							} else {
								auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->begin(), enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->end(), enode);
								// must find it in the false succs otherwise there is something wrong
								assert(pos_ != enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->end());
								enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->erase(pos_);
							}
							
				        } else {
				        	// extra sanity check: it can't be a Branch_ or a Branch_c
				        	assert(enode->CntrlPreds->at(idx_to_delete)->type != Branch_c && enode->CntrlPreds->at(idx_to_delete)->type != Branch_);

					        auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->begin(), enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->end(), enode);
							assert(pos_ != enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->end());
							enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->erase(pos_);
				        }


						// 2nd delete the extra replicated predecessor from the CntrlPreds of the enode
				        auto elem_to_remove = enode->CntrlPreds->begin() + idx_to_delete;  // we want to erase the element at index 1
						assert(elem_to_remove != enode->CntrlPreds->end());
				        enode->CntrlPreds->erase(elem_to_remove);	

				        // sanity check to make sure we did what we desired
				        assert(enode->CntrlPreds->size() == 2);
					} /////// end of what I added on 22/08/2022

					// do the normal business
					// the select signal of the mux must be .at(0), so do re-adjustments!
					enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
					enode->CntrlPreds->at(0) = sel_always_enode;
					sel_always_enode->CntrlSuccs->push_back(enode);

					// sel_always_enode should be negated or not depending on is_sel_always_negated
					enode->is_negated_input->push_back(is_sel_always_negated); 
					// the other 2 inputs of the enode MUX shouldn't be negated!!
					enode->is_negated_input->push_back(false);
					enode->is_negated_input->push_back(false);

					// this is necessary so that the printDot checks if the inputs of the enode should eb negated or not
					enode->is_advanced_component = true;

					// sanity check
					assert(enode->CntrlPreds->size() == enode->is_negated_input->size());

				} else {
					// need to add a new branch, but let's first identify its condition!
					ENode* sel_notoken_enode = nullptr;
	
					bool is_sel_notoken_negated = false; // indicates if we should negate the SEL_NOTOKEN signal before connecting it

					if(sel_no_token_mux.is_valid) {
						// then SEL_MUX is calculated by one or more MUXes
						sel_notoken_enode = addNewMuxes_wrapper(sel_no_token_mux, enode->BB);
						assert(sel_notoken_enode != nullptr);
						
						is_sel_notoken_negated = false;

					} else {
						bool is_negated_2;
						sel_notoken_enode = returnSELCond(essential_sel_notoken, condition_variables, is_negated_2, dbg_file_2);
						assert(is_negated_2 == true || is_negated_2 == false);

						is_sel_notoken_negated = is_negated_2;

					}
					//////////////////
					assert(sel_notoken_enode != nullptr);

					// add a new branch
					ENode* notoken_branch = new ENode(Branch_n, "branch", enode->BB);
					notoken_branch->id = branch_id++;
					enode_dag->push_back(notoken_branch);
					// Aya: added this to help in removing redundant branches
					notoken_branch->producer_to_branch = sel_always_enode;
					notoken_branch->true_branch_succs = new std::vector<ENode*>;
					notoken_branch->false_branch_succs = new std::vector<ENode*>;
					notoken_branch->true_branch_succs_Ctrl = new std::vector<ENode*>;
					notoken_branch->false_branch_succs_Ctrl = new std::vector<ENode*>;

					// - Data should be .at(0) and condition should be .at(1)
					sel_always_enode->CntrlSuccs->push_back(notoken_branch);
					notoken_branch->CntrlPreds->push_back(sel_always_enode);

					// the condition of the branch is sel_notoken_enode
					sel_notoken_enode->CntrlSuccs->push_back(notoken_branch);
					notoken_branch->CntrlPreds->push_back(sel_notoken_enode);

					// check if the data input of the branch is negated or not dependent on is_sel_always_negated (since the input data of the MUX is SEL_ALWAYS!)
					notoken_branch->is_negated_input->push_back(is_sel_always_negated); 
					// check if the cond input of the branch is negated or not dependent on is_sel_notoken_negated (since the input cond of the MUX is SEL_NOTOKEN!)
					notoken_branch->is_negated_input->push_back(is_sel_notoken_negated);

					notoken_branch->is_advanced_component = true;

					// sanity check
					assert(notoken_branch->CntrlPreds->size() == notoken_branch->is_negated_input->size());

					// TODO: check that the way I'm deleting the extra replicated predecessor is still compatible with in0 when SEL = 0 and in1 when SEL = 1
					////// Aya: 22/08/2022: added the following to tackle a very special case of the Merge having one enode twice in its successors
					if(enode->CntrlPreds->size() == 3) {
						int idx_to_delete;
						// for now, I'm only supporting the case when two of the successors are the same identical enode!
						if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(1)) {
							// erase 1
							idx_to_delete = 1;
						} else {
							if(enode->CntrlPreds->at(0) == enode->CntrlPreds->at(2)) {
								// erase 2
								idx_to_delete = 2;
							} else {
								assert(enode->CntrlPreds->at(1) == enode->CntrlPreds->at(2));
								// erase 2
								idx_to_delete = 2;
							}
						}

						// 1st search for the enode in the successors of the repeated predecessor node to delete it from there
				        if(enode->CntrlPreds->at(idx_to_delete)->type == Branch_n) {
				        	// find out if the enode is in its true or false successors
				        	auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->begin(), enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->end(), enode);
							if(pos_ != enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->end()) {
								enode->CntrlPreds->at(idx_to_delete)->true_branch_succs->erase(pos_);
							} else {
								auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->begin(), enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->end(), enode);
								// must find it in the false succs otherwise there is something wrong
								assert(pos_ != enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->end());
								enode->CntrlPreds->at(idx_to_delete)->false_branch_succs->erase(pos_);
							}
							
				        } else {
				        	// extra sanity check: it can't be a Branch_ or a Branch_c
				        	assert(enode->CntrlPreds->at(idx_to_delete)->type != Branch_c && enode->CntrlPreds->at(idx_to_delete)->type != Branch_);

					        auto pos_ = std::find(enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->begin(), enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->end(), enode);
							assert(pos_ != enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->end());
							enode->CntrlPreds->at(idx_to_delete)->CntrlSuccs->erase(pos_);
				        }


						// 2nd delete the extra replicated predecessor from the CntrlPreds of the enode
						auto elem_to_remove = enode->CntrlPreds->begin() + idx_to_delete;  // we want to erase the element at index 1
						assert(elem_to_remove != enode->CntrlPreds->end());
				        enode->CntrlPreds->erase(elem_to_remove);	

				        // sanity check to make sure we did what we desired
				        assert(enode->CntrlPreds->size() == 2);
					} /////// end of what I added on 22/08/2022

					/////////////////////// connect the branch to the SEL of the given phi enode
					enode->isMux = true;
					// the select signal of the mux must be .at(0), so do re-adjustments!
					enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
					enode->CntrlPreds->at(0) = notoken_branch;

					// WHEN we are sure that the SEL of the enode is fed through a branch, we don't need to worry about negating any of the inputs of the enode!
	
					// the SEL of the MUX should be at the false side of the branch (i.e., when sel_notoken is false)!
					notoken_branch->false_branch_succs->push_back(enode);
					
				} 
				
			} // end of phi_type not loop_header if condition

		}  // end of phi_type if condition

	}  // end of loop over all enodes

}


// I'M CURRENTLY NO LONGER USING THE FOLLOWING FUNCTION!!!!
void CircuitGenerator::new_setMuxes() {
	std::ofstream dbg_file;
    dbg_file.open("f0_f1_Merge_check.txt");

	std::ofstream dbg_file_2;
    dbg_file_2.open("shannon's_debug.txt");

	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions;
	std::vector<std::vector<pierCondition>> sum_of_products;
	std::vector<pierCondition> one_product;

	BBNode* startBB = bbnode_dag->at(0);
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(auto & enode : *enode_dag) {
		merged_paths_functions.clear();
		if((enode->type == Phi_ || enode->type == Phi_n) && (!enode->isMux ) && !enode->is_merge_init ) {
			std::vector<ENode*>* enode_Preds;
			if(enode->type == Phi_ || enode->type == Phi_n) {
				enode_Preds = enode->CntrlPreds;
			} //else {
			//	assert(enode->type == Phi_c);
			//	enode_Preds = enode->JustCntrlPreds;
			//} 


			// Aya: 15/06/2022: removed the logic of converting the Phis used for loop regeneration or loop-carried dependencies to Muxes
				// since it is now done inside addSuppress!!

			// Merges that are at a loop header and fed through a backward edge.
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode_Preds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode_Preds->at(1)->BB)->Idx) ) {	
				continue;
			// hack of adding a MERGE
				// IMP TO NOTE: THIS WILL ONLY WORK IF THERE IS A SINGLE LOOP EXIT !!!!
			////////////////////////////////////////////////////////////////////////
				/*if(enode->type == Phi_ || enode->type == Phi_n) {
					enode->isMux = true;
					ENode* merge_for_sel = nullptr;
					merge_for_sel = addMergeLoopSel(enode);
					assert(merge_for_sel != nullptr);

					enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
					// the SEL signal of the mux must be .at(0)
					enode->CntrlPreds->at(0) = merge_for_sel;
					merge_for_sel->CntrlSuccs->push_back(enode);
				} else {
					assert(enode->type == Phi_c);

					enode->isMux = true;
					ENode* merge_for_sel_ctrl = nullptr;
					merge_for_sel_ctrl = addMergeLoopSel_ctrl(enode);
					assert(merge_for_sel_ctrl != nullptr);

					// Phi_c doesn't have any preds in the data CntrlPreds network, so I don't need to move anything!!
					//enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					//enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);

					// the SEL signal of the mux must be .at(0) in the data network
					assert(enode->CntrlPreds->size() == 0);
					enode->CntrlPreds->push_back(merge_for_sel_ctrl);
					merge_for_sel_ctrl->CntrlSuccs->push_back(enode);

				} */

			} else {
			// if it's LLVM Phi_, I can use LLVM's instruction to know the "coming from", but if it's a Phi_n of my insertion, I should get the "coming from" information from a field of my creation!
				//continue;

			// AYA: 26/03/2022: this case should not happen anyways because I only insert Phi_c for the regeneration of Start_ for constant triggering in case the constant is inside a loop
				if(enode->type == Phi_c)
					continue;

				dbg_file << "\n\n---------------------------------------------------\n";
				dbg_file << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlPreds->size() << " predecessors\n";

				dbg_file_2 << "\n\n---------------------------------------------------\n";
				dbg_file_2 << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlPreds->size() << " predecessors\n";

				for(int i = 0; i < enode->CntrlPreds->size(); i++) {
					dbg_file << "\nConsidering producer node name " << getNodeDotNameNew(enode->CntrlPreds->at(i)) << " of type " <<  getNodeDotTypeNew(enode->CntrlPreds->at(i)) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";

					BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
					bbnode_dag->push_back(virtualBB);
					BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);
					// insert a virtualBB at the edge corresponding to this producer
					insertVirtualBB(enode, virtualBB, llvm_predBB);

					// find f_i which is the conditions of pierBBs in the paths between the START and the virtualBB we just inserted, 
					////////////////////////// clearing the datastructures 
					sum_of_products.clear();

					// find all the paths between the startBB and the virtualBB
					/////////////////// variables needed inside enumerate_paths
					bool *visited = new bool[bbnode_dag->size()];
					int *path = new int[bbnode_dag->size()];
					int path_index = 0; 
					// needed to hold the enumerated paths 
					std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
					// 2nd enumerate all the paths between the current producer and this
					// reseting the data structures for a new prod_con pair
					for(int i = 0; i < bbnode_dag->size(); i++){
						visited[i] = false;
					}
					final_paths->clear();
					/////////////////////////// end of clearing the datastructures

					enumerate_paths_dfs_start_to_end(startBB, virtualBB, visited, path, path_index, final_paths);

					dbg_file << "Found " << final_paths->size() << " paths\n";
					// loop over the final_paths and identify the pierBBs with their conditions

					for(int kk = 0; kk < final_paths->size(); kk++) {
						dbg_file << "Printing path number " << kk << " details\n";
						for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
							dbg_file << final_paths->at(kk).at(hh) + 1 << " -> ";
						}
						dbg_file << "\n";

						// identifying the pierBBs in 1 path
						std::vector<int>* pierBBs = new std::vector<int>;
						find_path_piers(final_paths->at(kk), pierBBs, virtualBB, endBB);

						///// Debugging to check the pierBBs 
						dbg_file << "\n Printing all the piers of that path!\n";
						for(int u = 0; u < pierBBs->size(); u++) {
							dbg_file << pierBBs->at(u) + 1 << ", ";
						}	
						dbg_file << "\n\n";
						/////////////////////////////////////
						// loop on piers of that path to construct a product out of them, then push this product to the sum_of_products before moving to a new path
						one_product.clear();
						for(int u = 0; u < pierBBs->size(); u++) {
							pierCondition cond;
							cond.pierBB_index = pierBBs->at(u);

							int next_pierBB_index;
							if(u == pierBBs->size()-1) {
								next_pierBB_index = BBMap->at(llvm_predBB)->Idx;
							} else {
								next_pierBB_index = pierBBs->at(u+1);
							} 
							if(is_negated_cond(pierBBs->at(u), next_pierBB_index, enode->BB ,dbg_file)) {
								cond.is_negated = true;
							} else {
								cond.is_negated = false;
							}
							one_product.push_back(cond);
						}
						sum_of_products.push_back(one_product);

					}  // end of loop on all paths between START and the virtualBB

					// we should have 1 product per path!
					assert(sum_of_products.size() == final_paths->size());

					/// PRINTING the sum_of_products of all paths to that virtualBB
					dbg_file << "\nPrinting the sum of products from START to that virtualBB!\n";
					for(int u = 0; u < sum_of_products.size(); u++) {
						for(int v = 0; v < sum_of_products.at(u).size(); v++) {
							if(sum_of_products.at(u).at(v).is_negated)
								dbg_file << "not C";
							else
								dbg_file << "C";

							dbg_file << sum_of_products.at(u).at(v).pierBB_index +1 << " & ";
						}
						dbg_file << " + ";
					}
					dbg_file << "\n\n";
					
					///////////WHAT IS ENCLOSED HERE IS WRONG! LEAVE IT COMMENTED!!
					///// eliminating any redundant products in the sum_of_products, if any
					/*int ii = 0;
					int jj = 0;
					while(ii < sum_of_products.size()) {
						jj = ii+1;
						while(jj < sum_of_products.size()) {
							if(should_merge_paths(sum_of_products.at(ii), sum_of_products.at(jj))) 		{
								merge_paths(sum_of_products, ii, jj);
							} else {
								// if paths are merged, we get a new path at jj, so don't increment ot to compare with that new path!!
								jj++;
							} 
						}
						ii++;
					} */
					///////////////////////////////////////////////////
					std::vector<std::vector<pierCondition>>* sum_of_products_ptr = &sum_of_products;
					delete_repeated_paths(sum_of_products_ptr);


					////////////////////////////////////////////////////
					removeVirtualBB(enode, virtualBB, llvm_predBB); 	
					merged_paths_functions.push_back(sum_of_products);
				}  // end of loop on predecessors

				// DEBUGGING.. check the 2 functions (f0 and f1) of this Merge component!
				dbg_file << "The current Merge node the following functions: \n\t";
				for(int iter = 0; iter < merged_paths_functions.size(); iter++) {
					dbg_file << "f" << iter << " = ";
					for(int y = 0; y < merged_paths_functions.at(iter).size(); y++) {
						// take the conditions from a product by product
						for(int z = 0; z <  merged_paths_functions.at(iter).at(y).size(); z++) {
							if(merged_paths_functions.at(iter).at(y).at(z).is_negated)
								dbg_file << "not C";
							else
								dbg_file << "C";

							dbg_file << merged_paths_functions.at(iter).at(y).at(z).pierBB_index +1 << " & ";
						}
						dbg_file << " + ";
					}
					dbg_file << "\n\t";
				}	
				/////////////////////// end of debugging f0 and f1 of that phi enode	
				
				// extract the sum of minterms (i.e., f1 + don't-cares) in a binary string form
				std::vector<std::string>* minterms_only_in_binary = new std::vector<std::string>;
				std::vector<std::string>* minterms_plus_dont_cares_in_binary = new std::vector<std::string>;
				std::vector<int>* condition_variables = new std::vector<int>;

 				int number_of_dont_cares = get_binary_sop(merged_paths_functions, minterms_only_in_binary, minterms_plus_dont_cares_in_binary, condition_variables, dbg_file);

				// we should have at least a single minterm!
				assert(minterms_plus_dont_cares_in_binary->size() > 0);
				// debug_get_binary_sop(merged_paths_functions, dbg_file);

				// call the initialization object of Quine-McCLuskey
				Quine_McCluskey boolean_simplifier;
				int number_of_bits = minterms_plus_dont_cares_in_binary->at(0).length();
				int number_of_minterms = minterms_plus_dont_cares_in_binary->size() - number_of_dont_cares;
				boolean_simplifier.initialize(number_of_bits, number_of_minterms, number_of_dont_cares, minterms_only_in_binary, minterms_plus_dont_cares_in_binary);

				boolean_simplifier.solve(dbg_file);
				std::string bool_expression_name = "SEL_ALWAYS";
				boolean_simplifier.displayFunctions(dbg_file, bool_expression_name);
				std::vector<std::string> essential_sel_always =
						boolean_simplifier.getEssentialPrimeImpl();
				

				std::vector<std::vector<pierCondition>> sel_always = essential_impl_to_pierCondition(essential_sel_always);

				///////// debugging to check the sel_always expression
				dbg_file << "\n\nSEL_ALWAYS = ";
				for(int u = 0; u < sel_always.size(); u++) {
					for(int v = 0; v < sel_always.at(u).size(); v++) {
						if(sel_always.at(u).at(v).is_negated) 
							dbg_file << "not C";
						else
							dbg_file << "C";

						dbg_file << sel_always.at(u).at(v).pierBB_index +1 << " & ";
					}
					dbg_file << " + "; 
				} 
				dbg_file << "\n\n";
				
				///////////////////////////////////////////////////////

				Shannon_Expansion shannon;
				// apply Shannon's on the essential_sel_always!
				Shannon_Expansion::MUXrep sel_always_mux;
				sel_always_mux = shannon.apply_shannon(essential_sel_always, condition_variables, dbg_file_2);
				// corner case is to have the in0 and in1 of the resulting_mux of size 0 indicating that you don't need a MUX, and I'm handling it in the above function!

				////// debugging output
				// printShannons_mux(dbg_file, sel_always_mux);
				ENode* sel_always_enode = nullptr; // can be a MUX node that I create or an already existing BB condition
				bool is_sel_always_negated = false; // indicates if we should negate the SEL_always signal before connecting it
				if(sel_always_mux.is_valid) {
					// then SEL_ALWAYS is calculated by one or more MUXes
					sel_always_enode = addNewMuxes_wrapper(sel_always_mux, enode->BB);
					assert(sel_always_enode != nullptr);
					is_sel_always_negated = false;
				} else {
					// Shannon's expnasion didn't return any MUX, then SEL_ALWAYS must be just a single condition of an existing BB. It may or may not be an inverter..
					bool is_negated;
					sel_always_enode = returnSELCond(essential_sel_always, condition_variables, is_negated, dbg_file_2);
					///// the follwoing is for debugging purposes only!!
					assert(sel_always_enode != nullptr);
					assert(is_negated == true || is_negated == false);
					dbg_file_2 << "\n\n--------------------------------\n";
					dbg_file_2 << "It turns out that SEL_ALWAYS is composed of just a single condition coming from the enode called: " << getNodeDotNameNew(sel_always_enode) << " of type " <<  getNodeDotTypeNew(sel_always_enode)  << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(sel_always_enode->BB));
					if(is_negated) {
						dbg_file_2 << " and it should be inverted!!\n";
					} else {
						dbg_file_2 << " and it should NOT be inverted!!\n";
					}

					is_sel_always_negated = is_negated;
				}
					
				// Then, this sel_always_enode should be connected to the SEL input of the current PHI either directly or with a Branch in the middle.. The decision is based on whether SEL_NO_TOKEN is empty or not!!

				// To generate sel_no_token, I make use of the minterms and dont_cares I got above!
				dbg_file_2 << "\n\nI just calculated the SEL_NO_TOKEN and will pass it to Shannon's!\n";
				std::vector<std::string> sel_no_token = get_sel_no_token(minterms_only_in_binary, minterms_plus_dont_cares_in_binary);

				/////////////////// Run Quine_McCluskey for SEL_NOTOKEN
				// call the initialization object of Quine-McCLuskey
				Quine_McCluskey boolean_simplifier_2;
				int number_of_bits_2 = minterms_plus_dont_cares_in_binary->at(0).length();
				int number_of_minterms_2 = sel_no_token.size();
				int number_of_dont_cares_2 = 0;
				boolean_simplifier_2.initialize(number_of_bits_2, number_of_minterms_2, number_of_dont_cares_2, &sel_no_token, &sel_no_token);
				boolean_simplifier_2.solve(dbg_file);
				std::string bool_expression_name_2 = "SEL_NOTOKEN";
				boolean_simplifier_2.displayFunctions(dbg_file, bool_expression_name_2);
				std::vector<std::string> essential_sel_notoken =
						boolean_simplifier_2.getEssentialPrimeImpl();
				////////////////////////////////////////////////

				Shannon_Expansion::MUXrep sel_no_token_mux;
				sel_no_token_mux = shannon.apply_shannon(essential_sel_notoken, condition_variables, dbg_file_2);

				// We have 3 possibilities for sel_notoken
				// 1) Essential_sel_notoken is empty, in this case we don't need a branch!
				// 2) Shannon didn't add any MUX meaning there just a single condition in Essential_sel_notoken
				// 3) Shannon returned a normal MUX
				if(essential_sel_notoken.size() == 0) {
					// don't need a branch, connet sel_always_enode directly to the Phi under study (enode)
					enode->isMux = true;
					// the select signal of the mux must be .at(0), so do re-adjustments!
					enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
					enode->CntrlPreds->at(0) = sel_always_enode;
					sel_always_enode->CntrlSuccs->push_back(enode);

					// sel_always_enode should be negated or not depending on is_sel_always_negated
					enode->is_negated_input->push_back(is_sel_always_negated); 
					// the other 2 inputs of the enode MUX shouldn't be negated!!
					enode->is_negated_input->push_back(false);
					enode->is_negated_input->push_back(false);

					// this is necessary so that the printDot checks if the inputs of the enode should eb negated or not
					enode->is_advanced_component = true;

					// sanity check
					assert(enode->CntrlPreds->size() == enode->is_negated_input->size());

				} else {
					// need to add a new branch, but let's first identify its condition!
					ENode* sel_notoken_enode = nullptr;
	
					bool is_sel_notoken_negated = false; // indicates if we should negate the SEL_NOTOKEN signal before connecting it

					if(sel_no_token_mux.is_valid) {
						// then SEL_MUX is calculated by one or more MUXes
						sel_notoken_enode = addNewMuxes_wrapper(sel_no_token_mux, enode->BB);
						assert(sel_notoken_enode != nullptr);
						
						is_sel_notoken_negated = false;

					} else {
						bool is_negated_2;
						sel_notoken_enode = returnSELCond(essential_sel_notoken, condition_variables, is_negated_2, dbg_file_2);
						assert(is_negated_2 == true || is_negated_2 == false);

						is_sel_notoken_negated = is_negated_2;

					}
					//////////////////
					assert(sel_notoken_enode != nullptr);

					// add a new branch
					ENode* notoken_branch = new ENode(Branch_n, "branch", enode->BB);
					notoken_branch->id = branch_id++;
					enode_dag->push_back(notoken_branch);
					// Aya: added this to help in removing redundant branches
					notoken_branch->producer_to_branch = sel_always_enode;
					notoken_branch->true_branch_succs = new std::vector<ENode*>;
					notoken_branch->false_branch_succs = new std::vector<ENode*>;
					notoken_branch->true_branch_succs_Ctrl = new std::vector<ENode*>;
					notoken_branch->false_branch_succs_Ctrl = new std::vector<ENode*>;

					// - Data should be .at(0) and condition should be .at(1)
					sel_always_enode->CntrlSuccs->push_back(notoken_branch);
					notoken_branch->CntrlPreds->push_back(sel_always_enode);

					// the condition of the branch is sel_notoken_enode
					sel_notoken_enode->CntrlSuccs->push_back(notoken_branch);
					notoken_branch->CntrlPreds->push_back(sel_notoken_enode);

					// check if the data input of the branch is negated or not dependent on is_sel_always_negated (since the input data of the MUX is SEL_ALWAYS!)
					notoken_branch->is_negated_input->push_back(is_sel_always_negated); 
					// check if the cond input of the branch is negated or not dependent on is_sel_notoken_negated (since the input cond of the MUX is SEL_NOTOKEN!)
					notoken_branch->is_negated_input->push_back(is_sel_notoken_negated);

					notoken_branch->is_advanced_component = true;

					// sanity check
					assert(notoken_branch->CntrlPreds->size() == notoken_branch->is_negated_input->size());

					/////////////////////// connect the branch to the SEL of the given phi enode
					enode->isMux = true;
					// the select signal of the mux must be .at(0), so do re-adjustments!
					enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
					enode->CntrlPreds->at(0) = notoken_branch;

					// WHEN we are sure that the SEL of the enode is fed through a branch, we don't need to worry about negating any of the inputs of the enode!
	
					// the SEL of the MUX should be at the false side of the branch (i.e., when sel_notoken is false)!
					notoken_branch->false_branch_succs->push_back(enode);
					
				}
				
			} // end of phi_type not loop_header if condition

		}  // end of phi_type if condition

	}  // end of loop over all enodes

	// AYA: 28/02/2022: Added a function to fix the branch conditions! (i.e., to avoid that the condition of a Branch is negated
	//FixBranchesCond();
}

void CircuitGenerator::my_swapVectors(std::vector<ENode*>* vec_1, std::vector<ENode*>* vec_2) {
	// first mantain vec_1 in a temp storage
	std::vector<ENode*>* temp = new std::vector<ENode*>;

	for(int i = 0; i < vec_1->size(); i++) {
		temp->push_back(vec_1->at(i));
	}

	vec_1->clear();
	// copy vec_2 to vec_1
	for(int i = 0; i < vec_2->size(); i++) {
		vec_1->push_back(vec_2->at(i));
	}
	
	vec_2->clear();
	// copy temp to vec_2
	for(int i = 0; i < temp->size(); i++) {
		vec_2->push_back(temp->at(i));
	}

	delete temp;

} 


void CircuitGenerator::FixBranchesCond() {
	// loop on all branches and check if any of them has a negated condition!
	for(auto& enode: *enode_dag) {
		if(enode->type == Branch_n && enode->is_advanced_component) {
			assert(enode->is_negated_input->size() == 2);
			// check if the condition is negated
			if(enode->is_negated_input->at(1)) {
				// make it false and swap the true and false preds of the branch!
				enode->is_negated_input->at(1) = false;
				my_swapVectors(enode->true_branch_succs, enode->false_branch_succs);
			}	
			
		}
	}

}

// AYA: 11/02/2022
// TBD: FOR now I'm considering only the data merges not the control!!!
	// ALSO, IMP TO NOTE THAT ANY MERGE I ADD MYSELF (CONTROL OR DATA FOR LOOP REGENERATION OR FOR MULTIPLE REACHABLE POINTS, DOESN'T HAVE LLVM'S COMING FROM, SO I NEED TO FIND A WAY TO MODEL IT TO KNOW WHERE TO INSERT THE VIRTUALBB!!!!

/*void CircuitGenerator::new_setMuxes_OLD() {
	std::ofstream dbg_file;
    dbg_file.open("f0_f1_Merge_check.txt");


	// we have 2 merged paths (f0 and f1), each path is a vector of vectors since you could arrive at any of the two edges (corresponding virtualBB) through multiple paths.


	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions;
	// for reference, here are the fields of the struct that is declared in CircuitGenerator.h

	std::vector<std::vector<pierCondition>> sum_of_products;
	std::vector<pierCondition> one_product;

	BBNode* startBB = bbnode_dag->at(0);
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(auto & enode : *enode_dag) {
		// for every PHI
		if(enode->type == Phi_ || enode->type == Phi_n) {
			// skip Merges that are at a loop header and fed through a backward edge.
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(1)->BB)->Idx) ) {
					
					continue;
					// TO WORK ON REAL BENCHMARKS, I CAN TRY TO TEMPORARILY DO THE OLD SET_MUXES HERE FOR LOOP HEADERS, OR NOT DO ANYTHING AT ALL!!

					// Aya: 18/02/2022 added this code to temporarily connect the MUX to the condition					
					//if(enode->type == Phi_n) {
						// convert the Merge to Mux
						enode->isMux = true;

						// search for the BB that is the loop exit of this loop
						BBNode* loop_exit_bb = nullptr;
						for(auto& bbnode_ : *bbnode_dag) {
							if(bbnode_->is_loop_exiting && bbnode_->loop == BBMap->at(enode->BB)->loop) {
								loop_exit_bb = bbnode_;
								break;
							}
						}

						assert(loop_exit_bb != nullptr);

						//////////////////////////////
						llvm::BranchInst* BI;
						ENode* llvm_branch;
						for(auto& enode_ : *enode_dag) {
							if(enode_->type == Branch_ && enode_->BB == loop_exit_bb->BB) { 
								BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
								llvm_branch = enode_;
								break; // no need to continue looping 1 llvm branch is enough
							}
						}

						assert(!BI->isUnconditional());
						/////////////////////////////////

						enode->CntrlPreds->push_back(enode->CntrlPreds->at(1));
						enode->CntrlPreds->at(1) = enode->CntrlPreds->at(0);
						// the select signal of the mux must be .at(0)
						enode->CntrlPreds->at(0) = llvm_branch->CntrlPreds->at(0);
						llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(enode);
					//} 

						BasicBlock *falseBB, *trueBB;
						
						trueBB = BI->getSuccessor(0);
						falseBB = BI->getSuccessor(1);

						dbg_file << "\n\n TEMPORARILY checking the value of icmp of the LLVM's branch!!!!"<< "\n\n";

						dbg_file << "The trueBB is " << aya_getNodeDotbbID(BBMap->at(trueBB)) << " and the falseBB is " << aya_getNodeDotbbID(BBMap->at(falseBB)) << "\n\n";
					
				
			} else {
			// if it's LLVM Phi_, I can use LLVM's instruction to know the "coming from", but if it's a Phi_n of my insertion, I should get the "coming from" information from a field of my creation!
			dbg_file << "\n---------------------------------------------------\n";
			dbg_file << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << "\n";
		// PLZZZ TAKE CARE!!! TEMPORARILY COMMENTING THIS!!!
			// convert the Merge to a Mux!
			//enode->isMux = true;

			/*
				Convention is SEL = 1 if the input should arrive from the producer at pred->at(1) and SEL = 0 if the input should arrive from the producer at pred->at(0)

			
			// loop over the producers of the MUX
			for(int i = 0; i < enode->CntrlPreds->size(); i++) {
				dbg_file << "\nConsidering producer node name " << getNodeDotNameNew(enode->CntrlPreds->at(i)) << " of type " <<  getNodeDotTypeNew(enode->CntrlPreds->at(i)) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";

				BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
				bbnode_dag->push_back(virtualBB);
				BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);
				// insert a virtualBB at the edge corresponding to this producer
				insertVirtualBB(enode, virtualBB, llvm_predBB);

				// find f_i which is the conditions of pierBBs in the paths between the START and the virtualBB we just inserted
				sum_of_products.clear();
				one_product.clear();

				// find all the paths between the startBB and the virtualBB
				/////////////////// variables needed inside enumerate_paths
				bool *visited = new bool[bbnode_dag->size()];
				int *path = new int[bbnode_dag->size()];
				int path_index = 0; 
				// needed to hold the enumerated paths 
				std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
				// 2nd enumerate all the paths between the current producer and this
				// reseting the data structures for a new prod_con pair
				for(int i = 0; i < bbnode_dag->size(); i++){
					visited[i] = false;
				}
				final_paths->clear();

				enumerate_paths_dfs_start_to_end(startBB, virtualBB, visited, path, path_index, final_paths);

				dbg_file << "Found " << final_paths->size() << " paths\n";
				// loop over the final_paths and identify the pierBBs with their conditions
				for(int kk = 0; kk < final_paths->size(); kk++) {
					dbg_file << "Printing path number " << kk << " details\n";

					for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
						dbg_file << final_paths->at(kk).at(hh) << " -> ";
					}

					dbg_file << "\n";

					for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
						if(bbnode_dag->at(final_paths->at(kk).at(hh))->CntrlSuccs->size() >= 2){
							if(!my_is_post_dom(bbnode_dag->at(final_paths->at(kk).at(hh)), virtualBB, endBB)) 			{
								pierCondition cond;
								cond.pierBB_index = final_paths->at(kk).at(hh);

								dbg_file << "\n\n pierBB is in BB " << aya_getNodeDotbbID(BBMap->at(bbnode_dag->at(final_paths->at(kk).at(hh))->BB)) << "\n\n";

								// to identify if the condition of the pier is negated or not,
									// search for the llvm branch inside this pier BB
								llvm::BranchInst* BI;
								ENode* llvm_branch = nullptr;
								for(auto& e : *enode_dag) {
									if(e->type == Branch_ && e->BB == (bbnode_dag->at(final_paths->at(kk).at(hh)))->BB) { 
										BI = dyn_cast<llvm::BranchInst>(e->Instr);
										llvm_branch = e;
										break; // no need to continue looping 1 llvm branch is enough
									}
								}

								assert(llvm_branch!=nullptr);

							
								// connecting the branch successors
								BasicBlock *falseBB, *trueBB;
								
								trueBB = BI->getSuccessor(0);
								falseBB = BI->getSuccessor(1);
								
								// this pierBB is either directly connected to the BB containing the Merge (in which case, the true or false BB are the Merge BB). Otherwise, the true or falseBBs must be post-dominating the coming from BB)

								// check the coming_from BB of the Merge consumer
								if(llvm_predBB == bbnode_dag->at(final_paths->at(kk).at(hh))->BB) {
									// the consumerBB must be = to trueBB or falseBB!
									if(enode->BB == falseBB) {
										cond.is_negated = true;
									} else {
										assert(enode->BB == trueBB);
										cond.is_negated = false;
									}
								} else {
								// this coming_from BB must be post-dominated by one of the trueBB or falseBB 
									if(my_post_dom_tree->properlyDominates(llvm_predBB, falseBB) || llvm_predBB == falseBB) 	{
										cond.is_negated = true;
									} else {
										assert(my_post_dom_tree->properlyDominates(llvm_predBB, trueBB) || llvm_predBB == trueBB);
										cond.is_negated = false;
									}

								} 

								one_product.push_back(cond);	
						
							} 

						}

					} 
					sum_of_products.push_back(one_product);
				} 

				// check if any of the products in sum_of_products can be merged
				int ii = 0;
				int jj = 0;
				while(ii < sum_of_products.size()) {
					jj = ii+1;
					while(jj < sum_of_products.size()) {
						if(should_merge_paths(sum_of_products.at(ii), sum_of_products.at(jj))) {
							merge_paths(sum_of_products, ii, jj);
						} else {
							// if paths are merged, we get a new path at jj, so don't increment ot to compare with that new path!!
							jj++;
						} 
						
					}
					ii++;
				}

				dbg_file << "\n\n I'm currently pushing to merged_paths_functions!\n\n";
				merged_paths_functions.push_back(sum_of_products);

				removeVirtualBB(enode, virtualBB, llvm_predBB); 			

			} // done looping over the producers of this Merge node

			dbg_file << "\n\n Size of merged_paths_functions is :" << merged_paths_functions.size() << "\n\n";

			// DEBUGGING.. check the 2 functions (f0 and f1) of this Merge component!
			dbg_file << "Has the following functions: \n\t";

			for(int iter = 0; iter < merged_paths_functions.size(); iter++) {
				dbg_file << "f" << iter << " = ";
				for(int y = 0; y < merged_paths_functions.at(iter).size(); y++) {
					// take the conditions from a product by product
					for(int z = 0; z <  merged_paths_functions.at(iter).at(y).size(); z++) {
						if(merged_paths_functions.at(iter).at(y).at(z).is_negated)
							dbg_file << "not C";
						else
							dbg_file << "C";

						dbg_file << merged_paths_functions.at(iter).at(y).at(z).pierBB_index +1 << " & ";
					}
					dbg_file << " + ";
				}
				dbg_file << "\n\t";
			}				

			//////////////////////////////////////////////////////////////////////////
			// following the convention of LLVM's inputs to the Merge, merged_paths_functions.at(0) represents f0 and merged_paths_functions.at(1) represents f1

			// the following functions prepare a boolean expression for passing to a Quine Mc-CLuskey model to simplify it
			// the goal of this function is to return the sum of products of f1 (since this is when the function SEL = 1) + the don't-cares (to help in logic-minimization)

			std::vector<std::string> minterms_in_binary = get_binary_sop(merged_paths_functions);
			// we should have at least a single minterm!
			assert(minterms_in_binary.size() > 0);

			debug_get_binary_sop(merged_paths_functions, dbg_file);
			

			// call the initialization object of Quine Mc-Luskey
			Quine_McCluskey boolean_simplifier;
			int number_of_bits = minterms_in_binary.at(0).length();
			int number_of_minterms = minterms_in_binary.size();
			
			boolean_simplifier.initialize(number_of_bits, number_of_minterms, minterms_in_binary);

			// SHOULD RETURN THE SIMPLIFIED SUM OF PRODUCTS VECTOR OF VECTORS????
			//std::vector<std::vector<pierCondition>>* simplified_sum_of_products
			boolean_simplifier.solve();

			


			
			//////////////////////////////////////////////////////////////////////
			// find a way to apply Shannon's?!

			// I WILL NEED THE FOLLOWING LOGIC IN CONNECTING SEL TO THE MUX!!!
			/*assert(enode_2->type == Phi_ || enode_2->type == Phi_n);
			// Maintain the order of Phi_ since you made sure it's correct from above!
			enode_2->CntrlPreds->push_back(enode_2->CntrlPreds->at(1));
			enode_2->CntrlPreds->at(1) = enode_2->CntrlPreds->at(0);

			// the select signal of the mux must be .at(0)
			enode_2->CntrlPreds->at(0) = enode;

			// sanity check: at this moment, this Phi_ should have three predecessors
			assert(enode_2->CntrlPreds->size() == 3);
			
			}

		}

	}


}*/


void CircuitGenerator::setMuxes() {
	for(auto& enode : *enode_dag) {

		// 18/11/2021:
		std::vector<ENode*>* temp = new std::vector<ENode*>;

		//enode->isMux = false;
		//enode->isCntrlMg = false;

		// make sure that this Phi_c is part of the new redundant control network to not mess up with the other one!!
		if(enode->type == Phi_c && enode->is_redunCntrlNet && enode->CntrlOrderPreds->size() > 1) {
			// for every Phi_c in the redunNetwork, connect it to all the LLVM Phis (Phi_) in the same BB! To meet the convention, you must connect it to .at(0) of the Phi_'s preds 
			
			// sanity check: at this moment, this Phi_c enode is still not having any successors in the data path!!
			assert(enode->CntrlSuccs->size() == 0);

			// search for Phi_ in the same BB
			for(auto& enode_2 : *enode_dag) {
		// AYA: 05/10/2021: Added an extra condition to also convert my phis to muxes!!
		// AYA: 08/10/2021: Added an extra condition to also convert my control phis to muxes!!
				if((enode_2->type == Phi_ || enode_2->type == Phi_n ||(enode_2->type == Phi_c && !enode_2->is_redunCntrlNet)) && enode->BB == enode_2->BB) {
					// the Phi_c becomes a control merge
					enode->isCntrlMg = true;
					// the Phi_ becomes a Mux
					enode_2->isMux = true;

					// will push_back to the enode in the end!!					

					// adjust the connections, should happen on the data paths since this is the mux's control signal and should be at least 1 bit in size not data-less!! 
					//enode->CntrlSuccs->push_back(enode_2);
					temp->push_back(enode_2);

					// extra sanity check: LLVM phis should be having 2 predecessors and we will add a 3rd predecessor to convert it to a MUX
					assert(enode_2->CntrlPreds->size() == 2 || enode_2->JustCntrlPreds->size() == 2);
	
	////////////////////////////////////////////////////////////////////////////////////
		// ALready made sure of fixing the preds orders of Phi_ and Phi_c before!!
	////////////////////////////////////////////////////////////////////////////////////
					if(enode_2->type == Phi_c) {
						// the select signal of the mux must be .at(0) of the data network not the control network!!
						enode_2->CntrlPreds->push_back(enode);

						// sanity check: 
						assert(enode_2->CntrlPreds->size() == 1 && enode_2->JustCntrlPreds->size() == 2);

					} else {
						assert(enode_2->type == Phi_ || enode_2->type == Phi_n);
						// Maintain the order of Phi_ since you made sure it's correct from above!
						enode_2->CntrlPreds->push_back(enode_2->CntrlPreds->at(1));
						enode_2->CntrlPreds->at(1) = enode_2->CntrlPreds->at(0);

						// the select signal of the mux must be .at(0)
						enode_2->CntrlPreds->at(0) = enode;

						// sanity check: at this moment, this Phi_ should have three predecessors
						assert(enode_2->CntrlPreds->size() == 3);
					}				

				}
			}

			// AYA: 18/11/2021
			for(int i = 0; i < temp->size(); i++) {
				enode->CntrlSuccs->push_back(temp->at(i));
			}

			// Now check if we will need to add a fork or not!
			if(enode->CntrlSuccs->size() > 1) {
				// insert a fork
				ENode* fork = new ENode(Fork_, "fork", enode->BB);
				fork->id = fork_id++;
				enode_dag->push_back(fork);
				// insert the fork between Phi_c and all its successors
				aya_InsertNode(enode, fork);

			} 


		}

	}


	// IMP NOTE!!! In phis that will become Muxes, there is a convention that the input condition (select) of the mux that comes from a cmerge must be the first predecessor .at(0)

}


// I'M NOT USING THIS FUNCTION ANYMORE!!!
/* 
	This function creates a new network that keeps the control order and has CMerges that supply a control signal for merges that will become muxes..
*/

void CircuitGenerator::constructOrderCntrlNet(std::vector<ENode*>* enode_dag_with_llvm_br) {
	// loop over the BBs to make sure to place a Phi_c at the input of every BB and a Branch_c at the output of every BB (branches should branch to the next BB according to LLVM's not to far away branches)

	for (auto& bbnode : *bbnode_dag) {
		ENode* phiC_node = nullptr;
		llvm::BranchInst* BI = nullptr;
		ENode* llvm_branch_enode = nullptr; 

		if (bbnode == bbnode_dag->front()) {
			// No need to create a Phi here because there is a start node
			// Search for the start node in this BB and if it has forkC, connect the branch to it, otherwise, we might need to create forkC ourselves
			for(auto& cnt_node : *bbnode->Cntrl_nodes) {
				if(cnt_node->type == Start_) {
					// check if its successor is a Fork_c or not

					// sanity check: after all the earlier transformations, for sure any node has a single successor either in the data graph or control graph.. Exception to this is a branch which must have two successors in either network and a Fork which can have successors in both networks simultaneously, But it must be of type Fork_ not Fork_c
					assert(cnt_node->JustCntrlSuccs->size() == 1); 
					if(cnt_node->JustCntrlSuccs->at(0)->type == Fork_c){
						phiC_node = cnt_node->JustCntrlSuccs->at(0);
					} else {
						// This means there is one non-fork successor so for us to connect Start to Branch_c, we must add a fork that connects to both successors; one on the original control network and one in the new control network.

						// create a new Fork_c and connect the original succs to it
						ENode* fork_c = new ENode(Fork_c, "forkC", bbnode->BB);
						fork_c->id = fork_id++;
						enode_dag->push_back(fork_c);

						fork_c->is_redunCntrlNet = true;

						// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
						InsertCntrlNode(cnt_node, fork_c, false);
						phiC_node = fork_c;
						
					}
					break; // found the start node, no need to continue looping
				}
			}  

			assert(phiC_node != nullptr); 

        } else {
            // create new `phi_node` node
            phiC_node            = new ENode(Phi_c, "phiC", bbnode->BB);
            phiC_node->id        = phi_id++;
            phiC_node->isCntrlMg = false;
			//bbnode->Cntrl_nodes->push_back(phiC_node);
			enode_dag->push_back(phiC_node);

			phiC_node->is_redunCntrlNet = true;
        }

		// Creating branch at the end of the same BB
		if (bbnode->CntrlSuccs->size() > 0) { // BB branches/jumps to some other BB
            ENode* branchC_node = new ENode(Branch_c, "branchC", bbnode->BB);
            branchC_node->id    = branch_id++;
            //bbnode->Cntrl_nodes->push_back(branchC_node);
			setBBIndex(branchC_node, bbnode);
            enode_dag->push_back(branchC_node);

			branchC_node->is_redunCntrlNet = true;

			// Convention of Branch Predecessors is token at pred(0) and condition at pred(1)
			branchC_node->CntrlOrderPreds->push_back(phiC_node);
			phiC_node->CntrlOrderSuccs->push_back(branchC_node);
			
			// Connecting the condition predecessor at pred(1)
			// search for LLVM branch in the same BB to extract the condition from it
			for(auto& orig_enode : *enode_dag_with_llvm_br) {
				if(orig_enode->type == Branch_ && orig_enode->BB == bbnode->BB) { 
					BI = dyn_cast<llvm::BranchInst>(orig_enode->Instr);
					llvm_branch_enode = orig_enode;
					break; // no need to continue looping 1 llvm branch is enough
				}
			}
	
			// make sure that a branch instruction was actually returned or else assert!!!!
			assert(BI != nullptr && llvm_branch_enode != nullptr);

			if(BI->isUnconditional()) {
				// so llvm's branch has no predecessors, so I must create an enode of type constant and treat it as the condition
				ENode* cst_condition = new ENode(Cst_, std::to_string(1).c_str(), bbnode->BB);
				// this is very crucial to specify the value of the constant!!!
				cst_condition->cstValue = 1;

				cst_condition->id = cst_id++;
				enode_dag->push_back(cst_condition);

				cst_condition->is_redunCntrlNet = true;

				branchC_node->CntrlOrderPreds->push_back(cst_condition);
				cst_condition->CntrlOrderSuccs->push_back(branchC_node);

				// note that this constant must have the phiC node as its predecessor
				cst_condition->CntrlOrderPreds->push_back(phiC_node);
				phiC_node->CntrlOrderSuccs->push_back(cst_condition);

			} else {
			// the condition is the only predecessor of llvm's branch, need to check if it's having a fork, then feed our branch from the fork not from the enode direct!!!
				assert(llvm_branch_enode->CntrlPreds->size() > 0);

				// search for the location of the llvm branch condition in the original enode_dag
				auto pos = std::find(enode_dag->begin(), enode_dag->end(), llvm_branch_enode->CntrlPreds->at(0));
				assert(pos != enode_dag->end());
				int index = pos - enode_dag->begin();

				ENode* llvm_br_condition = enode_dag->at(index);
				
				// from previous transformations, we are sure that any node (excluding fork and branch nodes) should have just up to a single successor that is either in the control graph or data graph (NOTE: if this condition should feed multiple branches in the data graph and control graph, previous transformations handle this by setting its single successor to a data fork (Fork_) and this will have successors in both the data and control network
				
				
				if(llvm_br_condition->CntrlSuccs->size() + llvm_br_condition->JustCntrlSuccs->size() == 1) {
					// check to see if the single successor is in the data or control graph
					if(llvm_br_condition->CntrlSuccs->size() == 1) {
						if(llvm_br_condition->CntrlSuccs->at(0)->type == Fork_) {
							// directly connect to the Fork
							llvm_br_condition->CntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->CntrlSuccs->at(0));
						} else {
							// if the single successor is not a fork, then it must be already feeding just a single branch and since we will make it feed another branch as well, let's create a dara Fork_
							ENode* fork = new ENode(Fork_, "fork", bbnode->BB);
							fork->id = fork_id++;
							enode_dag->push_back(fork);

							fork->is_redunCntrlNet = true;

							// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
							aya_InsertNode(llvm_br_condition, fork);

							fork->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(fork);

						}
					} else {
						// then the single successor must be in the control graph
						assert(llvm_br_condition->JustCntrlSuccs->size() == 1);
						if(llvm_br_condition->JustCntrlSuccs->at(0)->type == Fork_c) {
							// directly connect to the Fork_C
							llvm_br_condition->JustCntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->JustCntrlSuccs->at(0));
						} else {
							// if the single successor is not a fork, then it must be already feeding just a single branchC in the control network and since we will make it feed another branch as well, let's create a control Fork_
							ENode* fork_c = new ENode(Fork_c, "forkC", bbnode->BB);
							fork_c->id = fork_id++;
							enode_dag->push_back(fork_c);

							fork_c->is_redunCntrlNet = true;

							// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
							InsertCntrlNode(llvm_br_condition, fork_c, false);

							fork_c->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(fork_c);

						}

					}

				} else {
					// then the condition node has no successors after removing the llvm branch, do a sanity check to make sure!
					assert(llvm_br_condition->CntrlSuccs->size() + llvm_br_condition->JustCntrlSuccs->size() == 0);
					// no successors in the data or control paths, so do a direct connection between the condition and our branch
					llvm_br_condition->CntrlOrderSuccs->push_back(branchC_node);
					branchC_node->CntrlOrderPreds->push_back(llvm_br_condition);
				}
				
			} 

		// Will connect the successors of all branches after this function to make sure we are done inserting Phi_c in the beginning of every BB!! 

		} else {
			// if the BBnode is the last one in the CFG, no need to create a branch, instead create a sink to act as the successor of the BB's phi_c
			if(bbnode_dag->size() > 1) {
				ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
				sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);

				sink_node->is_redunCntrlNet = true;

				sink_node->CntrlOrderPreds->push_back(phiC_node);
				phiC_node->CntrlOrderSuccs->push_back(sink_node);
			}

		} 

	}

}

// AYA: I'M ALSO NOT USING THIS FUNCTION ANYMORE SINCE THE ORIGINAL ADDING FORK FUNCTION NOW TAKES CARE OF THE NEW REDUNDANT CONTROL NETWORK ALONG WITH THE TWO ORIGINAL NETWORKS!!!
void CircuitGenerator::addFork_CntrlNet() {
	// loop over the phi_c of each BB and if it has multiple successors, insert a fork
	for(auto& bbnode : *bbnode_dag) {
		for(auto& enode : *enode_dag) {
			if(enode->BB == bbnode->BB && enode->type == Phi_c && enode->is_redunCntrlNet) {
				if(enode->CntrlOrderSuccs->size() > 1) {
					// the condition has a single successor that is not a fork!
					ENode* fork_c = new ENode(Fork_c, "forkC", bbnode->BB);
					fork_c->id = fork_id++;
					enode_dag->push_back(fork_c);

					fork_c->is_redunCntrlNet = true;

					// Fork must be inserted at index 0 of the enode->CntrlOrderSuccs and the original order of successors should be maintained in fork_c->CntrlOrderSuccs
					//////////////////////////////////////////////////////////////
					for (auto& succ : *(enode->CntrlOrderSuccs)) { 
						forkToSucc(succ->CntrlOrderPreds, enode, fork_c);
						fork_c->CntrlOrderSuccs->push_back(succ); // add connection between after and enode's successors
					}

					// successors of the enode should be now empty
					enode->CntrlOrderSuccs = new std::vector<ENode*>; 
					
					enode->CntrlOrderSuccs->push_back(fork_c); 
					fork_c->CntrlOrderPreds->push_back(enode); 

					//////////////////////////////////////////////////////////////

				}

				break; // found the phiC of this bbnode, no need to continue looping 
			}
		}
	}

}

// ALSO, NO LONGER USING THIS FUNCTION!!!!

void CircuitGenerator::connectBrSuccsPhiPreds_CntrlNet(std::vector<ENode*>* enode_dag_with_llvm_br) {
	
	// loop over the bbnodes
	for(int i = 0; i < bbnode_dag->size()-1; i++) {
		llvm::BranchInst* BI = nullptr;
		BasicBlock* trueBB = nullptr;
		BasicBlock* falseBB = nullptr;
		

		// first: identify the llvm branch instruction of this bbnode
		for(auto& enode : *enode_dag_with_llvm_br) {
			if(enode->type == Branch_ && enode->BB == bbnode_dag->at(i)->BB) { 
				BI = dyn_cast<llvm::BranchInst>(enode->Instr);
				break; // no need to continue looping 1 llvm branch is enough
			}	
		}
		
		assert(BI != nullptr);

		// Not all llvm branches have a true and a false successor, so need to check the number of successors first
		int llvm_br_num_of_succs = BI->getNumSuccessors();
		// sanity check: branch must have at least a single successor and no more than 2 succsessors
		assert(llvm_br_num_of_succs > 0 && llvm_br_num_of_succs < 3);
		if(llvm_br_num_of_succs == 1) {
			trueBB = BI->getSuccessor(0);
			falseBB = nullptr;
		} else {
			// then the number of successors must be 2!!
			assert(llvm_br_num_of_succs == 2);
			trueBB = BI->getSuccessor(0);
			falseBB = BI->getSuccessor(1);
		}

		assert(trueBB != nullptr);

		// search for the branchC in the current bbnode and connect it to the PhiC in the trueBB and falseBB if the falseBB is not a nullptr!
		for(auto& enode : *enode_dag) {
			if(enode->BB == bbnode_dag->at(i)->BB && enode->type == Branch_c && enode->is_redunCntrlNet) {

				ENode* true_phiC = nullptr;
				ENode* false_phiC = nullptr;

				// search for the phiC of this bbnode
				for(auto& tmp_enode : *enode_dag) {
					if(tmp_enode->BB == trueBB && tmp_enode->type == Phi_c && tmp_enode->is_redunCntrlNet) {
						true_phiC = tmp_enode;
						break;
				    }
				}
			
				assert(true_phiC != nullptr);

				true_phiC->CntrlOrderPreds->push_back(enode);
				enode->CntrlOrderSuccs->push_back(true_phiC);

				if(falseBB != nullptr) {
						// search for the phiC of this bbnode
					for(auto& tmp_enode : *enode_dag) {
						if(tmp_enode->BB == falseBB && tmp_enode->type == Phi_c && tmp_enode->is_redunCntrlNet) {
							false_phiC = tmp_enode;
							break;
						}
					}
					false_phiC->CntrlOrderPreds->push_back(enode);
					enode->CntrlOrderSuccs->push_back(false_phiC);
				} else{
					// no false successor so connect to a sink
					ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
					sink_node->id    = sink_id++;
					enode_dag->push_back(sink_node);

					sink_node->is_redunCntrlNet = true;

					sink_node->CntrlOrderPreds->push_back(enode);
					enode->CntrlOrderSuccs->push_back(sink_node);

				} 
				
				// CHECK???? CAN A BB HAVE MORE THAN ONE BRANCH_C??? SHOULD I ACCOUNT FOR THIS CASE HERE AND ABOVE DURING THE CREATION OF THOSE REDUNDANT CONTROL BRANCH???
				break; // found the branch of this bbnode, no need to continue looping
			} 
		}

	}

}

void CircuitGenerator::newAddSourceForConstants() {
	bool flag;
	 for (auto& enode : *enode_dag) {
		flag = false;
		if (enode->type == Cst_ && (enode->CntrlPreds->size() + enode->CntrlOrderPreds->size() + enode->JustCntrlPreds->size() == 0)) {
			// Note that constants for sure have just a single successor across the 3 networks.. As a sanity check:
			assert(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size() == 1);

			// AYA: 16/11/2021: replaced those conditions with fields to make sure that these constants are not feeding a branch condition or LSQ, MC.. ALSO, as an extra check, we eiliminate it if it's part of the redundant control network!!
			if(!enode->is_const_br_condition && !enode->is_const_feed_memory && !enode->is_redunCntrlNet) {	
				flag = true;
			} 

			// NOTE that I'm making sure that the successor isn't MC and not LSQ in all the three networks although the check needs to be done only in the CntrlOrder network!! But, I'm being extraaa safe
			/*if(enode->CntrlSuccs->size() == 1) {
				if(enode->CntrlSuccs->at(1)->type != Branch_n && enode->CntrlSuccs->front()->type != Branch_ && enode->CntrlSuccs->front()->type != MC_ && enode->CntrlSuccs->front()->type != LSQ_) {
					flag = true;
				}

			} else if(enode->CntrlOrderSuccs->size() == 1) {

				if(enode->CntrlOrderSuccs->front()->type != Branch_c && enode->CntrlOrderSuccs->front()->type != MC_ && enode->CntrlOrderSuccs->front()->type != LSQ_) {
					flag = true;
				}

			} else {
				assert(enode->JustCntrlSuccs->size() == 1);
				if(enode->JustCntrlSuccs->front()->type != Branch_c && enode->JustCntrlSuccs->front()->type != MC_ && enode->JustCntrlSuccs->front()->type != LSQ_) {
					flag = true;
				}
			}

			*/

			if(flag) {  // this means that this enode is a constant that requires to be triggered by a source_node
				ENode* source_node = new ENode(Source_, "source", enode->BB);
				source_node->id    = source_id++;

				enode->JustCntrlPreds->push_back(source_node);
				source_node->JustCntrlSuccs->push_back(enode);

				enode_dag->push_back(source_node);
			}

		}

	}


} 

///////////////////// AYA: 15/11/2021: Added the following function to add a dedicated source to trigger any constant except to constants feeding unconditional branches!!!
void CircuitGenerator::addSourceForConstants() {
	bool flag;
	 for (auto& enode : *enode_dag) {
		flag = false;
        if (enode->type == Cst_ && (enode->CntrlPreds->size() + enode->CntrlOrderPreds->size() + enode->JustCntrlPreds->size() == 0)) {
			// Note that constants for sure have just a single successor across the 3 networks.. As a sanity check:
			assert(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size() == 1);

			// AYA: 16/11/2021: replaced those conditions with fields to make sure that these constants are not feeding a branch condition or LSQ, MC.. ALSO, as an extra check, we eiliminate it if it's part of the redundant control network!!
			if(!enode->is_const_br_condition && !enode->is_const_feed_memory && !enode->is_redunCntrlNet) {	
				flag = true;
			} 

			// NOTE that I'm making sure that the successor isn't MC and not LSQ in all the three networks although the check needs to be done only in the CntrlOrder network!! But, I'm being extraaa safe
			/*if(enode->CntrlSuccs->size() == 1) {
				if(enode->CntrlSuccs->at(1)->type != Branch_n && enode->CntrlSuccs->front()->type != Branch_ && enode->CntrlSuccs->front()->type != MC_ && enode->CntrlSuccs->front()->type != LSQ_) {
					flag = true;
				}

			} else if(enode->CntrlOrderSuccs->size() == 1) {

				if(enode->CntrlOrderSuccs->front()->type != Branch_c && enode->CntrlOrderSuccs->front()->type != MC_ && enode->CntrlOrderSuccs->front()->type != LSQ_) {
					flag = true;
				}

			} else {
				assert(enode->JustCntrlSuccs->size() == 1);
				if(enode->JustCntrlSuccs->front()->type != Branch_c && enode->JustCntrlSuccs->front()->type != MC_ && enode->JustCntrlSuccs->front()->type != LSQ_) {
					flag = true;
				}
			}

			*/

			if(flag) {  // this means that this enode is a constant that requires to be triggered by a source_node
				ENode* source_node = new ENode(Source_, "source", enode->BB);
				source_node->id    = source_id++;

				enode->JustCntrlPreds->push_back(source_node);
				source_node->JustCntrlSuccs->push_back(enode);

				enode_dag->push_back(source_node);
			}

		}

	}

	/*std::vector<ENode*> sourceList;
    for (auto& enode : *enode_dag) {
        if (enode->type == Cst_) {

            if (enode->CntrlSuccs->size() > 0)
				if(enode->JustCntrlPreds->size() == 0 && enode->CntrlSuccs->front()->type != Branch_n && enode->CntrlSuccs->front()->type != Branch_ ){

// I don't need the following two conditions since if a constant has a CntrlSuccs of > 0 this means it's not triggering any control branch (I trigger control branch over the other two networks!!)
//&& enode->JustCntrlSuccs->front()->type != Branch_c && enode->CntrlOrderSuccs->front()->type != Branch_c) {
                    ENode* source_node = new ENode(Source_, "source", enode->BB);
                    source_node->id    = source_id++;

                    enode->CntrlPreds->push_back(source_node);
                    source_node->CntrlSuccs->push_back(enode);

                    sourceList.push_back(source_node);
                }
            if (enode->JustCntrlSuccs->size() > 0) {
                if (enode->JustCntrlSuccs->front()->type == Inst_)
                    if (enode->JustCntrlSuccs->front()->Instr->getOpcode() ==
                        Instruction::GetElementPtr) {
                        ENode* source_node = new ENode(Source_, "source", enode->BB);
                        source_node->id    = source_id++;

                        enode->CntrlPreds->push_back(source_node);
                        source_node->CntrlSuccs->push_back(enode);

                        sourceList.push_back(source_node);
                    }
            }

			// AYA: 15/11/2021: added the following to trigger constants that feed LSQs/MCs
			if(enode->CntrlOrderSuccs->size() > 0) {
				if(enode->CntrlOrderSuccs->front()->type == MC_ || enode->CntrlOrderSuccs->front()->type == LSQ_) {
				ENode* source_node = new ENode(Source_, "source", enode->BB);
				source_node->id    = source_id++;

				enode->CntrlPreds->push_back(source_node);
				source_node->CntrlSuccs->push_back(enode);

				sourceList.push_back(source_node);
				} 
			} 
        }
    }
    for (auto source_node : sourceList)
        enode_dag->push_back(source_node);

	*/
}

////////////////////////////////////////////////

// Aya: my implementation of the data-less control path 
void CircuitGenerator::addStartC() {
	/*
		JustCntrlSuccs and JustCntrlPreds denote the data-less Control path
	*/
	
	// Add the very start operation in the very first BB
	ENode* startC_node = new ENode(Start_, "start", (bbnode_dag->front())->BB);
	startC_node->id = start_id++;
	bbnode_dag->front()->Cntrl_nodes->push_back(startC_node);
	enode_dag->push_back(startC_node);

	// Aya: 15/11/2021, commented everything to not trigger the LLVM constants using the start node.. Rather, they should be triggered using Source_ nodes!!!

	// So any constant should have this startC_node as its predecessor.. it's important to remember that in the cntrl path we consider JustCntrlSucc and JustCntrlPreds

	// Aya: 27/09/2021, removed the part of connecting the return directly to start if the function is void and will instead connect the return to the last phi in the last BB in the reduncCntrlNetwork!!

	// AYA: 18/11/2021: Will let start trigger only the LLVM constants in its same BB
	for(auto& enode : *enode_dag) {
		if( (enode->type == Cst_ /*&& enode->BB == startC_node->BB*/)) {
			// constant nodes must have no predecessors!!
			assert(enode->JustCntrlPreds->size() == 0 && enode->CntrlPreds->size() == 0);

			startC_node->JustCntrlSuccs->push_back(enode);
			enode->JustCntrlPreds->push_back(startC_node);

			// AYA: 18/11/2021: to assure that we will not trigger this constant with SOurce!!
			enode->is_const_br_condition = true;

		}

	} 

	//for(auto& enode : *enode_dag) {
	//	if( (enode->type == Cst_) /*|| (enode->type == Inst_ && enode->Name.compare("ret") ==0 				&& enode->CntrlPreds->size() == 0)*/ ) {
			// constant nodes must have no predecessors!!
		//	assert(enode->JustCntrlPreds->size() == 0 && enode->CntrlPreds->size() == 0);

		//	startC_node->JustCntrlSuccs->push_back(enode);
		//	enode->JustCntrlPreds->push_back(startC_node);
		//}

	//} 

}

void CircuitGenerator::InsertCntrlNode(ENode* enode, ENode* after, bool has_data_flag) {
	 // removing all connections between enode and its JustCntrlsuccessors
		// and making these successors the successors of after
    for (auto& succ : *(enode->JustCntrlSuccs)) { // for all enode's successors
		forkToSucc(succ->JustCntrlPreds, enode, after);

        after->JustCntrlSuccs->push_back(succ); // add connection between after and enode's successors
        //succ->JustCntrlPreds->push_back(after); // add connection between after and enode's successors
    }

	// successors of the enode should be now empty
	enode->JustCntrlSuccs = new std::vector<ENode*>; 
	
	// if there is any data successors to the node, then the fork is already connected to the successor of the node 
	if(!has_data_flag) {
		enode->JustCntrlSuccs->push_back(after); // add connection between after and enode
		after->JustCntrlPreds->push_back(enode); // add connection between after and enode
	
	}

    
}

/*
void CircuitGenerator::addForkC() {
	// loop over all enode operations
	for(auto& enode: *enode_dag) {
		// put a fork after any node having multiple successors except for branches
		if (enode->JustCntrlSuccs->size() > 1 && enode->type != Fork_ && enode->type != Fork_c && enode->type != Branch_ && enode->type != Branch_c && enode->type != Branch_n) {
			ENode* forkC = new ENode(Fork_c, "forkC", enode->BB);
			forkC->id = fork_id++;

			// need to correctly insert the fork between the node and all of its JustCntrlSuccs, so i created a new function similar to InsertNode	
			InsertCntrlNode(enode, forkC);
			// need to push it to cntrl_succ of its bb
			BBMap->at(enode->BB)->Cntrl_nodes->push_back(forkC);  
			enode_dag->push_back(forkC);
		}

	} 

}
*/


// AYA: 20/03/2022: added the following functin to delete the return node from the enode_dag if it doesn't have any predecessors (i.e., if the function is void)
void CircuitGenerator::deleteReturn() {
	for(auto& enode: *enode_dag) { 
		if((enode->type == Inst_ && enode->Name.compare("ret") == 0)) {
			if(enode->CntrlPreds->size() == 0 ) {
				// remove it from enode_dag
				auto pos_enode_dag = std::find(enode_dag->begin(), enode_dag->end(), enode);
				assert(pos_enode_dag != enode_dag->end());
				if (pos_enode_dag != enode_dag->end()) {
					enode_dag->erase(pos_enode_dag);
				} 
			}
		}
	}
}

void CircuitGenerator::triggerReturn() {
	for(auto& enode: *enode_dag) { 
		if((enode->type == Inst_ && enode->Name.compare("ret") == 0)) {
			if(enode->CntrlPreds->size() == 0 ) {
				assert(enode->JustCntrlPreds->size() == 0);
				// then I want to trigger it from START_			
				for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
					if(cnt_node->type == Start_) {
						enode->JustCntrlPreds->push_back(cnt_node);
						cnt_node->JustCntrlSuccs->push_back(enode);
						break; // found the start node, no need to continue looping
					}
				}
			}
		}
	}
}

// This function should be called in the very end, after all the transformations are done!!!
void CircuitGenerator::addExitC() {
	// the exit of the graph is not inside any BB so pass NULL
	ENode* ExitC_node = new ENode(End_, "end", NULL);
	ExitC_node->id = 0;
	enode_dag->push_back(ExitC_node);


	// loop over all enodes searching for a return operation or memory operation
	for(auto& enode: *enode_dag) { 
		if((enode->type == Inst_ && enode->Name.compare("ret") == 0)) {
			// if the return has a predecessor, it means it returns data
			if(enode->CntrlPreds->size() > 0 ) {  // (i.e., if there is data returned)
				enode->CntrlSuccs->push_back(ExitC_node);
				ExitC_node->CntrlPreds->push_back(enode);
			} else {
				// 09/02/2022 if the return is not fed with data (i.e., the function is void), let PhiC of the last BB feed the return!
				/*for(int i = 0; i < enode_dag->size(); i++) {
					if(enode_dag->at(i)->BB == enode->BB && enode_dag->at(i)->is_redunCntrlNet && enode_dag->at(i)->type == Phi_c) {
						enode->CntrlOrderPreds->push_back(enode_dag->at(i));
						enode_dag->at(i)->CntrlOrderSuccs->push_back(enode);
						break;	
					} 
				} */
				/////////////////////
				
				// AYA: 20/03/2022: I added another function called before addFork to connect retirn to start_ if the function is void!!
				
				enode->JustCntrlSuccs->push_back(ExitC_node);
				ExitC_node->JustCntrlPreds->push_back(enode);
			} 	
		}  
		else 
			if(enode->type == MC_ || enode->type == LSQ_) {
				// connect it to ExitC_node
			// Aya: not sure if this should be JustCntrl or CntrlOrder ??????
				//enode->JustCntrlSuccs->push_back(ExitC_node);
				//ExitC_node->JustCntrlPreds->push_back(enode);
				
				enode->CntrlOrderSuccs->push_back(ExitC_node);
				ExitC_node->CntrlOrderPreds->push_back(enode);
				
			}

	}

	/* Note that the inputted function could return a value or could be void
	   If it returns a value, then it'll be triggered once this value is available
	   If it doesn't return a value, we need to trigger it by a Control Token

	   The function could have several return statements so Exit should OR them all... It is important to note that this Exit node also gets fed from memory operations to make sure they complete.. So, essentially, it should be made up by ANDing the memory operations with the ORing of the Retrun operations..

	   In case of a return to void that needs triggering, we can't trigger it directly by start since this will exit the code instantly before operations are done!!

	*/

}

///////////////// Another outdated code for how to connect the new redundant control branch to condition node
		/*

				if(llvm_br_condition->CntrlSuccs->size() == 1 || llvm_br_condition->JustCntrlSuccs->size() == 1) {
					if(llvm_br_condition->CntrlSuccs->size() == 1) {
						if(llvm_br_condition->CntrlSuccs->at(0)->type == Fork_) {
							// directly connect to the Fork
							llvm_br_condition->CntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->CntrlSuccs->at(0));
						} else {
							// the condition has a single successor that is not a fork!
							// so create a new fork and connect both to it
							ENode* fork = new ENode(Fork_, "fork", bbnode->BB);
							fork->id = fork_id++;
							enode_dag->push_back(fork);

							//fork->is_redunCntrlNet = true;

							// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
							aya_InsertNode(llvm_br_condition, fork);

							fork->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(fork);
						}
					} else {
						assert(llvm_br_condition->JustCntrlSuccs->size() == 1);
						if(llvm_br_condition->JustCntrlSuccs->at(0)->type == Fork_c) {
							// directly connect to the Fork_c
							llvm_br_condition->JustCntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->JustCntrlSuccs->at(0));
						} else {
							// the condition has a single successor that is not a fork!
							ENode* fork_c = new ENode(Fork_c, "forkC", bbnode->BB);
							fork_c->id = fork_id++;
							enode_dag->push_back(fork_c);

							//fork_c->is_redunCntrlNet = true;

							// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
							InsertCntrlNode(llvm_br_condition, fork_c, false);

							fork_c->CntrlOrderSuccs->push_back(branchC_node);
							branchC_node->CntrlOrderPreds->push_back(fork_c);
						}
					}
				
				} else {
					assert(llvm_br_condition->CntrlSuccs->size() + llvm_br_condition->JustCntrlSuccs->size() == 0);
					// no successors in the data or control paths, so do a direct connection between the condition and our branch
					llvm_br_condition->CntrlOrderSuccs->push_back(branchC_node);
					branchC_node->CntrlOrderPreds->push_back(llvm_br_condition);
				} 

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	*/


///////////////////////// Outdated code had it in thinking of inserting branches in the new control network!!!
/*int succs_situation = -1;

if(llvm_br_condition->CntrlSuccs->size() > 0 && llvm_br_condition->JustCntrlSuccs->size() == 0) {
	succs_situation = 1;
} else {
	if(llvm_br_condition->CntrlSuccs->size() == 0 && llvm_br_condition->JustCntrlSuccs->size() > 0) {
		succs_situation = 2;
	} else {
		if(llvm_br_condition->CntrlSuccs->size() > 0 && llvm_br_condition->JustCntrlSuccs->size() > 0) {
			succs_situation = 3;
		} else {
			succs_situation = 4;
		}
	}
}

assert(succs_situation != -1);
//////////////////////////////////////////////////////////////////

switch succs_situation {
	case 1:
		if(llvm_br_condition->CntrlSuccs->at(0)->type == Fork_) {	
			llvm_br_condition->CntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
			branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->CntrlSuccs->at(0));
		} else {
			assert(llvm_br_condition->CntrlSuccs->size() == 1);
			// create a new fork and connect to it the old and new succs

			ENode* fork = new ENode(Fork_, "fork", enode->BB);
			fork->id = fork_id++;
			enode_dag->push_back(fork);

			// Fork must be inserted at index 0 of the enode->CntrlSuccs and the original order of successors should be maintained in fork->CntrlSuccs
			aya_InsertNode(llvm_br_condition, fork);
			fork->CntrlOrderSuccs->push_back(branchC_node);
			branchC_node->CntrlOrderPreds->push_back(fork);
		}

	break;

	case 2:
		if(llvm_br_condition->JustCntrlSuccs->at(0)->type == Fork_c) {	
			llvm_br_condition->JustCntrlSuccs->at(0)->CntrlOrderSuccs->push_back(branchC_node);
			branchC_node->CntrlOrderPreds->push_back(llvm_br_condition->JustCntrlSuccs->at(0));
		} else {
			assert(llvm_br_condition->JustCntrlSuccs->size() == 1);
			// create a new fork and connect to it the old and new succs

			ENode* fork_c = new ENode(Fork_c, "forkC", enode->BB);
			fork_c->id = fork_id++;
			enode_dag->push_back(fork_c);

			// Fork must be inserted at index 0 of the enode->JustCntrlSuccs and the original order of successors should be maintained in fork_c->JustCntrlSuccs
			InsertCntrlNode(llvm_br_condition, fork_c, false);

			fork_c->CntrlOrderSuccs->push_back(branchC_node);
			branchC_node->CntrlOrderPreds->push_back(fork_c);
		}

	break;

	case 3:
		// since the condition has successors in both the data and control paths, for sure there is a fork_ successor!!

	break;

	case 4:

	break;

}*/

