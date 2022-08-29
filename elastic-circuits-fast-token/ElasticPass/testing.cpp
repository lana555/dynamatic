#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

/*void printCheckBrSuccs() {
	std::ofstream br_succ_file;
	br_succ_file.open("test_br_succ_file.txt");

	for(auto& enode : *enode_dag) {
		if(enode->type == Branch_n) {

} */


void CircuitGenerator::forkToSucc(std::vector<ENode*>* preds_of_orig_succs, ENode* orig, ENode* fork) {
    auto pos = std::find(preds_of_orig_succs->begin(), preds_of_orig_succs->end(), orig);
	
	assert(pos != preds_of_orig_succs->end());
	// Aya: TBD!!! Any kind of check for out-of-range!!!

	int index = pos - preds_of_orig_succs->begin();
	//preds_of_orig_succs->insert(index , fork);
	preds_of_orig_succs->at(index) = fork;
}

void CircuitGenerator::aya_InsertNode(ENode* enode, ENode* after) {
    // removing all connections between enode and its successors
    for (auto& succ : *(enode->CntrlSuccs)) { // for all enode's successors
        forkToSucc(succ->CntrlPreds, enode, after);

        after->CntrlSuccs->push_back(succ); // add connection between after and enode's successors in the same order as that of the original successors 
        

		// succ->CntrlPreds->push_back(after); // add connection between after and enode's successors
    }
    enode->CntrlSuccs = new std::vector<ENode*>; // all successors of enode removed

	// the following statements are still maintaining the correct order of succs and preds 
    after->CntrlPreds->push_back(enode); // add connection between after and enode
    enode->CntrlSuccs->push_back(after); // add connection between after and enode
}


// Lana: used to add forks in the appropriate places in a circuit BEFORE branches
void CircuitGenerator::addFork() {
	// loop over all enode operations
	for(auto& enode: *enode_dag) {
		// put a fork after any node having multiple successors in the data path or control path, except for branches
		
		if(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() > 1 && enode->type != 					Fork_ && enode->type != Fork_c && enode->type != Branch_ && enode->type != 							Branch_c && enode->type != Branch_n) {

			if(enode->CntrlSuccs->size() == 0) {
			// play in the control path only
				ENode* fork = new ENode(Fork_c, "forkC", enode->BB);
				fork->id = fork_id++;
				enode_dag->push_back(fork);
				// need to push it to cntrl_succ of its bb
				BBMap->at(enode->BB)->Cntrl_nodes->push_back(fork);  

				// need to correctly insert the fork between the node and all of its 						JustCntrlSuccs, so I created a new function similar to InsertNode	
				InsertCntrlNode(enode, fork, false);
		
			} else { // there is at least one successor in the data path and we may or may 							not have any successors in the control path
				ENode* fork = new ENode(Fork_, "fork", enode->BB);
				fork->id = fork_id++;
				enode_dag->push_back(fork);

				// Fork must be inserted at index 0 of the enode->CntrlSuccs and the original order of successors should be maintained in fork->CntrlSuccs
				aya_InsertNode(enode, fork);

				// if you have even a single control successor, 
				if(enode->JustCntrlSuccs->size() > 0) {
					InsertCntrlNode(enode, fork, true);
				} 

			}

		} 

		// assert(200!=200);
		if(enode->type == Branch_c){
			//fixBranchesSuccs(enode, true);
			addBrSuccs(enode, true);
		} else
			if(enode->type == Branch_n) {
				//fixBranchesSuccs(enode, false);
				addBrSuccs(enode, false);
			}

	} 
}

