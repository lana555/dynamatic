#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

void CircuitGenerator::Fix_my_PhiPreds(LoopInfo& LI, networkType network_flag) {
	// 08/10/2021: made the function generic to serve for both Phi_n and Phi_c (the non-redundant version!!)

	node_type phi_type;

	std::vector<ENode*> *enode_preds; 
	
	/*if(cntrl_path_flag) {
		phi_type = Phi_c;
	} else {
		phi_type = Phi_n;
	}*/
	switch(network_flag) {
		case data:
			phi_type = Phi_n;
			break;
		case constCntrl:
			phi_type = Phi_c;
			break;
		case memDeps: 
			phi_type = Phi_c;
	}


	/* 
		- The assumption is that for any Phi_n (of our creation), it should have a predecessor inside the loop (that the Phi_n is inside) and another outside the loop.. To decide which predecessor should come first, we need to check the predecessors of the LLVM phi instruction and be compatible with them.

		- Another assumption is that any Phi_ that is in the same BB as my Phi_n should also be having one predecessor inside the loop and another outside. This is because the BB of the Phi_n is always a loop header.
	*/

	////// Some sanity checks to make sure that assumptions we are having are valid
	for(auto & enode :*enode_dag) {
		if(enode->type == phi_type && !enode->is_redunCntrlNet) {
			// get the loop that the Phi_type is part of
			Loop* phi_n_loop = LI.getLoopFor(enode->BB);
			// search for a Phi_ in the same BB as my Phi_n
			for(auto& enode_2 : *enode_dag) {
				if(enode_2->type == Phi_ && enode_2->BB == enode->BB) {
					ENode* llvm_phi = enode_2;
					BasicBlock* llvm_predBB_0 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(0);
					BasicBlock* llvm_predBB_1 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(1);

					assert((phi_n_loop->contains(llvm_predBB_0) && !phi_n_loop->contains(llvm_predBB_1)) || (phi_n_loop->contains(llvm_predBB_1) && !phi_n_loop->contains(llvm_predBB_0)));

					break;
				}

			}
		}
	}
	///////// End of sanity checks

	for(auto & enode :*enode_dag) {
		if(enode->type == phi_type && !enode->is_redunCntrlNet) {
			// get the loop that the Phi_n is part of
			Loop* phi_n_loop = LI.getLoopFor(enode->BB);

			/*if(cntrl_path_flag) {
				enode_preds = enode->JustCntrlPreds;
			} else {
				enode_preds = enode->CntrlPreds;
			}*/

			switch(network_flag) {
				case data:
					enode_preds = enode->CntrlPreds;
					break;
				case constCntrl:
					enode_preds = enode->JustCntrlPreds;
					break;
				case memDeps: 
					enode_preds = enode->CntrlOrderPreds;
			}

			// Because both the CntrlOrderPreds and the JustCntrlPreds operate on Phi_c, we might have a Phi_c that is part of the JustCntrlPreds
				// so it doesn't make sense to do checks on the CntrlOrderPreds network
						// if the size of the JustCntlPreds is not 0, even if it is memDeps, pass the JustCntrlPreds network
			bool cntrl_network_skip_flag = false;
			if(network_flag == memDeps) {
				if(enode->JustCntrlPreds->size() > 0)
					cntrl_network_skip_flag = true;
			}
			if(!cntrl_network_skip_flag)
				assert(enode_preds->size() == 2);

			// search for a Phi_ in the same BB as my Phi_type
			for(auto& enode_2 : *enode_dag) {
				if(enode_2->type == Phi_ && enode_2->BB == enode->BB && !cntrl_network_skip_flag) {
					ENode* llvm_phi = enode_2;
					BasicBlock* llvm_predBB_0 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(0);
					BasicBlock* llvm_predBB_1 = (dyn_cast<PHINode>(llvm_phi->Instr))->getIncomingBlock(1);
					
					if(phi_n_loop->contains(llvm_predBB_0)) {
						// check if the 0th pred of the Phi_n is not inside the loop
						if(!phi_n_loop->contains(enode_preds->at(0)->BB)) {
							// then the loop must be contaning the 1st pred
							assert(phi_n_loop->contains(enode_preds->at(1)->BB));
							// then we swap the 0th and 1st preds to be compatible with llvm
							ENode* temp = enode_preds->at(0);
							enode_preds->at(0) = enode_preds->at(1);
							enode_preds->at(1) = temp;
						} 

					} else {
						// then the loop must contain llvm_predBB_1!
						assert(phi_n_loop->contains(llvm_predBB_1));
						// check if the 1st pred of the Phi_n is not inside the loop
						if(!phi_n_loop->contains(enode_preds->at(1)->BB)) {
							// then the loop must be contaning the 0th pred
							assert(phi_n_loop->contains(enode_preds->at(0)->BB));
							// then we swap the 0th and 1st preds to be compatible with llvm
							ENode* temp = enode_preds->at(0);
							enode_preds->at(0) = enode_preds->at(1);
							enode_preds->at(1) = temp;
						} 

					} 
					break;   // found the llvm phi, stop looping!!
				}

			}

		}

	}
}


