#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

/*

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

			// Now we need to check redundant_branches, worry only if its size is > 1
			 if(redundant_branches->size() > 1) {

				// Extra Sanity check to make sure that predecessors of all the redundant branches are the same
				for(int i = 1; i < redundant_branches->size(); i++) {
					if(cntrl_br_flag) {
						assert(redundant_branches->at(i)->JustCntrlPreds->size() == redundant_branches->at(0)->JustCntrlPreds->size()); 
						for(int k = 0; k < redundant_branches->at(i)->JustCntrlPreds->size(); k++) {	
						assert(redundant_branches->at(i)->JustCntrlPreds->at(k) == redundant_branches->at(0)->JustCntrlPreds->at(k));

				}

					} else {
						assert(redundant_branches->at(i)->CntrlPreds->size() == redundant_branches->at(0)->CntrlPreds->size()); 
						for(int k = 0; k < redundant_branches->at(i)->CntrlPreds->size(); k++) {	
						assert(redundant_branches->at(i)->CntrlPreds->at(k) == redundant_branches->at(0)->CntrlPreds->at(k));

				}
					} 
					
				}

		//////////////////////////////// End of sanity check !!!
				std::vector<ENode*>* br_to_keep_preds = (cntrl_br_flag)? redundant_branches->at(0)->JustCntrlPreds : redundant_branches->at(0)->CntrlPreds;

				std::vector<ENode*>* br_to_keep_preds_succ_1 = (cntrl_br_flag)? br_to_keep_preds->at(0)->JustCntrlPreds : br_to_keep_preds->at(0)->CntrlPreds;

				std::vector<ENode*>* br_to_keep_preds_succ_2 = (cntrl_br_flag)? br_to_keep_preds->at(1)->JustCntrlPreds : br_to_keep_preds->at(1)->CntrlPreds;

				std::vector<ENode*>* br_to_keep_true_succs = (cntrl_br_flag)? redundant_branches->at(0)->true_branch_succs_Ctrl : redundant_branches->at(0)->true_branch_succs;

				std::vector<ENode*>* br_to_keep_false_succs = (cntrl_br_flag)? redundant_branches->at(0)->false_branch_succs_Ctrl : redundant_branches->at(0)->false_branch_succs;

				std::vector<ENode*> *new_true_succs, *new_false_succs;
				std::vector<ENode*> *new_true_succs_preds, *new_false_succs_preds;
				
				// loop over all redundant_branches to delete them
				for(int i = 1; i < redundant_branches->size(); i++) {
					// erase the connection with the predecessors 
					br_to_keep_preds_succ_1->erase(std::remove(br_to_keep_preds_succ_1->begin(), br_to_keep_preds_succ_1->end(), redundant_branches->at(i) ), br_to_keep_preds_succ_1->end()); 

					br_to_keep_preds_succ_2->erase(std::remove(br_to_keep_preds_succ_2->begin(), br_to_keep_preds_succ_2->end(), redundant_branches->at(i) ), br_to_keep_preds_succ_2->end());

					

					new_true_succs = (cntrl_br_flag)? redundant_branches->at(i)->true_branch_succs_Ctrl: redundant_branches->at(i)->true_branch_succs;
					new_false_succs = (cntrl_br_flag)? redundant_branches->at(i)->false_branch_succs_Ctrl: redundant_branches->at(i)->false_branch_succs;

					// push the true successors
					for(auto& true_succ : *new_true_succs) {
						br_to_keep_true_succs->push_back(true_succ); 
						
						// for each of these successors, replace the redundnat branch with the branch_to_keep in the successor's predecessor array

						new_true_succs_preds = (cntrl_br_flag)? true_succ->JustCntrlPreds: true_succ->CntrlPreds;
						//if(new_true_succs_preds->size() > 0) {
							//int true_pos = returnPosRemoveNode(true_succ->CntrlPreds, redundant_branches->at(i));
							// new_true_succs_preds->at(true_pos) = redundant_branches->at(0);
					
						//} 
							
					}

					// push the false successors 
					for(auto& false_succ : *new_false_succs) {
						br_to_keep_false_succs->push_back(false_succ); 

						// for each of these successors, replace the redundnat branch with the branch_to_keep in the successor's predecessor array
						new_false_succs_preds = (cntrl_br_flag)? false_succ->JustCntrlPreds: false_succ->CntrlPreds;
						//int false_pos = returnPosRemoveNode(new_false_succs_preds, redundant_branches->at(i));
					
						//new_false_succs_preds->at(false_pos) = redundant_branches->at(0);


					}
		
					
					enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), redundant_branches->at(i) ), enode_dag->end());

				} // end of looping over redundnat_branches 

			} // end of if(redundant_branches->size() > 1)  
			

		}

	}

} 


*/