void CircuitGenerator::addBrSuccs(ENode* branch, bool cntrl_br_flag) {
	std::vector<ENode*> *true_succs, *false_succs; 
	std::vector<ENode*>*branch_CntrlSuccs;

	// assert(101!=101);

	ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
	std::vector<ENode*>* sink_preds;

	ENode* fork;
	node_type fork_type; // shall we create a Branch_c or Branch_n
	std::string fork_name;

	std::vector<ENode*>* fork_preds;
	std::vector<ENode*>* fork_succs;

	std::vector<ENode*>* one_succ_preds;

	if(cntrl_br_flag) {
		true_succs = branch->true_branch_succs_Ctrl;
		false_succs = branch->false_branch_succs_Ctrl;
		branch_CntrlSuccs = branch->JustCntrlSuccs;
		sink_preds = sink_node->JustCntrlPreds;

		fork_type = Fork_c;
		fork_name = "forkC";
		
	} else {
		true_succs = branch->true_branch_succs;
		false_succs = branch->false_branch_succs;
		branch_CntrlSuccs = branch->CntrlSuccs;
		sink_preds = sink_node->CntrlPreds;

		fork_type = Fork_;
		fork_name = "fork";
	} 

	fork = new ENode(fork_type, fork_name.c_str(), branch->BB);
	if(cntrl_br_flag) {
		fork_preds = fork->JustCntrlPreds;
		fork_succs = fork->JustCntrlSuccs;
	} else {
		fork_preds = fork->CntrlPreds;
		fork_succs = fork->CntrlSuccs;
	} 

	switch(true_succs->size()){
		case 0:
			// put a sink as the first entry then check the false_succ
			sink_node->id    = sink_id++;
			enode_dag->push_back(sink_node);
			branch_CntrlSuccs->push_back(sink_node);
			sink_preds->push_back(branch);

			assert(false_succs->size() > 0);		
			
			// the falseSuccs must have at least 1 entry
			if(false_succs->size() == 1) {
				branch_CntrlSuccs->push_back(false_succs->at(0));
			} else {
				if(false_succs->size() > 1) {
					// add a fork to the branch, then add the false_succs to the fork
					if(cntrl_br_flag) {	
						// need to push it to cntrl_succ of its bb
						BBMap->at(branch->BB)->Cntrl_nodes->push_back(fork);  
					} 
					fork->id = fork_id++;
					enode_dag->push_back(fork);

					branch_CntrlSuccs->push_back(fork); 
					fork_preds->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs->push_back(one_succ); 

						// place the fork in place of the branch!
						one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 
						// find the position of the branch in the one_succ_preds

						//int pos = returnPosRemoveNode(one_succ_preds, branch);
						//one_succ_preds->at(pos) = fork;

						forkToSucc(one_succ_preds, branch, fork);
					} 

					
				} 
			} 
		break;
		case 1:
			branch_CntrlSuccs->push_back(true_succs->at(0));
			
			if(false_succs->size() == 0) {
				// add sink
				sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);
				branch_CntrlSuccs->push_back(sink_node);
				sink_preds->push_back(branch);
				
			} else {
				if(false_succs->size() == 1) {
					branch_CntrlSuccs->push_back(false_succs->at(0));
				} else {  // if it's greater than 1 
					assert(false_succs->size() > 1);
					
					// add a fork to the branch, then add the false_succs to the fork
					if(cntrl_br_flag) {	
						// need to push it to cntrl_succ of its bb
						BBMap->at(branch->BB)->Cntrl_nodes->push_back(fork);  
					} 
					fork->id = fork_id++;
					enode_dag->push_back(fork);

					branch_CntrlSuccs->push_back(fork); 
					fork_preds->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs->push_back(one_succ); 

						// place the fork in place of the branch!
						one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 
						// find the position of the branch in the one_succ_preds

						//int pos = returnPosRemoveNode(one_succ_preds, branch);
						//one_succ_preds->at(pos) = fork;

						forkToSucc(one_succ_preds, branch, fork);
					} 

					////////////////////////////////////////

				} 

			} 

		break;

		default:  // which is any size > 1
			// for extra checking
			assert(true_succs->size() > 1);

			/////////////////////// adding a fork to connect the true successors
			// add a fork to the branch, then add the true_succs to the fork
			if(cntrl_br_flag) {	
				// need to push it to cntrl_succ of its bb
				BBMap->at(branch->BB)->Cntrl_nodes->push_back(fork);  
			} 
			fork->id = fork_id++;
			enode_dag->push_back(fork);

			branch_CntrlSuccs->push_back(fork); 
			fork_preds->push_back(branch);

			for (auto& one_succ : *(true_succs)) {
				fork_succs->push_back(one_succ); 

				// place the fork in place of the branch!
				one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 
				// find the position of the branch in the one_succ_preds

				//int pos = returnPosRemoveNode(one_succ_preds, branch);
				//one_succ_preds->at(pos) = fork;
				forkToSucc(one_succ_preds, branch, fork);
			} 
			////////////////////////////////// end of adding fork to the true successors
		
			/////////////////////// Checking and handling false successors
			if(false_succs->size() == 0) {
				// add sink
				sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);
				branch_CntrlSuccs->push_back(sink_node);
				sink_preds->push_back(branch);
				
			} else {
				if(false_succs->size() == 1) {
					branch_CntrlSuccs->push_back(false_succs->at(0));
				} else {  // if it's greater than 1 
					assert(false_succs->size() > 1);
					
					// add a fork to the branch, then add the false_succs to the fork
					if(cntrl_br_flag) {	
						// need to push it to cntrl_succ of its bb
						BBMap->at(branch->BB)->Cntrl_nodes->push_back(fork);  
					} 
					fork->id = fork_id++;
					enode_dag->push_back(fork);

					branch_CntrlSuccs->push_back(fork); 
					fork_preds->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs->push_back(one_succ); 

						// place the fork in place of the branch!
						one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 
						// find the position of the branch in the one_succ_preds

						//int pos = returnPosRemoveNode(one_succ_preds, branch);
						//one_succ_preds->at(pos) = fork;
						forkToSucc(one_succ_preds, branch, fork);
					} 

				} 
			}

			///////////////////////////////////// end of handling the false successors 
	}

} 
void CircuitGenerator::cut_keepBranchSuccs(bool cntrl_br_flag, std::vector<ENode*>* redundant_branches, int k, std::vector<ENode*> *to_keep_true_succs, std::vector<ENode*> *to_keep_false_succs) {
	std::vector<ENode*> *redun_true_succs, *redun_false_succs;
	int true_idx, false_idx; 
	
	if(cntrl_br_flag) {
		redun_true_succs = redundant_branches->at(k)->true_branch_succs_Ctrl;
		//to_keep_true_succs = redundant_branches->at(0)->true_branch_succs_Ctrl;

		redun_false_succs = redundant_branches->at(k)->false_branch_succs_Ctrl;
		//to_keep_false_succs = redundant_branches->at(0)->false_branch_succs_Ctrl;

	} else {
		redun_true_succs = redundant_branches->at(k)->true_branch_succs;
		//to_keep_true_succs = redundant_branches->at(0)->true_branch_succs;

		redun_false_succs = redundant_branches->at(k)->false_branch_succs;
		//to_keep_false_succs = redundant_branches->at(0)->false_branch_succs;
	} 

	for(auto& true_succs: *redun_true_succs) {
		to_keep_true_succs->push_back(true_succs);

		// want to place the branch_to_keep in the predecessor of this successor
		if(cntrl_br_flag) {
			true_idx = returnPosRemoveNode(true_succs->JustCntrlPreds, redundant_branches->at(k)); 
			true_succs->JustCntrlPreds->at(true_idx) = redundant_branches->at(0);

			//true_idx_2 = returnPosRemoveNode(true_succs->JustCntrlPreds, redundant_branches->at(0));
		} else {
			true_idx = returnPosRemoveNode(true_succs->CntrlPreds, redundant_branches->at(k)); 
			true_succs->CntrlPreds->at(true_idx) = redundant_branches->at(0);

			//true_idx_2 = returnPosRemoveNode(true_succs->CntrlPreds, redundant_branches->at(0));
		} 

		redun_true_succs->erase(std::remove(redun_true_succs->begin(), redun_true_succs->end(), true_succs ), redun_true_succs->end());

	}

	for(auto& false_succs: *redun_false_succs) {
		to_keep_false_succs->push_back(false_succs);

		// want to place the branch_to_keep in the predecessor of this successor
		if(cntrl_br_flag) {
			false_idx = returnPosRemoveNode(false_succs->JustCntrlPreds, redundant_branches->at(k)); 
			false_succs->JustCntrlPreds->at(false_idx) = redundant_branches->at(0);
		} else {
			false_idx = returnPosRemoveNode(false_succs->CntrlPreds, redundant_branches->at(k)); 
			false_succs->CntrlPreds->at(false_idx) = redundant_branches->at(0);
		} 

		redun_false_succs->erase(std::remove(redun_false_succs->begin(), redun_false_succs->end(), false_succs ), redun_false_succs->end());

	} 


} 

