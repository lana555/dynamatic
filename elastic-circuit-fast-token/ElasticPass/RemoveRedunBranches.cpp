#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

void CircuitGenerator::removeExtraBranchesWrapper(networkType network_flag) {
	removeExtraBranches(network_flag);
	deleteExtraBranches(network_flag);
} 

void CircuitGenerator::removeExtraBranches(networkType network_flag) {
	std::vector<int>* redun_br_indices = new std::vector<int>;  // indices of the redundant phis in the enode_dag

	///////////////////////////
	int idx_i, idx_j;
	int j = 0;
	std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;
	for(int i = 0; i < enode_dag->size(); i++) {
		enode_dag_cpy->push_back(enode_dag->at(i));
	} 

	//////////////////////////

	std::vector<ENode*> *redun_br_preds, *redun_br_succs;

	ENode* producer;
	ENode *enode_1, *enode_2;

	node_type branch_type;
	switch(network_flag) {
		case data:
			branch_type = Branch_n;
		break;

		case constCntrl:
		case memDeps:
			branch_type = Branch_c;
	}
	// node_type branch_type = (cntrl_br_flag)? Branch_c: Branch_n; 

	for(int i = 0; i < enode_dag_cpy->size(); i++) {
		enode_1 = enode_dag_cpy->at(i);
		redun_br_indices->clear();

		// check if this enode_1 is of type phi
		if(enode_1->type == branch_type && !enode_1->is_redunCntrlNet) {
			// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi or a Branch of our creation
			producer = enode_1->producer_to_branch;

			j = i+1;

			ENode* enode_1_branch_cond;

			switch(network_flag) {
				case data:
					enode_1_branch_cond = enode_1->CntrlPreds->at(1);
				break;

				case constCntrl:
				case memDeps:
					// branch condition is always in the data network even for Branch_c
					enode_1_branch_cond = enode_1->CntrlPreds->at(0);
			}
			/*if(cntrl_br_flag) {
				enode_1_branch_cond = enode_1->CntrlPreds->at(0);
			} else {
				enode_1_branch_cond = enode_1->CntrlPreds->at(1);
			}*/

			/////////////// To decide the idx to push
			idx_i = returnPosRemoveNode(enode_dag, enode_1);
			////////////////////////////

			redun_br_indices->push_back(idx_i);

			//for(int j = i+1; j < enode_dag_cpy->size(); j++) {
			while(j < enode_dag_cpy->size()) {
				enode_2 = enode_dag_cpy->at(j);
				
				if(enode_2->type == branch_type && enode_2->BB == enode_1->BB && enode_2->producer_to_branch == producer && enode_2 != enode_1
						) {

					ENode* enode_2_branch_cond;

					switch(network_flag) {
						case data:
							enode_2_branch_cond = enode_2->CntrlPreds->at(1);
						break;

						case constCntrl:
						case memDeps:
							// branch condition is always in the data network even for Branch_c
							enode_2_branch_cond = enode_2->CntrlPreds->at(0);
					}
					/*if(cntrl_br_flag) {
						enode_2_branch_cond = enode_2->CntrlPreds->at(0);
					} else {
						enode_2_branch_cond = enode_2->CntrlPreds->at(1);
					}*/

					// AYa: 06/06/2022: Added a condition to also check on the condition of the 2 branches AND negation,
						// since with the SUPPRESS algorithm, you could have 2 branches in the same BB but with different conditions!!
					assert(enode_1->is_advanced_component && enode_2->is_advanced_component); // sanity check: all my Branches are now SUPPRESSORs with potentially inverted inputs
					if(enode_1_branch_cond == enode_2_branch_cond && enode_1->is_negated_input->at(1) == enode_2->is_negated_input->at(1)) {
						/////////////// To decide the idx to push
						idx_j = returnPosRemoveNode(enode_dag, enode_2);
						////////////////////////////

						redun_br_indices->push_back(idx_j);

						// Erase this redundant Phi from the enode_dag_cpy to not loop over it again
						auto pos_enode_dag = std::find(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2);
						assert(pos_enode_dag != enode_dag_cpy->end());
						if (pos_enode_dag != enode_dag_cpy->end()) {
							enode_dag_cpy->erase(pos_enode_dag);
						} 
					} else {
						// if the value of the condition is the same but it is of opposite negation, consider them as equivalent
							// branches, but by changing the negation of enode_2 and moving the succs from the falseSuccs to the trueSuccs
						if(enode_1_branch_cond == enode_2_branch_cond && enode_1->is_negated_input->at(1) != enode_2->is_negated_input->at(1)) {
							
							// do the necessary adjustments to enode_2
							enode_2->is_negated_input->at(1) = enode_1->is_negated_input->at(1);

							switch(network_flag) {
								case data:
									assert(enode_2->false_branch_succs->size() == 1);
									enode_2->true_branch_succs->push_back(enode_2->false_branch_succs->at(0));
									enode_2->false_branch_succs->pop_back();
									assert(enode_2->false_branch_succs->size() == 0);
								break;

								case constCntrl:
									assert(enode_2->false_branch_succs_Ctrl->size() == 1);
									enode_2->true_branch_succs_Ctrl->push_back(enode_2->false_branch_succs_Ctrl->at(0));
									enode_2->false_branch_succs_Ctrl->pop_back();
									assert(enode_2->false_branch_succs_Ctrl->size() == 0);
								break;

								case memDeps:
									assert(enode_2->false_branch_succs_Ctrl_lsq->size() == 1);
									enode_2->true_branch_succs_Ctrl_lsq->push_back(enode_2->false_branch_succs_Ctrl_lsq->at(0));
									enode_2->false_branch_succs_Ctrl_lsq->pop_back();
									assert(enode_2->false_branch_succs_Ctrl_lsq->size() == 0);
							}
							/*if(cntrl_br_flag) {
								assert(enode_2->false_branch_succs_Ctrl->size() == 1);
								enode_2->true_branch_succs_Ctrl->push_back(enode_2->false_branch_succs_Ctrl->at(0));
								enode_2->false_branch_succs_Ctrl->pop_back();
								assert(enode_2->false_branch_succs_Ctrl->size() == 0);
							} else {
								assert(enode_2->false_branch_succs->size() == 1);
								enode_2->true_branch_succs->push_back(enode_2->false_branch_succs->at(0));
								enode_2->false_branch_succs->pop_back();
								assert(enode_2->false_branch_succs->size() == 0);
							}*/

							/////////////// To decide the idx to push
							idx_j = returnPosRemoveNode(enode_dag, enode_2);
							////////////////////////////

							redun_br_indices->push_back(idx_j);

							// Erase this redundant Phi from the enode_dag_cpy to not loop over it again
							auto pos_enode_dag = std::find(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2);
							assert(pos_enode_dag != enode_dag_cpy->end());
							if (pos_enode_dag != enode_dag_cpy->end()) {
								enode_dag_cpy->erase(pos_enode_dag);
							} 
						} else {
							// increment the iterator to move to the next enode_2 only if you didn't delete an element
							j++;
						}
					}
					////////////////////////////
				} else {
					// increment the iterator to move to the next enode_2 only if you didn't delete an element
					j++;
				}
			} 

		}
	
		// At this point, redundant_br contains a copy of all branches that are redundant to enode_1

		for(int k = 1; k < redun_br_indices->size(); k++) {
			bool constCntrl_flag = false;  // flag that is true if the redun br has preds and succs in the JUstCntrl network and false for the CntrlOrder network
			if(network_flag == constCntrl || network_flag == memDeps) {
				// since those two networks have the same br_type, we need to check the size of the preds and succs to know which network to pass
				if(enode_dag->at(redun_br_indices->at(k))->JustCntrlPreds->size() > 0) {
					// sanity checks
					assert(enode_dag->at(redun_br_indices->at(k))->true_branch_succs_Ctrl->size() > 0 || enode_dag->at(redun_br_indices->at(k))->false_branch_succs_Ctrl->size() > 0);
					assert(enode_dag->at(redun_br_indices->at(k))->true_branch_succs_Ctrl_lsq->size() == 0 && enode_dag->at(redun_br_indices->at(k))->false_branch_succs_Ctrl_lsq->size() == 0 && enode_dag->at(redun_br_indices->at(k))->CntrlOrderPreds->size() == 0);
					constCntrl_flag = true;

				} else {
					// sanity checks 
					assert( (enode_dag->at(redun_br_indices->at(k))->true_branch_succs_Ctrl_lsq->size() > 0 || enode_dag->at(redun_br_indices->at(k))->false_branch_succs_Ctrl_lsq->size() > 0) && enode_dag->at(redun_br_indices->at(k))->CntrlOrderPreds->size() > 0);
					assert(enode_dag->at(redun_br_indices->at(k))->true_branch_succs_Ctrl->size() == 0 && enode_dag->at(redun_br_indices->at(k))->false_branch_succs_Ctrl->size() == 0 && enode_dag->at(redun_br_indices->at(k))->JustCntrlPreds->size() == 0);
					constCntrl_flag = false;
				}

			}

			cutBranchPreds(network_flag, constCntrl_flag, redun_br_indices->at(k), enode_dag->at(redun_br_indices->at(k)));

			// need to connect its true_successors and false_successors to those of the branch to keep 	
			cut_keepBranch_trueSuccs(network_flag, constCntrl_flag, redun_br_indices->at(k), redun_br_indices->at(0));
		
			cut_keepBranch_falseSuccs(network_flag, constCntrl_flag, redun_br_indices->at(k), redun_br_indices->at(0));

		}		
	
	}
	
}

