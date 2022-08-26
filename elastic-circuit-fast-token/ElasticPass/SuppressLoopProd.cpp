/*
 * Authors: Ayatallah Elakhras
 * 			
 */
#include "ElasticPass/CircuitGenerator.h"
#include<iterator>
#include<algorithm>


void CircuitGenerator::debugLoopsOutputs(std::ofstream& dbg_file) {
	std::vector<BBNode*> loop_exits;
	for(auto& bbnode: *bbnode_dag) {
		loop_exits.clear();
		dbg_file << "\n We are starting a new BB with number: " << bbnode->Idx + 1 << "\n";
		if(bbnode->loop != nullptr) {
			dbg_file << "\tThe BB is inside a loop!!";
			dbg_file << "\t The loop exits of this loop are: ";
			FindLoopExits(bbnode->loop, loop_exits);
			for(int k = 0; k < loop_exits.size(); k++) {
				dbg_file << "BB" << loop_exits.at(k)->Idx + 1 << ", ";
			}
		} else {
			dbg_file << "\t The BB is not inside any loop!!";
		}
	}
}

void CircuitGenerator::addMergeRegeneration(std::ofstream& dbg_file, bool const_trigger_network_flag) {
	ENode* producer;
	ENode* consumer;

	node_type phi_type; // shall we create a Phi_c or Phi_n
	std::string phi_name; 
	
	if(const_trigger_network_flag) {
		phi_type = Phi_c;
		phi_name = "phiC";
	} else {
		phi_type = Phi_n;
		phi_name = "phi";
	}

	for(auto& enode : *enode_dag) {  // for every node
		if(enode->BB == nullptr)
			continue;

		std::vector<ENode*>* cons_preds;
		if(const_trigger_network_flag) {
			cons_preds = enode->JustCntrlPreds;
		} else {
			cons_preds = enode->CntrlPreds;
		}

		consumer = enode;

		for (int h = 0; h < cons_preds->size(); h++) {  // loop over its producers
			if(cons_preds->at(h)->BB == nullptr || cons_preds->at(h)->BB == enode->BB || (BBMap->at(consumer->BB))->loop == nullptr)
				continue; // skip if the consumer is not inside any BB or if the producer and consumer are in the same BB
							// Or, if the consumer is not inside any loop

			producer = cons_preds->at(h);

			if(producer->type == Phi_n || producer->type == Phi_c || producer->type == Phi_ ||
					consumer->type == Phi_n || consumer->type == Phi_c || consumer->type == Phi_)
				continue;


			// if the consumer is inside a loop that the producer is not inside
			//if((BBMap->at(producer->BB))->loop != (BBMap->at(consumer->BB))->loop && consumer->type != Phi_ && consumer->type != Phi_n && consumer->type != Phi_c) {
			if(!(BBMap->at(consumer->BB))->loop->contains(producer->BB)) {
				//////////// Temporarily for debugging!!
				if(const_trigger_network_flag) {
					dbg_file << "\n\n===========Consumer Loop does not contain the Producer=============\n\n";
					dbg_file << "\n(Producer,Consumer) pair: (BB" << BBMap->at(producer->BB)->Idx + 1 << ", BB" << BBMap->at(consumer->BB)->Idx  + 1<< ")\n";
					dbg_file << "\nConsumer of type " << getNodeDotTypeNew(consumer) << "!!\n\n";
				}
				///////////////////////////////////////////////
				// insert a new Phi_n between them!
				ENode* phi = new ENode(phi_type, phi_name.c_str(), (BBMap->at(consumer->BB))->loop->getHeader());  
                phi->id = phi_id++;
				// the naming of this field is confusing, but it's meant to hold the producer enode that feeds a Phi of our creation
				phi->producer_to_branch = producer;

				//////////// Temporarily for debugging!!
				if(const_trigger_network_flag) {
					dbg_file << "\tInserted a Phi_c node in BB" << BBMap->at((BBMap->at(consumer->BB))->loop->getHeader())->Idx + 1 << "\n";
				}
				///////////////////////////////////////////////

				if(const_trigger_network_flag) {
					// insert the Phi_ between the producer and consumer, and the 0th input of it will be to itself
					phi->JustCntrlSuccs->push_back(phi);
					phi->JustCntrlPreds->push_back(phi);

					// search for the original consumer in the succs of the original producer
					auto pos_1 = std::find(producer->JustCntrlSuccs->begin(), producer->JustCntrlSuccs->end(), consumer);
					assert(pos_1 != producer->JustCntrlSuccs->end());
					auto index_1 = pos_1 - producer->JustCntrlSuccs->begin();
					producer->JustCntrlSuccs->at(index_1) = phi;
					phi->JustCntrlPreds->push_back(producer);

					// search for the original prodcuer in the preds of the original consumer
					auto pos_2 = std::find(consumer->JustCntrlPreds->begin(), consumer->JustCntrlPreds->end(), producer);
					assert(pos_2 != consumer->JustCntrlPreds->end());
					auto index_2 = pos_2 - consumer->JustCntrlPreds->begin();
					consumer->JustCntrlPreds->at(index_2) = phi;
					phi->JustCntrlSuccs->push_back(consumer);

				} else {
					// insert the Phi_ between the producer and consumer, and the 0th input of it will be to itself
					phi->CntrlSuccs->push_back(phi);
					phi->CntrlPreds->push_back(phi);

					// search for the original consumer in the succs of the original producer
					auto pos_1 = std::find(producer->CntrlSuccs->begin(), producer->CntrlSuccs->end(), consumer);
					assert(pos_1 != producer->CntrlSuccs->end());
					auto index_1 = pos_1 - producer->CntrlSuccs->begin();
					producer->CntrlSuccs->at(index_1) = phi;
					phi->CntrlPreds->push_back(producer);

					// search for the original prodcuer in the preds of the original consumer
					auto pos_2 = std::find(consumer->CntrlPreds->begin(), consumer->CntrlPreds->end(), producer);
					assert(pos_2 != consumer->CntrlPreds->end());
					auto index_2 = pos_2 - consumer->CntrlPreds->begin();
					consumer->CntrlPreds->at(index_2) = phi;
					phi->CntrlSuccs->push_back(consumer);
				}

				enode_dag->push_back(phi);

			}
		}
	}
}