void CircuitGenerator::cutBranchPreds(bool cntrl_br_flag, std::vector<ENode*>* redundant_branches, int k, node_type branch_type ) {
	// Cut the connection with the predecessors
	std::vector<ENode*>* redun_preds;
	if(cntrl_br_flag) {
		redun_preds = redundant_branches->at(k)->JustCntrlPreds;
	} else {
		redun_preds = redundant_branches->at(k)->CntrlPreds;
	}

	for(auto& one_pred : *redun_preds) {
		if(one_pred->type == branch_type) {
			if(cntrl_br_flag) {
				// check if it's in true or false
				if(std::find(one_pred->true_branch_succs_Ctrl->begin(), one_pred->true_branch_succs_Ctrl->end(), redundant_branches->at(k)) != one_pred->true_branch_succs_Ctrl->end()) {

				one_pred->true_branch_succs_Ctrl->erase(std::remove(one_pred->true_branch_succs_Ctrl->begin(), one_pred->true_branch_succs_Ctrl->end(), redundant_branches->at(k) ), one_pred->true_branch_succs_Ctrl->end());

				} else {

				assert(std::find(one_pred->false_branch_succs_Ctrl->begin(), one_pred->false_branch_succs_Ctrl->end(), redundant_branches->at(k)) != one_pred->false_branch_succs_Ctrl->end());

				one_pred->false_branch_succs_Ctrl->erase(std::remove(one_pred->false_branch_succs_Ctrl->begin(), one_pred->false_branch_succs_Ctrl->end(), redundant_branches->at(k) ), one_pred->false_branch_succs_Ctrl->end());

				}				
			
			} else {
				///////////////////////
				// check if it's in true or false
				if(std::find(one_pred->true_branch_succs->begin(), one_pred->true_branch_succs->end(), redundant_branches->at(k)) != one_pred->true_branch_succs->end()) {
					one_pred->true_branch_succs->erase(std::remove(one_pred->true_branch_succs->begin(), one_pred->true_branch_succs->end(), redundant_branches->at(k) ), one_pred->true_branch_succs->end());

				} else {
					assert(std::find(one_pred->false_branch_succs->begin(), one_pred->false_branch_succs->end(), redundant_branches->at(k)) != one_pred->false_branch_succs->end());

					one_pred->false_branch_succs->erase(std::remove(one_pred->false_branch_succs->begin(), one_pred->false_branch_succs->end(), redundant_branches->at(k) ), one_pred->false_branch_succs->end());
				}
				//////////////////////

			}

		} else {
			if(cntrl_br_flag) {
				one_pred->JustCntrlSuccs->erase(std::remove(one_pred->JustCntrlSuccs->begin(), one_pred->JustCntrlSuccs->end(), redundant_branches->at(k) ), one_pred->JustCntrlSuccs->end());

			} else {
				one_pred->CntrlSuccs->erase(std::remove(one_pred->CntrlSuccs->begin(), one_pred->CntrlSuccs->end(), redundant_branches->at(k) ), one_pred->CntrlSuccs->end());
			}

		}

	}
		
	redun_preds->clear();

}

