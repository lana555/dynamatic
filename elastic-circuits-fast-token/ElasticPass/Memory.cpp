#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Memory.h"
#include "MemElemInfo/MemUtils.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

/**
 * @brief connectEnodeWithMCEnode
 * Performs the necessary connections between a memory instruction
 * enode and a memory controller/LSQ enode once it has been determined
 * that the two should connect.
 */
void connectEnodeWithMCEnode(ENode* enode, ENode* mcEnode, BBNode* bb) {
    enode->Mem = mcEnode;
    enode->CntrlSuccs->push_back(mcEnode);
    mcEnode->CntrlPreds->push_back(enode);
    enode->memPort = true;
    setBBIndex(enode, bb);
    setMemPortIndex(enode, mcEnode);
    setBBOffset(enode, mcEnode);
}

/**
 * @brief getOrCreateMCEnode
 * @param base: base address of the array associated with the MC
 * @returns pointer to MC Enode associate with the @p base
 */
ENode* CircuitGenerator::getOrCreateMCEnode(const Value* base,
                                            std::map<const Value*, ENode*>& baseToMCEnode) {
    auto iter     = baseToMCEnode.find(base);
    auto* mcEnode = iter->second;
    if (iter == baseToMCEnode.end()) {
        mcEnode             = new ENode(MC_, base->getName().str().c_str());
        baseToMCEnode[base] = mcEnode;
        // Add new MC to enode_dag
        enode_dag->push_back(mcEnode);
    }
    return mcEnode;
}

/**
 * @brief CircuitGenerator::addMCForEnode
 * For a given load or store instruction (which has not previously
 * been connected to an LSQ), associate it with an MC.
 * @param enode
 * @param baseToMCEnode
 */
void CircuitGenerator::addMCForEnode(ENode* enode, std::map<const Value*, ENode*>& baseToMCEnode) {
    Instruction* I = enode->Instr;
    auto* mcEnode  = getOrCreateMCEnode(findBase(I), baseToMCEnode);
    connectEnodeWithMCEnode(enode, mcEnode, findBB(enode));
    updateMCConstant(enode, mcEnode);
}

void CircuitGenerator::constructLSQNodes(std::map<const Value*, ENode*>& LSQnodes) {
    /* Create an LSQ enode for each of the LSQs reported
     * by MemElemInfo, and create a mapping between the unique
     * bases of the LSQs and the corresponding LSQ enodes */
    auto LSQlist = MEI.getLSQList();
    for (auto lsq : LSQlist) {
        auto* base     = lsq->base;
        auto* e        = new ENode(LSQ_, base->getName().str().c_str());
        LSQnodes[base] = e;
    }

    /* Append all the LSQ ENodes to the enode_dag */
    for (const auto& it : LSQnodes)
        enode_dag->push_back(it.second);
}

void CircuitGenerator::updateMCConstant(ENode* enode, ENode* mcEnode) {
    /* Each memory controller contains a notion of how many
     * store operations will be executed for a given BB, when
     * entering said BB. This constant is used to ensure that
     * all pending stores are completed, before the program is
     * allowed to terminate.
     * If present, locate this constant node and increment the
     * count, else create a new constant node.
     */

    if (enode->Instr->getOpcode() == Instruction::Store) {
        bool found = false;
        //for (auto& pred : *mcEnode->JustCntrlPreds) // Aya
		// Aya
		for (auto& pred : *mcEnode->CntrlOrderPreds)
            if (pred->BB == enode->BB && pred->type == Cst_) {
                pred->cstValue++;
                found = true;
             }
        if (!found) {
            ENode* cstNode    = new ENode(Cst_, enode->BB);
            cstNode->cstValue = 1;
            //cstNode->JustCntrlSuccs->push_back(mcEnode); // Aya
			// Aya
			cstNode->CntrlOrderSuccs->push_back(mcEnode);

            cstNode->id = cst_id++;
            //mcEnode->JustCntrlPreds->push_back(cstNode);  // Aya
			// Aya
			mcEnode->CntrlOrderPreds->push_back(cstNode);

            // Add new constant to enode_dag
            enode_dag->push_back(cstNode);

			// AYA: 16/11/2021:
			cstNode->is_const_feed_memory = true;


			// AYA: 15/11/2021: These constants need to be triggered when the flow enters the BB!
			
			// this constant needs to be triggered by start_ putting any bridges/ phis necessary, so we put it in our nice JustCntrl network 
			// search for the start_ node
			for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
				if(cnt_node->type == Start_) {
					cstNode->JustCntrlPreds->push_back(cnt_node);
					cnt_node->JustCntrlSuccs->push_back(cstNode);
					break; // found the start node, no need to continue looping
				}
			} 

        }
    }
}