void CircuitGenerator::cut_keepBranch_trueSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, int idx_of_br_to_keep) {

	int true_idx;

	ENode* redun_br = enode_dag->at(idx_of_redun_br);
	ENode* br_to_keep = enode_dag->at(idx_of_br_to_keep);

	switch(network_flag) {
		case data:
			if(redun_br->true_branch_succs->size() > 0) {
				assert(redun_br->true_branch_succs->size() == 1);
			
				br_to_keep->true_branch_succs->push_back(redun_br->true_branch_succs->at(0));

				// let this successor point to the branch_to_keep, instead of the redun_branch
				true_idx = returnPosRemoveNode(redun_br->true_branch_succs->at(0)->CntrlPreds, redun_br); 
				redun_br->true_branch_succs->at(0)->CntrlPreds->at(true_idx) = br_to_keep;
			}
		break;

		case constCntrl:
		case memDeps:
			if(constCntrl_flag) {
				if(redun_br->true_branch_succs_Ctrl->size() > 0) {
					assert(redun_br->true_branch_succs_Ctrl->size() == 1);
				
					br_to_keep->true_branch_succs_Ctrl->push_back(redun_br->true_branch_succs_Ctrl->at(0));

					// let this successor point to the branch_to_keep, instead of the redun_branch
					true_idx = returnPosRemoveNode(redun_br->true_branch_succs_Ctrl->at(0)->JustCntrlPreds, redun_br); 
			
					redun_br->true_branch_succs_Ctrl->at(0)->JustCntrlPreds->at(true_idx) = br_to_keep;

				}

			} else {
				if(redun_br->true_branch_succs_Ctrl_lsq->size() > 0) {
					assert(redun_br->true_branch_succs_Ctrl_lsq->size() == 1);
				
					br_to_keep->true_branch_succs_Ctrl_lsq->push_back(redun_br->true_branch_succs_Ctrl_lsq->at(0));

					// let this successor point to the branch_to_keep, instead of the redun_branch
					true_idx = returnPosRemoveNode(redun_br->true_branch_succs_Ctrl_lsq->at(0)->CntrlOrderPreds, redun_br); 
			
					redun_br->true_branch_succs_Ctrl_lsq->at(0)->CntrlOrderPreds->at(true_idx) = br_to_keep;

				}
			}
	}

	/* if(cntrl_br_flag) {
		if(redun_br->true_branch_succs_Ctrl->size() > 0) {
			assert(redun_br->true_branch_succs_Ctrl->size() == 1);
		
			br_to_keep->true_branch_succs_Ctrl->push_back(redun_br->true_branch_succs_Ctrl->at(0));

			// let this successor point to the branch_to_keep, instead of the redun_branch
			true_idx = returnPosRemoveNode(redun_br->true_branch_succs_Ctrl->at(0)->JustCntrlPreds, redun_br); 
	
			redun_br->true_branch_succs_Ctrl->at(0)->JustCntrlPreds->at(true_idx) = br_to_keep;

		}

	} else {
		if(redun_br->true_branch_succs->size() > 0) {
			assert(redun_br->true_branch_succs->size() == 1);
		
			br_to_keep->true_branch_succs->push_back(redun_br->true_branch_succs->at(0));

			// let this successor point to the branch_to_keep, instead of the redun_branch
			true_idx = returnPosRemoveNode(redun_br->true_branch_succs->at(0)->CntrlPreds, redun_br); 
			redun_br->true_branch_succs->at(0)->CntrlPreds->at(true_idx) = br_to_keep;
		}
	} */
}


