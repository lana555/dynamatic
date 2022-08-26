#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

#define BRANCH_SUCC_COUNT 2
#define STORE_SUCC_COUNT 1

// Initializing static class variables
int CircuitGenerator::ssa        = 1;
int CircuitGenerator::fork_id    = 0;
int CircuitGenerator::phi_id     = 0;
int CircuitGenerator::buff_id    = 0;
int CircuitGenerator::branch_id  = 0;
int CircuitGenerator::store_id   = 0;
int CircuitGenerator::memcont_id = 0;
int CircuitGenerator::start_id   = 0;
int CircuitGenerator::fifo_id    = 0;
int CircuitGenerator::idt        = 0;
int CircuitGenerator::ret_id     = 0;
int CircuitGenerator::call_id    = 0;
int CircuitGenerator::switch_id  = 0;
int CircuitGenerator::select_id  = 0;
int CircuitGenerator::cst_id     = 0;
int CircuitGenerator::op_id      = 0;
int CircuitGenerator::sink_id    = 0;
int CircuitGenerator::source_id  = 0;

int CircuitGenerator::inj_id  = 0;// AYA: 25/02/2022: the new component responsible for token regeneration

// AYA: 31/10/2021: The following function is a wrapper that calls the above functions of adding branches and adding phis but with a special condition, only to check if the extra constants added for serving as a condition for unconditional branches are triggered correctly!!!
// WE call each of the functions with the flag true because we are in the control network for triggering constants!!
/*void CircuitGenerator::triggerBrConstsCheck(Function& F, LoopInfo& LI) {
	checkLoops(F, LI, true);
	//removeExtraPhisWrapper(true);

	ConnectProds_Cons(true, true);
	removeExtraBranchesWrapper(true);

}*/


// AYA: 01/10/2021: Created the following function to make sure that the predecessor of a Phi_ (llvm Phi) is compatible with the LLVM sequential order of the CFG!
void CircuitGenerator::Fix_LLVM_PhiPreds() {
	for(auto& enode : *enode_dag) {
		if(enode->type == Phi_) {
			// loop over its predecessors
			for(int i = 0; i < enode->CntrlPreds->size(); i++) {
				BasicBlock *predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);
			
				// searching for the predecessor that is compatible with the predBB we have in hand!				
			
				ENode *currPred;
				int index_to_replace = -1;
				for(int j = 0; j < enode->CntrlPreds->size(); j++) {
					ENode* pred = enode->CntrlPreds->at(j);
					if(pred->Instr != NULL) {
						if(dyn_cast<PHINode>(enode->Instr)->getIncomingValue(i) == pred->Instr) {
							currPred = pred;
							index_to_replace = j;

						}
					} else {
						if(pred->A != NULL) {
							if(dyn_cast<PHINode>(enode->Instr)->getIncomingValue(i) == pred->A) {
								currPred = pred;
								index_to_replace = j;
							}
						} else {
							if(pred->BB == predBB) {
								currPred = pred;
								index_to_replace = j;
							}
						}
					} 

				}
				
				// at this point, currPred holds the correct predecessor that should be at index i of the CntrlPreds
				assert(currPred != nullptr && index_to_replace != -1);
				// swap the predecessors at i and index_to_replace

				ENode *temp = enode->CntrlPreds->at(i);
				enode->CntrlPreds->at(i) = currPred;
				enode->CntrlPreds->at(index_to_replace) = temp;
			}


		} 


	} 


}

// AYA: 09/11/2021: This function fills the freq field to store the execution frequency of each BB
void CircuitGenerator::setFreqs(const std::string& function_name) {
    // frequency information should be in a file called "<function_name>_freq.txt"
    std::string file_name = function_name + "_freq.txt";
    std::ifstream file(file_name.c_str());
    if (!file.is_open()) {
        std::cout << "couldn't open file " << function_name + "_freq.txt" << std::endl;
        return;
    }

    // Reading the frequencies from the file
    std::string line;
    std::map<std::pair<std::string, std::string>, int> frequencies;
    while (getline(file, line)) {
        std::stringstream iss(line);
        std::string src_bb, dst_bb;
        int freq;
        iss >> src_bb >> dst_bb >> freq;

        assert(frequencies.find({src_bb, dst_bb}) == frequencies.end());
        assert(freq >= 0);

        frequencies[{src_bb, dst_bb}] = freq;
    }

    // setting the frequencies in the BBNodes
    for (auto& bnd : *bbnode_dag) {
        std::string src_bb = bnd->BB->getName();
        for (auto& bnd_succ : *(bnd->CntrlSuccs)) {
            std::string dst_bb = bnd_succ->BB->getName();

            assert(frequencies.find({src_bb, dst_bb}) != frequencies.end());

            bnd->set_succ_freq(dst_bb, frequencies[{src_bb, dst_bb}]);
        }
    }
}


