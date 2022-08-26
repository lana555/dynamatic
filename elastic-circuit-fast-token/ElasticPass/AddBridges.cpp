#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

void CircuitGenerator::FindLoopDetails(LoopInfo& LI) {
	// TEMPORARY FOR DEBUGGING!!
	std::ofstream t_file;
	t_file.open("aya_loops_details.txt");

	std::vector<Loop*>* all_loops = new std::vector<Loop*>;

	for(auto& bbnode : *bbnode_dag) {
		// first identify and store all the loops present
		Loop* loop = LI.getLoopFor(bbnode->BB);
		// 12/10/2021: store this innermost loop, that the BB is inside, inside the BB
		bbnode->loop = loop;  // 02/02/2022: it could be a nullptr if the BB isnt inside any loop

		while(loop != nullptr) {
			// first, make sure that this loop wasn't stored before 
			auto pos = std::find(all_loops->begin(), all_loops->end(), loop);
			// if the loop isn't already stored, push it to the vector
			if (pos == all_loops->end()) {
				all_loops->push_back(loop);
			} 

			loop = loop->getParentLoop();
		}
	}

	// for each loop object, label its header, tail/exiting BBNodes
	for(auto& loop_ : *all_loops) {
		BasicBlock *loop_header = loop_->getHeader();
		BBMap->at(loop_header)->is_loop_header = true;
		BBMap->at(loop_header)->loop = loop_;

		for(int i = 0; i < bbnode_dag->size(); i++) {
			if(loop_->contains(bbnode_dag->at(i)->BB)) {
				if(loop_->isLoopLatch(bbnode_dag->at(i)->BB)) {
					bbnode_dag->at(i)->is_loop_latch = true;
					bbnode_dag->at(i)->loop_header_bb = loop_header;
					bbnode_dag->at(i)->loop = loop_;

					t_file << "\nBBNode " << aya_getNodeDotbbID(bbnode_dag->at(i)) << " is a loop latch of loop with loop_header called " << aya_getNodeDotbbID(BBMap->at(bbnode_dag->at(i)->loop_header_bb)) << "\n\n";
				} 

				if(loop_->isLoopExiting(bbnode_dag->at(i)->BB)) {
					bbnode_dag->at(i)->is_loop_exiting = true;
					bbnode_dag->at(i)->loop_header_bb = loop_header;
					bbnode_dag->at(i)->loop = loop_;

					t_file << "\nBBNode " << aya_getNodeDotbbID(bbnode_dag->at(i)) << " is a loop exiting of loop with loop_header called " << aya_getNodeDotbbID(BBMap->at(bbnode_dag->at(i)->loop_header_bb)) << "\n\n";
				}

			}
		} 
	} 

}


int CircuitGenerator::returnPosRemoveNode(std::vector<ENode*>* vec, ENode* elem) {
    auto pos = std::find(vec->begin(), vec->end(), elem);
	// std::string aya = getNodeDotNameNew(elem);

	assert(pos != vec->end());

//  && printf("\nFailed to find node (%s) in the array\n", aya.c_str()));


	int index = pos - vec->begin();

   	// vec->erase(pos);  // don't need to erase since I will overwrite (not pushback) that enode with the new branch enode 
    
    return index;
}

// Note: is_special_phi indicates the situation of the consumer being the PHI that needs us to discard either of its inputs by studying post-dominance on the edges!
// And llvm_predBB represents the predecessor of the BB containing the PHI (as indicated by LLVM)
// With the above note in mind, now we no longer need the last 2 parameters 
	// given that we currently use the function: ConnectProds_Cons_Merges and we will not need any of this when we move to FPL's algorithm
void CircuitGenerator::ConnectBrSuccs(llvm::BranchInst* BI, std::vector<ENode*>* branches, int j, bool cntrl_path_flag, bool is_special_phi, BasicBlock *llvm_predBB) {

	// AYA: 23/10/2021: added a check to see if the llvm Br has 1 or 2 succs!!!
	///////////////////////////////////////////////////////////////////////
	// Not all llvm branches have a true and a false successor, so need to check the number of successors first
	int llvm_br_num_of_succs = BI->getNumSuccessors();
	BasicBlock *falseBB, *trueBB;
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

	std::vector<ENode*> *true_succs, *false_succs;
	if(cntrl_path_flag) {
		true_succs = branches->at(j)->true_branch_succs_Ctrl;
		false_succs = branches->at(j)->false_branch_succs_Ctrl;
	} else {
		true_succs = branches->at(j)->true_branch_succs;
		false_succs = branches->at(j)->false_branch_succs;
	} 

	// if there is just one successor, then connect to the true blindly!
	if(falseBB == nullptr) {
		true_succs->push_back(branches->at(j+1));
	} else {
		// then it has 2 successors
		BasicBlock* bb_to_compare_with;
		if(j == branches->size()-2 && is_special_phi) {
			assert(llvm_predBB != nullptr);
			bb_to_compare_with = llvm_predBB;
		} else {
			bb_to_compare_with = branches->at(j+1)->BB;
		}

		//BasicBlock* bb_to_compare_with = branches->at(j+1)->BB;
		assert(bb_to_compare_with != nullptr);
		if(my_post_dom_tree->properlyDominates(bb_to_compare_with, falseBB) || bb_to_compare_with == falseBB) {
			false_succs->push_back(branches->at(j+1));
		} else {
			// 02/02/2022: If it's not in the true side, do another check for loops!
			assert(my_post_dom_tree->properlyDominates(bb_to_compare_with, trueBB) || bb_to_compare_with == trueBB);
			if(my_post_dom_tree->properlyDominates(bb_to_compare_with, trueBB) || bb_to_compare_with == trueBB) {
				true_succs->push_back(branches->at(j+1));
			} else {
				// if didn't work for the above conditions, then it must be inside a loop with multiple exits!!
				assert(BBMap->at(bb_to_compare_with)->loop != nullptr);

				// identify the BB that is its loop_latch
				Loop* check_loop = BBMap->at(bb_to_compare_with)->loop;
				// I use flags to loop over all BBs in case there are multiple loop latches, and to push back into the successor just once after checking all loop latches
				bool true_side_flag = false;
				bool false_side_flag = true;

				for(auto& bb : *bbnode_dag) {
					if(bb->loop == check_loop && bb->is_loop_latch) {
						// will compare with this bb!
						if(my_post_dom_tree->properlyDominates(bb->BB, falseBB) || bb->BB == falseBB) {
							false_side_flag = true;
						} else {
							if(my_post_dom_tree->properlyDominates(bb->BB, trueBB) || bb->BB == trueBB) {
								true_side_flag = true;
							}
						}
					}
				}

				if(false_side_flag) {
					false_succs->push_back(branches->at(j+1));
				} else {
					assert(true_side_flag);
					true_succs->push_back(branches->at(j+1));
				} 

			}

			// OLD: before 02/02/2022: and it must be post-dominating!
			/*assert(my_post_dom_tree->properlyDominates(bb_to_compare_with, trueBB) || bb_to_compare_with == trueBB );
			true_succs->push_back(branches->at(j+1));
			*/
		} 

	}  
	
}