void CircuitGenerator::cut_keepBranch_falseSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, int idx_of_br_to_keep) {

	int false_idx;

	ENode* redun_br = enode_dag->at(idx_of_redun_br);
	ENode* br_to_keep = enode_dag->at(idx_of_br_to_keep);

	switch(network_flag) {
		case data:
			if(redun_br->false_branch_succs->size() > 0) {
				assert(redun_br->false_branch_succs->size() == 1);
			
				br_to_keep->false_branch_succs->push_back(redun_br->false_branch_succs->at(0));

				// let this successor point to the branch_to_keep, instead of the redun_branch
				false_idx = returnPosRemoveNode(redun_br->false_branch_succs->at(0)->CntrlPreds, redun_br); 
				redun_br->false_branch_succs->at(0)->CntrlPreds->at(false_idx) = br_to_keep;
			}
		break;

		case constCntrl:
		case memDeps:
			if(constCntrl_flag) {
				if(redun_br->false_branch_succs_Ctrl->size() > 0) {
					assert(redun_br->false_branch_succs_Ctrl->size() == 1);
				
					br_to_keep->false_branch_succs_Ctrl->push_back(redun_br->false_branch_succs_Ctrl->at(0));
					// let this successor point to the branch_to_keep, instead of the redun_branch
					false_idx = returnPosRemoveNode(redun_br->false_branch_succs_Ctrl->at(0)->JustCntrlPreds, redun_br); 
			
					redun_br->false_branch_succs_Ctrl->at(0)->JustCntrlPreds->at(false_idx) = br_to_keep;

				}
			} else {
				if(redun_br->false_branch_succs_Ctrl_lsq->size() > 0) {
					assert(redun_br->false_branch_succs_Ctrl_lsq->size() == 1);
				
					br_to_keep->false_branch_succs_Ctrl_lsq->push_back(redun_br->false_branch_succs_Ctrl_lsq->at(0));
					// let this successor point to the branch_to_keep, instead of the redun_branch
					false_idx = returnPosRemoveNode(redun_br->false_branch_succs_Ctrl_lsq->at(0)->CntrlOrderPreds, redun_br); 
			
					redun_br->false_branch_succs_Ctrl_lsq->at(0)->CntrlOrderPreds->at(false_idx) = br_to_keep;

				}
			}

	}

	/*if(cntrl_br_flag) {
		if(redun_br->false_branch_succs_Ctrl->size() > 0) {
			assert(redun_br->false_branch_succs_Ctrl->size() == 1);
		
			br_to_keep->false_branch_succs_Ctrl->push_back(redun_br->false_branch_succs_Ctrl->at(0));
			// let this successor point to the branch_to_keep, instead of the redun_branch
			false_idx = returnPosRemoveNode(redun_br->false_branch_succs_Ctrl->at(0)->JustCntrlPreds, redun_br); 
	
			redun_br->false_branch_succs_Ctrl->at(0)->JustCntrlPreds->at(false_idx) = br_to_keep;

		}

	} else {
		if(redun_br->false_branch_succs->size() > 0) {
			assert(redun_br->false_branch_succs->size() == 1);
		
			br_to_keep->false_branch_succs->push_back(redun_br->false_branch_succs->at(0));

			// let this successor point to the branch_to_keep, instead of the redun_branch
			false_idx = returnPosRemoveNode(redun_br->false_branch_succs->at(0)->CntrlPreds, redun_br); 
			redun_br->false_branch_succs->at(0)->CntrlPreds->at(false_idx) = br_to_keep;
		}
	} */
}