// temporary approach for adding buffers 
void CircuitGenerator::addBuffersSimple_OLD() {
     std::vector<ENode*> nodeList;

     for (auto& enode : *enode_dag) {
         if (enode->type == Phi_ || enode->type == Phi_n || enode->type == Phi_c) {
		// Aya added an extra Or in the following condition!!
             if (enode->CntrlPreds->size() > 1 || enode->JustCntrlPreds->size() > 1 || enode->CntrlOrderPreds->size() > 1) {

                 ENode* new_node = NULL;

                 if (enode->Instr != NULL) {
                     new_node = new ENode(Bufferi_, "buffI", enode->Instr, NULL);
                 } else if (enode->A != NULL) {
                     new_node = new ENode(Buffera_, "buffA", enode->A, NULL);
                 } else
                     new_node = new ENode(Buffera_, "buffA");

                 new_node->id = CircuitGenerator::buff_id++;

                 //if (!enode->is_live_out) {
                 //    new_node->BB = enode->BB;
                 //}

				// We work under the assumption that a buffer will be inserted between the Phi and the successor .at(0).. In case of Phis of type Cmerge that has a 2nd output (successor) feeding a Mux, we don't need to place a abuffer for this 2nd successor as it's not involved in a Phi!!!
				/*
					Aya: we need to insert the buffer between the Phi and its successors in a way that doesn't violate the indices of the successor in the Phi successor array and also the index of the Phi in the predecessor array of a specific successor of Phi

					Predecessor of the new_node(buffer) should be the enode and the successor of the new_node(buffer) should be the original successor of the enode
				*/ 

				 if(enode->is_redunCntrlNet) {
					new_node->is_redunCntrlNet = true;
				 }

                 if (enode->type == Phi_c) {
				
					// Aya added this and changed the way the new_node(buffer) is connected to its successors and predecessors
					if(enode->is_redunCntrlNet) {
						assert(enode->CntrlOrderSuccs->size() > 0);
						
						// buffer should be the predecessor of the original enode's successor in place of the enode
						auto pos = std::find(enode->CntrlOrderSuccs->at(0)->CntrlOrderPreds->begin(), enode->CntrlOrderSuccs->at(0)->CntrlOrderPreds->end(), enode);

						assert(pos != enode->CntrlOrderSuccs->at(0)->CntrlOrderPreds->end());
						int index = pos - enode->CntrlOrderSuccs->at(0)->CntrlOrderPreds->begin();
						//preds_of_orig_succs->insert(index , fork);
						enode->CntrlOrderSuccs->at(0)->CntrlOrderPreds->at(index) = new_node;

						new_node->CntrlOrderSuccs->push_back(enode->CntrlOrderSuccs->at(0));

						
						// buffer should replace the original enode successor .at(0)
						enode->CntrlOrderSuccs->at(0) = new_node;
						new_node->CntrlOrderPreds->push_back(enode);

					} else {

						assert(enode->JustCntrlSuccs->size() > 0);
						// buffer should be the predecessor of the original enode's successor in place of the enode
						auto pos = std::find(enode->JustCntrlSuccs->at(0)->JustCntrlPreds->begin(), enode->JustCntrlSuccs->at(0)->JustCntrlPreds->end(), enode);

						assert(pos != enode->JustCntrlSuccs->at(0)->JustCntrlPreds->end());
						int index = pos - enode->JustCntrlSuccs->at(0)->JustCntrlPreds->begin();
						//preds_of_orig_succs->insert(index , fork);
						enode->JustCntrlSuccs->at(0)->JustCntrlPreds->at(index) = new_node;

						new_node->JustCntrlSuccs->push_back(enode->JustCntrlSuccs->at(0));

						// buffer should replace the original enode successor .at(0)
						enode->JustCntrlSuccs->at(0) = new_node;
						new_node->JustCntrlPreds->push_back(enode);

					}

                 } else {
					// then it must be data phi of our creation or LLVM phi and both have successor in the data path only!!
					assert(enode->type == Phi_ || enode->type == Phi_n);
					assert(enode->CntrlSuccs->size() > 0);
					// buffer should be the predecessor of the original enode's successor in place of the enode
					auto pos = std::find(enode->CntrlSuccs->at(0)->CntrlPreds->begin(), enode->CntrlSuccs->at(0)->CntrlPreds->end(), enode);

					assert(pos != enode->CntrlSuccs->at(0)->CntrlPreds->end());
					int index = pos - enode->CntrlSuccs->at(0)->CntrlPreds->begin();
					enode->CntrlSuccs->at(0)->CntrlPreds->at(index) = new_node;

					new_node->CntrlSuccs->push_back(enode->CntrlSuccs->at(0));

					// buffer should replace the original enode successor .at(0)
					enode->CntrlSuccs->at(0) = new_node;
					new_node->CntrlPreds->push_back(enode);
					
                 }

                 nodeList.push_back(new_node);

                 // enode_dag->push_back(new_node);
             }
         }
     }

     for (auto node : nodeList)
         enode_dag->push_back(node);
}

// This function must be called on the original enode_dag before inserting new nodes
void CircuitGenerator::identifyBackwardEdges(bool cntrl_path_flag) {
	std::vector<ENode*>* enode_preds;
	std::vector<bool>* isBackwardPreds;

// a field that keeps the index of successors and predecessors with a backward edge 
	for(auto& enode : *enode_dag) {
		///////////////////////// First identify the index of this node in the enode_dag
		auto pos_enode = std::find(enode_dag->begin(), enode_dag->end(), enode);
		assert(pos_enode != enode_dag->end());
		int index_enode = pos_enode - enode_dag->begin();
		////////////////////////////////////////////////

		enode_preds = (cntrl_path_flag)? enode->JustCntrlPreds : enode->CntrlPreds;
		isBackwardPreds = (cntrl_path_flag)? enode->isBackwardPreds_Cntrl : enode->isBackwardPreds;

		for(auto& pred : *enode_preds) {
	// normally the index of the predecessor in the enode_dag should be less than the index of the enode, if not, then this is a backward edge
			///////////////////////////////////////////////
			auto pos_pred = std::find(enode_dag->begin(), enode_dag->end(), pred);
			assert(pos_pred != enode_dag->end());
			int index_pred = pos_pred - enode_dag->begin();
			////////////////////////////////////////////////			

			if(index_pred > index_enode) {
				isBackwardPreds->push_back(true);
			} else {
				isBackwardPreds->push_back(false);
			}

		}

		// sanity check: size of isBackwardPreds should be the same as that of the cntrlPred
		assert(enode_preds->size() == isBackwardPreds->size());
	}

}


void CircuitGenerator::buildPostDomTree(Function& F) {
	/*
	IMP note: this post dominator tree considers that a node is not post-dominating itself. 
	*/ 
	my_post_dom_tree->recalculate(F);
}