void CircuitGenerator::ConnectBrPreds(llvm::BranchInst* BI, std::vector<ENode*>* branches, int j, ENode* enode, bool cntrl_path_flag) {

	std::vector<ENode*> *branch_preds;

	if(cntrl_path_flag) {
		branch_preds = 	branches->at(j)->JustCntrlPreds;
	} else {
		branch_preds = 	branches->at(j)->CntrlPreds;
	} 

/*
	-Our branch should have two predecessors: data and condition
	- Data should be .at(0) and condition should be .at(1)
*/ 		
	// connecting the data predecessor
	branch_preds->push_back(branches->at(j-1));
	// connecting the condition predecessor 		
	if(BI->isUnconditional()) {
		// so llvm's branch has no predecessors, so I must create an enode of type constant and treat it as the condition
		ENode* cst_condition = new ENode(Cst_, std::to_string(1).c_str(), enode->BB);
		// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
		cst_condition->cstValue = 1;
		enode_dag->push_back(cst_condition);
		cst_condition->id = cst_id++;

		// AYA: 16/11/2021
		cst_condition->is_const_br_condition = true;
		
		branch_preds->push_back(cst_condition);

		// AYA: 03/11/2021: Made a change to force the condition to be in the data network even if it's a branch_c node!!!
		// branches->at(j)->CntrlPreds->push_back(enode->CntrlPreds->at(0));
		
		if(cntrl_path_flag) {
			BBMap->at(enode->BB)->Cntrl_nodes->push_back(cst_condition);
			cst_condition->JustCntrlSuccs->push_back(branches->at(j));
		} else {
			cst_condition->CntrlSuccs->push_back(branches->at(j));
		} 
		

		/*
			Need to have the startC node as its control predecessor so that 							it can act as its producer and trigger it. 
		*/ 
		for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
			if(cnt_node->type == Start_) {
				cst_condition->JustCntrlPreds->push_back(cnt_node);
				cnt_node->JustCntrlSuccs->push_back(cst_condition);
				break; // found the start node, no need to continue looping
			}
		} 
						
	} else {
		// the condition is the only predecessor of llvm's branch, but we should connect it to the second predecessor of our branch!!

		// AYA: 03/11/2021: Made a change to force the condition to be in the data network even if it's a branch_c node!!!
		//branches->at(j)->CntrlPreds->push_back(enode->CntrlPreds->at(0));

		branch_preds->push_back(enode->CntrlPreds->at(0));
		if(cntrl_path_flag) {
			enode->CntrlPreds->at(0)->JustCntrlSuccs->push_back(branches->at(j));
		} else {
			enode->CntrlPreds->at(0)->CntrlSuccs->push_back(branches->at(j));
		}
		
	}

}

void CircuitGenerator::AddBridges(ENode* producer, ENode* consumer, std::vector<int>* bridges_indices, bool cntrl_path_flag, std::vector<ENode*>* branches, bool is_special_phi, BasicBlock *llvm_predBB, std::ofstream& h_file) {
	//std::vector<ENode*> branches;
	//if(bridges_indices == nullptr) 
	//	return;

	std::vector<ENode*>* producer_succs;
	std::vector<ENode*>* consumer_preds;

	node_type branch_type; // shall we create a Branch_c or Branch_n
	std::string branch_name; 
	
	if(cntrl_path_flag) {
		producer_succs = producer->JustCntrlSuccs;
		consumer_preds = consumer->JustCntrlPreds;
		branch_type = Branch_c;
		branch_name = "branchC";

	} else {
		producer_succs = producer->CntrlSuccs;
		consumer_preds = consumer->CntrlPreds;
		branch_type = Branch_n;
		branch_name = "branch";
	}
	
	// if there is actually a Bridge
	if(bridges_indices->size() > 0) {
		// removing the direct connection between the producer and consumer
		int prod_succ_index = returnPosRemoveNode(producer_succs, consumer);
		int cons_preds_index = returnPosRemoveNode(consumer_preds, producer);
		
		branches->push_back(producer);
		// create all the needed branches and put in a vector.. Note that bridges_indices.at(0) is the bridge closest to the consumer 
		for(int i = bridges_indices->size()-1; i >= 0; i--) {
	
			ENode* branch;
			branch = new ENode(branch_type, branch_name.c_str(), bbnode_dag->at(bridges_indices->at(i))->BB);
			if(cntrl_path_flag) {
				BBMap->at(bbnode_dag->at(bridges_indices->at(i))->BB)->Cntrl_nodes->push_back(branch);
			}

			branch->id = branch_id++;
			branches->push_back(branch);

			assert(branch->type == Branch_c || branch->type == Branch_n);
			
			enode_dag->push_back(branch);

			// Aya: added this to help in removing redundant branches
			branch->producer_to_branch = producer;
			branch->true_branch_succs = new std::vector<ENode*>;
			branch->false_branch_succs = new std::vector<ENode*>;
			branch->true_branch_succs_Ctrl = new std::vector<ENode*>;
			branch->false_branch_succs_Ctrl = new std::vector<ENode*>;

			h_file << "\n Added a Branch in BB number " << (bbnode_dag->at(bridges_indices->at(i))->Idx + 1) << "\n";
		}
		branches->push_back(consumer);
				
		producer_succs->at(prod_succ_index) = branches->at(1);

		for(int j = 1; j < branches->size()-1; j++) {
			//branches.at(j)->CntrlSuccs->push_back(branches.at(j+1));
			//branches.at(j)->CntrlPreds->push_back(branches.at(j-1));

			// search for LLVM branch in the same BB to extract the condition from it
			for(auto& enode : *enode_dag) {
				if(enode->type == Branch_ && enode->BB == branches->at(j)->BB) { 
					llvm::BranchInst* BI = dyn_cast<llvm::BranchInst>(enode->Instr);
					// connect the predecessors of the branch
					ConnectBrPreds(BI, branches, j, enode, cntrl_path_flag);

					ConnectBrSuccs(BI, branches, j, cntrl_path_flag, is_special_phi, llvm_predBB);

					break; // no need to continue looping 1 llvm branch is enough
				}
			}
		}
		consumer_preds->at(cons_preds_index) = branches->at(branches->size()-2);
	
/* Need to:
	1) Find the condition and connect it to the branch 
	2) Follow the conventions:
		- branch->CntrlPreds.at(0) should be the data producer enode
		- branch->CntrlPreds.at(1) should be the branch condition enode (I will get it by searching for a branch enode in the same BB as the bridgeBB)
		- branch->CntrlSuccs.at(0) should be in the true branch 
		- branch->CntrlSuccs.at(1) should be in the false branch
			
*/
	}  // braces of if(bridges_indices.size > 0)
}


// TBD: how to eliminate the redundancy of checking nodes repeated in different paths 

/*
Convention:
	- In the beginning, start is the first bbnode_index (i.e. consumer BB index) in a specific path.
	- Then we traverse starting from the consumer all the way to the producer searching for bridge BBs and pushing them into bridges_indices. Therefore, the first element in bridges_indices will be the bridge BB closest to the consumer.
*/