void CircuitGenerator::cutBranchPreds(networkType network_flag, bool constCntrl_flag, int idx_of_redun_br, ENode* redun_br) {
	// Some sanity checks
	switch(network_flag) {
		case data:
			assert(redun_br->CntrlPreds->size() == 2);
		break;
		case constCntrl:
			assert(redun_br->JustCntrlPreds->size() == 1 && redun_br->CntrlPreds->size() == 1);
		break;
		case memDeps:
			assert(redun_br->CntrlOrderPreds->size() == 1 && redun_br->CntrlPreds->size() == 1);
		break;
	}
	/*if(cntrl_br_flag) {
		assert(redun_br->JustCntrlPreds->size() == 1 && redun_br->CntrlPreds->size() == 1);
	} else {
		assert(redun_br->CntrlPreds->size() == 2);
	}*/
	//////////////////// End of sanity checks 

	std::vector<ENode*>* redun_br_preds_data_succs;
	std::vector<ENode*>* redun_br_preds_cond_succs;

	switch(network_flag) {
		case data:
			redun_br_preds_data_succs = redun_br->CntrlPreds->at(0)->CntrlSuccs;
			redun_br_preds_cond_succs = redun_br->CntrlPreds->at(1)->CntrlSuccs;
		break;

		case constCntrl:
		case memDeps:
			// the condition predecessor is always in the data network!
			redun_br_preds_cond_succs = redun_br->CntrlPreds->at(0)->CntrlSuccs;
			if(constCntrl_flag) {
				// the data predecessor
				redun_br_preds_data_succs = redun_br->JustCntrlPreds->at(0)->JustCntrlSuccs;
			} else {
				// the data predecessor
				redun_br_preds_data_succs = redun_br->CntrlOrderPreds->at(0)->CntrlOrderSuccs;
			}
	}	

	auto pos_enode_dag = std::find(redun_br_preds_data_succs->begin(), redun_br_preds_data_succs->end(), redun_br);
	assert(pos_enode_dag != redun_br_preds_data_succs->end());
	redun_br_preds_data_succs->erase(pos_enode_dag);

	pos_enode_dag = std::find(redun_br_preds_cond_succs->begin(), redun_br_preds_cond_succs->end(), redun_br);
	assert(pos_enode_dag != redun_br_preds_cond_succs->end());
	redun_br_preds_cond_succs->erase(pos_enode_dag); 

	/*if(cntrl_br_flag) {
		// the data predecessor
		auto pos_enode_dag = std::find(redun_br->JustCntrlPreds->at(0)->JustCntrlSuccs->begin(), redun_br->JustCntrlPreds->at(0)->JustCntrlSuccs->end(), redun_br);
		assert(pos_enode_dag != redun_br->JustCntrlPreds->at(0)->JustCntrlSuccs->end());
		redun_br->JustCntrlPreds->at(0)->JustCntrlSuccs->erase(pos_enode_dag);

		// the condition predecessor 
		auto pos_enode_dag_ = std::find(redun_br->CntrlPreds->at(0)->CntrlSuccs->begin(), redun_br->CntrlPreds->at(0)->CntrlSuccs->end(), redun_br);
		assert(pos_enode_dag_ != redun_br->CntrlPreds->at(0)->CntrlSuccs->end());
		redun_br->CntrlPreds->at(0)->CntrlSuccs->erase(pos_enode_dag_); 
	} else {
		// the data predecessor
		auto pos_enode_dag = std::find(redun_br->CntrlPreds->at(0)->CntrlSuccs->begin(), redun_br->CntrlPreds->at(0)->CntrlSuccs->end(), redun_br);
		assert(pos_enode_dag != redun_br->CntrlPreds->at(0)->CntrlSuccs->end());
		redun_br->CntrlPreds->at(0)->CntrlSuccs->erase(pos_enode_dag);

		// the condition predecessor 
		auto pos_enode_dag_ = std::find(redun_br->CntrlPreds->at(1)->CntrlSuccs->begin(), redun_br->CntrlPreds->at(1)->CntrlSuccs->end(), redun_br);
		assert(pos_enode_dag_ != redun_br->CntrlPreds->at(1)->CntrlSuccs->end());
		redun_br->CntrlPreds->at(1)->CntrlSuccs->erase(pos_enode_dag_); 
	}*/

}