/**
 * @brief CircuitGenerator::addMemoryInterfaces
 * This pass is responsible for emitting all connections between
 * memory accessing instructions and associated memory controllers.
 * If @p useLSQ, LSQs will be emitted for all instructions determined
 * to have possible data dependencies by the MemElemInfo pass.
 */
void CircuitGenerator::addMemoryInterfaces(const bool useLSQ) {
    std::map<const Value*, ENode*> LSQnodes;

    if (useLSQ) {
        constructLSQNodes(LSQnodes);
    }

    // for convenience, we store a mapping between the base arrays in a program,
    // and their associated memory interface ENode (MC or @todo LSQ)
    std::map<const Value*, ENode*> baseToMCEnode;

    /* For each memory instruction present in the enode_dag, connect it
     * to a previously created LSQ if MEI has determined so, else, connect
     * it to a regular MC.
     * Iterate using index, given that iterators are invalidated once a
     * container is modified (which happens during addMCForEnode).
     */
    for (unsigned int i = 0; i < enode_dag->size(); i++) {
        auto* enode = enode_dag->at(i);
        if (!enode->isLoadOrStore())
            continue;

        if (useLSQ && MEI.needsLSQ(enode->Instr)) {
            const LSQset& lsq = MEI.getInstLSQ(enode->Instr);
            connectEnodeWithMCEnode(enode, LSQnodes[lsq.base], findBB(enode));
            continue;
        }

        addMCForEnode(enode, baseToMCEnode);
    }

    // Check if LSQ and MC are targetting the same memory
    for (auto lsq : LSQnodes) {
        auto* base     = lsq.first;
        if (baseToMCEnode[base] != NULL) {
            auto *lsqNode = lsq.second;
            lsqNode->lsqToMC = true;
            auto *mcEnode = baseToMCEnode[base];
            mcEnode->lsqToMC = true;

            // LSQ will connect as a predecessor of MC
            // MC should keep track of stores arriving from LSQ
            for (auto& enode : *lsqNode ->CntrlPreds) 
               updateMCConstant(enode, mcEnode);
            
            lsqNode->CntrlSuccs->push_back(mcEnode);
            mcEnode->CntrlPreds->push_back(lsqNode);
            //setBBIndex(lsqNode, mcEnode->bbNode);
            setLSQMemPortIndex(lsqNode, mcEnode);
            setBBOffset(lsqNode, mcEnode);
        }
    }	

	// AYa: added this function to connect the LSQ to the CntrlOrder network
	addControlForLSQ();
 
}

/*
Plan for the flow of the paper: 
1) I had an initial idea that each BB should produce its own control signal denoting when it will execute and that this happens normally
    irrespective of any type of memory dependency and it happens by running FPL between the BB and START
2) Then, we have the problem of synchronizing between the dependent memory operations to know when to send the control tokens to the LSQ. This should 
    be done by synchronizing the 2 control signals of the 2 BBs involved in the memory dependency. For this, we need the Bx component because its
    specifications are important!  BUT, the issue is how to connect the Bx instances provided that the input of each Bx is already connected to START!
    Please note that in the paper I should only explain the specifications of the Bx component not its implementation using Fork and Buffer!!!

3) For our presentation in the paper, does it make sense to get rid of the Bx completely and say we have just a single source for the control token
    but it is not enough to run FPL on START and the BB, we also need to get a signal from the dependent BB that is has completed execution!!
         (i.e., the idea of HOLD that I had in the beginning!!)

*/