void CircuitGenerator::buildDagEnodes(Function& F) {

    // Find arguments of function create enodes for them
    BasicBlock& BB = *F.begin();
    BasicBlock* BBref;
    BBref = &BB;

    int id = 0; // Leonardo inserted line
    for (auto ai = F.arg_begin(), ae = F.arg_end(); ai != ae; ++ai) {
        Argument& A = *ai;
        Argument* Aref;
        Aref               = &A;
        ENode* enode2      = new ENode(Argument_, Aref->getName().str().c_str(), Aref, BBref);
        enode2->argName    = Aref->getName().str(); // Lana 13.08.19
        enode2->id         = id++;                  // Leonardo inserted line
        //enode2->is_live_in = true;
        enode_dag->push_back(enode2);
    }

    // Iterate over BBs, name them, and add them to bbnode_dag
    for (auto IIT = F.begin(), IE = F.end(); IIT != IE; ++IIT) {

        BasicBlock& BB = *IIT;
        BasicBlock* BBref;
        BBref = &BB;

        std::string name   = "block";
        std::string BBname = name + std::to_string(idt + 1);
        BB.setName(BBname.c_str());

        BBNode* bbnode1 = new BBNode(BBref, idt);

        bbnode_dag->push_back(bbnode1); // Add BBs as BBnodes
        idt++;
    }

    // Iterate over BBNodes and form the loops
    for (auto& bbnode : *bbnode_dag) {
        for (auto& bbnode1 : *bbnode_dag) {
            for (pred_iterator PI = pred_begin(bbnode->BB), E = pred_end(bbnode->BB); PI != E;
                 ++PI) {
                BasicBlock* Pred = *PI;
                if (bbnode1->BB == Pred) {
                    bbnode->CntrlPreds->push_back(bbnode1);
                    bbnode1->CntrlSuccs->push_back(bbnode);
                }
            }
        }
    }

    // Iterate over ever BB's instructions and form ENodes and form connections
    // b/w instructions and their pred instructions/input BBs (for branches)/
    // function arguments/immediates & predicates
    for (auto IIT = F.begin(), IE = F.end(); IIT != IE; ++IIT) {

        BasicBlock& BB = *IIT;
        BasicBlock* BBref;
        BBref = &BB;

        for (auto IIIT = BB.begin(), IIE = BB.end(); IIIT != IIE; ++IIIT) {

            Instruction& Inst = *IIIT;
            Instruction* Instref;
            Instref = &Inst;

            unsigned opcode = Inst.getOpcode();
            ENode* enode;
            if (opcode == Instruction::Br)
                enode = new ENode(Branch_, Inst.getOpcodeName(), Instref, BBref);
            else if (opcode == Instruction::PHI)
                enode = new ENode(Phi_, Inst.getOpcodeName(), Instref, BBref);
            else
                enode = new ENode(Inst_, Inst.getOpcodeName(), Instref, BBref);

            if (opcode == Instruction::Store)
                enode->id = store_id++;
            else if (opcode == Instruction::Ret)
                enode->id = ret_id++;
            else if (opcode == Instruction::Call)
                enode->id = call_id++;
            else if (opcode == Instruction::Switch)
                enode->id = switch_id++;
            else if (opcode == Instruction::Select)
                enode->id = select_id++;
            else
                enode->id = op_id++;

            std::string name    = "";
            std::string ssaname = name + std::to_string(ssa);
            if (Inst.getName() == "" && opcode != Instruction::Store && opcode != Instruction::Br &&
                opcode != Instruction::Switch && opcode != Instruction::Ret &&
                opcode != Instruction::Call) {
                Inst.setName(ssaname.c_str());
                ssa++;
            }

            if (opcode == Instruction::Br || opcode == Instruction::Switch)
                ssa++;

            for (size_t i = 0; i < Inst.getNumOperands(); i++) {
                if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Inst.getOperand(i))) {
                    ENode* cstNode =
                        new ENode(Cst_, std::to_string(CI->getSExtValue()).c_str(), BBref);
                    cstNode->CI = CI; // dyn_cast<llvm::ConstantInt>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        // cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);

                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                } else if (llvm::ConstantFP* CF = dyn_cast<llvm::ConstantFP>(Inst.getOperand(i))) {
                    ENode* cstNode = new ENode(Cst_, BBref);
                    cstNode->CF    = CF; // dyn_cast<llvm::ConstantFP>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        // cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);


                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                } else if (dyn_cast<llvm::UndefValue>(Inst.getOperand(i))) {
                    ENode* cstNode = new ENode(Cst_, BBref);
                    cstNode->CI    = 0; // dyn_cast<llvm::ConstantFP>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        // cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);

                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                }

                for (auto& nd2 : *enode_dag) {
                    if (nd2->type == Inst_ || nd2->type == Phi_ || nd2->type == Branch_) {
                        if (nd2->Instr == Inst.getOperand(i)) {
                            if (opcode != Instruction::PHI) {
                                enode->CntrlPreds->push_back(nd2);

                                nd2->CntrlSuccs->push_back(enode);
                            }
                        }
                    } else if (nd2->type == Argument_) {
                        if (nd2->A == Inst.getOperand(i)) {
                            if (enode->Instr->getOpcode() != Instruction::PHI) {
                                enode->CntrlPreds->push_back(nd2);

                                nd2->CntrlSuccs->push_back(enode);
                            }
                        }
                    }
                }
            }

            enode_dag->push_back(enode);
        }
    }

    for (BBNode* bbnode : *bbnode_dag) {
        BBMap->insert(std::make_pair(bbnode->BB, bbnode));
    }
}

// enode_dag is not complete - it has no loops at the moment -- this function adds the loops and
// produces the correct enode_dag
void CircuitGenerator::fixBuildDagEnodes() {
    for (auto& enode_0 : *enode_dag) {         // for each enode
        if (enode_0->type == Phi_) {           // if it is a phi, there is a loop
            for (auto& enode_1 : *enode_dag) { // go through enode dag again, for each enode
                for (size_t i = 0; i < enode_0->Instr->getNumOperands(); i++) { // for each operand
                    if (enode_1->type == Phi_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->Instr) { // if the phi is connected to that enode
                            // Lana 18.10.2018. Make sure there are no double edges between phis
                            //if (!contains(enode_0->CntrlPreds, enode_1))
                                enode_0->CntrlPreds->push_back(enode_1); // add connection
                            //if (!contains(enode_1->CntrlSuccs, enode_0))
                                enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }
                    if (enode_1->type == Inst_ || enode_1->type == Branch_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->Instr) { // if the phi is connected to that enode
                            // Lana 18.10.2018. Make sure there are no double edges between phis
                            enode_0->CntrlPreds->push_back(enode_1); // add connection
                            enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }

                    if (enode_1->type == Argument_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->A) { // if the phi is connected to that enode
                            enode_0->CntrlPreds->push_back(enode_1); // add connection
                            enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }
                }
            }
        }
    }
}

// Lana 20.10.2018. Removing some nodes that we don't need before we start modifying the DFG
void CircuitGenerator::removeRedundantBeforeElastic(std::vector<BBNode*>* bbnode_dag,
                                                    std::vector<ENode*>* enode_dag) {

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            // remove operand 0 from getelementptr, no need to connect in circuit
            if (enode->Instr->getOpcode() == Instruction::GetElementPtr) {

                ENode* predToRemove = NULL;
                for (auto& pred : *(enode->CntrlPreds)) {
                    if (pred->Instr != NULL) {
                        if (dyn_cast<Value>(pred->Instr) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }
                    } else if (pred->A != NULL) {
                        if (dyn_cast<Value>(pred->A) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }

                    } else if (pred->CI != NULL) {
                        if (dyn_cast<Value>(pred->CI) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }

                    } else if (pred->CF != NULL) {
                        if (dyn_cast<Value>(pred->CF) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }
                    }
                }

                predToRemove->CntrlSuccs->erase(std::remove(predToRemove->CntrlSuccs->begin(), predToRemove->CntrlSuccs->end(), enode), predToRemove->CntrlSuccs->end());
                enode->CntrlPreds->erase(
                    std::remove(enode->CntrlPreds->begin(), enode->CntrlPreds->end(), predToRemove),
                    enode->CntrlPreds->end());

            }
        }
    }

    std::vector<ENode*> tmpNodes;

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            // remove getelementptr when only one predecessor (can be directly propagated to load,
            // no extra computation)
            if ((enode->Instr->getOpcode() == Instruction::GetElementPtr &&
                 enode->CntrlPreds->size() == 1)) {
                tmpNodes.push_back(enode);
            }

            // we don't need alloca, all memories already 'allocated'
            if ((enode->Instr->getOpcode() == Instruction::Alloca)) {
                tmpNodes.push_back(enode);
            }

            // remove zext and sext if they are in front of alloca or getelementptr that we are
            // removing if getelementptr stays, keep zext/sext so that we can identify which ports
            // each pred is connected to
            if (enode->Instr->getOpcode() == Instruction::SExt ||
                enode->Instr->getOpcode() == Instruction::ZExt) {
                ENode* succ = enode->CntrlSuccs->front();
                if (succ->type == Inst_) {
                    if ((succ->Instr->getOpcode() == Instruction::Alloca)) {
                        tmpNodes.push_back(enode);
                    }
                    if ((succ->Instr->getOpcode() == Instruction::GetElementPtr &&
                         succ->CntrlPreds->size() == 1)) {
                        tmpNodes.push_back(enode);
                    }
                }
            }

        } else if (enode->type == Argument_) {
            // if argument is connected directly to store address (without getptr), address is zero
            // (there is no offset calculation)
            // removing arg to avoid unnecessary argument connections through circuit

            std::vector<ENode*> nodeList;
            for (auto& succ : *enode->CntrlSuccs) {
                if (succ->type == Inst_) {
                	bool add_const = false;
                    if (succ->Instr->getOpcode() == Instruction::Store) {
                        if (succ->Instr->getOperand(1) == dyn_cast<Value>(enode->A)) 
                        	add_const = true;                    
                    }
                    if (succ->Instr->getOpcode() == Instruction::Load) {
                        if (succ->Instr->getOperand(0) == dyn_cast<Value>(enode->A)) 
                        	add_const = true;
                    }

                    if (add_const) {
                    	succ->CntrlPreds->erase(std::remove(succ->CntrlPreds->begin(),
                                                                succ->CntrlPreds->end(), enode),
                                                    succ->CntrlPreds->end());

                        ENode* cstNode = new ENode(Cst_, std::to_string(0).c_str(), succ->BB);

                        cstNode->id = cst_id++;
                        succ->CntrlPreds->push_back(cstNode);
                        cstNode->CntrlSuccs->push_back(succ);
                        enode_dag->push_back(cstNode);
                        nodeList.push_back(succ);
                    }
                }
            }

            for (auto node : nodeList)
                enode->CntrlSuccs->erase(
                    std::remove(enode->CntrlSuccs->begin(), enode->CntrlSuccs->end(), node),
                    enode->CntrlSuccs->end());

            if (enode->CntrlSuccs->size() == 0)
                tmpNodes.push_back(enode);
        }
    }
    for (auto enode : tmpNodes) {

        if (enode->type != Argument_) {
            ENode* pred = enode->CntrlPreds->front();
            pred->CntrlSuccs->erase(
                std::remove(pred->CntrlSuccs->begin(), pred->CntrlSuccs->end(), enode),
                pred->CntrlSuccs->end());

            // ENode* succ = enode->CntrlSuccs->front();

            for (auto& succ : *enode->CntrlSuccs) {
                succ->CntrlPreds->erase(
                    std::remove(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode),
                    succ->CntrlPreds->end());

                pred->CntrlSuccs->push_back(succ);
                succ->CntrlPreds->push_back(pred);
            }
        }
        for (auto bbnode : *bbnode_dag) {
            if (contains(bbnode->Live_in, enode)) {
                bbnode->Live_in->erase(
                    std::remove(bbnode->Live_in->begin(), bbnode->Live_in->end(), enode),
                    bbnode->Live_in->end());
            }
            if (contains(bbnode->Live_out, enode)) {
                bbnode->Live_out->erase(
                    std::remove(bbnode->Live_out->begin(), bbnode->Live_out->end(), enode),
                    bbnode->Live_out->end());
            }
        }

        delete enode;
        enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode),
                         enode_dag->end());
    }
}