void CircuitGenerator::deleteExtraBranches(networkType network_flag) {
	std::vector<int>* redun_br_indices = new std::vector<int>;  // indices of the redundant branches in the enode_dag

	ENode* producer;
	ENode *enode_1, *enode_2;

	node_type branch_type;
	switch(network_flag) {
		case data:
			branch_type = Branch_n;
		break;

		case constCntrl:
		case memDeps:
			branch_type = Branch_c;
	}
	//node_type branch_type = (cntrl_br_flag)? Branch_c: Branch_n; 

	int j = 0;

	for(int i = 0; i < enode_dag->size(); i++) {
		enode_1 = enode_dag->at(i);
		redun_br_indices->clear();

		// check if this enode_1 is of type branch
		if(enode_1->type == branch_type && !enode_1->is_redunCntrlNet) {
			// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi or a Branch of our creation
			producer = enode_1->producer_to_branch;

			j = i+1;

			ENode* enode_1_branch_cond;

			switch(network_flag) {
				case data:
					enode_1_branch_cond = enode_1->CntrlPreds->at(1);
				break;

				case constCntrl:
				case memDeps:
					enode_1_branch_cond = enode_1->CntrlPreds->at(0);
			}
			/*if(cntrl_br_flag) {
				enode_1_branch_cond = enode_1->CntrlPreds->at(0);
			} else {
				enode_1_branch_cond = enode_1->CntrlPreds->at(1);
			}*/

			redun_br_indices->push_back(i);

			while(j < enode_dag->size()) {
				enode_2 = enode_dag->at(j);

				// AYa: 06/06/2022: Added a condition to also check on the condition of the 2 branches AND negation,
						// since with the SUPPRESS algorithm, you could have 2 branches in the same BB but with different conditions!!

				if(enode_2->type == branch_type && enode_2->BB == enode_1->BB && enode_2->producer_to_branch == producer && enode_2 != enode_1) {

					ENode* enode_2_branch_cond;
					switch(network_flag) {
						case data:
							enode_2_branch_cond = enode_2->CntrlPreds->at(1);
						break;

						case constCntrl:
						case memDeps:
							// branch condition is always in the data network even for Branch_c
							enode_2_branch_cond = enode_2->CntrlPreds->at(0);
					}
					/*if(cntrl_br_flag) {
						enode_2_branch_cond = enode_2->CntrlPreds->at(0);
					} else {
						enode_2_branch_cond = enode_2->CntrlPreds->at(1);
					}*/

					// AYa: 06/06/2022: Added a condition to also check on the condition of the 2 branches AND negation,
						// since with the SUPPRESS algorithm, you could have 2 branches in the same BB but with different conditions!!
					assert(enode_1->is_advanced_component && enode_2->is_advanced_component); // sanity check: all my Branches are now SUPPRESSORs with potentially inverted inputs
					if(enode_1_branch_cond == enode_2_branch_cond && enode_1->is_negated_input->at(1) == enode_2->is_negated_input->at(1)) {
						redun_br_indices->push_back(j);

						// delete this redundant phi!!
						///////////////////////////////////////////////////////
						auto pos_enode_dag = std::find(enode_dag->begin(), enode_dag->end(), enode_2);
						assert(pos_enode_dag != enode_dag->end());
						if (pos_enode_dag != enode_dag->end()) {
							enode_dag->erase(pos_enode_dag);
						} 
						
						// branch_id--;
						///////////////////////////////////////////////////////
					} else {
						j++;
					}
				} else {
					j++; 
				}
			} 

		}	
	
	}

}