void CircuitGenerator::FindBridges(BBNode* consumer_BB, BBNode* producer_BB, std::vector<int> path, int start, bool recur_flag, std::vector<int>* bridges_indices) {
	int dd;
	for(dd = start; dd < path.size(); dd++) {
		// Aya: 23/10/2021: If producer is in a loop the consumer is not in, add branches only in loop exit not loop tail, because we care for when we will exit the loop!

		// if producer is in a loop that the consumer is not in 
		if((bbnode_dag->at(path.at(dd))->is_loop_exiting /* || bbnode_dag->at(path.at(dd))->is_loop_latch*/) && bbnode_dag->at(path.at(dd))->loop->contains(producer_BB->BB) && !bbnode_dag->at(path.at(dd))->loop->contains(consumer_BB->BB)) {
			bridges_indices->push_back(path.at(dd));
			//recur_flag = true;   
		} 

///////////////////////////////////////////////////////////////////////
		// identify the case of backward edges between the producer and consumer
		if(consumer_BB->Idx < producer_BB->Idx) {

// AYA: 01/12/2021: I uncommented the (|| bbnode_dag->at(path.at(dd))->is_loop_latch) in the following if condition!

			// if you come across any loop exit in the path
			if((bbnode_dag->at(path.at(dd))->is_loop_exiting || bbnode_dag->at(path.at(dd))->is_loop_latch && bbnode_dag->at(path.at(dd))!= consumer_BB)) {
				//if( bbnode_dag->at(path.at(dd))->loop == consumer_BB->loop && bbnode_dag->at(path.at(dd))->loop == producer_BB->loop ) {
				// 11/10/2021: changed the condition to check ONLY that this loop is the innermost loop that the consumer is inside!
				if(bbnode_dag->at(path.at(dd))->loop == consumer_BB->loop) {
					bridges_indices->push_back(path.at(dd));
					//recur_flag = true;  // FindBridges starting at it
				}
			}
			
		} else {
		
			// if producer is in a loop that the consumer is not in but producer is in
			/*if((bbnode_dag->at(path.at(dd))->is_loop_exiting || bbnode_dag->at(path.at(dd))->is_loop_latch) && bbnode_dag->at(path.at(dd))->loop->contains(producer_BB->BB) && !bbnode_dag->at(path.at(dd))->loop->contains(consumer_BB->BB)) {
				bridges_indices->push_back(path.at(dd));
				//recur_flag = true;   
			} else {*/
				if(bbnode_dag->at(path.at(dd))->CntrlSuccs->size() >= 2){
					//The post-dominator tree counts a BB as not post-dominating itself
					if(!my_post_dom_tree->properlyDominates(consumer_BB->BB, bbnode_dag->at(path.at(dd))->BB) && consumer_BB->BB != bbnode_dag->at(path.at(dd))->BB) {
						// this BB should be a Bridge
						bridges_indices->push_back(path.at(dd));
						//recur_flag = true;  // FindBridges starting at it
				 	}
				}

			//}
		
		}
////////////////////////////////////////////////////////////////////////

		/*if(recur_flag == true) {
		    break;
		}*/

	}

	/*if(recur_flag == true) {
	    FindBridges(bbnode_dag->at(path.at(dd)), producer_BB, path, dd+1, false, bridges_indices);
	}*/
}

void CircuitGenerator::FindBridges_special_loops(BBNode* consumer_BB, std::vector<int>* bridges_indices, ENode* consumer, ENode* producer) {
	// First, we need to check if they are connected with a backward edge or not
	bool is_backward_edge_flag = false;

	if(consumer->type == Phi_) {
		// check indices of the producer and consumer operations (enodes)
		///////// identifying its index in the enode_dag
		auto pos_cons = std::find(enode_dag->begin(), enode_dag->end(), consumer);
		assert(pos_cons != enode_dag->end());
		int index_cons = pos_cons - enode_dag->begin();
		/////////////////////////////////////////

		///////// identifying its index in the enode_dag
		auto pos_prod = std::find(enode_dag->begin(), enode_dag->end(), producer);
		assert(pos_prod != enode_dag->end());
		int index_prod = pos_prod - enode_dag->begin();
		////////////////////////////////////////////////

		if(index_cons < index_prod) {
			is_backward_edge_flag = true;
		}
	} else 
		if(consumer->type == Phi_n || consumer->type == Phi_c) {
			if(consumer == producer) {
				is_backward_edge_flag = true;
			}
	}

	if(is_backward_edge_flag) {
		// the bridges here are simply the indices of the loop exit and that's it
		// get the loop object stored inside the consumer_BB or producer_BB, it doesn't matter, they are both the same BB!
		Loop* loop_ = consumer_BB->loop;

		// loop over all BBs searching for the BB that is loop_latch or loop_exiting of this loop object

		//for(int i = 0; i < bbnode_dag->size(); i++) {
		for(int i = bbnode_dag->size()-1; i >= 0; i--) {
			if((/*bbnode_dag->at(i)->is_loop_latch ||*/ bbnode_dag->at(i)->is_loop_exiting) && bbnode_dag->at(i)->loop == loop_ ) {
			bridges_indices->push_back(bbnode_dag->at(i)->Idx);
			// break;  // took it away to place a branch on every loop exiting/latch?? Needs checking??!!
			}
		}
	}
	
} 


void CircuitGenerator::FindBridgesHelper(BBNode* consumer_BB, BBNode* producer_BB, std::vector<std::vector<int>>* final_paths, std::vector<int>* bridges_indices, ENode* consumer, ENode* producer){
	// Loop over all paths, and for each path do the recursive function to find the bridges of that path, then repeat for other paths
	
	if(consumer_BB == producer_BB) {
		//std::vector<int>* one_path_bridges_indices = new std::vector<int>;
		FindBridges_special_loops(consumer_BB, bridges_indices, consumer, producer);
		//bridges_indices->push_back(one_path_bridges_indices);

	} else {
		/*if(consumer_BB->is_loop_header && !consumer_BB->loop->contains(producer_BB->BB)) {
			// insert branch in the loop_pre_header
			BasicBlock* loop_pre_header_BB = consumer_BB->loop->getLoopPreheader();
			if(loop_pre_header_BB!=nullptr) {
				BBNode* pre_header_node = BBMap->at(loop_pre_header_BB);
				// identify its index in the bbnode_dag
				///////////////////////////////////////////////
				auto pos = std::find(bbnode_dag->begin(), bbnode_dag->end(), pre_header_node);
				assert(pos != bbnode_dag->end());
				int index = pos - bbnode_dag->begin();
				////////////////////////////////////////////////
				bridges_indices->push_back(bbnode_dag->at(index)->Idx);
			}
		}*/
		for(int kk = 0; kk < final_paths->size(); kk++) {
			//std::vector<int>* one_path_bridges_indices = new std::vector<int>;
	   		FindBridges(consumer_BB, producer_BB, final_paths->at(kk), 0, false, bridges_indices);
			//bridges_indices->push_back(one_path_bridges_indices);
		} 
	}

	// there should be at least one path pushed to this vector of vectors, even if the pushed path has no piers! (i.e., bridges_indices.at(0).size() could be 0)
	//assert(bridges_indices.size() > 0);

}

// Aya: 
/*
    enumerates all paths starting at the consumer all the way to the producer, and fills an array of paths where each path is constituted of indices of BBs in the bbnode_dag
*/
void CircuitGenerator::enumerate_paths_dfs(BBNode* current_BB, BBNode* producer_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths) {
	visited[current_BB->Idx] =  true;
	path[path_index] = current_BB->Idx; // start traversing from the consumer so it'll be the first BB index in the path 
	path_index++;

	if(current_BB == producer_BB) {  // 1 path is completed!
	    // insert this path in the structure final_paths
	    std::vector<int> temp(path, path + path_index);
	    final_paths->push_back(temp);
	}else {  // current node is still not the destination
	    // loop on the predecessor BBs of the current_BB
	    for(auto& bb_pred : *current_BB->CntrlPreds){
			if(!visited[bb_pred->Idx])
		    	enumerate_paths_dfs(bb_pred, producer_BB, visited, path, path_index, final_paths);
	    }
	}

	// if you reached here this means current_BB isn't in the path so remove it
	path_index--;
	visited[current_BB->Idx] = false;
}

void CircuitGenerator::enum_paths_to_self(BBNode* current_BB, BBNode* producer_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths, bool first, std::ofstream& t_file) {
	visited[current_BB->Idx] =  true;
	path[path_index] = current_BB->Idx; // start traversing from the consumer so it'll be the first BB index in the path 
	path_index++;
	
	t_file << "\nHii, I'm inside ur enumerate_to_self cyclic function\n";

	if(current_BB == producer_BB && !first) {  // 1 path is completed!
	    // insert this path in the structure final_paths
	    std::vector<int> temp(path, path + path_index);
	    final_paths->push_back(temp);
	}else {  // current node is still not the destination
	    // loop on the predecessor BBs of the current_BB
	    for(auto& bb_pred : *current_BB->CntrlPreds){
			if(!visited[bb_pred->Idx] && bb_pred!= current_BB)
		    	enum_paths_to_self(bb_pred, producer_BB, visited, path, path_index, final_paths, false, t_file);
	    }
	}

	// if you reached here this means current_BB isn't in the path so remove it
	path_index--;
	visited[current_BB->Idx] = false;
}