void CircuitGenerator::setGetelementPtrConsts(std::vector<ENode*>* enode_dag) {
    // add constantsa to getelementptr which describe array dimensions
    // used to appropriately handle multi-dimensional arrays
    // (i.e., to convert multi-dim into single-dim address/index)

    std::vector<ENode*> nodeList;
    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            if (enode->Instr->getOpcode() == Instruction::GetElementPtr) {

                Instruction* I = enode->Instr;

                int cst_count = 0; // I->getNumOperands() - 2;

                // extract dimensions from getelementptr instruction
                for (gep_type_iterator In = gep_type_begin(I), E = gep_type_end(I); In != E; ++In) {
                    if (In.isBoundedSequential()) {
                        ENode* cstNode = new ENode(Cst_, enode->BB);
                        // save num elements in array dimension as constant value
                        // will be used in VHDL to calculate address
                        cstNode->cstValue = In.getSequentialNumElements();
                        cstNode->JustCntrlSuccs->push_back(enode);
                        cstNode->id = cst_id++;
                        enode->JustCntrlPreds->push_back(cstNode);
                        nodeList.push_back(cstNode);
                        cst_count++;
                    }
                }
            }
        }
    }
    for (auto node : nodeList)
        enode_dag->push_back(node);
}


// Lana 20.10. 2018. Removing nodes after elastic modifications
// later we could extend to remove 1-input merges etc.
void CircuitGenerator::removeRedundantAfterElastic(std::vector<ENode*>* enode_dag) {

    std::vector<ENode*> tmpNodes;

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        // remove forks with only one successor
        if ((enode->type == Fork_ || enode->type == Fork_c) &&
            (enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size()) == 1) {

            tmpNodes.push_back(enode);
        }

        // remove constants which are never read
        if ((enode->type == Cst_) &&
            (enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size()) == 0) {

            tmpNodes.push_back(enode);
        }
    }
    for (auto enode : tmpNodes) {

        if (enode->type == Fork_) {
            ENode* pred = enode->CntrlPreds->front();
            ENode* succ = enode->CntrlSuccs->front();

            pred->CntrlSuccs->erase(
                std::remove(pred->CntrlSuccs->begin(), pred->CntrlSuccs->end(), enode),
                pred->CntrlSuccs->end());
            succ->CntrlPreds->erase(
                std::remove(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode),
                succ->CntrlPreds->end());

            pred->CntrlSuccs->push_back(succ);
            succ->CntrlPreds->push_back(pred);
        } else if (enode->type == Fork_c) {
            ENode* pred = enode->JustCntrlPreds->front();
            ENode* succ = enode->JustCntrlSuccs->front();

            pred->JustCntrlSuccs->erase(
                std::remove(pred->JustCntrlSuccs->begin(), pred->JustCntrlSuccs->end(), enode),
                pred->JustCntrlSuccs->end());
            succ->JustCntrlPreds->erase(
                std::remove(succ->JustCntrlPreds->begin(), succ->JustCntrlPreds->end(), enode),
                succ->JustCntrlPreds->end());

            pred->JustCntrlSuccs->push_back(succ);
            succ->JustCntrlPreds->push_back(pred);
        }

        delete enode;
        enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode),
                         enode_dag->end());
    }
}

void CircuitGenerator::forkToSucc(std::vector<ENode*>* preds_of_orig_succs, ENode* orig, ENode* fork) {
    auto pos = std::find(preds_of_orig_succs->begin(), preds_of_orig_succs->end(), orig);
	
	assert(pos != preds_of_orig_succs->end());

	int index = pos - preds_of_orig_succs->begin();
	//preds_of_orig_succs->insert(index , fork);
	preds_of_orig_succs->at(index) = fork;
}

void CircuitGenerator::aya_InsertNode(ENode* enode, ENode* after) {
    // removing all connections between enode and its successors
    for (auto& succ : *(enode->CntrlSuccs)) { // for all enode's successors
        forkToSucc(succ->CntrlPreds, enode, after);
        after->CntrlSuccs->push_back(succ); // add connection between after and enode's successors in the same order as that of the original successors 
        
    }
    enode->CntrlSuccs = new std::vector<ENode*>; // all successors of enode removed

	// the following statements are still maintaining the correct order of succs and preds 
    after->CntrlPreds->push_back(enode); // add connection between after and enode
    enode->CntrlSuccs->push_back(after); // add connection between after and enode
}