void CircuitGenerator::identifyMemDeps(ENode* lsq_enode, std::vector<prod_cons_mem_dep>& all_mem_deps) {
    for(int i = 0; i < lsq_enode->CntrlPreds->size()-1; i++) {
        // The hazards are: 1) WAW so 2 ST in different BBs, 2) RAW and WAR are between loads and stores
            // I will identify the forward dependencies using the BB Ids (i.e., the smaller BB Id should be the producer)
            // I will identify the backward dependencies by checking if they are enclosed in a loop (i.e., the larger BB Id should be the producer)
        lsq_pred_type pred_type; 
        if(isa<LoadInst>(lsq_enode->CntrlPreds->at(i)->Instr))
            pred_type = load;
        else {
            if(isa<StoreInst>(lsq_enode->CntrlPreds->at(i)->Instr))
                pred_type = store;
            else
                pred_type = none;
        }

        if(pred_type == none)
            continue; // it is not a memory operation so no need to check for any dependencies

        for(int j = i+1; j < lsq_enode->CntrlPreds->size(); j++) {
            if(lsq_enode->CntrlPreds->at(i)->BB == lsq_enode->CntrlPreds->at(j)->BB)
                continue; // both are in the same BB, so do not worry about managing them as a prod, cons
            if(pred_type == load && isa<LoadInst>(lsq_enode->CntrlPreds->at(j)->Instr))
                continue;  // both are load instructions so no memory dependency
            if(!( isa<LoadInst>(lsq_enode->CntrlPreds->at(j)->Instr) || isa<StoreInst>(lsq_enode->CntrlPreds->at(j)->Instr) ))
                continue; // this pred is not a memory operation so no memory dependency

            // if we reach here, then the two preds are either both store or one is store and one is load
            // NOTE THAT here I'm thinking of forward dependencies ONLY... to identify the prod and cons, I compare the BB Ids
            prod_cons_mem_dep one_dep;
            if(BBMap->at(lsq_enode->CntrlPreds->at(j)->BB)->Idx > BBMap->at(lsq_enode->CntrlPreds->at(i)->BB)->Idx) {
                one_dep.prod_idx = i;
                one_dep.cons_idx = j;
                one_dep.is_backward = false;
            } else {
                assert(BBMap->at(lsq_enode->CntrlPreds->at(j)->BB)->Idx < BBMap->at(lsq_enode->CntrlPreds->at(i)->BB)->Idx);
                one_dep.prod_idx = j;
                one_dep.cons_idx = i;
                one_dep.is_backward = false;
            }
            all_mem_deps.push_back(one_dep);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THINK OF WHEN DO WE NEED AN ELASTIC JOIN AND THINK OF A HACK TO DO TO PREVENT NEEDING IT UNTIL WE ARE ABLE TO ADD THIS COMPONENT!!!
// AYA: 05/08/2022: Will face a problem later in how to do the Elastic Join??? BUT, I can go with a hack that I try to utilize the existing Bx component???

// AYA: 04/08/2022: I should here connect the LSQ to out1 of Bx,
        // But, first I should have something to check each pair of dependent memory operations connecting to the LSQ!
            // We can even simply loop over the preds of the LSQ and create the dep pairs ourselves by identifying the types of operations

 // loop over its predecessors and compare each predecessor to all other predecessors to identify the producer, consumer relationships 
                // depending on the different hazards that could arise
        // In doing so, we do not count WAW hazards arising by a single STORE operation and we do not count operations within a single BB!!
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CircuitGenerator::addControlForLSQ() {
	// Aya added this function to provide the control tokens to the LSQ
    // Everything about the control interfaces of the LSQ or the MC should be put in the CntrlOrder graph to be compatible with the printDot!

    std::vector<prod_cons_mem_dep> all_mem_deps; // one element of this should hold the index of the producer and consumer operations
            // (based on the type of memory dependency) in the CntrlPreds of the LSQ
    for(auto & enode : *enode_dag) {
        all_mem_deps.clear();
        if(enode->type == LSQ_) {
            identifyMemDeps(enode, all_mem_deps);  // STILL TODO: counting for the backward dependencies!!!

             // if the size of all_mem_deps is 0, all memory operations are in a single BB, so insert a single Bx and trigger it from START
            if(all_mem_deps.size() == 0){
                // sanity check: make sure that all preds of type memory operation are in the same BB!!
                int old_bb_idx = -1;
                int bb_idx = -1;

                bool sanity_check_flag = false;
                
                // the following loop does a sanity check to assure that all the LSQ's pred memory operations are in the same BB and extracts the ID of this BB
                for(int k = 0; k < enode->CntrlPreds->size(); k++) {
                    if(isa<LoadInst>(enode->CntrlPreds->at(k)->Instr) || isa<StoreInst>(enode->CntrlPreds->at(k)->Instr)) {
                        bb_idx = BBMap->at(enode->CntrlPreds->at(k)->BB)->Idx;
                        // compare the bb_idx to the one of the previous memory operation
                        if(old_bb_idx != -1) {
                            if(bb_idx != old_bb_idx) {
                                sanity_check_flag = true;
                                break;
                            }
                        }
                        old_bb_idx = bb_idx;
                    }
                }
                assert(sanity_check_flag == false);  // this flag is just for sanity check.. there is something wrong if the flag is true!!
                assert(bb_idx != -1);
                ///////////////////////////////// End of sanity check 

                // Inserting the Bx component (composed of Fork and Buffer) in this case doesn't work (deadlocks), so I will just connect START directly
                    // to the LSQ in this particular case
                
                // add a new Bx instance, then connect it to START_ and the next function calls from the main should apply FPL's algorithm!
                // 1st) add a fork 
                ENode* fork = new ENode(Fork_c, "forkC", bbnode_dag->at(bb_idx)->BB); 
                fork->id = fork_id++;
                enode_dag->push_back(fork);
                // search for the Start_ node
                for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
                    if(cnt_node->type == Start_) {
                        fork->CntrlOrderPreds->push_back(cnt_node);
                        cnt_node->CntrlOrderSuccs->push_back(fork);
                        break; // found the start node, no need to continue looping
                    }
                } 
                // the first output of the fork should be the LSQ
                fork->CntrlOrderSuccs->push_back(enode);
                enode->CntrlOrderPreds->push_back(fork);

                // IN that very particular case, I do not need the buffer node
                // 2nd) add a Buffer and connect its output to sink..
                ENode* buff_node = new ENode(Buffera_, "buffA");
                buff_node->id = CircuitGenerator::buff_id++;
                enode_dag->push_back(buff_node);

                fork->CntrlOrderSuccs->push_back(buff_node);
                buff_node->CntrlOrderPreds->push_back(fork);

                ENode* sink_node = new ENode(Sink_, "sink"); 
                sink_node->id    = sink_id++;
                enode_dag->push_back(sink_node);
                buff_node->CntrlOrderSuccs->push_back(sink_node);
                sink_node->CntrlOrderPreds->push_back(buff_node);
                
                //sink_node->CntrlOrderPreds->push_back(fork);
                //fork->CntrlOrderSuccs->push_back(sink_node);
            
               

            } else {
                // loop over the memory dependencies and connect the different Bx instance 
                for(int i = 0; i < all_mem_deps.size(); i++) {
                    // add new Bx instances in the BB of the prodcuer and the consumer and connect them to eachother, then connect START to the Bx of the producer
                    // 1st consider the producer BB
                    // add a new Bx instance, then connect it to START_ and the next function calls from the main should apply FPL's algorithm!
                        // 1st) add a fork 
                    ENode* fork_prod = new ENode(Fork_c, "forkC",  enode->CntrlPreds->at(all_mem_deps.at(i).prod_idx)->BB); 
                    fork_prod->id = fork_id++;
                    enode_dag->push_back(fork_prod);
                    // search for the Start_ node
                    for(auto& cnt_node : *bbnode_dag->front()->Cntrl_nodes) {
                        if(cnt_node->type == Start_) {
                            fork_prod->CntrlOrderPreds->push_back(cnt_node);
                            cnt_node->CntrlOrderSuccs->push_back(fork_prod);
                            break; // found the start node, no need to continue looping
                        }
                    } 
                        // the first output of the fork should be the LSQ
        // TODO: URGENT! I THINK THE ORDER OF THE CONTROL TOKENS FROM DIFFERENT BBS SHOULD CORRESPOND TO THE ORDER OF THE MEMORY OPERATIONS?????
                    // CHECK AND ENSURE THIS IF NEEDED!!!!!
                    fork_prod->CntrlOrderSuccs->push_back(enode);
                    enode->CntrlOrderPreds->push_back(fork_prod);

                        // 2nd) add a Buffer and connect its output to the Bx of the consumer
                    ENode* buff_node_prod = new ENode(Buffera_, "buffA");
                    buff_node_prod->id = CircuitGenerator::buff_id++;
                    enode_dag->push_back(buff_node_prod);

                    fork_prod->CntrlOrderSuccs->push_back(buff_node_prod);
                    buff_node_prod->CntrlOrderPreds->push_back(fork_prod);

            
                    // 2nd consider the consumer BB
                     // 1st) add a fork 
                    ENode* fork_cons = new ENode(Fork_c, "forkC",  enode->CntrlPreds->at(all_mem_deps.at(i).cons_idx)->BB); 
                    fork_cons->id = fork_id++;
                    enode_dag->push_back(fork_cons);
                   
                        // the first output of the fork should be the LSQ
        // TODO: URGENT! I THINK THE ORDER OF THE CONTROL TOKENS FROM DIFFERENT BBS SHOULD CORRESPOND TO THE ORDER OF THE MEMORY OPERATIONS?????
                    // CHECK AND ENSURE THIS IF NEEDED!!!!!
                    fork_cons->CntrlOrderSuccs->push_back(enode);
                    enode->CntrlOrderPreds->push_back(fork_cons);

                        // 2nd) add a Buffer and connect its output to a sink
                    ENode* buff_node_cons = new ENode(Buffera_, "buffA");
                    buff_node_cons->id = CircuitGenerator::buff_id++;
                    enode_dag->push_back(buff_node_cons);

                    fork_cons->CntrlOrderSuccs->push_back(buff_node_cons);
                    buff_node_cons->CntrlOrderPreds->push_back(fork_cons);

                    ENode* sink_node = new ENode(Sink_, "sink"); 
                    sink_node->id    = sink_id++;
                    enode_dag->push_back(sink_node);
                    buff_node_cons->CntrlOrderSuccs->push_back(sink_node);
                    sink_node->CntrlOrderPreds->push_back(buff_node_cons);


                    // 3rd connect the producer to the consumer 
                    buff_node_prod->CntrlOrderSuccs->push_back(fork_cons);
                    fork_cons->CntrlOrderPreds->push_back(buff_node_prod);
                }

                // TODO: for now I assume that a memory operation is involved in only a single type of dependency so I blindly add new Bx instances
                    // and I don't consider the need for an Elastic Fork!!

            }
        }
    }

    // TODO: check the rest of the functions here, do they rely on the old ordered control network or will they work fine?????
   
        
    // TODO: My Bx is a Fork with a Buffer on one output: Make sure that:
    // 1) it doesn't affect the Forks algorithm, 2) Do not add an extra Buffer in the smart buffers algorithm!
         
    ////////////////////////////////////////// Old code was used to interface the LSQ with the strictly ordered control network
    // The following code connected the LSQ to the strictly ordered control network (CntrlOrder) network
	// loop over all the enodes searching for a node of type LSQ
	/*for(auto & enode : *enode_dag) {
		if(enode->type == LSQ_) {
			// loop over its CntrlPreds
			for(auto & pred : *enode->CntrlPreds) {
				// search for the redun Phi_c in the BB of each preds
				for(auto & enode_2 : *enode_dag) {
					if(enode_2->BB == pred->BB && ((enode_2->type == Phi_c && enode_2->is_redunCntrlNet) || enode_2->type == Start_)) {
						// this enode_2 should be in the CntrlOrderPreds of the LSQ enode
						// first make sure that the connection is not there from before
						if(!contains(enode->CntrlOrderPreds, enode_2)) {	
							enode->CntrlOrderPreds->push_back(enode_2);
							enode_2->CntrlOrderSuccs->push_back(enode);
						} 
					}
				} 
			} 

		}
	}*/ 

}

// Aya: 21/08/2022: temporary for debugging!!
void CircuitGenerator::FixSingleBBLSQ_special() {
    // In the very end (after completing the FPL algorithm) I call this function to remove the fork (of the Bx) feeding the LSQ
        // in the special case of having all the memory deps inside a single BB!
    // To identify the special case, I call the identifyMemDeps function again

      std::vector<prod_cons_mem_dep> all_mem_deps; // one element of this should hold the index of the producer and consumer operations
            // (based on the type of memory dependency) in the CntrlPreds of the LSQ
    for(auto & enode : *enode_dag) {
        all_mem_deps.clear();
        if(enode->type == LSQ_) {
            identifyMemDeps(enode, all_mem_deps);  

             // if the size of all_mem_deps is 0, all memory operations are in a single BB, so insert a single Bx and trigger it from START
            if(all_mem_deps.size() == 0){
                // sanity check: assert if there is no predecessor to the LSQ in the CntrlOrderPreds
                assert(enode->CntrlOrderPreds->size() == 1);  
                assert(enode->CntrlOrderPreds->at(0)->type == Fork_c);
                assert(enode->CntrlOrderPreds->at(0)->CntrlOrderSuccs->at(1)->type == Sink_);

                // delete both the Fork and the sink it feeds
                auto pos_fork = std::find(enode_dag->begin(), enode_dag->end(), enode->CntrlOrderPreds->at(0));
                assert(pos_fork != enode_dag->end());
                enode_dag->erase(pos_fork);

                auto pos_sink = std::find(enode_dag->begin(), enode_dag->end(), enode->CntrlOrderPreds->at(0)->CntrlOrderSuccs->at(1));
                assert(pos_sink != enode_dag->end());
                enode_dag->erase(pos_sink);

                 // search for the Fork in the successors of enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)
                auto pos = std::find(enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)->CntrlOrderSuccs->begin(), enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)->CntrlOrderSuccs->end(), enode->CntrlOrderPreds->at(0));
                assert(pos != enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)->CntrlOrderSuccs->end());
                int pos_idx = pos - enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)->CntrlOrderSuccs->begin();

                // put the LSQ in the successors of the predecessor of the Fork in place of the Fork
                enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0)->CntrlOrderSuccs->at(pos_idx) = enode;
                // put the predecessor of the Fork in the predecessor of the LSQ in place of the Fork
                enode->CntrlOrderPreds->at(0) = enode->CntrlOrderPreds->at(0)->CntrlOrderPreds->at(0);

            }
        }
    }



}