// AYA: 13/02/2022: Please note that I inside this function I detect if a single prodcuer can reach a consumer through multiple reachability points, but I support only 2 reachability points and I assume that any BB has at most 2 predecessors!!!!
void CircuitGenerator::ConnectProds_Cons(bool cntrl_path_flag, bool extra_cst_check) {
	std::ofstream dbg_file;
    dbg_file.open("new_piers_paths_check.txt");

	std::ofstream h_file;
    h_file.open("add_bridges_dbg.txt");

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

    std::vector<ENode*>* enode_preds;
	
	// For each enode operation, loop over its producers
	for (auto& enode : *enode_dag) {
	// the following condition is to skip nodes that are not in any BB e.g. MC and LSQ
		if(enode->BB == nullptr || enode->type == MC_ || enode->type == LSQ_) 
			continue;   // skip this iteration
		
		// AYA: 16/11/2021: NOTE THAT I NO LONGER NEED THIS CONDITION THUS, I'M NEVER PASSING THIS FLAG TO TRUE
		// AYA: 31/10/2021
		if(extra_cst_check) {
			if(enode->type != Cst_)
				continue;
		}

		// deciding if we should check the predecessors in the data or control path
		if(cntrl_path_flag) {
			enode_preds = enode->JustCntrlPreds;
		} else {
			enode_preds = enode->CntrlPreds;
		}

		for (auto& pred : *enode_preds) {
			//if (enode->BB != pred->BB || ((BBMap->at(pred->BB)->is_loop_latch || BBMap->at(pred->BB)->is_loop_exiting) && enode->BB == BBMap->at(pred->BB)->loop->getHeader() )  ) {
			// AYA: 31/10/2021
			if(extra_cst_check) {
				if(pred->type == Branch_n || pred->type == Branch_c)
					continue;
			}


		// the following condition is to skip nodes that are not in any BB e.g., MC and LSQ
			if(pred->BB == nullptr || pred->type == MC_ || pred->type == LSQ_)
				continue;

			
			if (enode->BB != pred->BB || (enode->BB == pred->BB && BBMap->at(enode->BB)->is_loop_header) ) {

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				 dbg_file << "\n-------------------------------------\n";
				 dbg_file << "\n For (Producer,Consumer) = (" << getNodeDotNameNew(pred) << "in BB " << aya_getNodeDotbbID(BBMap->at(pred->BB)) << ", " << getNodeDotNameNew(enode) << "in BB " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << "):  \n";
				///////////////////////////////////////////////////
			
			// reseting the data structures for a new prod_con pair
				for(int i = 0; i < cfg_size; i++){
					visited[i] = false;
				}
				path_index = 0; 

				final_paths->clear();
				bridges_indices->clear();

				branches->clear();

				enumerate_paths_dfs(BBMap->at(enode->BB), BBMap->at(pred->BB), visited, path, path_index, final_paths);

				assert(final_paths->size() > 0);

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				// checking the enumerated paths
				for(int kk = 0; kk < final_paths->size(); kk++) {
					dbg_file << "Printing Path number " << kk << ":\n";
					for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
						dbg_file << final_paths->at(kk).at(hh) + 1 << ", ";
					} 
					dbg_file << "\n";
				} 
				////////////////////////////////////////////////////////////////

			// loop over all_paths and returns indices of bridges
				FindBridgesHelper(BBMap->at(enode->BB), BBMap->at(pred->BB), final_paths, bridges_indices, enode, pred);

				AddBridges(pred, enode, bridges_indices, cntrl_path_flag, branches, false, nullptr, h_file);

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				// checking the bridges_indices extracted from all enumerated paths
				dbg_file << "Printing bridges_indices:\n";
				if(bridges_indices->size() == 0) {
					dbg_file << "No Bridges for this pair!!";
				}
				for(int k = 0; k < bridges_indices->size(); k++)
					dbg_file << bridges_indices->at(k) + 1 << ", ";

				dbg_file << "\n\n";
				////////////////////////////////////////////////////////////////


			}

		}

	}

	delete[] visited;
	delete[] path;

	delete bridges_indices;

	delete branches;
}


