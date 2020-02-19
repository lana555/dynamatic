#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

/*
 * Adding phis
 * Data flow must strictly follow control flow 
 * (data must enter BB from its immediate) predecessor BBs
 * Preform liveness analysis to determine in which BBs the variables are live 
 * Place phis for all live ins and connect them to source ops or to phis in pred blocks
 */


void CircuitGenerator::setPhiLiveOuts() {
    // Mark nodes which send value to successor BB as liveout nodes

    for (auto& enode : *enode_dag) {
        if (enode->type == Phi_n || enode->type == Phi_) {
            BBNode* phiBB    = findBB(enode);
            enode->isMux     = false;
            enode->isCntrlMg = false;

            for (auto& succNode : *enode->CntrlSuccs)
                // phi has succ in different bb or loops back to its own bb to a phi
                if (findBB(succNode) != phiBB || succNode->type == Phi_n ||
                    succNode->type == Phi_) {
                    if (!contains(phiBB->Live_out, enode))
                        phiBB->Live_out->push_back(enode);
                    enode->is_live_out = true;
                }
        }
    }
}

typedef std::map<BasicBlock*, std::vector<ENode *>> bbOps;

bbOps getBBUses(std::vector<BBNode*>* bbnode_dag, std::vector<ENode*>* enode_dag) {
    // Get instructions used in bb but originating from another bb
    bbOps uses;

    for (auto& enode : *enode_dag) {

        // Iterate through predecessors of enode; if they are from other bbs, add to uses
        for (auto& pred : *enode->CntrlPreds) {

            BasicBlock *bb = enode->BB;

            // Ignore llvm phis (special case--llvm phi uses should not be included in liveness analysis)
            if (enode->BB != pred->BB && enode->type != Phi_)
                // Add to uses if not already added to use list
                if (std::find(uses[bb].begin(), uses[bb].end(), pred) == uses[bb].end()) 
                  uses[bb].push_back(pred);
                
        }
    }

    return uses;
}


bbOps getBBDefs(std::vector<BBNode*>* bbnode_dag, std::vector<ENode*>* enode_dag) {
    // Get all enodes originating from bb
    bbOps defs;

    for (auto& enode : *enode_dag) {
        BasicBlock* bb = enode->BB;
        defs[bb].push_back(enode);
    }

    return defs;
}

std::vector<ENode*> opUnion (std::vector<ENode*> v1, std::vector<ENode*> v2) {
    // Union of two vectors (assumes that enodes are not repeated in v1)
    for (int i = 0; i < v2.size(); i++) {
        ENode* enode = v2[i];
        if (std::find (v1.begin(), v1.end(), enode) == v1.end())
            v1.push_back(enode);
    }
    return v1;
}

std::vector<ENode*> opDiff (std::vector<ENode*> v1, std::vector<ENode*> v2) {
    // Diff of two vectors (v1-v2)
    std::vector<ENode*> diff;

    for (int i=0; i< v1.size(); i++) {
        ENode* enode = v1[i];
        if (std::find (v2.begin(), v2.end(), enode) == v2.end())
            diff.push_back(enode);
    }
    return diff;
}

bbOps livenesAnalysis (std::vector<BBNode*>* bbnode_dag, std::vector<ENode*>* enode_dag) {
    // Liveness algorithm from https://suif.stanford.edu/~courses/cs243/lectures/l2.pdf, slide 19

    bbOps bbLiveins;

    bbOps bbUses = getBBUses(bbnode_dag, enode_dag);

    bbOps bbDefs = getBBDefs(bbnode_dag, enode_dag);

    bool modified = true;

    // Iterate while liveness sets are modified
    while (modified) {
        modified = false;

        for (auto& bbnode : *bbnode_dag) {
            // Liveouts of BBs: union of liveins of all successors BBs
            std::vector<ENode*> liveOuts;
            for (auto& bbsucc : *(bbnode->CntrlSuccs)) 
                liveOuts = opUnion(liveOuts, bbLiveins[bbsucc->BB]);

            // diff: liveouts of BB which are not defined in BB
            std::vector<ENode*> diff = opDiff(liveOuts, bbDefs[bbnode->BB]);

            // tmp: liveins (bb uses U diff)
            std::vector<ENode*> tmp = opUnion(bbUses[bbnode->BB], diff);

            // If tmp liveins contain liveins which are not in current list
            // Update list and mark iteration as modified
            if (tmp.size() > bbLiveins[bbnode->BB].size()) {
                bbLiveins[bbnode->BB]=tmp;
                modified = true;
            }
        }
    }
    return bbLiveins;
}