// set index of BasicBlock in load/store connected to MC or LSQ
void setBBIndex(ENode* enode, BBNode* bbnode) { enode->bbId = bbnode->Idx + 1; }

// load/store port index for LSQ-MC connection
void setLSQMemPortIndex(ENode* enode, ENode* memnode) {

    assert (enode->type == LSQ_);
    int loads = 0;
    int stores = 0;
    for (auto& node : *memnode->CntrlPreds) {
	    if (enode == node)
	        break;
	    if (node->type == LSQ_){
	        loads++;
	        stores++;
	    }
	    else {
	        auto* I = node->Instr;
	        if (isa<LoadInst>(I))
	        	loads++;
	        else {
	        	assert (isa<StoreInst>(I));
	        	stores++;
	        }
	    }
	}
	enode->lsqMCLoadId = loads; 
    enode->lsqMCStoreId = stores;
}

// load/store port index for LSQ or MC
void setMemPortIndex(ENode* enode, ENode* memnode) {
    int offset = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (enode == node)
            break;
        if (!compareMemInst(enode, node))
            offset++;
    }
    enode->memPortId = offset;

    if (enode->type == LSQ_){
    	int loads = 0;
    	int stores = 0;
    	for (auto& node : *memnode->CntrlPreds) {
	        if (enode == node)
	            break;
	        if (node->type == LSQ_){
	            loads++;
	            stores++;
	        }
	        else {
	        	auto* I = node->Instr;
	        	if (isa<LoadInst>(I))
	        		loads++;
	        	else {
	        		assert (isa<StoreInst>(I));
	        		stores++;
	        	}
	        }
	    }
	    enode->lsqMCLoadId = loads; 
        enode->lsqMCStoreId = stores;
	}
}

