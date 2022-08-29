#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

// I want to detect just a single case which is the case of a consumer inside a loop that the producer is not part of...

/*ENode* CircuitGenerator::addInj_hack(Enode* prod_enode, Enode* cons_enode, BasicBlock* cons_loop_header) {
	// first, add a fork to replicate the output of prod_enode into 2
	ENode* fork = new ENode(Fork_, "fork", cons_enode->BB);
	fork->id = fork_id++;
	enode_dag->push_back(fork);

	// the first successor of the fork is the original consumer
	// the second successor of the fork is the register

	// second, add a buffer object
	// THIS I MIGHT NEED TO HAVE WITH AN ENABLE!!!
	new_node = new ENode(Buffera_, "buffA");
    new_node->id = CircuitGenerator::buff_id++;

	// third, add a MUX and this is what we return as the output of the function
	ENode* mux = new ENode(Phi_n, "phi", cons_enode->BB);  
    mux->id = phi_id++;
	enode_dag->push_back(mux);
	mux->isMux = true;

	// what will be the SEL of this MUX???


	return; 
}

void CircuitGenerator::addInj_hack_wrapper(bool cntrl_path_flag, std::ofstream& dbg_file) {
	std::vector<ENode*> *enode_succs;
	for (auto& enode : *enode_dag) {

		if(enode->BB == nullptr) 
			continue;   // skip the iteration 
        // loop over its consumers (successor nodes)
		//enode_succs = (cntrl_path_flag) ? enode->JustCntrlSuccs : enode->CntrlSuccs;
        for (int i = 0; i <  enode->CntrlSuccs->size(); i++) {
			ENode* succ = enode->CntrlSuccs->at(i);

			if(succ->BB == nullptr) 
				continue;

			// if the consumer isn't inside any loop, then skip the iteration..
			if((BBMap->at(succ->BB))->loop == nullptr) 
				continue;

            // they must be in different BBs (otherwise there is no token count mismatch!)
            if (enode->BB != succ->BB) {
				
				if((BBMap->at(enode->BB))->loop != (BBMap->at(succ->BB))->loop) {
					// the problem only occurs if the consumer is in a loop that the producer is not part of 
					BasicBlock *cons_loop_header = ((BBMap->at(succ->BB))->loop)->getHeader();

					// make sure to not consider the case handled by LLVM to maintain SSA
					if(cons_loop_header != succ->BB || (succ->type != Phi_ && succ->type != Phi_n && succ->type != Phi_c )) {
					
						// create one instance of the token regenerator
						ENode* token_inj = new ENode(Inj_n, "inj", cons_loop_header);  
						token_inj->id = inj_id++;
						// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi of our creation
						token_inj->producer_to_branch = enode;
						// enode_dag->push_back(token_inj);
						

						ENode* token_inj = addInjHack(cons_loop_header);

						// create the new node that should be the condition
						ENode* merge_for_sel = addMergeLoopSel_for_injector(token_inj);

						// its cond should be at in0, and it should be the output of a Merge that mimics an initialization token
						merge_for_sel->CntrlSuccs->push_back(token_inj);
						token_inj->CntrlPreds->push_back(merge_for_sel);

						auto pos_ = std::find(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode);
						assert(pos_!= succ->CntrlPreds->end());
						
						succ->CntrlPreds->at(pos_- succ->CntrlPreds->begin()) = token_inj;
						token_inj->CntrlSuccs->push_back(succ);

						// replace the connection between the producer and consumer with token_inj
						enode->CntrlSuccs->at(i) = token_inj;
						token_inj->CntrlPreds->push_back(enode);

						// enode_dag->push_back(token_inj);

					}

				}
			}

		}

	}

} */

void CircuitGenerator::addInj(bool cntrl_path_flag, std::ofstream& dbg_file) {

	std::vector<ENode*> *enode_succs;
	for (auto& enode : *enode_dag) {
		dbg_file << "\n\n-----------------------------------\n";
		
		dbg_file << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlSuccs->size() << " successors.. ";
		if((BBMap->at(enode->BB))->loop == nullptr)
			dbg_file << "It is not inside any loop!!\n";
		else 
			dbg_file << "It is inside a loop!!\n";

		if(enode->BB == nullptr) 
			continue;   // skip the iteration 
        // loop over its consumers (successor nodes)
		//enode_succs = (cntrl_path_flag) ? enode->JustCntrlSuccs : enode->CntrlSuccs;
        for (int i = 0; i <  enode->CntrlSuccs->size(); i++) {
			ENode* succ = enode->CntrlSuccs->at(i);

			dbg_file << "Succs name is " << getNodeDotNameNew(succ) << " of type " <<  getNodeDotTypeNew(succ) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(succ->BB));
		if((BBMap->at(enode->BB))->loop == nullptr)
			dbg_file << "Succ is not inside any loop!!\n";
		else 
			dbg_file << "Succ is inside a loop!!\n";

			if(succ->BB == nullptr) 
				continue;

			// if the consumer isn't inside any loop, then skip the iteration..
			if((BBMap->at(succ->BB))->loop == nullptr) 
				continue;

            // they must be in different BBs (otherwise there is no token count mismatch!)
            if (enode->BB != succ->BB) {
				
				if((BBMap->at(enode->BB))->loop != (BBMap->at(succ->BB))->loop) {
					// the problem only occurs if the consumer is in a loop that the producer is not part of 
					BasicBlock *cons_loop_header = ((BBMap->at(succ->BB))->loop)->getHeader();

					// make sure to not consider the case handled by LLVM to maintain SSA
					if(cons_loop_header != succ->BB || (succ->type != Phi_ && succ->type != Phi_n && succ->type != Phi_c )) {
					
						// create one instance of the token regenerator
						ENode* token_inj = new ENode(Inj_n, "inj", cons_loop_header);  
						token_inj->id = inj_id++;
						// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi of our creation
						token_inj->producer_to_branch = enode;
						// enode_dag->push_back(token_inj);

						// create the new node that should be the condition
						ENode* merge_for_sel = addMergeLoopSel_for_injector(token_inj);

						// its cond should be at in0, and it should be the output of a Merge that mimics an initialization token
						merge_for_sel->CntrlSuccs->push_back(token_inj);
						token_inj->CntrlPreds->push_back(merge_for_sel);

						auto pos_ = std::find(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode);
						assert(pos_!= succ->CntrlPreds->end());
						
						succ->CntrlPreds->at(pos_- succ->CntrlPreds->begin()) = token_inj;
						token_inj->CntrlSuccs->push_back(succ);

						// replace the connection between the producer and consumer with token_inj
						enode->CntrlSuccs->at(i) = token_inj;
						token_inj->CntrlPreds->push_back(enode);

						enode_dag->push_back(token_inj);

					}

				}
			}

		}

	}

    
}