void CircuitGenerator::ConnectProds_Cons_wrong(bool cntrl_path_flag, bool extra_cst_check) {

	std::ofstream dbg_file;
    dbg_file.open("new_piers_paths_check_wrong.txt");

	std::ofstream h_file;
    h_file.open("add_bridges_dbg_wrong.txt");

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

	// AYA: 13/02/2022: Will convert bridges_indices to a vector of vectors to not mix up pierBBs from different paths, then before Add_bridges, check whether paths can be Merged or not.. If not, then will need to insert a Merge to account for multiple reachability points between a single producer and a consumer..

	// 2) vector of indices of bbs in the bbnode_dag that are bridges in any of the paths
	//std::vector<int>* bridges_indices = new std::vector<int>;
	std::vector<std::vector<int>*>* bridges_indices = new std::vector<std::vector<int>*>;

    std::vector<ENode*>* enode_preds;
	
	// For each enode operation, loop over its producers
	for (auto& enode : *enode_dag) {
	// the following condition is to skip nodes that are not in any BB e.g. MC and LSQ
		if(enode->BB == nullptr || enode->type == MC_ || enode->type == LSQ_) 
			continue;   // skip this iteration
		
		// AYA: 16/11/2021: NOTE THAT I NO LONGER NEED THIS CONDITION THUS, I'M NEVER PASSING THIS FLAG TO TRUE
		// AYA: 31/10/2021
		if(extra_cst_check) {
			if(enode->type != Cst_)
				continue;
		}

		// deciding if we should check the predecessors in the data or control path
		if(cntrl_path_flag) {
			enode_preds = enode->JustCntrlPreds;
		} else {
			enode_preds = enode->CntrlPreds;
		}

		for (auto& pred : *enode_preds) {
			//if (enode->BB != pred->BB || ((BBMap->at(pred->BB)->is_loop_latch || BBMap->at(pred->BB)->is_loop_exiting) && enode->BB == BBMap->at(pred->BB)->loop->getHeader() )  ) {
			// AYA: 31/10/2021
			if(extra_cst_check) {
				if(pred->type == Branch_n || pred->type == Branch_c)
					continue;
			}


		// the following condition is to skip nodes that are not in any BB e.g., MC and LSQ
			if(pred->BB == nullptr || pred->type == MC_ || pred->type == LSQ_)
				continue;

			if (enode->BB != pred->BB || (enode->BB == pred->BB && BBMap->at(enode->BB)->is_loop_header) ) {

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				 dbg_file << "\n-------------------------------------\n";
				 dbg_file << "\n For (Producer,Consumer) = (" << getNodeDotNameNew(pred) << "in BB " << aya_getNodeDotbbID(BBMap->at(pred->BB)) << ", " << getNodeDotNameNew(enode) << "in BB " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << "):  \n";
				///////////////////////////////////////////////////
			
			// reseting the data structures for a new prod_con pair
				for(int i = 0; i < cfg_size; i++){
					visited[i] = false;
				}
				path_index = 0; 

				final_paths->clear();
				bridges_indices->clear();

				branches->clear();

				enumerate_paths_dfs(BBMap->at(enode->BB), BBMap->at(pred->BB), visited, path, path_index, final_paths);

				assert(final_paths->size() > 0);

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				// checking the enumerated paths
				for(int kk = 0; kk < final_paths->size(); kk++) {
					dbg_file << "Printing Path number " << kk << ":\n";
					for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
						dbg_file << final_paths->at(kk).at(hh) + 1 << ", ";
					} 
					dbg_file << "\n";
				} 
				////////////////////////////////////////////////////////////////

			// loop over all_paths and returns indices of bridges
				//FindBridgesHelper(BBMap->at(enode->BB), BBMap->at(pred->BB), final_paths, bridges_indices, enode, pred);

				// 13/02/2022: added this to see if the paths of pierBBs inside the bridges_indices vector of vectors can be merged, and if so, merge it!

				// ITS IMPlementation is STILL to be done!!!
				//merge_paths_if_possible(bridges_indices);
				
				// for sure we should have at least 1 path, even if it has no piers
				assert(bridges_indices->size() > 0);

				if(bridges_indices->size() > 1) {
					// 19/02/2022: TAKE CARE: THIS ASSERT SEEMS TO BE INCORRECT?!!!
					//assert(bridges_indices->size() == 2);

					// need to insert a Merge between the prod (pred), and cons and it should serve as the consumer!! The CntrlPred.at(0) should be at the 0th input of the Merge

					/*ENode* phi = new ENode(Phi_n, "phi", enode->BB);  
					phi->id = phi_id++;
					enode_dag->push_back(phi);
				// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi of our creation
					phi->producer_to_branch = pred;

					// if there are multiple reachability points, so for sure the consumerBB should have 2 predecessors!
					assert(BBMap->at(enode->BB)->CntrlPreds->size() == 2);
					// the goal of the following loop is to sort the vectors in the bridges_indices (if needed) to make sure that the path .at(0) enters the consumerBB through its 0th predecessor and same goes for the .at(1)
					for(int jj = 0; jj < BBMap->at(enode->BB)->CntrlPreds->size(); jj++) {
						BBNode* cons_predBB = BBMap->at(enode->BB)->CntrlPreds->at(jj);
						
						// I want to see which of the two paths in bridges_indices is taking this edge to arrive to the consumer BB
						BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
						bbnode_dag->push_back(virtualBB);
						insertVirtualBB(enode, virtualBB, cons_predBB);

						// take the path that has ALL its piers post-dominated by virtualBB!!
							// check first the longer path to give it priority over the other!
						sort_bridges_paths(bridges_indices); // makes the longest path first


						// 


						for(int kk = 0; kk < bridges_indices->size(); kk++) {
							bool all_post_dom = true;
							for(int zz = 0; zz < bridges_indices->at(kk)->size(); zz) {

							}
							if(all_post_dom) {
								// found the path corresponding to 

							} 
							if(!my_is_post_dom(BBMap->at(enode->CntrlPreds->at(i)->BB), virtualBB, endBB)) 			{
								break;
							}

						}

						removeVirtualBB(enode, virtualBB, llvm_predBB);

					}*/

				} else {
					assert(bridges_indices->size() == 1);
					AddBridges(pred, enode, bridges_indices->at(0), cntrl_path_flag, branches, false, nullptr, h_file);
				}
	

				/////////////////////////////////////// TEMP. DEBUGGING!!!
				// checking the bridges_indices extracted from all enumerated paths
				dbg_file << "Printing bridges_indices:\n";
				if(bridges_indices->size() == 0) {
					dbg_file << "No Bridges for this pair!!";
				}
				for(int k = 0; k < bridges_indices->size(); k++)
					dbg_file << bridges_indices->at(k) + 1 << ", ";

				dbg_file << "\n\n";
				////////////////////////////////////////////////////////////////

			// transform the enode_dag to account for the Bridges
			// Aya: 30/09/2021: the last two parameters have to do with adding branches for the special phis.. Here we don't care about them and can ignore them!!

				// TEMPORARILY, JUST PASSING TO IT A SINGLE PATH!!!
				//	AddBridges(pred, enode, bridges_indices->at(0), cntrl_path_flag, branches, false, nullptr, h_file);

				// temporarily for debugging!!
			 	//printDbgBranches(branches, cntrl_path_flag);	
			}  //BRACES OF THE IF THE 2 BBS ARENOT EQUAL TO EACHOTHER!!! 

		}
	}

	delete[] visited;
	delete[] path;

	delete bridges_indices;

	delete branches;
}


// AYA: 11/02/2022: as opposed to the original enumerate paths function available above, this function returns path starting from the producer to the consumer..

void CircuitGenerator::enumerate_paths_dfs_start_to_end(BBNode* start_BB, BBNode* end_BB, bool visited[], int path[], int path_index, std::vector<std::vector<int>>* final_paths) {
	visited[start_BB->Idx] =  true;
	path[path_index] = start_BB->Idx; // start traversing from the start_BB so it'll be the first BB index in the path 
	path_index++;

	if(start_BB == end_BB) {  // 1 path is completed!
	    // insert this path in the structure final_paths
	    std::vector<int> temp(path, path + path_index);
	    final_paths->push_back(temp);
	}else {  // current node is still not the destination
	    // loop on the successor BBs of the start_BB
	    for(auto& bb_succs : *start_BB->CntrlSuccs){
			if(!visited[bb_succs->Idx])
		    	enumerate_paths_dfs_start_to_end(bb_succs, end_BB, visited, path, path_index, final_paths);
	    }
	}

	// if you reached here this means current_BB isn't in the path so remove it
	path_index--;
	visited[start_BB->Idx] = false;
}

bool CircuitGenerator::my_is_post_dom(BBNode* firstBB, BBNode* secondBB, BBNode* endBB) {
	// variables needed inside enumerate_paths
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

	// find all the paths between the firstBB and the exit of the graph
	enumerate_paths_dfs_start_to_end(firstBB, endBB, visited, path, path_index, final_paths);

	/////////////////////////////////////// TEMP. DEBUGGING!!!
	// checking the enumerated paths
	/*for(int kk = 0; kk < final_paths->size(); kk++) {
		dbg_file << "Printing Path number " << kk << ":\n";
		for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
			dbg_file << final_paths->at(kk).at(hh) + 1 << ", ";
		} 
		dbg_file << "\n";
	} */

	/////////////////////////////////////////////////////////
	// check if the secondBB doesn't post-dominate the firstBB by checking if it doesn't exit in all paths between the producerBB and the endBB
	bool is_postDom = true;
	for(int kk = 0; kk < final_paths->size(); kk++) {
		bool found_secondBB = false;
		for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
			if(final_paths->at(kk).at(hh) == secondBB->Idx)
				found_secondBB = true;
		}

		if(!found_secondBB) {  // so virtualBB is not post-dom the producerBB
			is_postDom = false;
			break;
		}
	}	

	delete[] visited;
	delete[] path;
	delete final_paths;
	
	return is_postDom;

}

void CircuitGenerator::removeVirtualBB(ENode* enode, BBNode* virtualBB, BasicBlock *llvm_predBB) {

	auto pos = std::find(BBMap->at(llvm_predBB)->CntrlSuccs->begin(), BBMap->at(llvm_predBB)->CntrlSuccs->end(), virtualBB);

	assert(pos != BBMap->at(llvm_predBB)->CntrlSuccs->end());

	int index = pos - BBMap->at(llvm_predBB)->CntrlSuccs->begin();

	auto pos_2 = std::find(BBMap->at(enode->BB)->CntrlPreds->begin(), BBMap->at(enode->BB)->CntrlPreds->end(), virtualBB);

	assert(pos_2 != BBMap->at(enode->BB)->CntrlPreds->end());

	int index_2 = pos_2 - BBMap->at(enode->BB)->CntrlPreds->begin();

	BBMap->at(llvm_predBB)->CntrlSuccs->at(index) = BBMap->at(enode->BB);
	BBMap->at(enode->BB)->CntrlPreds->at(index_2) = BBMap->at(llvm_predBB);

	auto pos_3 = std::find(bbnode_dag->begin(), bbnode_dag->end(), virtualBB);
	assert(pos_3 != bbnode_dag->end());
	bbnode_dag->erase(pos_3);

}