// ld/st offset within bb (for LSQ or MC grop allocator)
void setBBOffset(ENode* enode, ENode* memnode) {
    int offset = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (enode == node)
            break;
        if (compareMemInst(enode, node) && enode->BB == node->BB)
            offset++;
    }
    enode->bbOffset = offset;
}

int getBBIndex(ENode* enode) { return enode->bbId; }

int getMemPortIndex(ENode* enode) { return enode->memPortId; }

int getBBOffset(ENode* enode) { return enode->bbOffset; }

// check if two mem instructions are different type (for offset calculation)
bool compareMemInst(ENode* enode1, ENode* enode2) {
	// When MC connected to LSQ
	// LSQ has both load and store port connected to MC 
	// Need to increase offset of both load and store
    if (enode1->type == LSQ_ || enode2->type == LSQ_)
        return false;

    auto* I1 = enode1->Instr;
    auto* I2 = enode2->Instr;

    return ((isa<LoadInst>(I1) && isa<StoreInst>(I2)) || (isa<LoadInst>(I2) && isa<StoreInst>(I1)));
}

// total count of loads connected to lsq
int getMemLoadCount(ENode* memnode) {
    int ld_count = 0;
    for (auto& enode : *memnode->CntrlPreds) {
        if (enode->type != Cst_ && enode->type != LSQ_) {
            auto* I = enode->Instr;
            if (isa<LoadInst>(I))
                ld_count++;
        }
    }
    return ld_count;
}