void CircuitGenerator::removeRedunBranches(bool cntrl_path_flag) {
	// temporarily for debugging
	std::ofstream redun_br_file;
	redun_br_file.open("redun_branches.txt");

	// save the enode_dag in a copy so that we can modify it
    std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;	

	for(auto& enode : *enode_dag) {
		enode_dag_cpy->push_back(enode);
	}

	ENode* producer;
	node_type branch_type = (cntrl_path_flag)? Branch_c: Branch_n;

	// at a specific enode_1 iteration 
	std::vector<ENode*>* redundant_branches = new std::vector<ENode*>;

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

			if(redundant_branches->size() > 0) {
				redun_br_file << "\nRedundant Branches for enode_1 called: " << enode_1->Name << " that is inside BBNode number: " << bbnode->Idx + 1 << " are " << redundant_branches->size()<< " branches of the following names: \n";
			}
			for(int i = 1; i < redundant_branches->size(); i++ ) {
				redun_br_file << redundant_branches->at(1)->Name << ", ";
			} 

		}

	}


/*
// temporarily for debugging
	std::ofstream redun_br_file;
	redun_br_file.open("redun_branches.txt");
	
	// loop over BBs
	// search for branch_n enodes in one BB and belonging to the same producer
	// these should be merged into one branch

	// save the enode_dag in a copy so that we can modify it
    std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;	

	for(auto& enode : *enode_dag) {
		enode_dag_cpy->push_back(enode);
	}
	
	redun_br_file << "\n Original Size of enode_dag_cpy is " << enode_dag_cpy->size() << "\n";

	ENode* producer;
	node_type branch_type = (cntrl_path_flag)? Branch_c: Branch_n;

	// at a specific enode_1 iteration 
	std::vector<ENode*>* redundant_branches = new std::vector<ENode*>;

	for(auto& bbnode : *bbnode_dag) {
		for(auto& enode_1 : *enode_dag_cpy) {
			redundant_branches->clear(); 
			redun_br_file << "\n New enode_1 iteration, redundant_branches size is " << redundant_branches->size() << "\n";
			// search for our branch nodes in this BBnode
			if(enode_1->type == branch_type && BBMap->at(enode_1->BB) == bbnode) {
				redundant_branches->push_back(enode_1);
				producer = enode_1->producer_to_branch;
				// find another branch enode in the same bbnode and belonging to the same producer
				for(auto& enode_2 : *enode_dag_cpy) { 
					if(enode_2->type == branch_type && BBMap->at(enode_2->BB) == bbnode && enode_1 != enode_2) {
						if(enode_2->producer_to_branch == producer) {
							redundant_branches->push_back(enode_2);
							enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2 ), enode_dag_cpy->end());

							redun_br_file << "\n Inside Enode_2 if Size of enode_dag_cpy is " << enode_dag_cpy->size() << "\n";

							assert(enode_dag_cpy->size() != enode_dag->size());
						} 
					}

				} 
			} 

			// at this moment you should have all the redundant branches in a specific BBnode getting fed from a specific producer
			enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_1 ), enode_dag_cpy->end()); 

			redun_br_file << "\n End of enode_1 Size of enode_dag_cpy is " << enode_dag_cpy->size() << "\n";
				// Print the redundant branches that are redundant to this enode_1
			
			if(redundant_branches->size() > 0) {
				redun_br_file << "\nRedundant Branches for enode_1 called: " << enode_1->Name << " that is inside BBNode number: " << bbnode->Idx + 1 << " are " << redundant_branches->size()<< " branches of the following names: \n";
			}
			for(int i = 1; i < redundant_branches->size(); i++ ) {
				redun_br_file << redundant_branches->at(1)->Name << ", ";
			} 

			// mergeRedundantBranches(cntrl_path_flag, redundant_branches);   
		}
	}
*/ 
}


/////////////////////// OLD FUNCTIONS TOWARDS REMOVING REDUNDANCY