void CircuitGenerator::checkPhiModify(Loop* cons_loop, BasicBlock* producer_BB, BasicBlock* consumer_BB, ENode* orig_producer_enode, ENode* orig_consumer_enode, networkType network_flag, std::ofstream& h_file, std::vector<ENode*>* new_phis) {

	// AYA: 23/10/2021: the new parameter: new_phis should contain the phi nodes
			// added here so that I push them to the enode_dag in the end!!

    std::vector<ENode*> phis;

	int idx_1, idx_2;

	node_type phi_type; // shall we create a Phi_c or Phi_n
	std::string phi_name; 

	//std::vector<ENode*> *phi_succs, *phi_preds; 
	std::vector<ENode*> *orig_producer_succs, *orig_consumer_preds; 
	
	/*if(cntrl_path_flag) {
		phi_type = Phi_c;
		phi_name = "phiC";
	
		orig_producer_succs = orig_producer_enode->JustCntrlSuccs;
		orig_consumer_preds = orig_consumer_enode->JustCntrlPreds;

	} else {
		phi_type = Phi_n;
		phi_name = "phi";

		orig_producer_succs = orig_producer_enode->CntrlSuccs;
		orig_consumer_preds = orig_consumer_enode->CntrlPreds;
	}*/
	switch(network_flag) {
		case data:
			phi_type = Phi_n;
			phi_name = "phi";

			orig_producer_succs = orig_producer_enode->CntrlSuccs;
			orig_consumer_preds = orig_consumer_enode->CntrlPreds;
			break;
		case constCntrl:
			phi_type = Phi_c;
			phi_name = "phiC";
		
			orig_producer_succs = orig_producer_enode->JustCntrlSuccs;
			orig_consumer_preds = orig_consumer_enode->JustCntrlPreds;
			break;
		case memDeps:
			phi_type = Phi_c;
			phi_name = "phiC";
		
			orig_producer_succs = orig_producer_enode->CntrlOrderSuccs;
			orig_consumer_preds = orig_consumer_enode->CntrlOrderPreds;
	}
	
	// phis.clear();
    phis.push_back(orig_consumer_enode);
	
	while(cons_loop != nullptr) {	
		if(!cons_loop->contains(producer_BB)) {
        	BasicBlock *loop_header = cons_loop->getHeader();

			if(loop_header != consumer_BB || (orig_consumer_enode->type != Phi_ && orig_consumer_enode->type != Phi_n && orig_consumer_enode->type != Phi_c )){
				
                ENode* phi = new ENode(phi_type, phi_name.c_str(), loop_header);  
                phi->id = phi_id++;

				// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi of our creation
				phi->producer_to_branch = orig_producer_enode;

				// AYAA 23/10/2021: Will postpone pushing to the enode_dag to the very end!
                //enode_dag->push_back(phi);
				new_phis->push_back(phi);

				// for the case of nested loops, multiple phis will be inserted between the producer and consumer, so after finalizing the number of phis to be added, do the connections between the producer and consumer in the end 
                phis.push_back(phi);

				/*if(cntrl_path_flag) {
					h_file << "Added a new Phi in loop_header BB with BB ID " << BBMap->at(loop_header)->Idx << " and name " << getNodeDotNameNew(phi) << "\n";

				}*/
			}

			cons_loop = cons_loop->getParentLoop(); 

		} else {
			// if the producer is already inside this loop, stop iterating 
			cons_loop = nullptr;
		}  

	} 

    phis.push_back(orig_producer_enode);

	// check if any phis were already inserted
	if(phis.size() > 2) {

		// do the modifications in producer-consumer relationships 
       idx_1 = returnPosRemoveNode(orig_producer_succs, orig_consumer_enode);
       idx_2 = returnPosRemoveNode(orig_consumer_preds, orig_producer_enode);

       orig_producer_succs->at(idx_1) = phis.at(phis.size()-2);
        
        for(int j = phis.size()-2; j >= 1; j--) {
			/*if(cntrl_path_flag) {
				phis.at(j)->JustCntrlPreds->push_back(phis.at(j+1));
				phis.at(j)->JustCntrlSuccs->push_back(phis.at(j-1));
				
				// for every phi, make it feed itself to replicate the data
				phis.at(j)->JustCntrlSuccs->push_back(phis.at(j));
				phis.at(j)->JustCntrlPreds->push_back(phis.at(j));
			} else {
				phis.at(j)->CntrlPreds->push_back(phis.at(j+1));
				phis.at(j)->CntrlSuccs->push_back(phis.at(j-1));
			
				// for every phi, make it feed itself to replicate the data
				phis.at(j)->CntrlSuccs->push_back(phis.at(j));
				phis.at(j)->CntrlPreds->push_back(phis.at(j));
			} 

			// Assertions for debugging
			if(cntrl_path_flag) {
				assert(phis.at(j)->JustCntrlSuccs->size() == 2);
			} else {
				assert(phis.at(j)->CntrlSuccs->size() == 2);
			}*/
			switch(network_flag) {
				case data:
					phis.at(j)->CntrlPreds->push_back(phis.at(j+1));
					phis.at(j)->CntrlSuccs->push_back(phis.at(j-1));
				
					// for every phi, make it feed itself to replicate the data
					phis.at(j)->CntrlSuccs->push_back(phis.at(j));
					phis.at(j)->CntrlPreds->push_back(phis.at(j));

					// Assertions for debugging
					assert(phis.at(j)->CntrlSuccs->size() == 2);
					break;
				case constCntrl:
					phis.at(j)->JustCntrlPreds->push_back(phis.at(j+1));
					phis.at(j)->JustCntrlSuccs->push_back(phis.at(j-1));
					
					// for every phi, make it feed itself to replicate the data
					phis.at(j)->JustCntrlSuccs->push_back(phis.at(j));
					phis.at(j)->JustCntrlPreds->push_back(phis.at(j));

					// Assertions for debugging
					assert(phis.at(j)->JustCntrlSuccs->size() == 2);
					break;
				case memDeps:
					phis.at(j)->CntrlOrderPreds->push_back(phis.at(j+1));
					phis.at(j)->CntrlOrderSuccs->push_back(phis.at(j-1));
					
					// for every phi, make it feed itself to replicate the data
					phis.at(j)->CntrlOrderSuccs->push_back(phis.at(j));
					phis.at(j)->CntrlOrderPreds->push_back(phis.at(j));

					// Assertions for debugging
					assert(phis.at(j)->CntrlOrderSuccs->size() == 2);
			}

        }

      orig_consumer_preds->at(idx_2) = (phis.at(1));
		
	}
    
}