void CircuitGenerator::addLoopsExitstoDeps_Phi_regeneration(ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps) {
	// extra sanity check that they are both in the same loop
	assert((BBMap->at(producer->BB))->loop != nullptr && (BBMap->at(producer->BB))->loop == (BBMap->at(consumer->BB))->loop);

	std::vector<Loop*> loops_in_nest;
 	FindLoopsInNest(BBMap->at(producer->BB)->loop, loops_in_nest);

 	std::vector<BBNode*> one_loop_exits;

 	// loop over all the loops in the loop-nest 
	for(int i = 0; i < loops_in_nest.size(); i++) {
		one_loop_exits.clear();
		FindLoopExits(loops_in_nest.at(i), one_loop_exits);

		// push back all of the loop exit BBs of the one_loop_exits
		for(int k = 0; k < one_loop_exits.size(); k++) {
			modif_consBB_deps.push_back(one_loop_exits.at(k)->Idx);
		}
	} 
}

/**
 * @brief The following adds loop exits to the dependency of the consumer in the situtaion of loop-carried dependency where the consumer is PHI_ and the producer is coming later in the loop 
 *  @param 
 *  @note it shouldn't add the consumerBB to the list of dependencies if it happens to be a loop exit, but it can add the producerBB if it happens to be a loop exit
 */
void CircuitGenerator::addLoopsExitstoDeps_ProdInConsIn_BackEdge(ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps) {
	// Removed the following assertion since I might have a backward edge from an inner loop to a PHI_ in an outer loop!!
	// extra sanity check that they are both in the same loop
	//assert((BBMap->at(producer->BB))->loop != nullptr && (BBMap->at(producer->BB))->loop == (BBMap->at(consumer->BB))->loop);

	std::vector<BBNode*> loop_exits;
	FindLoopExits((BBMap->at(consumer->BB))->loop, loop_exits);

	// erase the consumerBB if it happened to be a loop exit!!
	if(producer->BB != consumer->BB){
		auto pos = std::find(loop_exits.begin(), loop_exits.end(), (BBMap->at(consumer->BB)));
		if(pos != loop_exits.end()) {
			loop_exits.erase(pos);
		}
	}

	// push the indices of the BBs in loop_exits to modif_consBB_deps
	for(int i = 0; i < loop_exits.size(); i++) {
		modif_consBB_deps.push_back(loop_exits.at(i)->Idx);
	}
}

/**
 * @brief 
 * @param 
 */
void CircuitGenerator::addLoopsExitstoDeps_ProdInConsOut(std::ofstream& dbg_file, ENode* producer, ENode* consumer, std::vector<int>& modif_consBB_deps) {
	std::vector<BBNode*> loop_exits;
	FindLoopExits_ProdInConsOut(dbg_file, producer, consumer, loop_exits);

	// push the indices of the BBs in loop_exits to modif_consBB_deps
	for(int i = 0; i < loop_exits.size(); i++) {
		modif_consBB_deps.push_back(loop_exits.at(i)->Idx);
	}
}