void CircuitGenerator::fixBranchesSuccsHelper(std::vector<ENode*>*succs,
	std::vector<ENode*>*branch_CntrlSuccs, bool cntrl_br_flag, ENode* branch) {

	ENode* fork;

	node_type fork_type; // shall we create a Branch_c or Branch_n
	std::string fork_name; 

	// clear the branch successors to organize them properly here 
	// They are expected to come empty anyways, this is just an extra step
	branch_CntrlSuccs->clear();

	if(cntrl_br_flag) {
		fork_type = Fork_c;
		fork_name = "forkC";

	} else {
		fork_type = Fork_;
		fork_name = "fork";
	}
	

	if(succs->size() == 1) {
	// no need for a fork, add directly the CntrlSuccs
		branch_CntrlSuccs->push_back(succs->at(0));
	} else 
	if(succs->size() > 1) {
		// this means the branch feeds multiple enodes, so need to place a fork
		fork = new ENode(fork_type, fork_name.c_str(), branch->BB);

		if(cntrl_br_flag) {	
			// need to push it to cntrl_succ of its bb
			BBMap->at(branch->BB)->Cntrl_nodes->push_back(fork);  
		} 

		fork->id = fork_id++;
		enode_dag->push_back(fork);

		branch_CntrlSuccs->push_back(fork); 
		
		// loop over all of the true successors to connect the fork in their predecessors in place of the branch
		for (auto& one_succ : *(succs)) {
			if(cntrl_br_flag) {	
				forkToSucc(one_succ->JustCntrlPreds, branch, fork);
				fork->JustCntrlSuccs->push_back(one_succ);
			} else {
				forkToSucc(one_succ->CntrlPreds, branch, fork);
				fork->CntrlSuccs->push_back(one_succ);
			}
			   
		}
	}

	// the case when the trueSucc or falseSucc is of size 0 is handled inside addSink

}

void CircuitGenerator::fixBranchesSuccs(ENode* enode, bool cntrl_br_flag) {
	/*
			The convention is that the true successor should be at CntrlSuccs.at(0) and the 			false successor should be at CntrlSuccs.at(1), but we will assure this 				correctness for the false successor when adding sinks.
	*/
	std::vector<ENode*> *true_succs, *false_succs; 
	std::vector<ENode*>*branch_CntrlSuccs;

	ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
	std::vector<ENode*>* sink_preds;

	if(cntrl_br_flag) {
		true_succs = enode->true_branch_succs_Ctrl;
		false_succs = enode->false_branch_succs_Ctrl;
		branch_CntrlSuccs = enode->JustCntrlSuccs;
		sink_preds = sink_node->JustCntrlPreds;
		
	} else {
		true_succs = enode->true_branch_succs;
		false_succs = enode->false_branch_succs;
		branch_CntrlSuccs = enode->CntrlSuccs;
		sink_preds = sink_node->CntrlPreds;
	} 

	if(true_succs->size() == 1 && false_succs->size() == 0) {
		branch_CntrlSuccs->push_back(true_succs->at(0));

		sink_node->id    = sink_id++;
		enode_dag->push_back(sink_node);

		// PROBLEM IS IN ADDING FORK???
		assert(branch_CntrlSuccs->size()!= 0);

		branch_CntrlSuccs->push_back(sink_node);
		sink_preds->push_back(enode);

	} else {
		if(false_succs->size() == 1 && true_succs->size() == 0) {
			sink_node->id    = sink_id++;
			enode_dag->push_back(sink_node);

			branch_CntrlSuccs->push_back(sink_node);
			sink_preds->push_back(enode);

			branch_CntrlSuccs->push_back(false_succs->at(0));

		} else {
			if(false_succs->size() == 1 && true_succs->size() == 1) {
				branch_CntrlSuccs->push_back(true_succs->at(0));
				branch_CntrlSuccs->push_back(false_succs->at(0));

			}

		}

	} 



	//fixBranchesSuccsHelper(true_succs, branch_CntrlSuccs, cntrl_br_flag, enode);
	//fixBranchesSuccsHelper(false_succs, branch_CntrlSuccs, cntrl_br_flag, enode);

}