void CircuitGenerator::cutPhiPreds(networkType network_flag, bool constCntrl_flag, int idx_of_redun_phi, std::vector<ENode*> *redun_phi_preds) {
	// Some sanity checks
	assert(redun_phi_preds->size() == 2);
	//////////////////// End of sanity checks 

	std::vector<ENode*>* redun_phi_preds_0_succs;
	std::vector<ENode*>* redun_phi_preds_1_succs;
	switch(network_flag) {
		case data:
			redun_phi_preds_0_succs = redun_phi_preds->at(0)->CntrlSuccs;
			redun_phi_preds_1_succs = redun_phi_preds->at(1)->CntrlSuccs;
		break;

		case constCntrl:
		case memDeps:
			if(constCntrl_flag) {
				// extra sanity check
				assert(redun_phi_preds->at(0)->JustCntrlSuccs->size() > 0);
				assert(redun_phi_preds->at(1)->JustCntrlSuccs->size() > 0);
				redun_phi_preds_0_succs = redun_phi_preds->at(0)->JustCntrlSuccs;
				redun_phi_preds_1_succs = redun_phi_preds->at(1)->JustCntrlSuccs;
			} else {
				// extra sanity check
				assert(redun_phi_preds->at(0)->CntrlOrderSuccs->size() > 0);
				assert(redun_phi_preds->at(1)->CntrlOrderSuccs->size() > 0);
				redun_phi_preds_0_succs = redun_phi_preds->at(0)->CntrlOrderSuccs;
				redun_phi_preds_1_succs = redun_phi_preds->at(1)->CntrlOrderSuccs;
			}
	}

	auto pos_enode_dag = std::find(redun_phi_preds_0_succs->begin(), redun_phi_preds_0_succs->end(), enode_dag->at(idx_of_redun_phi));
	assert(pos_enode_dag != redun_phi_preds_0_succs->end());
	redun_phi_preds_0_succs->erase(pos_enode_dag);

	pos_enode_dag = std::find(redun_phi_preds_1_succs->begin(), redun_phi_preds_1_succs->end(), enode_dag->at(idx_of_redun_phi));
	assert(pos_enode_dag != redun_phi_preds_1_succs->end());
	redun_phi_preds_1_succs->erase(pos_enode_dag);
	
}