/**
 * @brief 
 * @param 
 */
 void CircuitGenerator::FindLoopExits_ProdInConsOut(std::ofstream& dbg_file, ENode* producer, ENode* consumer, std::vector<BBNode*>& loop_exits) {
 	std::vector<Loop*> loops_in_nest;
 	FindLoopsInNest(BBMap->at(producer->BB)->loop, loops_in_nest);

 	std::vector<BBNode*> one_loop_exits;
 	dbg_file << "\n****************Inside FindLoopExits_ProdInConsOut*********\n";

 	// loop over all the loops in the loop-nest that the producer is inside and push the loop exits of the loops that the 
 		// consumer is not inside
	for(int i = 0; i < loops_in_nest.size(); i++) {
		dbg_file << "\n\tStarting a new loop in the loop nest containing the producerBB!!\n";

		if(!loops_in_nest.at(i)->contains(consumer->BB)) {
			dbg_file << "\n\tThe consumer is not inside this loop!!\n";
			one_loop_exits.clear();
			FindLoopExits(loops_in_nest.at(i), one_loop_exits);

			dbg_file << "\n\tThe loop exits of this loop are: ";

			// push back all of the loop exit BBs of the one_loop_exits
			for(int k = 0; k < one_loop_exits.size(); k++) {
				loop_exits.push_back(one_loop_exits.at(k));
				dbg_file << "BB" << one_loop_exits.at(k)->Idx + 1 << " , ";
			}
			dbg_file <<"\n\n";

		} else {
			dbg_file << "\n\tThe consumer is inside this loop!!\n";
			break; // consumer is inside that loop and all the coming loops in the nest, so no need to continue looping
		}
	} 

 }

/**
 * @brief 
 * @param 
 */
void CircuitGenerator::FindLoopsInNest(Loop* loop, std::vector<Loop*>& loops_in_nest) {
	Loop* current_loop = loop;
	do {
		loops_in_nest.push_back(current_loop);
		current_loop = current_loop->getParentLoop();
	} while(current_loop!=nullptr);
}

/**
 * @brief 
 * @param 
 */
void CircuitGenerator::FindLoopExits(Loop* loop, std::vector<BBNode*>& loop_exits) {
	// loop over all the BBs in the CFG and push to loop_exits the BBs that are loop exits of this loop
	for(auto& bbnode: *bbnode_dag) {
		if(bbnode->is_virtBB)
			continue;
		if(loop->isLoopExiting(bbnode->BB) && loop->contains(bbnode->BB))
			loop_exits.push_back(bbnode);
	}
}


// AT the moment, I'm not using this function anywhere!! I had it there during my initial plans only!!
/**
 * @brief 
 * @param 
 */
void CircuitGenerator::addSuppLoopExit(bool const_trigger_network_flag) {
	ENode* producer;
	ENode* consumer;

	std::vector<Loop*> loops_in_nest;
	std::vector<BBNode*> loop_exits;
	
	// loop over all enodes and check for a case of producer inside a loop that the consumer is not inside.
	for(auto& enode: *enode_dag) {
		producer = enode;
		std::vector<ENode*>* prod_succs;
		if(const_trigger_network_flag) {
			prod_succs = enode->JustCntrlSuccs;
		} else {
			prod_succs = enode->CntrlSuccs;
		}

		for(auto& succ: *prod_succs) {
			consumer = succ;
			if((BBMap->at(producer->BB))->loop == nullptr) // producer is not inside any loop
				continue;

			// find the entire loop-nest that the producer is inside
			loops_in_nest.clear();
			FindLoopsInNest(BBMap->at(producer->BB)->loop, loops_in_nest);

			// for each loop in the producer's loop-nest check if the consumer is not inside this loop
				// Note that loops_in_nest contains first (.at(0)) the innermost loop that the producer is inside and then loops at outer levels
			for(int i = 0; i < loops_in_nest.size(); i++) {
				if(!loops_in_nest.at(i)->contains(consumer->BB)) {
					// identify the loop exits of this loop object (could be a multi-exit loop)
					loop_exits.clear();
					FindLoopExits(loops_in_nest.at(i), loop_exits);

					// extract a sum of products expression out of the loop_exits of the current loop under study
					std::vector<std::vector<pierCondition>> f_supp;
					for(int j = 0; j < loop_exits.size(); j++) {
						// get the condition of each BB
					}
				} else {
					break;  // consumer is inside that loop and all the coming loops in the nest, so no need to continue looping
				}
			}


			/*if(!(BBMap->at(producer->BB))->loop->contains(consumer->BB)) {
				// the loop field of an enode represents the inner most loop that this enode object is inside
			
				// find the entire loop-nest that the producer is inside

				// check if there are other outer loops that the producer is inside but the consumer is outside
				loops_in_nest.clear();
				FindLoopsInNest(BBMap->at(producer->BB)->loop, loops_in_nest);
				std::vector<std::vector<pierCondition>> f_supp; // sum of products representing the conditions of suppressing the token
				
				// loop over the loops in the nest starting at (and including) the innermost loop that the producer is inside
				for(int i = 0; i < loops_in_nest.size(); i++) {
					// get the loop exit condition of each loop of these (note that this loop exit condition could be
						// a complicated boolean expression if the loop happens to have multiple exits)

					// identify the loop exits of this loop object (could be a multi-exit loop)
					loop_exits.clear();
					FindLoopExits(loops_in_nest.at(i), loop_exits);
					// the conditions of those loop exits should contribute to the condition of the single SUPPRESS that we will insert

					// TODO, for each loop we get a boolean expression (just a single condition if the loop has a single exit)
						// then we OR all the boolean expressions coming from the different loops in the loop nest
							// and finally apply SHannon then insert the single Branch

					// TODO, in applying SHannon, I will probably have to extend it to support more MUX levels!!!!
				}

			} */

		}
	}
}