void CircuitGenerator::removeExtraBranches(bool cntrl_br_flag) {
	// save the enode_dag in a copy so that we can modify it
    std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;	

	for(auto& enode : *enode_dag) {
		enode_dag_cpy->push_back(enode);
	}

	ENode* producer;
	node_type branch_type = (cntrl_br_flag)? Branch_c: Branch_n;


	// at a specific enode_1 iteration 
	std::vector<ENode*>* redundant_branches = new std::vector<ENode*>;

	std::vector<ENode*> *to_keep_true_succs, *to_keep_false_succs;

	for(auto& bbnode : *bbnode_dag) {
		for(auto& enode_1 : *enode_dag_cpy) {
			redundant_branches->clear();

			// check if this enode_1 is of type branch and is inside this bbnode
			if(enode_1->type == branch_type && BBMap->at(enode_1->BB) == bbnode) {
				// push it to the redundant_branches, search for redundant branches to this branch and delete it from enode_dag_cpy
				redundant_branches->push_back(enode_1);
				producer = enode_1->producer_to_branch;

				for(auto& enode_2 : *enode_dag_cpy) {
					if(enode_2->type == branch_type && BBMap->at(enode_2->BB) == bbnode && enode_2->producer_to_branch == producer) {
						redundant_branches->push_back(enode_2);
						enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2 ), enode_dag_cpy->end()); 
					}
				} 

				enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_1 ), enode_dag_cpy->end()); 
			} 
			
			// We will keep the first branch ->at(0), so loop over the rest
			if(redundant_branches->size() > 0) {
				if(cntrl_br_flag) {
					to_keep_true_succs = redundant_branches->at(0)->true_branch_succs_Ctrl;
					to_keep_false_succs = redundant_branches->at(0)->false_branch_succs_Ctrl;

				} else {
					to_keep_true_succs = redundant_branches->at(0)->true_branch_succs;
					to_keep_false_succs = redundant_branches->at(0)->false_branch_succs;
				} 

			} 
			for(int k = 1; k < redundant_branches->size(); k++) {
				cutBranchPreds(cntrl_br_flag, redundant_branches, k, branch_type);

				// need to connect its true_successors and false_successors to those of the branch to keep 
				cut_keepBranchSuccs(cntrl_br_flag, redundant_branches, k, to_keep_true_succs, to_keep_false_succs);

				enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), redundant_branches->at(k) ), enode_dag->end());
			}
	

		}
	}

} 