/////////////////////////////////
/*
void CircuitGenerator::cutBranchConnections(bool cntrl_path_flag, ENode* branch_to_delete){
	// Remove the branch from the CntrlSuccs array of the branches predecessors 
	std::vector<ENode*> *branch_to_delete_preds, *pred_succ, *succ_pred;
	std::vector<ENode*> *branch_to_delete_true_succs, *branch_to_delete_false_succs;

	if(cntrl_path_flag) {
		branch_to_delete_preds = branch_to_delete->JustCntrlPreds;
	} else {
		branch_to_delete_preds = branch_to_delete->CntrlPreds;
	} 

	for(auto& pred : *(branch_to_delete_preds)) {
		// if the branch's predecessor is of type Branch_n, then our branch should exist in the true or false successor arrays of that predecessor.. Otherwise, it should exist in the node's CntrlSucc or JustCntrlSucc array

		if(pred->type == Branch_n){
			if(cntrl_path_flag) {
				// decide if it's in the true or false array
				if(std::find(pred->true_branch_succs_Ctrl->begin(), pred->true_branch_succs_Ctrl->end(), branch_to_delete) != pred->true_branch_succs_Ctrl->end()) {

					pred_succ = pred->true_branch_succs_Ctrl;

				} else {
					assert(std::find(pred->false_branch_succs_Ctrl->begin(), pred->false_branch_succs_Ctrl->end(), branch_to_delete) != pred->false_branch_succs_Ctrl->end());
					
					pred_succ = pred->false_branch_succs_Ctrl;
				} 
				
			} else {
				//////////////////////////
				if(std::find(pred->true_branch_succs->begin(), pred->true_branch_succs->end(), branch_to_delete) != pred->true_branch_succs->end()) {

					pred_succ = pred->true_branch_succs;

				} else {
					assert(std::find(pred->false_branch_succs->begin(), pred->false_branch_succs->end(), branch_to_delete) != pred->false_branch_succs->end());
					
					pred_succ = pred->false_branch_succs;
				} 

				////////////////////////////
			}
		} 
		
		// We should delete the branch from the predecessor's successor array
		else {
			if(cntrl_path_flag) {
				pred_succ = pred->JustCntrlSuccs;
			} else {
				pred_succ = pred->CntrlSuccs;
			} 
		}
		// with this we will delete all the enodes in the predecessor array of that branch 
		removeNode(pred_succ, branch_to_delete);
	} 

	//////////////////////// repeating the same exact thing for the branch successors 
	if(cntrl_path_flag) {
		branch_to_delete_true_succs = branch_to_delete->true_branch_succs_Ctrl;
		branch_to_delete_false_succs = branch_to_delete->false_branch_succs_Ctrl;
	} else {
		branch_to_delete_true_succs = branch_to_delete->true_branch_succs;
		branch_to_delete_false_succs = branch_to_delete->false_branch_succs;
	} 

	// for the true successors 
	for(auto& succ : *(branch_to_delete_true_succs)) {
		if(cntrl_path_flag) {
			succ_pred = succ->JustCntrlPreds;
		} else {
			succ_pred = succ->CntrlPreds;
		} 
		removeNode(succ_pred, branch_to_delete);
	} 

	// for the false successors
	for(auto& succ : *(branch_to_delete_false_succs)) {
		if(cntrl_path_flag) {
			succ_pred = succ->JustCntrlPreds;
		} else {
			succ_pred = succ->CntrlPreds;
		} 
		removeNode(succ_pred, branch_to_delete);
	} 

}

void CircuitGenerator::mergeRedundantBranches(bool cntrl_path_flag, std::vector<ENode*>* redundant_branches){  
	// all these branches should be having the same predecessors (data and control), we only need to adjust the successors and delete the redundant branches from the enode_dag, leaving just one of them

	std::vector<ENode*> *redun_true_succs, *redun_false_succs;

	ENode* new_branch;
	std::vector<ENode*> *new_true_succs, *new_false_succs;
	node_type branch_type = (cntrl_path_flag)? Branch_c : Branch_n; // shall we create a Branch_c or Branch_n
	std::string branch_name = (cntrl_path_flag)? "branchC":"branch"; 

	 if(redundant_branches->size() > 0) {
		// create a new branch to merge these two
		new_branch = new ENode(branch_type, branch_name.c_str(), redundant_branches->at(0)->BB);

		if(cntrl_path_flag) {
			BBMap->at(redundant_branches->at(0)->BB)->Cntrl_nodes->push_back(new_branch);
		}

		new_branch->id = branch_id++;
		enode_dag->push_back(new_branch);

		// ENode* branch_to_keep = redundant_branches->at(0); 
		if(cntrl_path_flag) {
			new_true_succs = new_branch->true_branch_succs_Ctrl;
			new_false_succs = new_branch->false_branch_succs_Ctrl;
		} else {
			new_true_succs = new_branch->true_branch_succs;
			new_false_succs = new_branch->false_branch_succs;
		}
	} 

	// Note that at this point we still didn't fill the successors vectors of our branches, we are only playing with the true_branch_succs and false_branch_succs
	/*for(int i = 0; i < redundant_branches->size(); i++) {
		if(cntrl_path_flag) {
			redun_true_succs = redundant_branches->at(i)->true_branch_succs_Ctrl;
			redun_false_succs = redundant_branches->at(i)->false_branch_succs_Ctrl;
		} else {
			redun_true_succs = redundant_branches->at(i)->true_branch_succs;
			redun_false_succs = redundant_branches->at(i)->false_branch_succs;
		} 

		for(auto& true_succ : *redun_true_succs ) {
			new_true_succs->push_back(true_succ);
		} 

		for(auto& false_succ : * redun_false_succs) {
			new_false_succs->push_back(false_succ);
		}

		// remove this redundant branch from the enode_dag and cut all connections between it and its predecessors/successors 
		//cutBranchConnections(cntrl_path_flag, redundant_branches->at(i));
		//enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), redundant_branches->at(i) ), enode_dag->end());
		
	} */
		// the predecessors of all redundant branches should be the same, but here is a sanity check!!
		/* for(int j = 1; j < redundant_branches->size(); j++) {
			if(cntrl_path_flag) {
				assert(redundant_branches->at(j)->JustCntrlPreds->at(0) == 							redundant_branches->at(0)->JustCntrlPreds->at(0));

				assert(redundant_branches->at(j)->JustCntrlPreds->at(1) == 							redundant_branches->at(0)->JustCntrlPreds->at(1));
				
			} else {
				assert(redundant_branches->at(j)->CntrlPreds->at(0) == 							redundant_branches->at(0)->CntrlPreds->at(0));

				assert(redundant_branches->at(j)->CntrlPreds->at(1) == 							redundant_branches->at(0)->CntrlPreds->at(1));
			}
		}
		//////// end of sanity check

		if(redundant_branches->size() > 0) {
			if(cntrl_path_flag) {
				for(int k = 0; k < redundant_branches->at(0)->JustCntrlPreds->size(); k++) {
					new_branch->JustCntrlPreds->push_back(redundant_branches->at(0)->JustCntrlPreds->at(k));
				}
			} else {
				for(int k = 0; k < redundant_branches->at(0)->CntrlPreds->size(); k++) {
					new_branch->CntrlPreds->push_back(redundant_branches->at(0)->CntrlPreds->at(k));
				}
			}
		} 
		
}

void CircuitGenerator::removeRedundantBranches(bool cntrl_path_flag){
	// loop over BBs
	// search for branch_n enodes in one BB and belonging to the same producer
	// these should be merged into one branch

	// save the enode_dag in a copy so that we can modify it
    std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;	

	for(auto& enode : *enode_dag) {
		enode_dag_cpy->push_back(enode);
	}

	ENode* producer;
	node_type branch_type = (cntrl_path_flag)? Branch_c: Branch_n;

	// at a specific enode_1 iteration 
	std::vector<ENode*>* redundant_branches = new std::vector<ENode*>;

	for(auto& bbnode : *bbnode_dag) {
		for(auto& enode_1 : *enode_dag_cpy) {
			redundant_branches->clear(); 
			// search for our branch nodes in this BBnode
			if(enode_1->type == branch_type && BBMap->at(enode_1->BB) == bbnode) {
				redundant_branches->push_back(enode_1);
				producer = enode_1->producer_to_branch;
				// find another branch enode in the same bbnode and belonging to the same producer
				for(auto& enode_2 : *enode_dag_cpy) {
					if(enode_2->type == branch_type && BBMap->at(enode_2->BB) == bbnode && enode_1 != enode_2) {
						if(enode_2->producer_to_branch == producer) {
							redundant_branches->push_back(enode_2);
							enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2 ), enode_dag_cpy->end());

							assert(enode_dag_cpy->size() != enode_dag->size());
						} 
					}

				} 
			} 

			// at this moment you should have all the redundant branches in a specific BBnode getting fed from a specific producer

			mergeRedundantBranches(cntrl_path_flag, redundant_branches);   
		}
	}

} */