// AYA: 17/11/2021
void CircuitGenerator::new_addFork_redunCntrl() {
	std::vector<ENode*>* new_forks = new std::vector<ENode*>;
	std::vector<ENode*> *other_networks_succ;
	node_type fork_type;	
	std::string fork_name; 

	for(auto& enode: *enode_dag) {
		if(enode->CntrlOrderSuccs->size() > 0 && enode->type != Fork_ && enode->type != Fork_c && enode->type != Branch_ && enode->type != Branch_c && enode->type != Branch_n && enode->type != MC_ && enode->type != LSQ_ && !enode->isLoadOrStore()) {
			assert(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() <=1);
			// for sure it has AT MOST 1 successor in any of the 2 other networks
			if(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() == 1) {
				if(enode->CntrlSuccs->size() == 1) {
					other_networks_succ = enode->CntrlSuccs;
					fork_type = Fork_;
					fork_name = "fork";
				} else {
					assert(enode->JustCntrlSuccs->size() == 1);
					other_networks_succ = enode->JustCntrlSuccs;
					fork_type = Fork_c;
					fork_name = "forkC";
				}

				// check if the single successor is a fork
				if(other_networks_succ->at(0)->type == Fork_ || other_networks_succ->at(0)->type == Fork_c) {
					ENode* existing_fork = other_networks_succ->at(0);
					// we should insert this existing fork between the enode and all of its CntrlOrderSuccs!!
					// loop over all the CntrlOrderSuccs
					for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
						forkToSucc(order_succ->CntrlOrderPreds, enode, existing_fork);
						existing_fork->CntrlOrderSuccs->push_back(order_succ);
					} 
					enode->CntrlOrderSuccs = new std::vector<ENode*>;

				} else {
					// should create a new fork to feed to the CntrlOrder network as well as to the other network(s)
					ENode* fork = new ENode(fork_type, fork_name.c_str(), enode->BB);
					fork->id = fork_id++;

					new_forks->push_back(fork);

					// the following is done in a stupid coding!
					if(enode->CntrlSuccs->size() == 1) {
						// insert this fork between the enode and the CntrlSuccs->at(0)
						forkToSucc(enode->CntrlSuccs->at(0)->CntrlPreds, enode, fork);
						fork->CntrlSuccs->push_back(enode->CntrlSuccs->at(0));

						// insert this fork between the enode and its CntrlOrderSuccs
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, fork);
							fork->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 
						enode->CntrlSuccs = new std::vector<ENode*>;

						enode->CntrlSuccs->push_back(fork);
						fork->CntrlPreds->push_back(enode);


					} else {
						assert(enode->JustCntrlSuccs->size() == 1);

						// insert this fork between the enode and the JustCntrlSuccs->at(0)
						forkToSucc(enode->JustCntrlSuccs->at(0)->JustCntrlPreds, enode, fork);
						fork->JustCntrlSuccs->push_back(enode->JustCntrlSuccs->at(0));

						// insert this fork between the enode and its CntrlOrderSuccs
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, fork);
							fork->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 
						enode->JustCntrlSuccs = new std::vector<ENode*>;

						enode->JustCntrlSuccs->push_back(fork);
						fork->JustCntrlPreds->push_back(enode);
						/////////////////////////////////////////////////
					} 
					
				}


			} else if(enode->CntrlOrderSuccs->size() > 1){
				// insert a fork in this purely redundant network!
				ENode* fork_c = new ENode(Fork_c, "forkC", enode->BB);
				fork_c->id = fork_id++;
			
				new_forks->push_back(fork_c);

				// 17/11/2021: insert all forks in the end!!!
				// enode_dag->push_back(fork_c);

				fork_c->is_redunCntrlNet = true;  // fork is purely in the redunCntrlNet!

				////////// insert the fork between the enode and its succs
				for (auto& succ : *(enode->CntrlOrderSuccs)) { 
					forkToSucc(succ->CntrlOrderPreds, enode, fork_c);
					fork_c->CntrlOrderSuccs->push_back(succ); // add connection between after and enode's successors
				}
				// successors of the enode should be now empty
				enode->CntrlOrderSuccs = new std::vector<ENode*>; 
				enode->CntrlOrderSuccs->push_back(fork_c); 
				fork_c->CntrlOrderPreds->push_back(enode);
				////////////////////////////////////// 
			} 

		}

	}

	// push back all forks to the enode_dag!
	for(int i = 0; i < new_forks->size(); i++) {
		assert(new_forks->at(i)->type == Fork_ || new_forks->at(i)->type == Fork_c);
		enode_dag->push_back(new_forks->at(i));
	}

}