// total count of stores connected to lsd
int getMemStoreCount(ENode* memnode) {
    int st_count = 0;
    for (auto& enode : *memnode->CntrlPreds) {
        if (enode->type != Cst_ && enode->type != LSQ_) {
            auto* I = enode->Instr;
            if (isa<StoreInst>(I))
                st_count++;
        }
    }
    return st_count;
}

// total count of bbs connected to lsq (corresponds to control fork connection
// count)
int getMemBBCount(ENode* memnode) { 
	// return memnode->JustCntrlPreds->size();  /// Aya
	return memnode->CntrlOrderPreds->size();
}

// total input count to MC/LSQ
int getMemInputCount(ENode* memnode) {
    return getMemBBCount(memnode) + 2 * getMemStoreCount(memnode) + getMemLoadCount(memnode);
}

// total output count to MC/LSQ
int getMemOutputCount(ENode* memnode) { return getMemLoadCount(memnode); }

// adress index for dot (fork connections go first)
int getDotAdrIndex(ENode* enode, ENode* memnode) {
    int count = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (node == enode)
            break;
        auto* I = node->Instr;
        if (isa<StoreInst>(I))
            count += 2;
        else
            count++;
    }

    return getMemBBCount(memnode) + count + 1;
}

// data index for dot
int getDotDataIndex(ENode* enode, ENode* memnode) {
    auto* I = enode->Instr;
    if (isa<StoreInst>(I))
        return getDotAdrIndex(enode, memnode) + 1;
    else {
        int count = 0;

        for (auto& node : *memnode->CntrlPreds) {
            if (node == enode)
                break;
            auto* I = node->Instr;
            if (isa<LoadInst>(I))
                count++;
        }

        return count + 1;
    }
}