void CircuitGenerator::printBrSuccs() {
    std::ofstream br_succ_file;
	br_succ_file.open("br_succ_file.txt");
	int false_idx, true_idx;

	for(auto& enode : *enode_dag) {
		if(enode->type == Branch_n) {
			br_succ_file << "\n ENode name is: " << enode->Name << ", in BBNode number" << BBMap->at(enode->BB) + 1 <<  "\n ENode list of falseSuccessors:\n"; 
			for(auto& false_succs: *enode->false_branch_succs) {
				br_succ_file << " FalseSucc name is: " << false_succs->Name << " The index of the branch in the preds of falseSuccs is: ";
				false_idx = returnPosRemoveNode(false_succs->CntrlPreds, enode); 
				br_succ_file << false_idx << "\n"; 
				
			} 

			br_succ_file << "\n\nENode list of trueSuccessors:\n";

			for(auto& true_succs: *enode->true_branch_succs) {
				br_succ_file << " TrueSucc name is: " << true_succs->Name << " The index of the branch in the preds of trueSuccs is: ";
				true_idx = returnPosRemoveNode(true_succs->CntrlPreds, enode); 
				br_succ_file << true_idx << "\n"; 
			} 
		}
	}

}

// Sink node for missing Branch and Store outputs
// Store needs to be adjusted to LSQ analysis
void CircuitGenerator::addSink() {
	for (auto& enode : *enode_dag) {
        if (enode->type == Branch_n) {
			// if any the size of true_branch_succs or false_branch_succs is 0 then we need to place a sink in the CntrlSuccs of that branch enode, but must follow the convention of true, false indices in doing so..

			// if the size of both vectors is 0, this is an error!
			assert(enode->true_branch_succs->size()!= 0 || enode->false_branch_succs->size()!= 0);
			
			// if there is no false enode
			if(enode->false_branch_succs->size() == 0) {
				// just create and push_back the sink node
				ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);

				// PROBLEM IS IN ADDING FORK???
				assert(enode->CntrlSuccs->size()!= 0);

                enode->CntrlSuccs->push_back(sink_node);
                sink_node->CntrlPreds->push_back(enode);
			} 

			// if there is no true enode
			if(enode->true_branch_succs->size() == 0) {
				// Need to place a sink node in enode->CntrlSucc->at(0) and move the node that was already at index 0 to index 1 
				
				// just create and push_back the sink node
				ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);
				
				// make sure if this syntax will do what you want!!
				
				enode->CntrlSuccs->push_back(sink_node);
				std::reverse(enode->CntrlSuccs->begin(), enode->CntrlSuccs->end());
                sink_node->CntrlPreds->push_back(enode);
			} 
		}

	//////////////////////////// Redo the same everything but for the control path
	 if (enode->type == Branch_c) {
			// if any the size of true_branch_succs or false_branch_succs is 0 then we need to place a sink in the CntrlSuccs of that branch enode, but must follow the convention of true, false indices in doing so..

			// if the size of both vectors is 0, this is an error!
			assert(enode->true_branch_succs_Ctrl->size()!= 0 || enode->false_branch_succs_Ctrl->size()!= 0);
			
			// if there is no false enode
			if(enode->false_branch_succs_Ctrl->size() == 0) {
				// just create and push_back the sink node
				ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);

                enode->JustCntrlSuccs->push_back(sink_node);
                sink_node->JustCntrlPreds->push_back(enode);
			} 

			// if there is no true enode
			if(enode->true_branch_succs_Ctrl->size() == 0) {
				// Need to place a sink node in enode->CntrlSucc->at(0) and move the node that was already at index 0 to index 1 
				

				// just create and push_back the sink node
				ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;
				enode_dag->push_back(sink_node);
				
				// make sure if this syntax will do what you want!!
				enode->JustCntrlSuccs->push_back(sink_node);
				std::reverse(enode->JustCntrlSuccs->begin(), enode->JustCntrlSuccs->end());
                sink_node->JustCntrlPreds->push_back(enode);
			} 
		}

	/////////////////////////////////////

	}
}