void CircuitGenerator::FixPhiSuccs(networkType network_flag, bool constCntrl_flag, int idx_of_redun_phi, int idx_of_phi_to_keep, std::vector<ENode*> *redun_phi_succs) {
	// The purpose of this function is to add the successors of the redundant phi to the phi that we will keep, if the redundant phi happens to have any remaining succs!!
	
	int idx; 
	if(redun_phi_succs->size() > 0) {
		switch(network_flag) {
			case data:
				idx = returnPosRemoveNode(redun_phi_succs->at(0)->CntrlPreds, enode_dag->at(idx_of_redun_phi));

				redun_phi_succs->at(0)->CntrlPreds->at(idx) = enode_dag->at(idx_of_phi_to_keep);

				// Now we need to push_back this successor to the JustCntrlSuccs of the phi to keep
				enode_dag->at(idx_of_phi_to_keep)->CntrlSuccs->push_back(redun_phi_succs->at(0));

				break;
			case constCntrl:
			case memDeps:
				if(constCntrl_flag) {
					// extra sanity check:
					assert(redun_phi_succs->at(0)->JustCntrlPreds->size() > 0);

					idx = returnPosRemoveNode(redun_phi_succs->at(0)->JustCntrlPreds, enode_dag->at(idx_of_redun_phi));

					redun_phi_succs->at(0)->JustCntrlPreds->at(idx) = enode_dag->at(idx_of_phi_to_keep);

					// Now we need to push_back this successor to the JustCntrlSuccs of the phi to keep
					enode_dag->at(idx_of_phi_to_keep)->JustCntrlSuccs->push_back(redun_phi_succs->at(0));

				} else {
					// extra sanity check:
					assert(redun_phi_succs->at(0)->CntrlOrderPreds->size() > 0);

					idx = returnPosRemoveNode(redun_phi_succs->at(0)->CntrlOrderPreds, enode_dag->at(idx_of_redun_phi));

					redun_phi_succs->at(0)->CntrlOrderPreds->at(idx) = enode_dag->at(idx_of_phi_to_keep);

					// Now we need to push_back this successor to the JustCntrlSuccs of the phi to keep
					enode_dag->at(idx_of_phi_to_keep)->CntrlOrderSuccs->push_back(redun_phi_succs->at(0));

				}

		}

	} 

}

void CircuitGenerator::removeExtraPhisWrapper(networkType network_flag) {
	removeExtraPhis(network_flag);
	deleteExtraPhis(network_flag);
} 

void CircuitGenerator::deleteExtraPhis(networkType network_flag) {
	std::vector<int>* redun_phi_indices = new std::vector<int>;  // indices of the redundant phis in the enode_dag

	ENode* producer;
	ENode *enode_1, *enode_2;

	node_type phi_type;

	switch(network_flag) {
		case data:
			phi_type = Phi_n;
		break;
		case constCntrl:
		case memDeps:
			phi_type = Phi_c;
	}

	int j = 0;

	for(int i = 0; i < enode_dag->size(); i++) {
		enode_1 = enode_dag->at(i);
		redun_phi_indices->clear();

		// check if this enode_1 is of type phi
		if(enode_1->type == phi_type && !enode_1->is_redunCntrlNet) {
			// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi or a Branch of our creation
			producer = enode_1->producer_to_branch;

			j = i+1;

			redun_phi_indices->push_back(i);

			// for(int j = i+1; j < enode_dag->size(); j++) {
			while(j < enode_dag->size()) {
				enode_2 = enode_dag->at(j);
				if(enode_2->type == phi_type && enode_2->BB == enode_1->BB && enode_2->producer_to_branch == producer && enode_2 != enode_1) {
					redun_phi_indices->push_back(j);

					// delete this redundant phi!!
					///////////////////////////////////////////////////////
					auto pos_enode_dag = std::find(enode_dag->begin(), enode_dag->end(), enode_2);
					assert(pos_enode_dag != enode_dag->end());
					if (pos_enode_dag != enode_dag->end()) {
						enode_dag->erase(pos_enode_dag);
					}
					///////////////////////////////////////////////////////
				} else {
					j++; 
				}
			} 

		}	
	
	}

}