void CircuitGenerator::addFork_redunCntrl() {
/*
	- From above we are guaranteed the following:
		Any node (other than branches and forks will have exactly one successor that is either in the Cntrl network or in the JustCntrl network)..

	- This function needs to make sure of two things:
		1) If there is at least 1 element in the CntrlOrder network and 1 element in the CntrlSucc or the JustCntrl and this element is not fork, create a new fork. But, if it's a fork, just push_back the CntrlOrder element(s) to it!! 

		2) If there is more than 1 element in the CntrlOrder and no elements in the other two networks, create a new control fork and push_back all these elements to it

		3) If there is exactly one element in the CntrlOrder and no elements in the other two networks, do nothing.
	
*/
	for(auto& enode: *enode_dag) {
		// 1st check if there is any successor in the CntrlOrder network
		
		if(enode->CntrlOrderSuccs->size() > 0 && enode->type != Fork_ && enode->type != Fork_c && enode->type != Branch_ && enode->type != Branch_c && enode->type != Branch_n && enode->type != MC_ && enode->type != LSQ_ && !enode->isLoadOrStore()) {
			// sanity check that any node should have just a single successor in just one of the two networks.. It could have no successors in these two networks at all if the node is created in the redunCntrlNetwork!
			assert(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() <= 1);

			// if it has any successor in any of these networks
			if(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() == 1) {
				// check if this successor is in the CntrlSuccs network
				/*if(enode->CntrlSuccs->size() == 1) {
					if(enode->CntrlSuccs->at(0)->type == Fork_) {
						ENode* existing_fork = enode->CntrlSuccs->at(0);
						// we should insert this existing fork between the enode and all of its CntrlOrderSuccs!!
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, existing_fork);
							existing_fork->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 
						
					} else {
						// this means there is a single node in the CntrlSuccs that is not a fork_, so we need to create a new fork and adjust the connections
						ENode* fork = new ENode(Fork_, "fork", enode->BB);
						fork->id = fork_id++;
						enode_dag->push_back(fork);

						// insert this fork between the enode and the CntrlSuccs->at(0)
						forkToSucc(enode->CntrlSuccs->at(0)->CntrlPreds, enode, fork);
						fork->CntrlSuccs->push_back(enode->CntrlSuccs->at(0));

						// insert this fork between the enode and its CntrlOrderSuccs
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, fork);
							fork->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 
						enode->CntrlSuccs = new std::vector<ENode*>;

						enode->CntrlSuccs->push_back(fork);
						fork->CntrlPreds->push_back(enode);
					}
				} else {
					// then this succ must be in the JustCntrl network
					assert(enode->JustCntrlSuccs->size() == 1);
					if(enode->JustCntrlSuccs->at(0)->type == Fork_c) {
						ENode* existing_fork = enode->JustCntrlSuccs->at(0);
						// we should insert this existing fork between the enode and all of its CntrlOrderSuccs!!
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, existing_fork);
							existing_fork->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 

					} else {
						// this means there is a single node in the JustCntrlSuccs that is not a fork_c, so we need to create a new fork_c and adjust the connections
						ENode* fork_c = new ENode(Fork_c, "forkC", enode->BB);
						fork_c->id = fork_id++;
						enode_dag->push_back(fork_c);

						// insert this fork between the enode and the JustCntrlSuccs->at(0)
						forkToSucc(enode->JustCntrlSuccs->at(0)->JustCntrlPreds, enode, fork_c);
						fork_c->JustCntrlSuccs->push_back(enode->JustCntrlSuccs->at(0));

						// insert this fork between the enode and its CntrlOrderSuccs
						// loop over all the CntrlOrderSuccs
						for(auto& order_succ : *(enode->CntrlOrderSuccs)) {
							forkToSucc(order_succ->CntrlOrderPreds, enode, fork_c);
							fork_c->CntrlOrderSuccs->push_back(order_succ);
						} 
						enode->CntrlOrderSuccs = new std::vector<ENode*>; 
						enode->JustCntrlSuccs = new std::vector<ENode*>;

						enode->JustCntrlSuccs->push_back(fork_c);
						fork_c->JustCntrlPreds->push_back(enode);

					}

				}*/
			} else {
				// this means the node has no successors in any of the two networks
				// check if it has more than 1 successor in the CntrlOrder network then we must insert a control fork
				
				if(enode->CntrlOrderSuccs->size() > 1) {
					ENode* fork_c = new ENode(Fork_c, "forkC", enode->BB);
					fork_c->id = fork_id++;
					enode_dag->push_back(fork_c);

					fork_c->is_redunCntrlNet = true;  // fork is purely in the redunCntrlNet!

					/*for (auto& succ : *(enode->CntrlOrderSuccs)) { 
						forkToSucc(succ->CntrlOrderPreds, enode, fork_c);
						fork_c->CntrlOrderSuccs->push_back(succ); // add connection between after and enode's successors
					}

					// successors of the enode should be now empty
					enode->CntrlOrderSuccs = new std::vector<ENode*>; 
					enode->CntrlOrderSuccs->push_back(fork_c); 
					fork_c->CntrlOrderPreds->push_back(enode); 
					*/
				} // otherwise, we do nothing!
				
			}

		 }

	}   // braces for looping over enode

}

// AYA: 06/10/2021 added this to take care of adding forks after loads in case the read is by multiple consumers!!
void CircuitGenerator::addFork_Mem_loads() {
	int mem_index;  // holds the index of the LSQ or MC succ of the load instruction
	for(auto& enode: *enode_dag) {
		if(enode->type == Inst_ && isa<LoadInst>(enode->Instr)) {
			mem_index = -1;
			// check the number of CntrlSuccs
			if(enode->CntrlSuccs->size() > 2) {
				// identify which succs is the MC or LSQ!!
				for(int i = 0; i < enode->CntrlSuccs->size(); i++) {
					if(enode->CntrlSuccs->at(i)->type == MC_ || enode->CntrlSuccs->at(i)->type == LSQ_) 		{
						mem_index = i;
					}
				}
				// sanity chek
				assert(mem_index != -1); 
				// keep a copy of the lsq or mc succ, delete from the array of succs then append it in the end after the fork!!
				ENode* memory = enode->CntrlSuccs->at(mem_index);
				// the following function (implemented in utils.h) will erase the element
				removeNode(enode->CntrlSuccs, memory);

				// will enforce that the MC or LSQ be the second succ and a fork will be inserted in the first succ and all other succs will be connected to it!!
				ENode* fork = new ENode(Fork_, "fork", enode->BB);
				fork->id = fork_id++;
				enode_dag->push_back(fork);
				// Fork will be inserted at index 0 of the enode->CntrlSuccs and the original order of successors should be maintained in fork->CntrlSuccs
				aya_InsertNode(enode, fork);	

				enode->CntrlSuccs->push_back(memory);			

				// sanity check
				assert(enode->CntrlSuccs->size() == 2);

			} else {
				// sanity check, the succs should be at least 2!!!
				assert(enode->CntrlSuccs->size() == 2);
			} 

		} 

	} 

}

void CircuitGenerator::addFork() {
	// TEMPORARILY FOR DEBUGGING!!
	std::ofstream h_file;
	h_file.open("NOWCHECK_FORKSSS.txt");

	// AYA: 17/11/2021
	std::vector<ENode*>* new_forks = new std::vector<ENode*>;

	// loop over all enode operations
	for(auto& enode: *enode_dag) {
		// put a fork after any node having multiple successors in the data path or control path, except for branches
		h_file << "\nNode Name: " << getNodeDotNameNew(enode) << "\n";
		
		if(enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() > 1 && enode->type != Fork_ && enode->type != Fork_c && enode->type != Branch_ && enode->type != Branch_c && enode->type != Branch_n && enode->type != MC_ && enode->type != LSQ_ && !enode->isLoadOrStore() && enode->BB != nullptr) {
			h_file << "Entered the big if_condition\n";

			if(enode->CntrlSuccs->size() == 0) {
				h_file << "Entered the enode->CntrlSuccs->size() == 0 condition\n";
			// this means no successors in the data path, so all the successors must be in the control path
				ENode* fork = new ENode(Fork_c, "forkC", enode->BB);
				fork->id = fork_id++;

				// AYA: 17/11/2021: Will push all forks in the end!!
				//enode_dag->push_back(fork);
				new_forks->push_back(fork);

				// need to push it to cntrl_succ of its bb
				BBMap->at(enode->BB)->Cntrl_nodes->push_back(fork);  

				// need to correctly insert the fork between the node and all of its 						JustCntrlSuccs, so I created a new function similar to InsertNode	
				InsertCntrlNode(enode, fork, false);
		
			} else { // there is at least one successor in the data path and we may or may not have any successors in the control path, so we blindly create a data fork to feed the data successor(s) and can also feed control successors, if any
				h_file << "Didn't enter the enode->CntrlSuccs->size() == 0 condition\n";
				ENode* fork = new ENode(Fork_, "fork", enode->BB);
				fork->id = fork_id++;
				
				// AYA: 17/11/2021: Will push all forks in the end!!
				// enode_dag->push_back(fork);
				new_forks->push_back(fork);

				// Fork must be inserted at index 0 of the enode->CntrlSuccs and the original order of successors should be maintained in fork->CntrlSuccs

				aya_InsertNode(enode, fork);

				// if you have even a single control successor, 
				if(enode->JustCntrlSuccs->size() > 0) {
					InsertCntrlNode(enode, fork, true);
				} 

			}
		
		} 

// The following is meant for correctly adding the successors of branches in the JustCntrl and Cntrl network!
	// Aya: 19/08/2022: extended the following logic to connect the succs of Branch_c in the CntrlOrder network!
		if(enode->type == Branch_n) {
			addBrSuccs(enode, data);
		} else {
			if(enode->type == Branch_c) {
				// depending on where the successors are placed we will know if the Branch is part of the JustCntrl network or the CntrlOrder network
				if(enode->true_branch_succs_Ctrl->size() > 0 || enode->false_branch_succs_Ctrl->size() > 0) {
					addBrSuccs(enode, constCntrl);
				} else {
					assert(enode->true_branch_succs_Ctrl_lsq->size() > 0 || enode->false_branch_succs_Ctrl_lsq->size() > 0);
					addBrSuccs(enode, memDeps);
				}
				
			}
		}


		/*if(enode->type == Branch_c && !enode->is_redunCntrlNet){
			//fixBranchesSuccs(enode, true);
			addBrSuccs(enode, true);
		} else
			if(enode->type == Branch_n) {
				//fixBranchesSuccs(enode, false);
				addBrSuccs(enode, false);
			}*/

	} 

	// Pushing all new forks to the enode_dag!
	for(int i = 0; i < new_forks->size(); i++) {
		assert(new_forks->at(i)->type == Fork_ || new_forks->at(i)->type == Fork_c);
		enode_dag->push_back(new_forks->at(i));
	}

	h_file << "Finished looping, about to go to Redundant things function\n";
	////////////////////////////////////////////////////////////////////
	// the following function adds/adjusts forks for nodes in the CntrlOrder network
	new_addFork_redunCntrl();

	////////////////////////////////////////////////////////////////////
	// the following function checks if a load operation feeds multiple consumers thus require adding a fork!!
	addFork_Mem_loads();
}