void CircuitGenerator::insertVirtualBB(ENode* enode, BBNode* virtualBB, BasicBlock *llvm_predBB) {

	///////////////////////////////////////////
	// 1st: find the index of the BB of the enode (i.e., the BB containing the Merge currently under study) in the successors of llvm_predBB BB 
	auto pos = std::find(BBMap->at(llvm_predBB)->CntrlSuccs->begin(), BBMap->at(llvm_predBB)->CntrlSuccs->end(), BBMap->at(enode->BB));

	assert(pos != BBMap->at(llvm_predBB)->CntrlSuccs->end());

	int index = pos - BBMap->at(llvm_predBB)->CntrlSuccs->begin();

	BBMap->at(llvm_predBB)->CntrlSuccs->at(index) = virtualBB;

	virtualBB->CntrlPreds->push_back(BBMap->at(llvm_predBB));

	///////////////////////////////////////////
	// 2nd: find the index of the llvm_predBB BB in the predecessors of the BB of the enode (i.e., the BB containing the Merge currently under study)
	auto pos_2 = std::find(BBMap->at(enode->BB)->CntrlPreds->begin(), BBMap->at(enode->BB)->CntrlPreds->end(), BBMap->at(llvm_predBB));

	assert(pos_2 != BBMap->at(enode->BB)->CntrlPreds->end());

	int index_2 = pos_2 - BBMap->at(enode->BB)->CntrlPreds->begin();

	BBMap->at(enode->BB)->CntrlPreds->at(index_2) = virtualBB;

	virtualBB->CntrlSuccs->push_back(BBMap->at(enode->BB));
	//////////////////////////////////////////////

	virtualBB->is_virtBB = true;
		
}


// Aya: 08/02/2022: Properly connecting the producers to consumers of type Merge by inserting a virtualBB and running the bridger algorithm between the producer and the virtualBB instead of the original Merge consumer..
void CircuitGenerator::ConnectProds_Cons_Merges() {
	// Draft of LLVM functions I thought might be useful if I were to rely on LLVM's post-dominator tree
	/*
	BasicBlock* virt_bb = BasicBlock::Create(enode->BB->getContext(), "", &F, enode->BB);
	post_dom_tree_copy->addNewBlock(virt_bb, llvm_predBB);
	*/

	std::ofstream dbg_file;
    dbg_file.open("virtualBBs_check.txt");

	
	// save a copy of the last BB (END) in the CFG before doing any modifications in bbnode_dag
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(auto & enode : *enode_dag) {
		// for every LLVM PHI
		if(enode->type == Phi_) {
			// skip Merges that are at a loop header and fed through a backward edge.
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(1)->BB)->Idx) )
				continue;

			// loop over the predecessors (producers feeding it)
			for(int i = 0; i < enode->CntrlPreds->size(); i++) {
				// check that piers were not already inserted by the previous ConnectProds_Cons
				if(enode->CntrlPreds->at(i)->type != Branch_ && enode->CntrlPreds->at(i)->type != Branch_n) {
					// 1st insert a virtualBB at the edge between the Merge consumer and the compiler's "coming from BB" corresponding to the current producer

					BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
					bbnode_dag->push_back(virtualBB);

					BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);

					insertVirtualBB(enode, virtualBB, llvm_predBB);
	
					// 3rd check if the producer post-dom the virutalBB
					if(!my_is_post_dom(BBMap->at(enode->CntrlPreds->at(i)->BB), virtualBB, endBB)) {
						dbg_file << "BB " << aya_getNodeDotbbID(virtualBB) << " does not post-dominate BB " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";	

						// insert a branch in the producer BB
						ENode* branch = new ENode(Branch_n, "branch", enode->CntrlPreds->at(i)->BB);
						branch->id = branch_id++;
						enode_dag->push_back(branch);

						// Aya: added this to help in removing redundant branches
						branch->producer_to_branch = enode->CntrlPreds->at(i);
						branch->true_branch_succs = new std::vector<ENode*>;
						branch->false_branch_succs = new std::vector<ENode*>;
						branch->true_branch_succs_Ctrl = new std::vector<ENode*>;
						branch->false_branch_succs_Ctrl = new std::vector<ENode*>;

					// search for LLVM branch in the same BB to extract the condition from it
						llvm::BranchInst* BI;
						ENode* llvm_branch;
						for(auto& enode_ : *enode_dag) {
							if(enode_->type == Branch_ && enode_->BB == branch->BB) { 
								BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
								llvm_branch = enode_;
								break; // no need to continue looping 1 llvm branch is enough
							}
						}

						// connecting the branch successors
						BasicBlock *falseBB, *trueBB;
						
						trueBB = BI->getSuccessor(0);
						falseBB = BI->getSuccessor(1);

						// check the coming_from BB of the Merge consumer
						if(llvm_predBB == enode->CntrlPreds->at(i)->BB) {
							// the consumerBB must be = to trueBB or falseBB!
							if(enode->BB == falseBB) {
								branch->false_branch_succs->push_back(enode);
							} else {
								assert(enode->BB == trueBB);
								branch->true_branch_succs->push_back(enode);
							}
						} else {
						// this coming_from BB must be post-dom one of the trueBB or falseBB 
							if(my_post_dom_tree->properlyDominates(llvm_predBB, falseBB) || llvm_predBB == falseBB) 	{
								// as a sanity check, it shouldn't post-dom the trueBB!!
								assert(!my_post_dom_tree->properlyDominates(llvm_predBB, trueBB) || llvm_predBB == trueBB);
								branch->false_branch_succs->push_back(enode);
							} else {
								assert(my_post_dom_tree->properlyDominates(llvm_predBB, trueBB) || llvm_predBB == trueBB);
								branch->true_branch_succs->push_back(enode);
							}

						} 

						// connecting the branch predecessors

						/*
							-Our branch should have two predecessors: data and condition
							- Data should be .at(0) and condition should be .at(1)
						*/ 	
						// connecting the data predecessor
						branch->CntrlPreds->push_back(enode->CntrlPreds->at(i));
	
						// connecting the condition predecessor 
						if(BI->isUnconditional()) {
							// so llvm's branch has no predecessors, so I must create an enode of type constant and treat it as the condition
							ENode* cst_condition = new ENode(Cst_, std::to_string(1).c_str(), enode->BB);
							// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
							cst_condition->cstValue = 1;
							enode_dag->push_back(cst_condition);
							cst_condition->id = cst_id++;

							// AYA: 16/11/2021
							cst_condition->is_const_br_condition = true;

							branch->CntrlPreds->push_back(cst_condition);
							cst_condition->CntrlSuccs->push_back(branch);

							/*
								Need to have the startC node as its control predecessor so that 								it can act as its producer and trigger it. 
							*/ 
							for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
								if(cnt_node->type == Start_) {
									cst_condition->JustCntrlPreds->push_back(cnt_node);
									cnt_node->JustCntrlSuccs->push_back(cst_condition);
									break; // found the start node, no need to continue looping
								}
							} 

						} else {
							llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(branch);
							branch->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
						} 
						//////////////////////////////////////////////

						// cut the original connection between the data producer and consumer
						auto pos_cons = std::find(enode->CntrlPreds->at(i)->CntrlSuccs->begin(), enode->CntrlPreds->at(i)->CntrlSuccs->end(), enode);

						assert(pos_cons != enode->CntrlPreds->at(i)->CntrlSuccs->end());

						int index_cons = pos_cons - enode->CntrlPreds->at(i)->CntrlSuccs->begin();
						enode->CntrlPreds->at(i)->CntrlSuccs->at(index_cons) = branch;

						enode->CntrlPreds->at(i) = branch;
						////////////////////////////////////////////////////////

					} 

					// 4th remove the inserted virtualBB
					removeVirtualBB(enode, virtualBB, llvm_predBB); 			

				}

			}

		}

	}

}