void CircuitGenerator::removeExtraPhis(networkType network_flag) {
	std::vector<int>* redun_phi_indices = new std::vector<int>;  // indices of the redundant phis in the enode_dag

	// TEMPORARILY FOR DEBUGGING!!!!
	std::ofstream t_file;
	t_file.open("aya_check_redun_phi.txt");


	///////////////////////////
	int idx_i, idx_j;
	int j = 0;
	std::vector<ENode*>* enode_dag_cpy = new std::vector<ENode*>;
	for(int i = 0; i < enode_dag->size(); i++) {
		enode_dag_cpy->push_back(enode_dag->at(i));
	} 

	//////////////////////////

	std::vector<ENode*> *redun_phi_preds, *redun_phi_succs;

	ENode* producer;
	ENode *enode_1, *enode_2;

	node_type phi_type;
	switch(network_flag) {
		case data:
			phi_type = Phi_n;
			break;
		case constCntrl:
		case memDeps:
			phi_type = Phi_c;
	}


	// loop over all enodes and search for two enodes of type Phi_n or Phi_c (depending on the network you are currently working) having the same producer
	for(int i = 0; i < enode_dag_cpy->size(); i++) {
		enode_1 = enode_dag_cpy->at(i);
		redun_phi_indices->clear();

		// check if this enode_1 is of type phi
		if(enode_1->type == phi_type && !enode_1->is_redunCntrlNet) {
			// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi or a Branch of our creation
			producer = enode_1->producer_to_branch;

			j = i+1;

			/////////////// To decide the idx to push
			idx_i = returnPosRemoveNode(enode_dag, enode_1);
			////////////////////////////

			redun_phi_indices->push_back(idx_i);

			t_file << "\nNEWW ENode 1 called: " << getNodeDotNameNew(enode_1) << "\n";

			t_file << "\nENode 1 called: " << getNodeDotNameNew(enode_1) << " has producer called " << getNodeDotNameNew(enode_1->producer_to_branch) << "\n";

			while(j < enode_dag_cpy->size()) {
				enode_2 = enode_dag_cpy->at(j);
				t_file << "\nENode 2 called: " << getNodeDotNameNew(enode_2) << " has producer called " << getNodeDotNameNew(enode_2->producer_to_branch) << "\n";
				if(enode_2->type == phi_type && enode_2->BB == enode_1->BB && enode_2->producer_to_branch == producer && enode_2 != enode_1) {

					/////////////// To decide the idx to push
					idx_j = returnPosRemoveNode(enode_dag, enode_2);
					////////////////////////////
			
					redun_phi_indices->push_back(idx_j);
	
					t_file << "\nREDUN ENode 2 called: " << getNodeDotNameNew(enode_2) << "\n";
				// Erase this redundant Phi from the enode_dag_cpy to not loop over it again
					auto pos_enode_dag = std::find(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_2);
					assert(pos_enode_dag != enode_dag_cpy->end());
					if (pos_enode_dag != enode_dag_cpy->end()) {
						enode_dag_cpy->erase(pos_enode_dag);
					} 
					////////////////////////////
				} else {
					// increment the iterator only if you didn't delete an element
					j++;
				}
			} 

		}
	
		// At this point, redundant_phis contains a copy of all phis that are redundant to enode_1

		for(int k = 1; k < redun_phi_indices->size(); k++) {
			/*if(cntrl_phi_flag) {
				redun_phi_preds = enode_dag->at(redun_phi_indices->at(k))->JustCntrlPreds;
				redun_phi_succs = enode_dag->at(redun_phi_indices->at(k))->JustCntrlSuccs;
			} else {
				redun_phi_preds = enode_dag->at(redun_phi_indices->at(k))->CntrlPreds;
				redun_phi_succs = enode_dag->at(redun_phi_indices->at(k))->CntrlSuccs;
			}*/

			bool constCntrl_flag = false;  // flag that is true if the redun phi has preds and succs in the JUstCntrl network and false for the CntrlOrder network

			switch(network_flag) {
				case data:
					redun_phi_preds = enode_dag->at(redun_phi_indices->at(k))->CntrlPreds;
					redun_phi_succs = enode_dag->at(redun_phi_indices->at(k))->CntrlSuccs;
					break;
				case constCntrl:
				case memDeps:
					// since those two networks have the same phi_type, we need to check the size of the preds and succs to know which network to pass
					if(enode_dag->at(redun_phi_indices->at(k))->JustCntrlPreds->size() > 0) {
						// sanity checks
						assert(enode_dag->at(redun_phi_indices->at(k))->JustCntrlSuccs->size() > 0);
						assert(enode_dag->at(redun_phi_indices->at(k))->CntrlOrderSuccs->size() == 0 && enode_dag->at(redun_phi_indices->at(k))->CntrlOrderPreds->size() == 0);

						redun_phi_preds = enode_dag->at(redun_phi_indices->at(k))->JustCntrlPreds;
						redun_phi_succs = enode_dag->at(redun_phi_indices->at(k))->JustCntrlSuccs;

						constCntrl_flag = true;

					} else {
						// sanity checks 
						assert(enode_dag->at(redun_phi_indices->at(k))->CntrlOrderSuccs->size() > 0 && enode_dag->at(redun_phi_indices->at(k))->CntrlOrderPreds->size() > 0);
						assert(enode_dag->at(redun_phi_indices->at(k))->JustCntrlSuccs->size() == 0 && enode_dag->at(redun_phi_indices->at(k))->JustCntrlPreds->size() == 0);

						redun_phi_preds = enode_dag->at(redun_phi_indices->at(k))->CntrlOrderPreds;
						redun_phi_succs = enode_dag->at(redun_phi_indices->at(k))->CntrlOrderSuccs;

						constCntrl_flag = false;
					}
						
			}

			// cut the connection between the producer and the redundant PHI
			cutPhiPreds(network_flag, constCntrl_flag, redun_phi_indices->at(k), redun_phi_preds);
			// cut the connection between the consumer and the redundant PHI, and connect the successors of the PHI to the first PHI that we will keep 
			FixPhiSuccs(network_flag, constCntrl_flag, redun_phi_indices->at(k), redun_phi_indices->at(0), redun_phi_succs);

		}		
	
	}
	
}