void CircuitGenerator::addBrSuccs(ENode* branch, networkType network_flag) {
	std::vector<ENode*> *true_succs, *false_succs; 
	std::vector<ENode*>*branch_CntrlSuccs;


	ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
	std::vector<ENode*>* sink_preds;

	ENode* fork;
	node_type fork_type; // depending on if it is a Branch_c or Branch_n
	std::string fork_name;

	std::vector<ENode*>* fork_preds;
	std::vector<ENode*>* fork_succs;

	std::vector<ENode*>* one_succ_preds;

	switch(network_flag) {
		case data:
			true_succs = branch->true_branch_succs;
			false_succs = branch->false_branch_succs;
			branch_CntrlSuccs = branch->CntrlSuccs;
			sink_preds = sink_node->CntrlPreds;

			fork_type = Fork_;
			fork_name = "fork";
		break;

		case constCntrl:
			true_succs = branch->true_branch_succs_Ctrl;
			false_succs = branch->false_branch_succs_Ctrl;
			branch_CntrlSuccs = branch->JustCntrlSuccs;
			sink_preds = sink_node->JustCntrlPreds;

			fork_type = Fork_c;
			fork_name = "forkC";
		break;

		case memDeps:
			true_succs = branch->true_branch_succs_Ctrl_lsq;
			false_succs = branch->false_branch_succs_Ctrl_lsq;
			branch_CntrlSuccs = branch->CntrlOrderSuccs;
			sink_preds = sink_node->CntrlOrderPreds;

			fork_type = Fork_c;
			fork_name = "forkC";
	}

	/*if(cntrl_br_flag) {
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
	} */

	fork = new ENode(fork_type, fork_name.c_str(), branch->BB);

	switch(network_flag) {
		case data:
			fork_preds = fork->CntrlPreds;
			fork_succs = fork->CntrlSuccs;
		break;

		case constCntrl:
			fork_preds = fork->JustCntrlPreds;
			fork_succs = fork->JustCntrlSuccs;
		break;

		case memDeps:
			fork_preds = fork->CntrlOrderPreds;
			fork_succs = fork->CntrlOrderSuccs;
	}
	
	/*if(cntrl_br_flag) {
		fork_preds = fork->JustCntrlPreds;
		fork_succs = fork->JustCntrlSuccs;
	} else {
		fork_preds = fork->CntrlPreds;
		fork_succs = fork->CntrlSuccs;
	} */

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
					fork->id = fork_id++;
	
		// AYA: 28/09/2021 changed the BB of the fork here to be that of the falseSuccs!!
					fork->BB = false_succs->at(0)->BB;

					enode_dag->push_back(fork);

					branch_CntrlSuccs->push_back(fork); 
					fork_preds->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs->push_back(one_succ); 

						// place the fork in place of the branch!
						switch(network_flag) {
							case data:
								one_succ_preds = one_succ->CntrlPreds; 
							break;

							case constCntrl:
								one_succ_preds = one_succ->JustCntrlPreds;
							break;

							case memDeps:
								one_succ_preds = one_succ->CntrlOrderPreds; 

						}
						//one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 

						forkToSucc(one_succ_preds, branch, fork);
						//one_succ_preds->push_back(fork);
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
					
					fork->id = fork_id++;
			
					// AYA: 28/09/2021 changed the BB of the fork here to be that of the falseSuccs!!
					fork->BB = false_succs->at(0)->BB;

					enode_dag->push_back(fork);

					branch_CntrlSuccs->push_back(fork); 
					fork_preds->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs->push_back(one_succ); 

						// place the fork in place of the branch!
						switch(network_flag) {
							case data:
								one_succ_preds = one_succ->CntrlPreds; 
							break;

							case constCntrl:
								one_succ_preds = one_succ->JustCntrlPreds;
							break;

							case memDeps:
								one_succ_preds = one_succ->CntrlOrderPreds; 

						}
						//one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 

						forkToSucc(one_succ_preds, branch, fork);
						//one_succ_preds->push_back(fork);
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
			fork->id = fork_id++;

			// AYA: 28/09/2021 changed the BB of the fork here to be that of the trueSuccs!!
			fork->BB = true_succs->at(0)->BB;

			enode_dag->push_back(fork);

			branch_CntrlSuccs->push_back(fork); 
			fork_preds->push_back(branch);

			for (auto& one_succ : *(true_succs)) {
				fork_succs->push_back(one_succ); 

				// place the fork in place of the branch!
				switch(network_flag) {
					case data:
						one_succ_preds = one_succ->CntrlPreds; 
					break;

					case constCntrl:
						one_succ_preds = one_succ->JustCntrlPreds;
					break;

					case memDeps:
						one_succ_preds = one_succ->CntrlOrderPreds; 

				}
				//one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 

				forkToSucc(one_succ_preds, branch, fork);
				//one_succ_preds->push_back(fork);
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
					
					// AYA: 28/09/2021 in case of multiple true and false succs, create a new fork to have one in the trueBB and one in the falseBB
					ENode* fork_2 = new ENode(fork_type, fork_name.c_str(), false_succs->at(0)->BB);
					std::vector<ENode*>* fork_preds_2;
					std::vector<ENode*>* fork_succs_2;

					switch(network_flag) {
						case data:
							fork_preds_2 = fork_2->CntrlPreds;
							fork_succs_2 = fork_2->CntrlSuccs;
						break;

						case constCntrl:
							fork_preds_2 = fork_2->JustCntrlPreds;
							fork_succs_2 = fork_2->JustCntrlSuccs;
						break;

						case memDeps:
							fork_preds_2 = fork_2->CntrlOrderPreds;
							fork_succs_2 = fork_2->CntrlOrderSuccs;
					}
					
					/*if(cntrl_br_flag) {
						fork_preds_2 = fork_2->JustCntrlPreds;
						fork_succs_2 = fork_2->JustCntrlSuccs;
					} else {
						fork_preds_2 = fork_2->CntrlPreds;
						fork_succs_2 = fork_2->CntrlSuccs;
					}*/

					fork_2->id = fork_id++;
					enode_dag->push_back(fork_2);
					branch_CntrlSuccs->push_back(fork_2); 
					fork_preds_2->push_back(branch);

					for (auto& one_succ : *(false_succs)) {
						fork_succs_2->push_back(one_succ); 

						// place the fork in place of the branch!
						switch(network_flag) {
							case data:
								one_succ_preds = one_succ->CntrlPreds; 
							break;

							case constCntrl:
								one_succ_preds = one_succ->JustCntrlPreds;
							break;

							case memDeps:
								one_succ_preds = one_succ->CntrlOrderPreds; 

						}
						//one_succ_preds = (cntrl_br_flag)? one_succ->JustCntrlPreds : one_succ->CntrlPreds; 
						
						forkToSucc(one_succ_preds, branch, fork_2);
						//one_succ_preds->push_back(fork);
					} 

				} 
			}

			///////////////////////////////////// end of handling the false successors 
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