void setPhiPredecessors(std::vector<BBNode*>* bbnode_dag, std::vector<ENode*>* enode_dag) {
	// Set temporary dummy nodes as predecessors of original llvm phi, placed in the incoming (direct predecessor) blocks of phi
	// Otherwise, liveness analysis will not work for llvm phis
	// One phi predecessor would be incorrectly propagated as a livein from all phi predecessor blocks 
    // (it should be livein only from one incoming block)
	// Just excluding phis from getBBUses without adding dummies would not add all merges for phi predecessors 
	// (if phi predecessor is not in direct predecessor block of phi block)


	int dummy_id = 0;
	for (auto& enode : *enode_dag) {
		if (enode->type == Phi_) {

			std::vector<ENode*> nodesToErase;
			std::vector<ENode*> nodesToAdd;

			for (int i = 0; i< enode->CntrlPreds->size(); i++) {

				// Place dummy nodes in **direct** predecessor blocks of phi (incoming blocks)
				BasicBlock *predBB = dyn_cast<PHINode>(enode->Instr)->getIncomingBlock(i);

                // The predecessors might be ordered differently than the incoming blocks
                // Identify the appropriate one by value (if inst or arg) or by BB (if cst or undef)
                ENode *currPred;
                for (auto& pred : *(enode->CntrlPreds)) {
                  if (pred->Instr!=NULL) {
                    if (dyn_cast<PHINode>(enode->Instr)->getIncomingValue(i) == pred->Instr)
                       currPred = pred;
                  }
                   else if (pred->A!=NULL) {
                    if (dyn_cast<PHINode>(enode->Instr)->getIncomingValue(i) == pred->A)
                       currPred = pred;
                    }
                    else {
                        // Otherwise, it is a constant or an undef which is placed in IncomingBlock (see AddComp.cpp)
                        if (pred->BB == predBB)
                           currPred = pred;
                   }


                }
                assert (currPred != NULL);

				ENode* newnode = new ENode(dummy_, "dummy", 
                                             predBB); 
				
				newnode->id = dummy_id++;

				// Reconnect nodes
				removeNode(currPred->CntrlSuccs, enode);
                newnode->CntrlPreds->push_back(currPred);
                newnode->CntrlSuccs->push_back(enode);
                currPred->CntrlSuccs->push_back(newnode);
                enode_dag->push_back(newnode);
                nodesToErase.push_back(currPred);
                nodesToAdd.push_back(newnode);

			}
			// Remove old nodes from phi predecessor list
			for (auto& node : nodesToErase) 
				removeNode(enode->CntrlPreds, node);

			// Add dummy nodes to phi predecessor list
			for (auto& node : nodesToAdd) 
				enode->CntrlPreds->push_back(node);
		}
	}
}

void CircuitGenerator::addPhi() {

	// Set temporary (dummy) nodes as phi predecessors
	setPhiPredecessors(bbnode_dag, enode_dag);

	// Calculate liveness
    bbOps bbLiveins = livenesAnalysis(bbnode_dag, enode_dag);

    std::vector<ENode*> phiNodes;

    // For liveins of each block, add phis to block
    for (auto& bb : bbLiveins) {
        std::vector<ENode*> liveOps = bb.second;
        for (auto& enode : liveOps) {
            ENode* phi_node;
            if (enode->Instr != NULL)
               phi_node = new ENode(Phi_n, "phi", enode->Instr, bb.first);
            else
                phi_node = new ENode(Phi_n, "phi", enode->A, bb.first);
               
            phi_node->id = phi_id++;
            enode_dag->push_back(phi_node);
            phiNodes.push_back(phi_node);
            // Put original node as phi predecessor (temporary)
            phi_node->CntrlPreds->push_back(enode);
            enode->CntrlSuccs->push_back(phi_node);
        }
    }

    // Reconnect all operations within block through phi of that block
    for (auto& phi : phiNodes) {

    	// Get original (livein) operation
        ENode* orig = phi->CntrlPreds->front();

        for (auto& enode : *enode_dag)
            // Connect only non-phi nodes in BBs other than orig BB
            if (enode->BB == phi->BB && enode->BB != orig->BB && enode!=phi) 
                while (contains (enode->CntrlPreds, orig)) {
                    removeNode(orig->CntrlSuccs, enode);
                    removeNode(enode->CntrlPreds, orig);
                    enode->CntrlPreds->push_back(phi);
                    phi->CntrlSuccs->push_back(enode);
                }
    }

    // Reconnect phis to phis from direct predecessor blocks
    for (auto& phi : phiNodes) {
        BBNode* phiBB    = findBB(phi);
        for (auto& pred : *(phiBB->CntrlPreds)) {
        	ENode* orig = phi->CntrlPreds->front();
        	// Go through other phis and connect unless its the orig bb
            for (auto& pred_phi : phiNodes) {
                if (pred_phi->BB == pred->BB && pred->BB != orig->BB) {
                	if (phi->CntrlPreds->front() == pred_phi->CntrlPreds->front()){
                        pred_phi->CntrlSuccs->push_back(phi);
                        phi->CntrlPreds->push_back(pred_phi);
                	}
                }
            }
        }
    }

    // Remove all connections that go across BBs but not to phis of direct successor BBs
    for (auto& enode : *enode_dag) {
   		std::vector<ENode*> nodesToErase;
   		BBNode* currBB    = findBB(enode);

  		for (auto& succ : *(enode->CntrlSuccs)) {

  			bool found = false;

      		for (auto& succBB: *(currBB->CntrlSuccs)) {
       			if (succBB->BB == succ->BB)
       				found = true;
       		}
       		// Successor is not phi
      		if ((succ->BB != enode->BB) && (! (succ->type==Phi_ || succ->type == Phi_n))) {
       			nodesToErase.push_back(succ);
       		}
       		// Successor is not in direct successor BB
			if (!found && ((succ->type==Phi_ || succ->type == Phi_n))) {
				nodesToErase.push_back(succ);
			}
		}

	    for (auto& succ : nodesToErase) { 
			removeNode(enode->CntrlSuccs, succ);
			removeNode(succ->CntrlPreds, enode);
		}
    }
   
    // Erase dummy nodes, no longer needed
    std::vector<ENode*> nodesToErase;

    for (auto& enode : *enode_dag) {
    	
    	if (enode->type == dummy_) {
    		ENode * predNode = enode->CntrlPreds->front();
    		ENode * succNode = enode->CntrlSuccs->front();
    		removeNode(predNode->CntrlSuccs, enode);
			removeNode(succNode->CntrlPreds, enode);
    		predNode->CntrlSuccs->push_back(succNode);
    		succNode->CntrlPreds->push_back(predNode);
    		nodesToErase.push_back(enode);
    	}
    }

    for (auto& enode : nodesToErase) {
    	delete enode;
        enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode),
                         enode_dag->end());
    }

    // Set liveouts (neede for adding branches in next step)
    setPhiLiveOuts();

}