/*

void CircuitGenerator::RedunPreds_SanityCheck(bool cntrl_br_flag, std::vector<int>* redun_br_indices) {
	for(int i = 1; i < redun_br_indices->size(); i++) {
		if(cntrl_br_flag) {
			assert(enode_dag->at(redun_br_indices->at(i))->type == Branch_c); 
			assert(enode_dag->at(redun_br_indices->at(i))->JustCntrlPreds->size() == 2);
			assert(enode_dag->at(redun_br_indices->at(i))->JustCntrlPreds->size() == enode_dag->at(redun_br_indices->at(0))->JustCntrlPreds->size());
			assert(enode_dag->at(redun_br_indices->at(i))->JustCntrlPreds->at(0) == enode_dag->at(redun_br_indices->at(0))->JustCntrlPreds->at(0));
			assert(enode_dag->at(redun_br_indices->at(i))->JustCntrlPreds->at(1) == enode_dag->at(redun_br_indices->at(0))->JustCntrlPreds->at(1));

		} else {
			assert(enode_dag->at(redun_br_indices->at(i))->type == Branch_n); 
			assert(enode_dag->at(redun_br_indices->at(i))->CntrlPreds->size() == 2);
			assert(enode_dag->at(redun_br_indices->at(i))->CntrlPreds->size() == enode_dag->at(redun_br_indices->at(0))->CntrlPreds->size());
			assert(enode_dag->at(redun_br_indices->at(i))->CntrlPreds->at(0) == enode_dag->at(redun_br_indices->at(0))->CntrlPreds->at(0));
			assert(enode_dag->at(redun_br_indices->at(i))->CntrlPreds->at(1) == enode_dag->at(redun_br_indices->at(0))->CntrlPreds->at(1));
			
		} 

	} 

} 

*/