/*void CircuitGenerator::cut_keepBranchSuccs(bool cntrl_br_flag, std::vector<ENode*>* redundant_branches, int k, std::vector<ENode*> *to_keep_true_succs, std::vector<ENode*> *to_keep_false_succs) {
	std::vector<ENode*> *redun_true_succs, *redun_false_succs;
	int true_idx, false_idx, true_idx_2; 
	
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

	assert(to_keep_true_succs!=nullptr);

	for(auto& true_succs: *redun_true_succs) {
		to_keep_true_succs->push_back(true_succs);

		// want to place the branch_to_keep in the predecessor of this successor
		if(cntrl_br_flag) {
			true_idx = returnPosRemoveNode(true_succs->JustCntrlPreds, redundant_branches->at(k)); 
			true_succs->JustCntrlPreds->at(true_idx) = redundant_branches->at(0);

			true_idx_2 = returnPosRemoveNode(true_succs->JustCntrlPreds, redundant_branches->at(0));
		} else {
			true_idx = returnPosRemoveNode(true_succs->CntrlPreds, redundant_branches->at(k)); 
			true_succs->CntrlPreds->at(true_idx) = redundant_branches->at(0);

			true_idx_2 = returnPosRemoveNode(true_succs->CntrlPreds, redundant_branches->at(0));
		} 

		//redun_true_succs->erase(std::remove(redun_true_succs->begin(), redun_true_succs->end(), true_succs ), redun_true_succs->end());

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

		//redun_false_succs->erase(std::remove(redun_false_succs->begin(), redun_false_succs->end(), false_succs ), redun_false_succs->end());

	} 


} */

/*

void CircuitGenerator::removeExtraBranches(bool cntrl_br_flag) {
	// temporarily for debugging!!!
	std::ofstream br_succ_file;
	br_succ_file.open("aya_redun_debug.txt");

	// save the enode_dag in a copy so that we can modify it
  

	ENode* producer;
	node_type branch_type;
	if(cntrl_br_flag){
		branch_type = Branch_c;
	} else {
		branch_type = Branch_n;
	} 


	// at a specific enode_1 iteration 
	std::vector<ENode*>* redundant_branches = new std::vector<ENode*>;

	std::vector<ENode*> *to_keep_true_succs, *to_keep_false_succs;

	//for(auto& bbnode : *bbnode_dag) {
		for(auto& enode_1 : *enode_dag) {
			redundant_branches->clear();

			// check if this enode_1 is of type branch and is inside this bbnode
			if(enode_1->type == branch_type) {
				// push it to the redundant_branches, search for redundant branches to this branch and delete it from enode_dag_cpy
				redundant_branches->push_back(enode_1);
				producer = enode_1->producer_to_branch;

				for(auto& enode_2 : *enode_dag) {
					if(enode_2->type == branch_type && enode_2->BB == enode_1->BB && enode_2->producer_to_branch == producer) {
						redundant_branches->push_back(enode_2);
						//enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode_2 ), enode_dag->end()); 
					}
				} 

				//enode_dag_cpy->erase(std::remove(enode_dag_cpy->begin(), enode_dag_cpy->end(), enode_1 ), enode_dag_cpy->end()); 
			} 
			
			// We will keep the first branch ->at(0), so loop over the rest
			if(redundant_branches->size() > 0) {
				if(cntrl_br_flag) {
					//to_keep_true_succs = redundant_branches->at(0)->true_branch_succs_Ctrl;
					//to_keep_false_succs = redundant_branches->at(0)->false_branch_succs_Ctrl;
					to_keep_true_succs = enode_1->true_branch_succs_Ctrl;
					to_keep_false_succs = enode_1->false_branch_succs_Ctrl;

				} else {
					//to_keep_true_succs = redundant_branches->at(0)->true_branch_succs;
					//to_keep_false_succs = redundant_branches->at(0)->false_branch_succs;

					to_keep_true_succs = enode_1->true_branch_succs;
					to_keep_false_succs = enode_1->false_branch_succs;
				} 

			} 
			for(int k = 1; k < redundant_branches->size(); k++) {
				cutBranchPreds(cntrl_br_flag, redundant_branches, k, branch_type);

				// need to connect its true_successors and false_successors to those of the branch to keep 
				cut_keepBranch_trueSuccs(cntrl_br_flag, redundant_branches, k, enode_1, to_keep_true_succs);
			
				cut_keepBranch_falseSuccs(cntrl_br_flag, redundant_branches, k, to_keep_true_succs, to_keep_false_succs);

				enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), redundant_branches->at(k) ), enode_dag->end());
				branch_id--;
				//redundant_branches->at(0)->id = branch_id;

			}
	
			if(redundant_branches->size() > 1) {
				br_succ_file << "\n For branch name: " << getNodeDotNameNew(redundant_branches->at(0)) << " of type " <<  getNodeDotTypeNew(redundant_branches->at(0)) << " in BasicBlock number " << (redundant_branches->at(0)->BB->getName().str().c_str()) << "\n";

				if(cntrl_br_flag) {
					br_succ_file << "\n It has " << redundant_branches->at(0)->true_branch_succs_Ctrl->size() << " TrueBranch_Ctrl Successors and " << redundant_branches->at(0)->false_branch_succs_Ctrl->size() << "FalseBranch_Ctrl Successors\n" ;

				} else {
					br_succ_file << "\n It has " << redundant_branches->at(0)->true_branch_succs->size() << " TrueBranch Successors and " << redundant_branches->at(0)->false_branch_succs->size() << "FalseBranch Successors\n" ;
				}
			} 
		}
	//}

}


*/

/*

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



*/