// DIDN'T DO IT IN THE END!!!!!!!!
void CircuitGenerator::ConnectProds_Cons_Merges_cntrl() {
	// Draft of LLVM functions I thought might be useful if I were to rely on LLVM's post-dominator tree
	/*
	BasicBlock* virt_bb = BasicBlock::Create(enode->BB->getContext(), "", &F, enode->BB);
	post_dom_tree_copy->addNewBlock(virt_bb, llvm_predBB);
	*/

	std::ofstream dbg_file;
    dbg_file.open("virtualBBs_check_cntrl.txt");

	
	// save a copy of the last BB (END) in the CFG before doing any modifications in bbnode_dag
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(auto & enode : *enode_dag) {
		// for every LLVM PHI
		if(enode->type == Phi_) {
			// skip Merges that are at a loop header and fed through a backward edge.
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(1)->BB)->Idx) )
				continue;

			// loop over the predecessors (producers feeding it)
			for(int i = 0; i < enode->CntrlPreds->size(); i++) {
				// check that piers were not already inserted by the previous ConnectProds_Cons
				if(enode->CntrlPreds->at(i)->type != Branch_ && enode->CntrlPreds->at(i)->type != Branch_n) {
					// 1st insert a virtualBB at the edge between the Merge consumer and the compiler's "coming from BB" corresponding to the current producer

					BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
					bbnode_dag->push_back(virtualBB);

					BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);

					insertVirtualBB(enode, virtualBB, llvm_predBB);
	
					// 3rd check if the producer post-dom the virutalBB
					if(!my_is_post_dom(BBMap->at(enode->CntrlPreds->at(i)->BB), virtualBB, endBB)) {
						dbg_file << "BB " << aya_getNodeDotbbID(virtualBB) << " does not post-dominate BB " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";	

						// insert a branch in the producer BB
						ENode* branch = new ENode(Branch_n, "branch", enode->CntrlPreds->at(i)->BB);
						branch->id = branch_id++;
						enode_dag->push_back(branch);

						// Aya: added this to help in removing redundant branches
						branch->producer_to_branch = enode->CntrlPreds->at(i);
						branch->true_branch_succs = new std::vector<ENode*>;
						branch->false_branch_succs = new std::vector<ENode*>;
						branch->true_branch_succs_Ctrl = new std::vector<ENode*>;
						branch->false_branch_succs_Ctrl = new std::vector<ENode*>;

					// search for LLVM branch in the same BB to extract the condition from it
						llvm::BranchInst* BI;
						ENode* llvm_branch;
						for(auto& enode_ : *enode_dag) {
							if(enode_->type == Branch_ && enode_->BB == branch->BB) { 
								BI = dyn_cast<llvm::BranchInst>(enode_->Instr);
								llvm_branch = enode_;
								break; // no need to continue looping 1 llvm branch is enough
							}
						}

						// connecting the branch successors
						BasicBlock *falseBB, *trueBB;
						
						trueBB = BI->getSuccessor(0);
						falseBB = BI->getSuccessor(1);

						// check the coming_from BB of the Merge consumer
						if(llvm_predBB == enode->CntrlPreds->at(i)->BB) {
							// the consumerBB must be = to trueBB or falseBB!
							if(enode->BB == falseBB) {
								branch->false_branch_succs->push_back(enode);
							} else {
								assert(enode->BB == trueBB);
								branch->true_branch_succs->push_back(enode);
							}
						} else {
						// this coming_from BB must be post-dom one of the trueBB or falseBB 
							if(my_post_dom_tree->properlyDominates(llvm_predBB, falseBB) || llvm_predBB == falseBB) 	{
								// as a sanity check, it shouldn't post-dom the trueBB!!
								assert(!my_post_dom_tree->properlyDominates(llvm_predBB, trueBB) || llvm_predBB == trueBB);
								branch->false_branch_succs->push_back(enode);
							} else {
								assert(my_post_dom_tree->properlyDominates(llvm_predBB, trueBB) || llvm_predBB == trueBB);
								branch->true_branch_succs->push_back(enode);
							}

						} 

						// connecting the branch predecessors

						/*
							-Our branch should have two predecessors: data and condition
							- Data should be .at(0) and condition should be .at(1)
						*/ 	
						// connecting the data predecessor
						branch->CntrlPreds->push_back(enode->CntrlPreds->at(i));
	
						// connecting the condition predecessor 
						if(BI->isUnconditional()) {
							// so llvm's branch has no predecessors, so I must create an enode of type constant and treat it as the condition
							ENode* cst_condition = new ENode(Cst_, std::to_string(1).c_str(), enode->BB);
							// this is very important to specify the value of the constant which should be 1 here to make the branch true all the time!!
							cst_condition->cstValue = 1;
							enode_dag->push_back(cst_condition);
							cst_condition->id = cst_id++;

							// AYA: 16/11/2021
							cst_condition->is_const_br_condition = true;

							branch->CntrlPreds->push_back(cst_condition);
							cst_condition->CntrlSuccs->push_back(branch);

							/*
								Need to have the startC node as its control predecessor so that 								it can act as its producer and trigger it. 
							*/ 
							for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
								if(cnt_node->type == Start_) {
									cst_condition->JustCntrlPreds->push_back(cnt_node);
									cnt_node->JustCntrlSuccs->push_back(cst_condition);
									break; // found the start node, no need to continue looping
								}
							} 

						} else {
							llvm_branch->CntrlPreds->at(0)->CntrlSuccs->push_back(branch);
							branch->CntrlPreds->push_back(llvm_branch->CntrlPreds->at(0));
						} 
						//////////////////////////////////////////////

						// cut the original connection between the data producer and consumer
						auto pos_cons = std::find(enode->CntrlPreds->at(i)->CntrlSuccs->begin(), enode->CntrlPreds->at(i)->CntrlSuccs->end(), enode);

						assert(pos_cons != enode->CntrlPreds->at(i)->CntrlSuccs->end());

						int index_cons = pos_cons - enode->CntrlPreds->at(i)->CntrlSuccs->begin();
						enode->CntrlPreds->at(i)->CntrlSuccs->at(index_cons) = branch;

						enode->CntrlPreds->at(i) = branch;
						////////////////////////////////////////////////////////

					} 

					// 4th remove the inserted virtualBB
					removeVirtualBB(enode, virtualBB, llvm_predBB); 			

				}

			}

		}

	}

}


// 08/02/2022: if the above function works, I will not use the following function anymore!!