int getBBLdCount(ENode* enode, ENode* memnode) {
    int ld_count = 0;

    for (auto& node : *memnode->CntrlPreds) {

        if (node->type != Cst_) {
            auto* I = node->Instr;
            if (isa<LoadInst>(I) && node->BB == enode->BB)
                ld_count++;
        }
    }

    return ld_count;
}

int getBBStCount(ENode* enode, ENode* memnode) {
    int st_count = 0;

    for (auto& node : *memnode->CntrlPreds) {

        if (node->type != Cst_) {
            auto* I = node->Instr;
            if (isa<StoreInst>(I) && node->BB == enode->BB)
                st_count++;
        }
    }

    return st_count;
}

int getLSQDepth(ENode* memnode) {
    // Temporary: determine LSQ depth based on the BB with largest ld/st count
    // Does not guarantee maximal throughput!

    int d = 0;

    // for (auto& enode : *memnode->JustCntrlPreds) {  // Aya
	// Aya
	for (auto& enode : *memnode->CntrlOrderPreds) {
        BBNode* bb = enode->bbNode;

        int ld_count = getBBLdCount(enode, memnode);
        int st_count = getBBStCount(enode, memnode);

        d = (d < ld_count) ? ld_count : d;
        d = (d < st_count) ? st_count : d;
    }

    int power = 16;

    // round up to power of two
    while (power < d)
        power *= 2;

    // increase size for performance (not optimal!)
    // power *= 2;

    return power;
}