void CircuitGenerator::checkLoops(Function& F, LoopInfo& LI, networkType network_flag) {
	std::ofstream h_file;
	//if(cntrl_path_flag) {
		h_file.open("aya_DEBUG_LOOPS_PHIS.txt");
	//} 
	
	// AYA: 23/10/2021
	std::vector<ENode*>* new_phis = new std::vector<ENode*>;

	std::vector<ENode*> *enode_succs;
	std::vector<ENode*> *orig_producer_succs, *orig_consumer_preds, *phi_succs, *phi_preds;

	node_type phi_type; // shall we create a Phi_c or Phi_n
	std::string phi_name; 

	int idx;

    std::vector<ENode*> branches;

    for (auto& enode : *enode_dag) {
		if(enode->BB == nullptr) 
			continue;   // skip the iteration 
        // loop over its consumers (successor nodes)
		// enode_succs = (cntrl_path_flag) ? enode->JustCntrlSuccs : enode->CntrlSuccs;

		switch(network_flag) {
			case data:
				enode_succs = enode->CntrlSuccs;
				break;
			case constCntrl:
				enode_succs = enode->JustCntrlSuccs;
				break;
			case memDeps:
				enode_succs = enode->CntrlOrderSuccs;
		}

        for (auto& succ : *enode_succs) {
			if(succ->BB == nullptr) 
				continue;
            // they must be in different BBs
            if (enode->BB != succ->BB) { 
               	checkPhiModify(LI.getLoopFor(succ->BB), enode->BB, succ->BB, enode, succ, network_flag, h_file, new_phis); 
            }
        }
    }

	// 23/10/2021:
	// push all the phis added here to the enode_dag!!
	for(int i = 0; i < new_phis->size(); i++) {
		assert(new_phis->at(i)->type == Phi_n || new_phis->at(i)->type == Phi_c);
		enode_dag->push_back(new_phis->at(i));
	}
}