// new function for adding extra branches in case the pred of a Phi_ is not guaranteed to come although the producer and consumer BBs are post-dominating!!
/*void CircuitGenerator::ConnectProds_Cons_extraPhis() {
	std::ofstream h_file;

	// variables needed inside enumerate_paths
	int cfg_size = bbnode_dag->size();
	bool *visited = new bool[cfg_size];

	int *path = new int[cfg_size];
	int path_index = 0; 

	// Data structures that will hold the results of different intermediate analysis
	// 1) vector of all the possible paths between the producer and consumer
	std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
	
	// 13/02/2022 TAKE CARE: I changed the type of bridges_indices to a vector of vectors to account for multiple reachability points between one producer and consumer
	// 2) vector of indices of bbs in the bbnode_dag that are bridges in any of the paths
	std::vector<std::vector<int>*> bridges_indices;// = new std::vector<int>;

	// the array of branches (can be a single branch) added to the enode_dag in AddBridges()
	std::vector<ENode*>* branches = new std::vector<ENode*>;

	// For the BB that the LLVM instruction tells this PHi predecessor is coming from, put another BB between it and the PHi consumer and assume that it's the consumer to identify the bridge BBs, then insert the actual BB between the original producer and consumer!!

	for(auto & enode : *enode_dag) {
		// for every LLVM PHI
		if(enode->type == Phi_) {
			for(int i = 0; i < enode->CntrlPreds->size(); i++) {
				// resetting the data structures for a new prod_con pair
				for(int i = 0; i < cfg_size; i++){
					visited[i] = false;
				}
				path_index = 0; 
				final_paths->clear();
				bridges_indices.clear();
				branches->clear();

	// need to identify its current predecessor BBs and see if they are different from the real LLVM predecessor BBs (also from the CFG)
				BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);

				if(enode->CntrlPreds->at(i)->type != Branch_ && enode->CntrlPreds->at(i)->type != Branch_n) {
					if(llvm_predBB != enode->CntrlPreds->at(i)->BB) {
				// Sanity check: there must be a direct connection between the llvm_predBB and the consumerBB 
						assert(contains(BBMap->at(enode->BB)->CntrlPreds, BBMap->at(llvm_predBB)));

// find the bridges between the original producer BB and the llvm_predBB instead of the original consumer's BB!!
						enumerate_paths_dfs(BBMap->at(llvm_predBB), BBMap->at(enode->CntrlPreds->at(i)->BB), visited, path, path_index, final_paths);

			// loop over all_paths and returns indices of bridges
			// note here I'm passing the producer and consumer enodes, but they will not be used anywhere inside the function in this case, so don't worry about them!!
						FindBridgesHelper(BBMap->at(llvm_predBB), BBMap->at(enode->CntrlPreds->at(i)->BB), final_paths, bridges_indices, enode, enode->CntrlPreds->at(i));

			// transform the enode_dag to account for the Bridges
			// take care, now that we identified the bridges indices, place them between the ORIGINAL producer and consumer!!
						AddBridges(enode->CntrlPreds->at(i), enode, bridges_indices.at(0), false, branches, true, llvm_predBB, h_file);  // the false parameter is to tell that we are in the data path not the control path!! And the true parameter is to tell the function we are currently doing connections for this special Phi_


					} else {
			   // Sanity check: the coming from BB of LLVM must be the same as the original producer BB of the Phi operation and there must be a direct connection between the original producer of the Phi and the consumerBB.. 
						assert(llvm_predBB == enode->CntrlPreds->at(i)->BB 
								&&
 								contains(BBMap->at(enode->BB)->CntrlPreds, BBMap->at(enode->CntrlPreds->at(i)->BB))
							  );

						// check, if the llvm_predBB (which is the same as the original producerBB) that is a direct predecessor of the PhiBB has 2 successors, this means you will need to insert a single branch between the original producer and consumer!!
						// It is guaranteed to be a single branch because the 2 BBs are connected with a direct connection!!!
						if(BBMap->at(enode->CntrlPreds->at(i)->BB)->CntrlSuccs->size() > 1) {
							// Sanity check: it is at most 2!
							assert(BBMap->at(enode->CntrlPreds->at(i)->BB)->CntrlSuccs->size() == 2);
							bridges_indices.at(0)->push_back(BBMap->at(enode->CntrlPreds->at(i)->BB)->Idx);
							AddBridges(enode->CntrlPreds->at(i), enode, bridges_indices.at(0), false, branches, false, nullptr, h_file);

							
						}  // otherwise, we don't need a branch! do nothing!!
						
					} 

				} 
			}

		}
	} 

	delete[] visited;
	delete[] path;

	delete final_paths;
	for(int i = 0; i < bridges_indices.size(); i++) {
		delete bridges_indices.at(i);
	} 
	 
	delete branches;

} 
*/


// The following function is temporarily for debugging!!
void CircuitGenerator::printDbgBranches(std::vector<ENode*>* branches, bool cntrl_br_flag) {
	std::ofstream br_succ_file;
	br_succ_file.open("aya_br_succ_file.txt");

	std::vector<ENode*>* true_succs, *false_succs, *true_succ_preds, *false_succ_preds;

	for(int i = 0; i < branches->size(); i++) {
		br_succ_file << "Node name is " << getNodeDotNameNew(branches->at(i)) << " of type " <<  getNodeDotTypeNew(branches->at(i)) << " in BasicBlock number " << (branches->at(i)->BB->getName().str().c_str()) << "\n";

		br_succ_file << "\n**********************************************\n";
		
		if(cntrl_br_flag) {
			//true_succs = branches->at(i)->true_branch_succs_Ctrl;		
			false_succs = branches->at(i)->false_branch_succs_Ctrl;
		} else {
			//true_succs = branches->at(i)->true_branch_succs;	
			false_succs = branches->at(i)->false_branch_succs;
		}
		
		/*br_succ_file << "List of true Successors: \n";
		if(branches->at(i)->type == Branch_n || branches->at(i)->type == Branch_c) {
			for(auto& true_: *true_succs) {
			br_succ_file << "True Succ Node name is " << getNodeDotNameNew(true_) << " of type " <<  getNodeDotTypeNew(true_) << " in BasicBlock number " << (true_->BB->getName().str().c_str()) << "\n";

			true_succ_preds = (cntrl_br_flag)? true_->JustCntrlPreds: true_->CntrlPreds;
			
			br_succ_file << "***************\n";
			int pos = returnPosRemoveNode(true_succ_preds, branches->at(i));
			br_succ_file << "Position of the branch in the predecessors of this successor is " << pos << "\n"; 
	
			} 
		} else {
			if(cntrl_br_flag) {
				for(auto& succ: *branches->at(i)->JustCntrlSuccs) {
					br_succ_file << "Normal Succ Node name is " << getNodeDotNameNew(succ) << " of type " <<  getNodeDotTypeNew(succ) << " in BasicBlock number " << (succ->BB->getName().str().c_str()) << "\n";

				}

			} else {

				for(auto& succ: *branches->at(i)->CntrlSuccs) {
					br_succ_file << "Normal Succ Node name is " << getNodeDotNameNew(succ) << " of type " <<  getNodeDotTypeNew(succ) << " in BasicBlock number " << (succ->BB->getName().str().c_str()) << "\n";

				}


			}

		} */
		// br_succ_file << "\n**********************************************\n";

		br_succ_file << "List of false Successors: \n";
		if(branches->at(i)->type == Branch_n || branches->at(i)->type == Branch_c) {
			for(auto& false_: *false_succs) {
			br_succ_file << "False Succ Node name is " << getNodeDotNameNew(false_) << " of type " <<  getNodeDotTypeNew(false_) << " in BasicBlock number " << (false_->BB->getName().str().c_str()) << "\n";

			false_succ_preds = (cntrl_br_flag)? false_->JustCntrlPreds: false_->CntrlPreds;
			
			br_succ_file << "***************\n";
			int pos = returnPosRemoveNode(false_succ_preds, branches->at(i));
			br_succ_file << "Position of the branch in the predecessors of this successor is " << pos << "\n"; 
	
			} 
		}



	} 

	 
}

void CircuitGenerator::deleteLLVM_Br() {

	for(auto& enode : *enode_dag) {
		if(enode->type == Branch_) { 
			// enode is an llvm branch that we want to delete
			// if it's a conditional branch, since LLVM branches have only a single predecessor if they are conditional and no predecessors if they are unconditional 
			if(enode->CntrlPreds->size() > 0) {
				auto pos_cond_preds = std::find(enode->CntrlPreds->at(0)->CntrlSuccs->begin(), enode->CntrlPreds->at(0)->CntrlSuccs->end(), enode);
				assert(pos_cond_preds != enode->CntrlPreds->at(0)->CntrlSuccs->end());
				if (pos_cond_preds != enode->CntrlPreds->at(0)->CntrlSuccs->end()) {
					enode->CntrlPreds->at(0)->CntrlSuccs->erase(pos_cond_preds);
				}
			
				// TEMPORARILY DECIDE IF WE SHOULD CLEAR THE CNTRLPREDS OR NOT????!!!!
				//enode->CntrlPreds->clear(); // all predecessors of enode removed
			}
			// remove it from enode_dag
			auto pos_enode_dag = std::find(enode_dag->begin(), enode_dag->end(), enode);
			assert(pos_enode_dag != enode_dag->end());
			if (pos_enode_dag != enode_dag->end()) {
				enode_dag->erase(pos_enode_dag);
			} 
		}
	}

}


