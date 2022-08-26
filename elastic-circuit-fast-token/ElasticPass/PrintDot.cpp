#include "ElasticPass/ComponentsTiming.h"
#include "ElasticPass/Memory.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Pragmas.h"

using namespace std;

std::string NEW_LINE("\n");
std::string DHLS_VERSION("0.1.1");

#define BB_MINLEN 3
#define DATA_SIZE 32
#define COND_SIZE 1
#define CONTROL_SIZE 0

#define BRANCH_CONDITION_IN 1
#define MUX_CONDITION_IN 0
#define CMERGE_CONDITION_OUT 1

std::string inputSuffix (ENode* enode, unsigned i);
std::string outputSuffix(ENode* enode, unsigned i);

std::ofstream dotfile;

////////////////////////////// Aya's stuff
void aya_printDotDFG(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string name, std::string serial_number) {
    std::string output_filename = name;

    dotfile.open(output_filename);

    std::string infoStr;

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(std::ctime(&t));

    infoStr = "Digraph G {\n\tsplines=spline;\n";

    // infoStr += "//Graph created: " + date;
    infoStr += "//DHLS version: " + DHLS_VERSION;
    infoStr += "\" [shape = \"none\" pos = \"20,20!\"]\n";

    dotfile << infoStr;

    /* printDotNodes should print the following: 
	- write the basic block number 
	- write the names of all the nodes inside this basic block
    */
    aya_printDotNodes(enode_dag, serial_number, bbnode_dag);
	
	aya_printotherNodes(enode_dag, serial_number);  // print the nodes that are not inside any BB!
	dotfile <<"\n";

    aya_printDotEdges(enode_dag, bbnode_dag);

    dotfile << "\n}";

    dotfile.close();
}

void aya_printDotEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag) {
    dotfile << printMemEdges(enode_dag);
    dotfile << aya_printDataflowEdges(enode_dag, bbnode_dag);
}

// Aya: 21/10/2021: function to take care of printing output edges of a fork.. It handles 
	// the case where a fork has 2 or more successors that are the same operation
std::string aya_printForkEdges_helper(ENode* fork_succ, ENode* fork_enode, int occur_num) {
	string str = "";
	// it must occur at least once
	assert(occur_num > 0);
	int index;
	// search by node id for the fork in the preds of the succs to make sure it's unique 
	for(int j = 0; j < fork_succ->CntrlPreds->size(); j++) {
		ENode* pred_of_succ = fork_succ->CntrlPreds->at(j);
		if(pred_of_succ == fork_enode) {
			occur_num--;
			if(occur_num == 0) {
				index = j;
				break;
			}
		}
	}

	// AYA: 06/06/2022: added the following since SUPPRESS now has the condition always in CntrlPreds even if it is Branch_c!!
	if(fork_succ->type == Branch_c) {
		str += ", to = \"in2\"";
		str += "];\n";
	} else {
		str += ", to = \"in" + to_string(index + 1) + "\"";
		str += "];\n"; 
	}
	

	return str;
}

// Aya: 21/10/2021: function to take care of printing output edges of a fork.. It handles 
	// the case where a fork has 2 or more successors that are the same operation
std::string aya_printForkEdges(ENode* fork_enode) {
	string str = "";
	int occur_num;
	// will initially loop over all CntrlSuccs to identify the indices of repeated CntrlSuccs, if any..
	for (int i = 0; i < fork_enode->CntrlSuccs->size(); i++ ) {
		occur_num = 0;

		ENode* fork_succ = fork_enode->CntrlSuccs->at(i);
		str += printEdge(fork_enode, fork_succ); // print that they are connected
		str += " [";
		str += printColor(fork_enode, fork_succ);

		str += ", from = \"out" + to_string(i + 1) + "\"";

		//////////////////////
		// count which occurrence of this succ are we currently at
		for(int k = 0; k <= i; k++) {
			if(fork_enode->CntrlSuccs->at(k) == fork_succ)
				occur_num++;  // will be 1 if there is a single occurrence
		}
		/////////////////////

		if(fork_succ->type == Inst_) {
			if(isa<LoadInst>(fork_succ->Instr)){
				// load takes input from MC and from another operation, so we are hardcoding the input that comes from the other operation to 2!!!
				//AYa: TODO find a way better than hardcoding the index!!
				str += ", to = \"in2\"";
				str += "];\n";
			} else {
				str += aya_printForkEdges_helper(fork_succ, fork_enode, occur_num);
			}
		} else {
			// AYA: 04/11/2021: Removed what I added on 03/11/2021

			// AYA: 03/11/2021: Added this for correctly printing the condition that is now in the data network even for the control branch!
			//if(fork_succ->type == Branch_c) {
				//str += ", to = \"in2\"";
				//str += "];\n";
			//} else {
				str += aya_printForkEdges_helper(fork_succ, fork_enode, occur_num);
			//}
		} 

	} 

	return str;

		// there are chances that two or more of its successors are 1 node!
				// count the number of occurences of this successor and identify which occurence number is this particular successor node..
				/*std::vector<ENode*>::iterator iter = enode->CntrlSuccs->begin();
				std::vector<ENode*>::iterator current_succ_pos = succ_index + enode->CntrlSuccs->begin();
				occur_time = 1;
				int ii = 0;
				// check how many times this succ exists in the CNtrlSucc before current_succ_pos, if any.. 
				while(iter!= enode->CntrlSuccs->end() && iter != current_succ_pos && ii < enode->CntrlSuccs->size()) {
					iter = std::find(iter, enode->CntrlSuccs->end(), enode_succ);
					if(iter != enode->CntrlSuccs->end()) {
						if(occur_time > 1)
							many_occur = true; 
						occur_time++;
					} 
					ii++;
				}
				if(ii > 0)   // i.e., if you entered the while loop
					occur_time--;

				str += ", from = \"out" + to_string(indexOf(enode->CntrlSuccs, enode_succ) + occur_time) + "\"";
				*/
}

std::string aya_printDataflowEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag) {
	
	// TEMP FOR DEBUGGING!!!
	std::ofstream g_file;
	g_file.open("aya_FORKSS_MULSS.txt");
	
	/*
		IMP Note: A single fork enode can possibly have successors in the data path and 		others in the control path. But it is a single enode so we need to make sure that 			the out indices are increasing to prevent the case that two successors are mapped 			to out2 for instance.  
	*/ 

    string str = "";
	int fork_data_index, fork_control_index, index_to_count_from;

	// loop over all enodes 
	for (auto& enode : *enode_dag) {
		//if(enode->is_redunCntrlNet)
			//continue;

		// AYa: 21/10/2021: prints only the CntrlSuccs of a fork.
			// it doesn't do anything for JustCntrlSuccs and CntrlOrderSuccs, they are still handled in the body of this function as originally!!
		if(enode->type == Fork_) {
			string fork_print = aya_printForkEdges(enode);
			str += fork_print;
		}

		fork_data_index = -1;
		fork_control_index = -1;
		index_to_count_from = 0;
		

		// loop over all the data path successors of one enode	
 		for (auto& enode_succ : *(enode->CntrlSuccs)) {
			// exclude all the memory operations since we handle them in the previous function
			if(enode_succ->type != LSQ_ && enode_succ->type != MC_ && enode->type != LSQ_ && enode->type != MC_ && enode->type != Fork_) {
				str += printEdge(enode, enode_succ); // print that they are connected

				str += " [";
				str += printColor(enode, enode_succ);

				// phis that are cntrlmerges in the redundant control network will be having 1 successor in the CntrlSuccs and another successor in the CntrlOrderSuccs, we should increment the index in printing to assure  
				if(enode->type == Phi_c && enode->is_redunCntrlNet) {
					// sanity check: this enode should be having a single successor in the CntrlSuccs and a single successor in the enode->CntrlOrderSuccs

					// AYA: TEMPORARILY COMMENTING THIS FOR DEBUGGING!! 16/11/2021

					//assert(enode->CntrlSuccs->size() == 1 && enode->CntrlOrderSuccs->size() == 1);
					// the convention is that the CntrlOrderSuccs is printed at out1 and the CntrlSuccs at out2, we need here to enforce that of CntrlSuccs but that of CntrlOrderSuccs will happen naturally without any extra condition..
					// TODO: reach for this in a way better than hardcoding!!
					str += ", from = \"out2\"";

				} else {
					// The assumption is that predecessors and successors are placed at the correct indices, which output it is comes from the position of the successor in the node's CntrlSuccs array
					// AYA: 16/10/2021: But there are cases where a single fork feeds the 2 operands of an operation (eg. x*x, the multiplier will get two edges from the fork)
					//if(enode->type == Fork_) {
					//} else {
						str += ", from = \"out" + to_string(indexOf(enode->CntrlSuccs, enode_succ) + 1) + "\"";
					//} 

				} 

//////////////////////////////////////////////////////////////////////////////////////////	
				//if(enode_succ->isLoadOrStore()) {
				if(enode_succ->type == Inst_) {
					if(isa<LoadInst>(enode_succ->Instr)){

						// load takes input from MC and from another operation, so we are hardcoding the input that comes from the other operation to 2!!!
						//AYa: TODO find a way better than hardcoding the index!!
						str += ", to = \"in2\"";
						str += "];\n";
					} else {
						// which input it is comes from the position of the the node in the successor's CntrlPreds array
						str += ", to = \"in" + to_string(indexOf(enode_succ->CntrlPreds, enode) + 1) + "\"";
						str += "];\n";
					}
				} else {

					//AYa: TODO find a way better than hardcoding the index for the case where the successor is the end node!! 
					if(enode_succ->type == End_) {
						// check if there are any predecessor(s) of this End_ node in the CntrlOrder network (i.e., if there is a MC_, LSQ_ or both or none at all!!).. Then there must be either a single predecessor to this End_ in the CntrlPreds or in the JustCntrlPreds depending on whether the function returns data or not!!

						// as a sanity check: this End_ node shouldm't be having any JustCntrlPreds!!
						assert(enode_succ->JustCntrlPreds->size() == 0);

						str += ", to = \"in" + to_string(enode_succ->CntrlOrderPreds->size() + 1) + "\"";
						str += "];\n";
						
					} else {
						// AYA: 04/11/2021: Removed what I added on 03/11/2021!!

						// AYA: added this since even the control Branches should have their condition in the data network, so it should be forced to be in2!!
						if(enode_succ->type == Branch_c) {
							str += ", to = \"in2\"";
							str += "];\n";

						} else {
							// which input it is comes from the position of the the node in the successor's CntrlPreds array
							str += ", to = \"in" + to_string(indexOf(enode_succ->CntrlPreds, enode) + 1) + "\"";
							str += "];\n";

						}
	
					} 

				} 
//////////////////////////////////////////////////////////////////////////////////////

			}

		}

		// if it is a fork and has data successors, save the index of those data successors to update it for control successors of that fork, if any.. 
		if(enode->type == Fork_) {
			if(enode->CntrlSuccs->size() > 0) {
				fork_data_index = enode->CntrlSuccs->size();
			}
		}  

		// loop over all the control path successors of one enode
 		for (auto& enode_cntrl_succ : *(enode->JustCntrlSuccs)) {
			str += printEdge(enode, enode_cntrl_succ); // print that they are connected

            str += " [";
            str += printColor(enode, enode_cntrl_succ);
// The assumption is that predecessors and successors are placed at the correct indices
			// which output it is comes from the position of the successor in the node's JustCntrlSuccs array
			if(enode->type == Fork_ && fork_data_index != -1) {
				// this means that this fork has both data and control successors so should increment the data index for connecting the control successor
				fork_data_index ++; 
				str += ", from = \"out" + to_string(fork_data_index) + "\"";
			} else {
				 str += ", from = \"out" + to_string(indexOf(enode->JustCntrlSuccs, 					enode_cntrl_succ) + 1) + "\"";
			} 


//////////////////////////////////////////////////////////////////////
			//AYa: TODO find a way better than hardcoding the index for the case where the successor is the end node!! 
			if(enode_cntrl_succ->type == End_) {
				// check if there are any predecessor(s) of this End_ node in the CntrlOrder network (i.e., if there is a MC_, LSQ_ or both or none at all!!).. Then there must be either a single predecessor to this End_ in the CntrlPreds or in the JustCntrlPreds depending on whether the function returns data or not!!

				// as a sanity check: this End_ node shouldn't be having any CntrlPreds!!
				assert(enode_cntrl_succ->CntrlPreds->size() == 0);

				str += ", to = \"in" + to_string(enode_cntrl_succ->CntrlOrderPreds->size() + 1) + "\"";
				str += "];\n";
				
			} else {
			// Aya: 08/10/2021: Added the following condition to correctly increment the inputs of the inputs of getelementptr node which has some inputs in the data network and others in the JustCntrl network

				if(enode_cntrl_succ->type == Inst_) {
					if(enode_cntrl_succ->Instr->getOpcode() == Instruction::GetElementPtr) {
						str += ", to = \"in" + to_string(indexOf(enode_cntrl_succ->JustCntrlPreds, enode) + enode_cntrl_succ->CntrlPreds->size() + 1) + "\"";
						str += "];\n";

					} else {
						// for any other instruction, do the usual thing
						str += ", to = \"in" + to_string(indexOf(enode_cntrl_succ->JustCntrlPreds, enode) + 1) + "\"";
						str += "];\n";
					} 
				} else {
					if(enode_cntrl_succ->type == Phi_c && !enode_cntrl_succ->is_redunCntrlNet) {
						str += ", to = \"in" + to_string(indexOf(enode_cntrl_succ->JustCntrlPreds, enode) + enode_cntrl_succ->CntrlPreds->size() + 1) + "\"";
						str += "];\n";
					} else {
					// which input it is comes from the position of the the node in the successor's JustCntrlPreds array
						str += ", to = \"in" + to_string(indexOf(enode_cntrl_succ->JustCntrlPreds, enode) + 1) + "\"";
						str += "];\n";
					}

				} 

			} 

/////////////////////////////////////////////////////////////////////
           
		}

///////////////////////////////////////////////////// NEW REDUNDANT CONTROL NETWORK STUFF!!!
		fork_data_index = -1; // to avoid confusion with the value assigned above!
		fork_control_index = -1;
		if(enode->type == Fork_) {
			if(enode->CntrlSuccs->size() > 0) {
				fork_data_index = enode->CntrlSuccs->size();
			}

			if(enode->JustCntrlSuccs->size() > 0) {
				fork_control_index = enode->JustCntrlSuccs->size();
			}
		} else {
			if(enode->type == Fork_c) {
				if(enode->JustCntrlSuccs->size() > 0) {
					fork_control_index = enode->JustCntrlSuccs->size();
				}
			}
		}

		// loop over all the new control path successors of one enode
 		for (auto& enode_redun_cntrl_succ : *(enode->CntrlOrderSuccs)) {
			if ( (enode_redun_cntrl_succ->type != LSQ_ && enode_redun_cntrl_succ->type != MC_ && enode->type != LSQ_ && enode->type != MC_) ||
					 (enode->type == Fork_c && enode_redun_cntrl_succ->type == LSQ_) ) {
				str += printEdge(enode, enode_redun_cntrl_succ); // print that they are connected
				str += " [";
				str += printColor(enode, enode_redun_cntrl_succ);
				// The assumption is that predecessors and successors are placed at the correct indices
				// which output it is comes from the position of the successor in the node's JustCntrlSuccs array
				index_to_count_from = fork_data_index + fork_control_index; 
				if(enode->type == Fork_ && fork_data_index != -1 && fork_control_index != -1) {
					// this means that this fork has both data and control successors so should increment the data index for connecting the control successor
					index_to_count_from++; 
					str += ", from = \"out" + to_string(index_to_count_from) + "\"";
				} else {
					// Aya: 03/10/2021: to handle the case of a Fork_ having data preds as well as CntrlOrderPreds but no JustCntrlPreds
					if(enode->type == Fork_ && fork_data_index != -1) {
						fork_data_index++; 
						str += ", from = \"out" + to_string(fork_data_index) + "\"";
					} else {
						if(enode->type == Fork_c && fork_control_index != -1){
							fork_control_index ++; 
							str += ", from = \"out" + to_string(fork_control_index) + "\"";
						} else {
							str += ", from = \"out" + to_string(indexOf(enode->CntrlOrderSuccs, enode_redun_cntrl_succ) + 1) + "\"";
						}


					}

				} 


				// which input it is comes from the position of the the node in the successor's JustCntrlPreds array
				str += ", to = \"in" + to_string(indexOf(enode_redun_cntrl_succ->CntrlOrderPreds, enode) + 1) + "\"";
				str += "];\n";

			}

		}
////////////////////////////////////////////////// END OF THE NEW REDUNDANT CONTROL NETWORK STUFF!!!

	}
    return str;
}

void aya_printotherNodes(std::vector<ENode*>* enode_dag, std::string serial_number) {
// Target is to loop over basic blocks and print the operations existing in each basic block
	string dotline = "";

	// loop over basic blocks 
	//for (auto& bnd : *bbnode_dag) {
		// loop over enodes searching for those inside the current basic block
		for (auto& enode : *enode_dag) {
			// print only nodes in the same basic block as the current one
			// AYA: ADDED an extra part to the condition to not print nodes involved in the new network!!
			if(!skipNodePrint(enode) && enode->BB == nullptr){
				dotline += "\t";
				dotline += getNodeDotNameNew(enode);
				dotline += " [";
				dotline += getNodeDotTypeNew(enode);

				// AYA: 07/11/2021: Such nodes will be given an imaginary bbID of 0
				dotline += "\", ";
				dotline += "bbID= 0";
				//dotline += aya_getNodeDotbbID(bnd);

				if (enode->type == Inst_)
					dotline += getNodeDotOp(enode);

				////////////////////
				if (OptimizeBitwidth::isEnabled()) {
					dotline += getNodeDotInputs_withOB(enode);
					dotline += getNodeDotOutputs_withOB(enode);
	
				}else {
					dotline += getNodeDotInputs(enode);
					dotline += getNodeDotOutputs(enode);
				}
				/////////////////////

				dotline += getNodeDotParams(enode, serial_number);
				if (enode->type == MC_ || enode->type == LSQ_)
					dotline += getNodeDotMemParams(enode);
				dotline += "];\n";
	
			}
		}
	
		//assert(666 != 666);	
	//dotline += "\n\t}\n"; // braces to close the cluster, but here we have no clusters
	dotfile << dotline;  
}

void aya_printDotNodes(std::vector<ENode*>* enode_dag, std::string serial_number, std::vector<BBNode*>* bbnode_dag) {
// Target is to loop over basic blocks and print the operations existing in each basic block
	string dotline = "";
	int count = 0;

	// loop over basic blocks 
	for (auto& bnd : *bbnode_dag) {
		//llvm::BasicBlock* branchSucc1;
		//bool unconditional = false;
		dotline += "\n\tsubgraph cluster_" + to_string(count) + " {\n"; // draw cluster around BB
		dotline += "\tcolor = \"darkgreen\";\n";

		// AYA: 10/02/2020: added the following to print virtualBBs in case of debugging
		//if(bnd->BB == nullptr)
			//dotline += "\t\tlabel = \"virtBB\";\n";
		//else
		dotline += "\t\tlabel = \"" + string(bnd->BB->getName().str().c_str()) + "\";\n";

		// loop over enodes searching for those inside the current basic block
		for (auto& enode : *enode_dag) {
			// print only nodes in the same basic block as the current one
			
			if((enode->BB == bnd->BB) && !skipNodePrint(enode)){

				dotline += "\t\t";
				dotline += getNodeDotNameNew(enode);
				dotline += " [";
				dotline += getNodeDotTypeNew(enode);
				dotline += "\", ";
				dotline += aya_getNodeDotbbID(bnd);
				if (enode->type == Inst_)
					dotline += getNodeDotOp(enode);
////////////////////
				if (OptimizeBitwidth::isEnabled()) {
					dotline += getNodeDotInputs_withOB(enode);
					dotline += getNodeDotOutputs_withOB(enode);

				}else {
					dotline += getNodeDotInputs(enode);
					dotline += getNodeDotOutputs(enode);
				}
/////////////////////
				dotline += getNodeDotParams(enode, serial_number);
				if (enode->type == MC_ || enode->type == LSQ_)
					dotline += getNodeDotMemParams(enode);
				dotline += "];\n";
				
			}
		}
		count += 1;
		dotline += "\n\t}\n"; // put braces to close the cluster
	}
	dotfile << dotline;
    
}

std::string aya_getNodeDotbbID(BBNode* bbnode) {
    string name = "bbID= ";
    name += to_string(bbnode->Idx + 1);
    return name;
}


/////////////////////////////////////////////////////////////// END OF AYA'S STUFF

// type of int comparison, obtained from llvm instruction
std::string getIntCmpType(ENode* enode) {

    int instr_type = dyn_cast<CmpInst>(enode->Instr)->getPredicate();
    std::string s  = "";

    switch (instr_type) {
        case CmpInst::ICMP_EQ:
            s = "icmp_eq";
            break;

        case CmpInst::ICMP_NE:
            s = "icmp_ne";
            break;

        case CmpInst::ICMP_UGT:
            s = "icmp_ugt";
            break;

        case CmpInst::ICMP_UGE:
            s = "icmp_uge";
            break;

        case CmpInst::ICMP_ULT:
            s = "icmp_ult";
            break;

        case CmpInst::ICMP_ULE:
            s = "icmp_ule";
            break;

        case CmpInst::ICMP_SGT:
            s = "icmp_sgt";
            break;

        case CmpInst::ICMP_SGE:
            s = "icmp_sge";
            break;

        case CmpInst::ICMP_SLT:
            s = "icmp_slt";
            break;

        case CmpInst::ICMP_SLE:
            s = "icmp_sle";
            break;

        default:
            s = "icmp_arch";
            break;
    }
    return s;
}

// type of float comparison, obtained from llvm instruction
std::string getFloatCmpType(ENode* enode) {

    int instr_type = dyn_cast<CmpInst>(enode->Instr)->getPredicate();
    std::string s  = "";

    switch (instr_type) {

        case CmpInst::FCMP_OEQ:
            s = "fcmp_oeq";
            break;

        case CmpInst::FCMP_OGT:
            s = "fcmp_ogt";
            break;

        case CmpInst::FCMP_OGE:
            s = "fcmp_oge";
            break;

        case CmpInst::FCMP_OLT:
            s = "fcmp_olt";
            break;

        case CmpInst::FCMP_OLE:
            s = "fcmp_ole";
            break;

        case CmpInst::FCMP_ONE:
            s = "fcmp_one";
            break;

        case CmpInst::FCMP_ORD:
            s = "fcmp_ord";
            break;

        case CmpInst::FCMP_UNO:
            s = "fcmp_uno";
            break;

        case CmpInst::FCMP_UEQ:
            s = "fcmp_ueq";
            break;

        case CmpInst::FCMP_UGE:
            s = "fcmp_uge";
            break;

        case CmpInst::FCMP_ULT:
            s = "fcmp_ult";
            break;

        case CmpInst::FCMP_ULE:
            s = "fcmp_ule";
            break;

        case CmpInst::FCMP_UNE:
            s = "fcmp_une";
            break;

        case CmpInst::FCMP_UGT:
            s = "fcmp_ugt";
            break;

        default:
            s = "fcmp_arch";
            break;
    }

    return s;
}

bool isLSQport(ENode* enode) {
    for (auto& succ : *(enode->CntrlSuccs))
        if (succ->type == LSQ_)
            return true;
    return false;
}

// check llvm instruction operands to connect enode with instruction successor
int getOperandIndex(ENode* enode, ENode* enode_succ) {
    const Value* V;

    // force load address from predecessor to port 2
    if (enode_succ->Instr->getOpcode() == Instruction::Load && enode->type != MC_ &&
        enode->type != LSQ_)
        return 2;

    // purposefully not in if-else because of issues with dyn_cast
    if (enode->Instr != NULL)
        V = dyn_cast<Value>(enode->Instr);
    if (enode->A != NULL)
        V = dyn_cast<Value>(enode->A);
    if (enode->CI != NULL)
        V = dyn_cast<Value>(enode->CI);
    if (enode->CF != NULL)
        V = dyn_cast<Value>(enode->CF);

    // for getelementptr, use preds ordering because operands can have identical values
    if (enode_succ->Instr->getOpcode() == Instruction::GetElementPtr)
        return indexOf(enode_succ->CntrlPreds, enode) + 1;

    // if both operands are constant, values could be the same
    // return index in predecessor array
    if (enode->CI != NULL || enode->CF != NULL) {
        if (enode_succ->CntrlPreds->size() >= 2)
            if (enode_succ->Instr->getOperand(0) == enode_succ->Instr->getOperand(1))
                return indexOf(enode_succ->CntrlPreds, enode) + 1;
    }

    for (int i = 0; i < (int)enode_succ->CntrlPreds->size(); i++)
        if (V == enode_succ->Instr->getOperand(i)) {
            return i + 1;
        }

    // special case for store which predecessor not found
    if (enode_succ->Instr->getOpcode() == Instruction::Store)
        return 2;

    return 1;
}

bool skipNodePrint(ENode* enode) {
    if (enode->type == Argument_ && enode->CntrlSuccs->size() == 0)
        return true;
    if ((enode->type == Branch_n) || (enode->type == Branch_c))
        if (enode->CntrlSuccs->size() == 1)
            if (enode->CntrlSuccs->front()->type == Fork_ ||
                enode->CntrlSuccs->front()->type == Branch_ ||
                enode->CntrlSuccs->front()->type == Branch_n)
                return true;
    if (enode->type == Branch_ && enode->JustCntrlPreds->size() != 1)
        return true;
    if (enode->type == MC_) // enode->memPort
        return false;
    return false;
}

int getInPortSize(ENode* enode, int index) {

    if (enode->type == Argument_) {
        return DATA_SIZE;
    } else if (enode->type == Branch_n) {
        return (index == 0) ? DATA_SIZE : COND_SIZE;
    } else if (enode->type == Branch_c) {
        return (index == 0) ? CONTROL_SIZE : COND_SIZE;
    } else if (enode->type == Branch_) {
        return COND_SIZE;

    } else if (enode->type == Inst_) {
        if (enode->Instr->getOpcode() == Instruction::Select && index == 0)
            return COND_SIZE;
        else if (enode->Instr->getOpcode() == Instruction::GetElementPtr)
            return DATA_SIZE;
        else
            return (enode->JustCntrlPreds->size() > 0) ? CONTROL_SIZE : DATA_SIZE;
    } else if (enode->type == Fork_) {
        return (enode->CntrlPreds->front()->type == Branch_ ||
                (enode->CntrlPreds->front()->isCntrlMg &&
                 enode->CntrlPreds->front()->type == Phi_c))
                   ? COND_SIZE
                   : DATA_SIZE;

    } else if (enode->type == Fork_c) {
        return CONTROL_SIZE;
    } else if (enode->type == Buffera_ || enode->type == Bufferi_ || enode->type == Fifoa_ ||
               enode->type == Fifoi_) {
        return enode->CntrlPreds->size() > 0 ? DATA_SIZE : CONTROL_SIZE;

    } else if (enode->type == Phi_ || enode->type == Phi_n) {
        return (index == 0 && enode->isMux) ? COND_SIZE : DATA_SIZE;

// AYA: Added a condition to handle the case of an irredundant Phi_c becoming a mux
    } else if (enode->type == Phi_c) {
		if(enode->is_redunCntrlNet) {
			return CONTROL_SIZE;
		} else {
			return (index == 0 && enode->isMux) ? COND_SIZE : CONTROL_SIZE;
		}
       // return CONTROL_SIZE;
    } else if (enode->type == Cst_) {
        return DATA_SIZE;
    } else if (enode->type == Start_) {
        return CONTROL_SIZE;
    } else if (enode->type == Sink_ || enode->type == End_) {
        return enode->CntrlPreds->size() > 0 ? DATA_SIZE : CONTROL_SIZE;
    } else if(enode->type == Inj_n) {
		return (index == 0) ? COND_SIZE : DATA_SIZE;
	}
}

int getOutPortSize(ENode* enode, int index) {
    if (enode->type == Inst_) {
        if (enode->Instr->getOpcode() == Instruction::ICmp ||
            enode->Instr->getOpcode() == Instruction::FCmp)
            return COND_SIZE;
        else if (enode->Instr->getOpcode() == Instruction::GetElementPtr)
            return DATA_SIZE;
        else
            return (enode->JustCntrlPreds->size() > 0) ? CONTROL_SIZE : DATA_SIZE;
    } else if (enode->type == Source_) {
        return (enode->CntrlSuccs->size() > 0 ? DATA_SIZE : CONTROL_SIZE);
    } else if (enode->type == Phi_ || enode->type == Phi_n) {
        return DATA_SIZE;
    } else {
        return getInPortSize(enode, 0);
    }
}
unsigned int getConstantValue(ENode* enode) {

    if (enode->CI != NULL)
        return enode->CI->getSExtValue();
    else if (enode->CF != NULL) {
        ConstantFP* constfp_gv = llvm::dyn_cast<llvm::ConstantFP>(enode->CF);
        float gv_fpval         = (constfp_gv->getValueAPF()).convertToFloat();
        return *(unsigned int*)&gv_fpval;
    } else
        return enode->cstValue;
}

void printDotCFG(std::vector<BBNode*>* bbnode_dag, std::string name) {

    std::string output_filename = name;

    dotfile.open(output_filename);

    std::string s;

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(std::ctime(&t));

    s = "Digraph G {\n\tsplines=spline;\n";

    // infoStr += "//Graph created: " + date;
    s += "//DHLS version: " + DHLS_VERSION;
    s += "\" [shape = \"none\" pos = \"20,20!\"]\n";

    for (auto& bnd : *bbnode_dag) {
        s += "\t\t\"";
		// AYA: 10/02/2022: Added the following check to be able to print the virtualBBs (I do so only for debugging because in the end I delete the virtualBBs!!)
		//if(bnd->BB == nullptr)
			//s+= "virtBB";
		//else
    	s += bnd->BB->getName().str().c_str();
        s += "\";\n";
    }

    for (auto& bnd : *bbnode_dag) {
        for (auto& bnd_succ : *(bnd->CntrlSuccs)) {
            s += "\t\t\"";
			// AYA: 10/02/2022: 
			//if(bnd->BB == nullptr)
				//s+= "virtBB";
			//else
        	s += bnd->BB->getName().str().c_str();

            s += "\" -> \"";

			// AYA: 10/02/2022: 
			//if(bnd_succ->BB == nullptr)
				//s+= "virtBB";
			//else
        	s += bnd_succ->BB->getName().str().c_str();

            s += "\" [color = \"";
            // back edge if BB succ has id smaller or equal (self-edge) to bb
            s += (bnd_succ->Idx <= bnd->Idx) ? "red" : "blue";
            s += "\", freq = ";
            // freq extracted through profiling
			// AYA: 10/02/2022: 
			int freq;
			//if(bnd_succ->BB == nullptr)
				//freq = bnd->get_succ_freq("virtBB");
			//else
        	freq = bnd->get_succ_freq(bnd_succ->BB->getName());

            s += to_string(freq);
            s += "];\n";
        }
    }

    dotfile << s;
    dotfile << "}";

    dotfile.close();
}

std::string getNodeDotNameNew(ENode* enode) {
    string name = "\"";
    switch (enode->type) {
        case Branch_: // treat llvm branch as constant
            name += "brCst_";
            name += enode->BB->getName().str().c_str();
            break;
        case LSQ_:
            name += "LSQ_";
            name += enode->Name;
            break;
        case MC_:
            name += "MC_";
            name += enode->Name;
            break;
        case Cst_:
            name += "cst_";
            name += to_string(enode->id);
            break;
        case Argument_:
            name += enode->argName.c_str();
            break;
        case Phi_n:
		// AYA: 25/02/2022: added the following case to account for the token injector
		case Inj_n:
            name += enode->Name;
            name += "_n";
            name += to_string(enode->id);
            break;
		
        default:
            name += enode->Name;
            name += "_";
            name += to_string(enode->id);
            break;
    }
    name += "\"";
    return name;
}

std::string getNodeDotTypeNew(ENode* enode) {
    string name = "type = \"";

    switch (enode->type) {
        case Inst_:
            name += "Operator";
            break;
        case Phi_:
        case Phi_n:
            if (enode->isMux)
                name += "Mux";
            else
                name += "Merge";
            break;
        case Phi_c:
            if (enode->isCntrlMg)
                name += "CntrlMerge";
            else {
				if (enode->isMux) {
					assert(!enode->is_redunCntrlNet);
					name += "Mux";
				} else {
					name += "Merge";
				}
			} 
            break;
        case Fork_:
        case Fork_c:
            name += "Fork";
            break;
        case Buffera_:
        case Bufferi_:
            name += "Buffer";
            break;
        case Fifoa_:
        case Fifoi_:
            name += "Fifo";
            break;
        case Start_:
            name += "Entry\", control= \"true";
            break;
        case Source_:
            name += "Source";
            break;
        case Sink_:
            name += "Sink";
            break;
        case End_:
            name += "Exit";
            break;
        case LSQ_:
            name += "LSQ";
            break;
        case MC_:
            name += "MC";
            break;
        case Cst_:
            name += "Constant";
            break;
        case Argument_:
            name += "Entry";
            break;
        case Branch_n:
        case Branch_c:
            name += "Branch";
            break;
        case Branch_: // treat llvm branch as constant
            name += "Constant";
            break;
		// AYA: 25/02/2022: added the following case to account for the token injector
		case Inj_n:
			name += "Inj";
			break;
        default:
            break;
    }
	// Aya:
    //name += "\", ";
    return name;
}

std::string getNodeDotbbID(ENode* enode) {
    string name = "bbID= ";
    name += to_string(getBBIndex(enode));
    return name;
}

std::string getFuncName(ENode* enode) {
    Instruction* I  = enode->Instr;
    StringRef fname = cast<CallInst>(I)->getCalledFunction()->getName();
    return fname.str();
}

std::string getNodeDotOp(ENode* enode) {
    string name = ", op = \"";
    switch (enode->Instr->getOpcode()) {
        case Instruction::ICmp:
            name += getIntCmpType(enode);
            name += "_op\"";
            break;
        case Instruction::FCmp:
            name += getFloatCmpType(enode);
            name += "_op\"";
            break;
        case Instruction::Load:
            name += (isLSQport(enode) ? "lsq_load" : "mc_load");
            name += "_op\",";
			// Aya commented this
            name += "bbID= " + to_string(getBBIndex(enode));
            name += ", portId= " + to_string(getMemPortIndex(enode));
            name += ", offset= " + to_string(getBBOffset(enode));
            break;
        case Instruction::Store:
            name += (isLSQport(enode) ? "lsq_store" : "mc_store");
            name += "_op\",";
			// Aya commented this
            name += "bbID= " + to_string(getBBIndex(enode));
            name += ", portId= " + to_string(getMemPortIndex(enode));
            name += ", offset= " + to_string(getBBOffset(enode));
            break;
        case Instruction::Call:
            name += enode->Name;
            name += "_op\"";
            name += ", function = \"" + getFuncName(enode) + "\" ";
            break;
        default:
            name += enode->Name;
            name += "_op\"";
            break;
    }

    return name;
}

std::string inputSuffix (ENode* enode, int i) {
    if (i == 0 && enode->isMux)
        return "?";
    if (enode->type == Inst_)
        if (enode->Instr->getOpcode() == Instruction::Select) {
            if (i == 0)
                return "?";
            if (i == 1)
                return "+";
            if (i == 2)
                return "-";
        }
    return "";
}

/*
Aya: the following function is for printing the input operands of different operations including those that are part of the new control network

	Note that the only operations involved in the control graph are:
		Phi_c, Branch_c, Cst_, Sink_, Fork_, Fork_c, Bufferi_, Buffera_, Start_
			NOT SURE ABOUT Fifoi_ AND Fifoa_????????

	But, need to do something special for the following types: Fork_, Fork_c and Start_ since they could potenitally have succs or preds in the other graphs not just in the new redundant control graph!!! BUT, ASSUMING THAT before adding the control network, Start_ is always connected to at least a single constant, therefore, a fork_c will be added and everything in the redundant control network will be connected to the CntrlOrderSuccs of Fork_c not Start_!!! 	

*/

std::string getNodeDotInputs(ENode* enode) {
    string name = "";

    switch (enode->type) {
        case Argument_:
        case Branch_:
        case Fork_:
        case Fork_c:
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Start_:
        case Sink_:
            name += ", in = \"in1:" + to_string(getInPortSize(enode, 0)) + "\"";
            break;

		// AYA: 25/02/2022: added the following case to account for the token injector
		case Inj_n:
			name += ",  in = \"in1?:" + to_string(getInPortSize(enode, 0));
			name += " in2:" + to_string(getInPortSize(enode, 1));
			break;

        case Branch_n:
        case Branch_c:
            name += ",  in = \"in1:" + to_string(getInPortSize(enode, 0));

			// AYA: 24/02/2022: added the following to account for negated inputs
			if(enode->is_advanced_component) {
				// check if the first input of the branch should be negated!
				if(enode->is_negated_input->at(0)) {
					name += "*i";
				}
			} 
			///////////////////////////////////////////////////			

            name += " in2?:" + to_string(getInPortSize(enode, 1));

			// AYA: 24/02/2022: added the following to account for negated inputs
			if(enode->is_advanced_component) {
				// check if the first input of the branch should be negated!
				if(enode->is_negated_input->at(1)) {
					name += "*i";
				}
			} 
			///////////////////////////////////////////////////
			name += "\"";

            break;

        case Inst_:
        case Phi_:
        case Phi_n:
        case Phi_c:
			// check first, if the node is Phi_c and is part of the redundant control graph, then all the conditions should be on CntrlOrderPreds

			// Aya: 27/09/2021 added the following condition to deal with the case where we trigger a return instruction through the redunCntrlNetwork in case the function returns to void
			if( (enode->type == Phi_c && enode->is_redunCntrlNet) || (enode->type == Inst_ && enode->Name.compare("ret") == 0 && enode->CntrlPreds->size() == 0) ) {
					// Aya added this line!!
	 			if (enode->CntrlOrderPreds->size() > 0)
		             name += ", in = \"";
		        // name += ", in = \"";

		        for (int i = 0; i < (int)enode->CntrlOrderPreds->size(); i++) {
		            name += "in" + to_string(i + 1);
		            name += inputSuffix(enode, i);
		            name += ":";
		            name += to_string(getInPortSize(enode, i)) + " ";
		        }
		      
				// Aya added this line			
				if (enode->CntrlOrderPreds->size() > 0 )
		             name += "\"";
		       // name += "\"";

			} else {
					// Aya added this line!!
	 			if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0)
		             name += ", in = \"";
		        // name += ", in = \"";

				// AYA: 24/02/2022: I added the following condition to check which loop to go to to account for the case of "advanced_components" that could have negated inputs!
				if(enode->is_advanced_component) {
					for (int i = 0; i < (int)enode->CntrlPreds->size(); i++) {
				        name += "in" + to_string(i + 1);
				        name += inputSuffix(enode, i);
				        name += ":";
				        name += to_string(getInPortSize(enode, i));
				
						if(enode->is_negated_input->at(i)) {
							name += "*i";
						}

						name += " ";
		        	}

				} else {

					// do the original loop as was there before!
			       for (int i = 0; i < (int)enode->CntrlPreds->size() + (int)enode->JustCntrlPreds->size(); i++) {
				        name += "in" + to_string(i + 1);
				        name += inputSuffix(enode, i);
				        name += ":";
				        name += to_string(getInPortSize(enode, i)) + " ";
		        	}

				} 

		        if (enode->type == Inst_)
		            if (enode->Instr->getOpcode() == Instruction::Load)
		                name += "in2:32";

				// Aya added this line			
				if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0 )
		             name += "\"";
		       // name += "\"";

			} 
		
            break;

        case LSQ_:
        case MC_:
            name += ", in = \"";

			// Aya commented this and changed it to CntrlOrderPreds
           /* for (int i = 0; i < (int)enode->JustCntrlPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }
		  */
			// Aya did this for the new redundant control network instead of using JustCntrlPreds!!
			for (int i = 0; i < (int)enode->CntrlOrderPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }

            for (auto& pred : *enode->CntrlPreds) {
                if (pred->type != Cst_ && pred->type !=LSQ_) {

                    name +=
                        "in" + to_string(getDotAdrIndex(pred, enode)) + ":" + to_string(ADDR_SIZE);
                    name += (pred->Instr->getOpcode() == Instruction::Load) ? "*l" : "*s";
                    name += to_string(pred->memPortId) + "a ";

                    if (pred->Instr->getOpcode() == Instruction::Store) {
                        name += "in" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                to_string(DATA_SIZE);
                        name += "*s" + to_string(pred->memPortId) + "d ";
                    }
                }
                if (pred->type == LSQ_) {
                	int inputs = getMemInputCount(enode);
                	int pred_ld_id = pred->lsqMCLoadId;
                    int pred_st_id = pred->lsqMCStoreId;
                    name += "in" + to_string (inputs + 1) + ":" + to_string(ADDR_SIZE) + "*l" + to_string(pred_ld_id) + "a ";
                    name += "in" + to_string (inputs + 2) + ":" + to_string(ADDR_SIZE)+ "*s" + to_string(pred_st_id) + "a ";
                    name += "in" + to_string (inputs + 3) + ":" + to_string(DATA_SIZE)+ "*s" + to_string(pred_st_id) + "d ";

                }
            }
            if (enode->type == LSQ_ && enode->lsqToMC == true) {
            	int inputs = getMemInputCount(enode);
                name += "in" + to_string (inputs + 1) + ":" + to_string(DATA_SIZE) + "*x0d ";
            }
            name += "\"";
            break;
        case End_:

            name += ", in = \"";
			for (auto& pred : *enode->CntrlOrderPreds) {
                auto it =
                    std::find(enode->CntrlOrderPreds->begin(), enode->CntrlOrderPreds->end(), pred);
                int ind = distance(enode->CntrlOrderPreds->begin(), it);
                name += "in" + to_string(ind + 1) + ":" + to_string(CONTROL_SIZE);

                // if its not memory, it is the single control port
                name += (pred->type == MC_ || pred->type == LSQ_) ? "*e " : "";
            }


            for (auto i = enode->CntrlOrderPreds->size();
                 i < (enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size());
                 i++) {
                 name += "in" + to_string(i + 1) + ":" + to_string(CONTROL_SIZE) + " ";
                // if its not memory, it is the single control port
				// AYA COMMENTED THE FOLLOWING SINCE I CONNECT THE MC_ OR LSQ_ OVER THE NEW REDUNDANT CONTROL NETWORK!!
                // name += (pred->type == MC_ || pred->type == LSQ_) ? "*e " : "";
            }

            for (auto i = enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size();
                 i < (enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size() + enode->CntrlPreds->size());
                 i++) // if data ports, count from zero
                name += "in" + to_string(i + 1) + ":" + to_string(DATA_SIZE) + " ";
            name += "\"";
            break;
        default:
            break;
    }

    return name;
}

std::string getNodeDotOutputs(ENode* enode) {
    string name = "";

    switch (enode->type) {
        case Argument_:
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Start_:
        case Branch_:
        case Phi_:
        case Phi_n:
        case Source_:
		case Inj_n:
            name += ", out = \"out1:" + to_string(getOutPortSize(enode, 0)) + "\"";
            break;
        case Branch_n:
        case Branch_c:
            name += ", out = \"out1+:" + to_string(getInPortSize(enode, 0));
            name += " out2-:" + to_string(getInPortSize(enode, 0)) + "\"";
            break;
        case Fork_:
        case Fork_c:
        case Inst_:

            if (enode->CntrlSuccs->size() > 0 || enode->JustCntrlSuccs->size() > 0 || enode->CntrlOrderSuccs->size() > 0) {
                name += ", out = \"";
                for (int i = 0; i < (int)enode->CntrlSuccs->size() + (int)enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size(); i++) {

					// Aya: THIS SPECIAL CONDITION SHOULD BE REMOVED ONCE WE PROPERLY ADD SUCC AND PREDS OF STORE WITH MC AND LSQS!!
					/*if(enode->type == Inst_ && enode->Instr->getOpcode() == Instruction::Store) {
						name += "out2:32";
						break;
					}*/

                    name += "out" + to_string(i + 1) + ":";
                    name += to_string(getOutPortSize(enode, 0)) + " ";
                }
            }

			// THE FOLLOWING CONDITION IS MEANT TO NOT GIVE ERRORS IF WE ARE DEBUGGING BEFORE PLACING THE MC AND/OR THE LSQ!!
			if(enode->type == Inst_ && enode->Instr->getOpcode() == Instruction::Store && enode->CntrlSuccs->size() == 0 && enode->JustCntrlSuccs->size() == 0 && enode->CntrlOrderSuccs->size() == 0) {
				name += ", out = \"";
				//break;
			}
			////////////////////////////////////////////////
            
			// Aya commented this!!!
			if (enode->type == Inst_)
                if (enode->Instr->getOpcode() == Instruction::Store) {
					// Aya added this line!
                    //name += ", out = \"";
					name += "out2:32";
					// Aya added this line!!
					//name += "\"";
				}
			name += "\"";

			// Aya added this line!!
			/*if (enode->CntrlSuccs->size() > 0 || enode->JustCntrlSuccs->size() > 0 || enode->CntrlOrderSuccs->size() > 0)
                name += "\"";
           
			*/
            break;

        case Phi_c:
            name += ", out = \"out1:" + to_string(CONTROL_SIZE);
            if (enode->isCntrlMg)
                name += " out2?:" + to_string(COND_SIZE);
            name += "\"";
            break;

        case LSQ_:
        case MC_:
            name += ", out = \"";
           
            if (getMemOutputCount(enode) > 0 || enode->lsqToMC) {
                for (auto& pred : *enode->CntrlPreds) {
                    if (pred->type != Cst_ && pred->type != LSQ_)
                        if (pred->Instr->getOpcode() == Instruction::Load) {
                            name += "out" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                    to_string(DATA_SIZE);
                            name += "*l" + to_string(pred->memPortId) + "d ";
                        }
                    if (pred->type == LSQ_)
                        name += "out" + to_string (getMemOutputCount(enode ) + 1) + ":" + to_string(DATA_SIZE)+ "*l" + to_string(pred->lsqMCLoadId) + "d ";
                }
            }

            if (enode->type == MC_ && enode->lsqToMC == true)
                name += "out" + to_string(getMemOutputCount(enode) + 2) + ":" +
                        to_string(CONTROL_SIZE) + "*e ";
            else 
                name += "out" + to_string(getMemOutputCount(enode) + 1) + ":" +
                    to_string(CONTROL_SIZE) + "*e ";

            if (enode->type == LSQ_ && enode->lsqToMC == true) {
                name += "out" + to_string (getMemOutputCount(enode) + 2) + ":" + to_string(ADDR_SIZE) + "*x0a ";
                name += "out" + to_string (getMemOutputCount(enode) + 3) + ":" + to_string(ADDR_SIZE) + "*y0a ";
                name += "out" + to_string (getMemOutputCount(enode) + 4) + ":" + to_string(DATA_SIZE) + "*y0d ";
            }
            
            name += "\"";
            break;
        case End_:
            name += ", out = \"out1:";

            if (enode->CntrlPreds->size() > 0)
                name += to_string(DATA_SIZE);
            else 
                name += to_string(CONTROL_SIZE);
            name += "\"";
            break;
        default:
            break;
    }

    return name;
}


std::string inputSuffix (ENode* enode, unsigned idx) {    
    switch (enode->type)
    {
    case Inst_:
        if (enode->Instr->getOpcode() == Instruction::Select) {
            if (idx == 0)
                return "?";
            else if (idx == 1)
                return "+";
            else if (idx == 2)
                return "-";
        }
        break;

    case Branch_c:
    case Branch_n:
        return idx == BRANCH_CONDITION_IN ? "?" : "";

    case Phi_:
    case Phi_c:
    case Phi_n:
        if (enode->isMux && idx == MUX_CONDITION_IN)
            return "?";
        return "";
    
    default:
        break;
    }

    return "";
}
int getInPortSize_withOB(ENode* enode, unsigned index) {
    switch (enode->type)
    {
    case Branch_: // 1 input nodes
    case Cst_:
    case Fork_:
    case Fork_c:
    case Buffera_:
    case Bufferi_:
    case Start_:
    case Source_:
    case Sink_:
        return enode->sizes_preds.at(0);
    case Argument_:
        return DATA_SIZE; // force Arguments to be 32 bits

    case Branch_n:
    case Branch_c: {
        bool isCondition = index == BRANCH_CONDITION_IN;

        unsigned i = 0;
        for (const ENode* pred : *enode->CntrlPreds) {
			// AYA: 03/11/2021 Added this to make the constant predecessor of a branch work
			if(pred->type == Cst_ && index == BRANCH_CONDITION_IN)
				return enode->sizes_preds.at(i);

            if (isBranchConditionEdge(pred, enode) == isCondition)
                return enode->sizes_preds.at(i);
            i += 1;
        }
        for (const ENode* pred : *enode->JustCntrlPreds) {
            if (isBranchConditionEdge(pred, enode) == isCondition)
                return enode->sizes_preds.at(i);
            i += 1;
        }

		// AYA: 02/11/2021: Added an extra loop for CntrlOrder
		for (const ENode* pred : *enode->CntrlOrderPreds) {
            if (isBranchConditionEdge(pred, enode) == isCondition)
                return enode->sizes_preds.at(i);
            i += 1;
        }

        assert (false && "Illegal state : branch should have both a condition and non-condition input");
    }

    case Phi_: // n inputs
    case Phi_n:
    case Phi_c:

        // if node is a mux, and index holds MuxCondition
        if (enode->isMux && index == MUX_CONDITION_IN) {
            int i = 0;
            for (const ENode* pred : *enode->CntrlPreds) {
                if (isMuxConditionEdge(pred, enode))
                    return enode->sizes_preds.at(i);
                i += 1;
            }
            for (const ENode* pred : *enode->JustCntrlPreds) {
                if (isMuxConditionEdge(pred, enode))
                    return enode->sizes_preds.at(i);
                i += 1;
            }

			// AYA: 02/11/2021: Added an extra loop for CntrlOrder 
			for (const ENode* pred : *enode->CntrlOrderPreds) {
                if (isMuxConditionEdge(pred, enode))
                    return enode->sizes_preds.at(i);
                i += 1;
            }

            assert (false && "Expected 1 port holding the Mux condition");
        }
        // else return the output size ; careful to take the correct output if enode is CMerge
        else if (enode->isCntrlMg) {

            int i = 0;
            for (const ENode* succ : *enode->CntrlSuccs) {
                if (!isCMergeConditionEdge(enode, succ))
                    return enode->sizes_succs.at(i);
                i += 1;
            }
            for (const ENode* succ : *enode->JustCntrlSuccs) {
                if (!isCMergeConditionEdge(enode, succ))
                    return enode->sizes_succs.at(i);
                i += 1;
            }

			// AYA: 02/11/2021: added an extra loop for CntrlOrder
			for (const ENode* succ : *enode->CntrlOrderSuccs) {
                if (!isCMergeConditionEdge(enode, succ))
                    return enode->sizes_succs.at(i);
                i += 1;
            }
            
            assert (false && "Expected 1 port not holding the CMerge condition");
        }
        else {
            return enode->sizes_succs.at(0);
        }
    
    case Inst_: {
        unsigned opcode = enode->Instr->getOpcode();
        // special instructions that do not take output_size as input
        if (opcode == Instruction::ICmp || opcode == Instruction::FCmp
            || opcode == Instruction::LShr || opcode == Instruction::AShr
            || opcode == Instruction::SDiv || opcode == Instruction::UDiv
            || opcode == Instruction::SRem || opcode == Instruction::URem) {

			// AYA: 02/11/2021: Added the size of CntrlOrder as well to all loop bounds!!!            

            // search for maximum in input/output and use it
            unsigned max_bw = 0;
            for (unsigned i = 0 ; i < enode->CntrlPreds->size() + enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size() ; ++i)
                max_bw = std::max<unsigned>(max_bw, enode->sizes_preds.at(i));
            for (unsigned i = 0 ; i < enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size() ; ++i)
                max_bw = std::max<unsigned>(max_bw, enode->sizes_succs.at(i));

            // if the comparison is signed, and that bitwidth < 32 (unsigned inputs), add 1 bit to make sure sign bit=0
            if (opcode == Instruction::ICmp && dyn_cast<ICmpInst>(enode->Instr)->isSigned()) 
                max_bw = std::min<unsigned>(DATA_SIZE, max_bw + 1);
            
            return max_bw;
        }
        else if (opcode == Instruction::Select) {
            // condition input ?
            if (index == 0)
                return enode->sizes_preds.at(0);
            else
                return std::max(enode->sizes_preds.at(1), enode->sizes_preds.at(2));
        }
        else if (opcode == Instruction::Load) {
 	    // Lana 7.6.2021. Load single predecessor is address
            return enode->sizes_preds.at(0);
        }
        else if (opcode == Instruction::Store) {
            // Lana 7.6.2021. Operands may be reversed (data predecessor is not necessarily the 1st one)
            // GetOperandIndex returns 2 for address and 1 for data predecessor, pick size accordingly
            ENode* pred = (index == 0) ? enode->CntrlPreds->front() : enode->CntrlPreds->back();
            return enode->sizes_preds.at(getOperandIndex(pred, enode)-1);
        }
        else {
            return enode->sizes_succs.at(0);
        }
    }
    default:
        break;
    }
    
    return DATA_SIZE;
}

int getOutPortSize_withOB(ENode* enode, unsigned index) {
    switch (enode->type) {
        case Phi_c: // CMerge? 
            if (enode->isCntrlMg) {
                bool isCondition = index == CMERGE_CONDITION_OUT;
                
                unsigned i = 0;
                for (const ENode* succ : *enode->CntrlSuccs) {
                    if (isCMergeConditionEdge(enode, succ) == isCondition)
                        return enode->sizes_succs.at(i);
                    i += 1;
                }
                for (const ENode* succ : *enode->JustCntrlSuccs) {
                    if (isCMergeConditionEdge(enode, succ) == isCondition)
                        return enode->sizes_succs.at(i);
                    i += 1;
                }

				// AYA: 02/11/2021: added an extra loop for CntrlOrderSuccs
				for (const ENode* succ : *enode->CntrlOrderSuccs) {
                    if (isCMergeConditionEdge(enode, succ) == isCondition)
                        return enode->sizes_succs.at(i);
                    i += 1;
                }


                assert (false && "Illegal state : CMerges must have both a condition and non-condition output");
            }
            // else:
        case Buffera_: // 1 output node
        case Bufferi_:
        case Branch_:
        case Phi_:
        case Phi_n:
        case Start_:
        case Source_:
        case Cst_:
        case End_:
            return enode->sizes_succs.at(0);
        case Argument_:
            return DATA_SIZE; // force Arguments to be 32 bits

        case Branch_n:
        // case Branch_c: // output sizes depend on input_size (non-condition) : they're the same anyway
            return enode->sizes_succs.at(0);
		// AYA: 03/11/2021: separated the Branch_c since sizes_succs of it is wrongly filled! And, blindly the outputs of a control branch will always be of CONTROL_SIZE!!
		case Branch_c:
			return CONTROL_SIZE;

        case Fork_:
        case Fork_c: // output sizes == input_size
            return enode->sizes_preds.at(0);

        case Inst_:
            return enode->sizes_succs.at(index);
        default:
            break;
    }

    return DATA_SIZE;
}

// Lana 9.6.2021. Find largest address bitwidth among all lds/sts connected to enode
int getMemAddrSize(ENode* enode) {
    int max_size = 0;
    for (auto& pred : *enode->CntrlPreds) {
        int curr_size = 0;
        // LSQ connected to MC, check LSQ lds/sts
        if (pred->type == LSQ_)
            curr_size =  getMemAddrSize(pred);
        else if (pred->type == Inst_) {
            if (pred->Instr->getOpcode() == Instruction::Load)
                curr_size = getInPortSize_withOB(pred, 0);
            else if (pred->Instr->getOpcode() == Instruction::Store)
                curr_size = getInPortSize_withOB(pred, 1);
        }
        if (curr_size > max_size)
            max_size = curr_size;
    }
    return max_size;
}

// Lana 9.6.2021. Find largest data bitwidth among all lds/sts connected to enode
int getMemDataSize(ENode* enode) {
    int max_size = 0;
    for (auto& pred : *enode->CntrlPreds) {
        int curr_size = 0;
        // LSQ connected to MC, check LSQ lds/sts
        if (pred->type == LSQ_)
            curr_size =  getMemDataSize(pred);
        else if (pred->type == Inst_) {
            if (pred->Instr->getOpcode() == Instruction::Load)
                curr_size = (getOutPortSize_withOB(pred, 0));
            else if (pred->Instr->getOpcode() == Instruction::Store)
                curr_size = getInPortSize_withOB(pred, 0);
        }
        if (curr_size > max_size)
            max_size = curr_size;
    }
    return max_size;
}

std::string getNodeDotInputs_withOB(ENode* enode) {
    string name = "";
    int mem_addr_size;
    int mem_data_size;

    switch (enode->type) {
        case Argument_:
        case Branch_:
        case Fork_:
        case Fork_c:
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Start_:
        case Sink_:
            name += ", in = \"in1:" + to_string(getInPortSize_withOB(enode, 0)) + "\"";
            break;

        case Branch_n:
        //case Branch_c:
            name += ",  in = \"in1:" + to_string(getInPortSize_withOB(enode, 0));
            name += " in2?:" + to_string(getInPortSize_withOB(enode, 1)) + "\"";
            break;

		// AYA: 03/11/2021: Made the control branch a separate case and hardcoded the data input since it should always blindly be 0 bits!!
		case Branch_c:
			name += ",  in = \"in1:0";
            name += " in2?:" + to_string(getInPortSize_withOB(enode, 1)) + "\"";
            break;

        case Inst_:
			// AYA: 01/11/2021: made changes to fit my CntrlOrder network!
			////////////////////////////////////////////////////////////////////
			if (enode->Instr->getOpcode() == Instruction::Load) {
				name += ", in = \"";

                // Data input (from memory)
                name += "in1:";
                name += to_string(getOutPortSize_withOB(enode, 0)) + " ";
                // Address input (from predecessor)
                name += "in2:";
                name += to_string(getInPortSize_withOB(enode, 0)) + " ";

				// AYA: 03/11/2021: Added this!!
				name += "\"";

            }
            else if (enode->Instr->getOpcode() == Instruction::Store) {
				name += ", in = \"";

                // Data input (from predecessor)
                name += "in1:";
                name += to_string(getInPortSize_withOB(enode, 0)) + " ";
                // Address input (from predecessor)
                name += "in2:";
                name += to_string(getInPortSize_withOB(enode, 1)) + " ";

				// AYA: 03/11/2021: Added this!!
				name += "\"";

            }
            else if(enode->Name.compare("ret") == 0 && enode->CntrlPreds->size() == 0) {
					// Aya added this line!!
		 			if (enode->CntrlOrderPreds->size() > 0)
				         name += ", in = \"";
				    // name += ", in = \"";

				    for (int i = 0; i < (int)enode->CntrlOrderPreds->size(); i++) {
				        name += "in" + to_string(i + 1);
				        name += inputSuffix(enode, i);
				        name += ":";
				        name += to_string(getInPortSize_withOB(enode, i)) + " ";
				    }
		      
					// Aya added this line			
					if (enode->CntrlOrderPreds->size() > 0 )
				         name += "\"";
				   // name += "\"";

			} else {

						// Aya added this line!!
	 			if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0)
		             name += ", in = \"";
		        // name += ", in = \"";

		        for (int i = 0; i < (int)enode->CntrlPreds->size() + (int)enode->JustCntrlPreds->size(); i++) {
		            name += "in" + to_string(i + 1);
		            name += inputSuffix(enode, i);
		            name += ":";
		            name += to_string(getInPortSize_withOB(enode, i)) + " ";
		        }
		       
				// Aya added this line			
				if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0 )
		             name += "\"";
		       // name += "\""; 

			}

			////////////////////////////////////////////////////////////////////
           /* name += ", in = \"";

            // conflict in sizes with MC_/LSQ_ nodes
            //workaround: force IOs to 32
            //if (enode->Instr->getOpcode() == Instruction::Load || enode->Instr->getOpcode() == Instruction::Store) {
            //    name += "in1:32 in2:32";
            // }
	    // Lana 7.6.2021. Load and store sizes
            if (enode->Instr->getOpcode() == Instruction::Load) {
                // Data input (from memory)
                name += "in1:";
                name += to_string(getOutPortSize_withOB(enode, 0)) + " ";
                // Address input (from predecessor)
                name += "in2:";
                name += to_string(getInPortSize_withOB(enode, 0)) + " ";
            }
            else if (enode->Instr->getOpcode() == Instruction::Store) {
                // Data input (from predecessor)
                name += "in1:";
                name += to_string(getInPortSize_withOB(enode, 0)) + " ";
                // Address input (from predecessor)
                name += "in2:";
                name += to_string(getInPortSize_withOB(enode, 1)) + " ";
            }
            else {
                for (unsigned i = 0 ; i < enode->CntrlPreds->size() + enode->JustCntrlPreds->size() ; ++i) {
                    name += "in" + to_string(i + 1);
                    name += inputSuffix(enode, i);
                    name += ":";
                    name += to_string(getInPortSize_withOB(enode, i)) + " ";
                }
            }
            name += "\"";
		*/
            break;

        case Phi_:
        case Phi_n:
        case Phi_c:
		////////////////////////////////////////////////////////////////////////////////
						// check first, if the node is Phi_c and is part of the redundant control graph, then all the conditions should be on CntrlOrderPreds

			// Aya: 27/09/2021 added the following condition to deal with the case where we trigger a return instruction through the redunCntrlNetwork in case the function returns to void
			if(enode->type == Phi_c && enode->is_redunCntrlNet) {
					// Aya added this line!!
	 			if (enode->CntrlOrderPreds->size() > 0)
		             name += ", in = \"";
		        // name += ", in = \"";

		        for (int i = 0; i < (int)enode->CntrlOrderPreds->size(); i++) {
		            name += "in" + to_string(i + 1);
		            name += inputSuffix(enode, i);
		            name += ":";
		            name += to_string(getInPortSize_withOB(enode, i)) + " ";
		        }
		      
				// Aya added this line			
				if (enode->CntrlOrderPreds->size() > 0 )
		             name += "\"";
		       // name += "\"";

			} else {
					// Aya added this line!!
	 			if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0)
		             name += ", in = \"";
		        // name += ", in = \"";

		        for (int i = 0; i < (int)enode->CntrlPreds->size() + (int)enode->JustCntrlPreds->size(); i++) {
		            name += "in" + to_string(i + 1);
		            name += inputSuffix(enode, i);
		            name += ":";
		            name += to_string(getInPortSize_withOB(enode, i)) + " ";
		        }

				// Aya added this line			
				if (enode->CntrlPreds->size() > 0 || enode->JustCntrlPreds->size() > 0 )
		             name += "\"";
		       // name += "\"";

			} 


		////////////////////////////////////////////////////////////////////////////////

           /* name += ", in = \"";

            for (unsigned i = 0 ; i < enode->CntrlPreds->size() + enode->JustCntrlPreds->size() ; ++i) {
                name += "in" + to_string(i + 1);
                name += inputSuffix(enode, i);
                name += ":";
                name += to_string(getInPortSize_withOB(enode, i)) + " ";
            }

            name += "\""; */
            break;

        case LSQ_:
        case MC_:
		////////////////////////////////////////////////////////////////////////
			 name += ", in = \"";
			 mem_addr_size = getMemAddrSize(enode);
			 mem_data_size = getMemDataSize(enode);

			// Aya commented this and changed it to CntrlOrderPreds
           /* for (int i = 0; i < (int)enode->JustCntrlPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }
		  */
			// Aya did this for the new redundant control network instead of using JustCntrlPreds!!
			for (int i = 0; i < (int)enode->CntrlOrderPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }

            for (auto& pred : *enode->CntrlPreds) {
                if (pred->type != Cst_ && pred->type !=LSQ_) {

                    name +=
                        "in" + to_string(getDotAdrIndex(pred, enode)) + ":" + to_string(mem_addr_size);
                    name += (pred->Instr->getOpcode() == Instruction::Load) ? "*l" : "*s";
                    name += to_string(pred->memPortId) + "a ";

                    if (pred->Instr->getOpcode() == Instruction::Store) {
                        name += "in" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                to_string(mem_data_size);
                        name += "*s" + to_string(pred->memPortId) + "d ";
                    }
                }
                if (pred->type == LSQ_) {
                	int inputs = getMemInputCount(enode);
                	int pred_ld_id = pred->lsqMCLoadId;
                    int pred_st_id = pred->lsqMCStoreId;
                    name += "in" + to_string (inputs + 1) + ":" + to_string(mem_addr_size) + "*l" + to_string(pred_ld_id) + "a ";
                    name += "in" + to_string (inputs + 2) + ":" + to_string(mem_addr_size)+ "*s" + to_string(pred_st_id) + "a ";
                    name += "in" + to_string (inputs + 3) + ":" + to_string(mem_data_size)+ "*s" + to_string(pred_st_id) + "d ";

                }
            }
            if (enode->type == LSQ_ && enode->lsqToMC == true) {
            	int inputs = getMemInputCount(enode);
                name += "in" + to_string (inputs + 1) + ":" + to_string(mem_data_size) + "*x0d ";
            }
            name += "\"";

		////////////////////////////////////////////////////////////////////////
           /* name += ", in = \"";
            mem_addr_size = getMemAddrSize(enode);
            mem_data_size = getMemDataSize(enode);

            // Lana 9.6.2021 For MC constants, keep max size (input constants are sized)
            for (int i = 0; i < (int)enode->JustCntrlPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }

            for (auto& pred : *enode->CntrlPreds) {
                if (pred->type != Cst_ && pred->type !=LSQ_) {

                    name +=
                        "in" + to_string(getDotAdrIndex(pred, enode)) + ":" + to_string(mem_addr_size);
                    name += (pred->Instr->getOpcode() == Instruction::Load) ? "*l" : "*s";
                    name += to_string(pred->memPortId) + "a ";

                    if (pred->Instr->getOpcode() == Instruction::Store) {
                        name += "in" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                to_string(mem_data_size);
                        name += "*s" + to_string(pred->memPortId) + "d ";
                    }
                }
                if (pred->type == LSQ_) {
                	int inputs = getMemInputCount(enode);
                	int pred_ld_id = pred->lsqMCLoadId;
                    int pred_st_id = pred->lsqMCStoreId;
                    name += "in" + to_string (inputs + 1) + ":" + to_string(mem_addr_size) + "*l" + to_string(pred_ld_id) + "a ";
                    name += "in" + to_string (inputs + 2) + ":" + to_string(mem_addr_size)+ "*s" + to_string(pred_st_id) + "a ";
                    name += "in" + to_string (inputs + 3) + ":" + to_string(mem_data_size)+ "*s" + to_string(pred_st_id) + "d ";

                }
            }
            if (enode->type == LSQ_ && enode->lsqToMC == true) {
            	int inputs = getMemInputCount(enode);
                name += "in" + to_string (inputs + 1) + ":" + to_string(mem_data_size) + "*x0d ";
            }
            name += "\"";*/
            break;

        case End_: {
		/////////////////////////////////////////////////////////////////////////////////
			name += ", in = \"";

			unsigned maxBitwidth = 0;
			for(unsigned i = 0; i < enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size() + enode->CntrlPreds->size(); i++)
				maxBitwidth = std::max(maxBitwidth, enode->sizes_preds.at(i));


			for (auto& pred : *enode->CntrlOrderPreds) {
                auto it =
                    std::find(enode->CntrlOrderPreds->begin(), enode->CntrlOrderPreds->end(), pred);
                int ind = distance(enode->CntrlOrderPreds->begin(), it);
                name += "in" + to_string(ind + 1) + ":" + to_string(CONTROL_SIZE);

                // if its not memory, it is the single control port
                name += (pred->type == MC_ || pred->type == LSQ_) ? "*e " : "";
            }


            for (auto i = enode->CntrlOrderPreds->size();
                 i < (enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size());
                 i++) {
                 name += "in" + to_string(i + 1) + ":" + to_string(CONTROL_SIZE) + " ";
                // if its not memory, it is the single control port
				// AYA COMMENTED THE FOLLOWING SINCE I CONNECT THE MC_ OR LSQ_ OVER THE NEW REDUNDANT CONTROL NETWORK!!
                // name += (pred->type == MC_ || pred->type == LSQ_) ? "*e " : "";
            }

            for (auto i = enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size();
                 i < (enode->JustCntrlPreds->size() + enode->CntrlOrderPreds->size() + enode->CntrlPreds->size());
                 i++) // if data ports, count from zero
                name += "in" + to_string(i + 1) + ":" + to_string(maxBitwidth) + " ";

            name += "\"";

		/////////////////////////////////////////////////////////////////////////////////
		/*{
            name += ", in = \"";
            
            unsigned maxBitwidth = 0;
            for (unsigned i = 0 ; i < enode->JustCntrlPreds->size() + enode->CntrlPreds->size() ; ++i)
                maxBitwidth = std::max(maxBitwidth, enode->sizes_preds.at(i));
                
            for (unsigned i = 0 ; i < enode->JustCntrlPreds->size() ; ++i) {
                const ENode* pred = enode->JustCntrlPreds->at(i);

                name += "in" + to_string(i + 1) + ":" + to_string(CONTROL_SIZE);
                // if its not memory, it is the single control port
                if (pred->type == MC_ || pred->type == LSQ_)
                    name += "*e";
                name += " ";
            }

            for (unsigned i = 0 ; i < enode->CntrlPreds->size() ; ++i)
                name += "in" + to_string(enode->JustCntrlPreds->size() + i + 1) + ":" + to_string(maxBitwidth) + " ";

            name += "\"";

        }*/
	    }	break;

        default:
            break;
    }
    return name;
}

std::string outputSuffix (ENode* enode, unsigned idx) {
    switch (enode->type)
    {
    case Branch_n:
    case Branch_c:
        return idx == 0 ? "+" : "-";

    case Phi_:
    case Phi_n:
    case Phi_c:
        if (enode->isCntrlMg && idx == CMERGE_CONDITION_OUT)
            return "?";
    
    default:
        break;
    }
    return "";
}

std::string getNodeDotOutputs_withOB(ENode* enode) {
    string name = "";
    int mem_addr_size;
    int mem_data_size;

    switch (enode->type) {
        case Argument_: // 1 output node
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Branch_:
        case Phi_:
        case Phi_n:
        case Start_:
        case Source_:
        case End_:
            name += ", out = \"out1:" + to_string(getOutPortSize_withOB(enode, 0)) + "\"";
            break;

        case Branch_n:
        case Branch_c:
            name += ", out = \"out1+:" + to_string(getOutPortSize_withOB(enode, 0));
            name += " out2-:" + to_string(getOutPortSize_withOB(enode, 0)) + "\"";
            break;

        case Fork_:
        case Fork_c:
            name += ", out = \"";
			// AYA: 03/11/2021: added the CNtrlOrderSuccs as well!!
            for (unsigned i = 0 ; i < enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size() ; i++) {
                name += "out" + to_string(i + 1) + ":";
                name += to_string(getOutPortSize_withOB(enode, 0)) + " ";
            }
            name += "\"";
            break;

        case Inst_:
			// AYA: 03/11/2021: added the CNtrlOrderSuccs as well!!
            if (!enode->CntrlSuccs->empty() || !enode->JustCntrlSuccs->empty() || !enode->CntrlOrderSuccs->empty()) {    
                name += ", out = \"";
                
                //if (enode->Instr->getOpcode() == Instruction::Load || enode->Instr->getOpcode() == Instruction::Store) {
                   // name += "out1:32 out2:32";
                //}
                // Lana 7.6.2021. Load and store sizes
                if (enode->Instr->getOpcode() == Instruction::Load) {
                    // Data output
                    name += "out1:";
                    name += to_string(getOutPortSize_withOB(enode, 0)) + " ";
                    // Address output (to memory)
                    name += "out2:";
                    name += to_string(getInPortSize_withOB(enode, 0)) + " ";
                }
                else if (enode->Instr->getOpcode() == Instruction::Store) {
                    // Data output (to memory)
                    name += "out1:";
                    name += to_string(getInPortSize_withOB(enode, 0)) + " ";
                    // Address output (to memory)
                    name += "out2:";
                    name += to_string(getInPortSize_withOB(enode, 1)) + " ";
                }
                else {
					// AYA: 03/11/2021: added the CNtrlOrderSuccs as well!!
                    for (unsigned i = 0 ; i < enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size() + enode->CntrlOrderSuccs->size() ; i++) {
                        name += "out" + to_string(i + 1) + ":";
                        name += to_string(getOutPortSize_withOB(enode, i)) + " ";
                    }
                }
            
                name += "\"";
            }
            break;

        case Phi_c:
            name += ", out = \"";
            name += "out1:" + to_string(getOutPortSize_withOB(enode, 0));
            if (enode->isCntrlMg)
                name += " out2?:" + to_string(getOutPortSize_withOB(enode, 1));
            name += "\"";
            break;

        case LSQ_:
        case MC_:
            name += ", out = \"";
            mem_addr_size = getMemAddrSize(enode);
            mem_data_size = getMemDataSize(enode);
           
            if (getMemOutputCount(enode) > 0 || enode->lsqToMC) {
                for (auto& pred : *enode->CntrlPreds) {
                    if (pred->type != Cst_ && pred->type != LSQ_)
                        if (pred->Instr->getOpcode() == Instruction::Load) {
                            name += "out" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                    to_string(mem_data_size);
                            name += "*l" + to_string(pred->memPortId) + "d ";
                        }
                    if (pred->type == LSQ_)
                        name += "out" + to_string (getMemOutputCount(enode ) + 1) + ":" + to_string(mem_data_size)+ "*l" + to_string(pred->lsqMCLoadId) + "d ";
                }
            }

            if (enode->type == MC_ && enode->lsqToMC == true)
                name += "out" + to_string(getMemOutputCount(enode) + 2) + ":" +
                        to_string(CONTROL_SIZE) + "*e ";
            else 
                name += "out" + to_string(getMemOutputCount(enode) + 1) + ":" +
                    to_string(CONTROL_SIZE) + "*e ";

            if (enode->type == LSQ_ && enode->lsqToMC == true) {
                name += "out" + to_string (getMemOutputCount(enode) + 2) + ":" + to_string(mem_addr_size) + "*x0a ";
                name += "out" + to_string (getMemOutputCount(enode) + 3) + ":" + to_string(mem_addr_size) + "*y0a ";
                name += "out" + to_string (getMemOutputCount(enode) + 4) + ":" + to_string(mem_data_size) + "*y0d ";
            }
            
            name += "\"";
            break;

        default:
            break;
    }

    return name;
}


std::string getFloatValue(float x) {
#define nodeDotNameSIZE 100
    char* nodeDotName = new char[nodeDotNameSIZE];
    snprintf(nodeDotName, nodeDotNameSIZE, "%2.3f", x);
    return string(nodeDotName);
}

std::string getHexValue(int x) {
#define nodeDotNameSIZE 100
    char* nodeDotName = new char[nodeDotNameSIZE];
    snprintf(nodeDotName, nodeDotNameSIZE, "%08X", x);
    return string(nodeDotName);
}

std::string getNodeDotParams(ENode* enode, std::string serial_number) {
    string name = "";
    switch (enode->type) {
        case Branch_:
            name += ", value = \"0x1\"";
            break;
        case Inst_:
            if (enode->Instr->getOpcode() == Instruction::GetElementPtr)
                 name += ", constants=" + to_string(enode->JustCntrlPreds->size());
            if (enode->Instr->getOpcode() == Instruction::Select)
                 name += ", trueFrac=0.2";
            name += ", delay=" + getFloatValue(get_component_delay(enode->Name, DATA_SIZE, serial_number));
            
            if (isLSQport(enode))
                name += ", latency=" + to_string(get_component_latency(("lsq_" + enode->Name), DATA_SIZE, serial_number));
            else
                name += ", latency=" + to_string(get_component_latency((enode->Name), DATA_SIZE, serial_number));
            
            name += ", II=1";
            break;
        case Phi_:
        case Phi_n:
            name += ", delay=";
            if (enode->CntrlPreds->size() == 1)
                name += getFloatValue(get_component_delay(ZDC_NAME, DATA_SIZE, serial_number));
            else
                name += getFloatValue(get_component_delay(enode->Name, DATA_SIZE, serial_number));
            break;
        case Phi_c:
            name += ", delay=" + getFloatValue(get_component_delay(enode->Name, DATA_SIZE, serial_number));
            break;
        case Cst_:
            name += ", value = \"0x" + getHexValue(getConstantValue(enode)) + "\"";
        default:
            break;
    }

    return name;
}

std::string getLSQJsonParams(ENode* memnode) {
    // Additional LSQ configuration params for json file

    string fifoDepth = ", fifoDepth = ";
    string numLd     = ", numLoads = \"{";
    string numSt     = "numStores = \"{";
    string ldOff     = "loadOffsets = \"{";
    string stOff     = "storeOffsets = \"{";
    string ldPts     = "loadPorts = \"{";
    string stPts     = "storePorts = \"{";

    int count = 0;
    int depth = getLSQDepth(memnode);
    fifoDepth += to_string(depth);

   // for (auto& enode : *memnode->JustCntrlPreds) {
	// AYa commented the above to make it to the new redunCntrlNetwork!!
	for (auto& enode : *memnode->CntrlOrderPreds) {
        BBNode* bb = enode->bbNode;

        int ld_count = getBBLdCount(enode, memnode);
        int st_count = getBBStCount(enode, memnode);

        numLd += to_string(ld_count);
        //numLd += count == (memnode->JustCntrlPreds->size() - 1) ? "" : "; ";
		// Aya commented the above to change it to the redundant control nework
		numLd += count == (memnode->CntrlOrderPreds->size() - 1) ? "" : "; ";

        numSt += to_string(st_count);

		//numSt += count == (memnode->JustCntrlPreds->size() - 1) ? "" : "; ";
		// Aya commented the above to change it to the redundant control nework
        numSt += count == (memnode->CntrlOrderPreds->size() - 1) ? "" : "; ";

        ldOff += (ldOff.find_last_of("{") == ldOff.find_first_of("{")) ? "{" : ";{";     
        stOff += (stOff.find_last_of("{") == stOff.find_first_of("{")) ? "{" : ";{";  
        ldPts += (ldPts.find_last_of("{") == ldPts.find_first_of("{")) ? "{" : ";{";  
        stPts += (stPts.find_last_of("{") == stPts.find_first_of("{")) ? "{" : ";{";  

        for (auto& node : *memnode->CntrlPreds) {

            if (node->type != Cst_) {
                auto* I = node->Instr;
                if (isa<LoadInst>(I) && node->BB == enode->BB) {
                    string d = (ldOff.find_last_of("{") == ldOff.size() - 1) ? "" : ";";
                    ldOff += d;
                    ldOff += to_string(getBBOffset(node));
                    ldPts += d;
                    ldPts += to_string(getMemPortIndex(node));

                } else if (isa<StoreInst>(I) && node->BB == enode->BB) {
                    string d = (stOff.find_last_of("{") == stOff.size() - 1) ? "" : ";";
                    stOff += d;
                    stOff += to_string(getBBOffset(node));
                    stPts += d;
                    stPts += to_string(getMemPortIndex(node));
                }
            }
        }

        // zeroes up to lsq depth needed for json config
        for (int i = ld_count; i < depth; i++) {
            string d = (i == 0) ? "" : ";";
            ldOff += d;
            ldOff += "0";
            ldPts += d;
            ldPts += "0";
        }

        for (int i = st_count; i < depth; i++) {
            string d = (i == 0) ? "" : ";";
            stOff += d;
            stOff += "0";
            stPts += d;
            stPts += "0";
        }
        ldOff += "}";
        stOff += "}";
        ldPts += "}";
        stPts += "}";
        count++;
    }

    numLd += "}\", ";
    numSt += "}\", ";
    ldOff += "}\", ";
    stOff += "}\", ";
    ldPts += "}\", ";
    stPts += "}\"";

    return fifoDepth + numLd + numSt + ldOff + stOff + ldPts + stPts;
}

std::string getNodeDotMemParams(ENode* enode) {
    string name = ", memory = \"" + string(enode->Name) + "\", ";
	// AYA: URGENT!! getMemBBCount needs changing in the implmentation to access our redunCntrl network
    name += "bbcount = " + to_string(getMemBBCount(enode)) + ", ";

    int ldcount = getMemLoadCount(enode);
    int stcount = getMemStoreCount(enode);
    if (enode->type == MC_ && enode->lsqToMC) {
        ldcount++; 
        stcount++;
    }

    name += "ldcount = " + to_string(ldcount) + ", ";
    name += "stcount = " + to_string(stcount);

    if (enode->type == LSQ_)
        name += getLSQJsonParams(enode);
    return name;
}

void printDotNodes(std::vector<ENode*>* enode_dag, bool full, std::string serial_number) {

    for (auto& enode : *enode_dag) {
        if (!skipNodePrint(enode)) {
            // enode->memPort = false; //temporary, disable lsq connections
            string dotline = "\t\t";

            dotline += getNodeDotNameNew(enode);

            if (full) {

                dotline += " [";

                dotline += getNodeDotTypeNew(enode);
                dotline += getNodeDotbbID(enode);

                if (enode->type == Inst_)
                    dotline += getNodeDotOp(enode);

                if (OptimizeBitwidth::isEnabled()) {
                    dotline += getNodeDotInputs_withOB(enode);
                    dotline += getNodeDotOutputs_withOB(enode);
                }
                else {
                    dotline += getNodeDotInputs(enode);
                    dotline += getNodeDotOutputs(enode);
                }
                dotline += getNodeDotParams(enode, serial_number);
                if (enode->type == MC_ || enode->type == LSQ_)
                    dotline += getNodeDotMemParams(enode);

                dotline += "];\n";
            }

            dotfile << dotline;
        }
    }
}

std::string printEdge(ENode* from, ENode* to) {
    string s = "\t";
    s += getNodeDotNameNew(from);
    s += " -> ";
    s += getNodeDotNameNew(to);
    return s;
}
std::string printEdgeWidth(const ENode* from, const ENode* to) {
    // printEdgeWidth may be called when sizes are not computed yet,
    //in which case we print nothing
    if (from->sizes_succs.empty() && to->sizes_preds.empty())
        return "";

    int idxSuccInFrom = getSuccIdxInPred(to, from);
    unsigned size_from = from->sizes_succs.at(idxSuccInFrom);
    assert(idxSuccInFrom != -1);
    
    int idxPredInTo = getPredIdxInSucc(from, to);
    unsigned size_to = to->sizes_preds.at(idxPredInTo);
    assert(idxPredInTo != -1);

    std::string label = ", label=\"";
    if (size_from == size_to)
        label += to_string(size_from);
    // conflict in sizes ? print it
    // else {
    //     label += "from:" + to_string(size_from) 
    //         + ",to:" + to_string(size_to);
    
    //     label += "\\nfrom=" + from->Name + "(type=" + to_string(from->type)
    //         + "), to=" + to->Name + "(type=" + to_string(to->type) + ")";
    //     label += "\\nfrom_succ=" + (idxSuccInFrom < from->CntrlSuccs->size() ? 
    //         from->CntrlSuccs->at(idxSuccInFrom) : 
    //         from->JustCntrlSuccs->at(idxSuccInFrom - from->CntrlSuccs->size()))->Name;
    //     label += "\\nto_pred=" + (idxPredInTo < to->CntrlPreds->size() ? 
    //         to->CntrlPreds->at(idxPredInTo) : 
    //         to->JustCntrlPreds->at(idxPredInTo - to->CntrlPreds->size()))->Name;
    // }

    label += '"';
    return label;
}

std::string printColor(ENode* from, ENode* to) {
    string s = "color = \"";

    if (from->type == Phi_c && (to->type == Fork_ || to->type == Phi_ || to->type == Phi_n))
        s += "green"; // connection from CntrlMg to Muxes
    else if (from->type == Phi_c || from->type == Branch_c || from->type == Fork_c ||
             to->type == Phi_c || to->type == Branch_c || to->type == Fork_c ||
             from->type == Start_ || /*(to->type == End_ && from->JustCntrlSuccs->size() > 0)*/ (to->type == End_ && from->CntrlOrderSuccs->size() > 0))  // Aya replaced JustCntrl with the new redundant control network!!
        s += "gold3"; // control-only
    else if (from->type == MC_ || from->type == LSQ_ || to->type == MC_ || to->type == LSQ_)
        s += "darkgreen"; // memory
    else if (from->type == Branch_n)
        s += "blue"; // outside BB
    else if (from->type == Branch_ || to->type == Branch_)
        s += "magenta"; // condition
    else
        s += "red"; // inside BB

    s += "\"";
    if (from->type == Branch_n || from->type == Branch_c)
        s += ", minlen = " + to_string(BB_MINLEN);

    return s;
}

std::string printMemEdges(std::vector<ENode*>* enode_dag) {
    string memstr = "";
    std::map<void*, int> incount;

    // print lsq/mc connections
    for (auto& enode : *enode_dag) {
        if (enode->type == MC_ || enode->type == LSQ_) {
            int index = 1;
            //for (auto& pred : *(enode->JustCntrlPreds)) {
			// AYa changed to the new control network! 
			for (auto& pred : *(enode->CntrlOrderPreds)) {
				
                if (pred->type == Cst_) {
                    memstr += printEdge(pred, enode);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", from = \"out1\"";
                    memstr += ", to = \"in" + to_string(index) + "\"";
                    memstr += "];\n";
                    index++;
                }
            }
            int end_ind = getMemOutputCount(enode) + 1;
            if (enode->type == MC_ && enode->lsqToMC)
                end_ind++;
            //for (auto& succ : *(enode->JustCntrlSuccs)) {
			// AYa changed to the new control network!
			for (auto& succ : *(enode->CntrlOrderSuccs)) {
                if (succ->type == End_) {
                    memstr += printEdge(enode, succ);
                    memstr += " [";
                    memstr += printColor(enode, succ);

                    //auto it = std::find(succ->JustCntrlPreds->begin(), succ->JustCntrlPreds->end(), enode);
                    //int succ_ind = distance(succ->JustCntrlPreds->begin(), it) + 1;
					// AYa changed to the new control network!
					auto it = std::find(succ->CntrlOrderPreds->begin(), succ->CntrlOrderPreds->end(), enode);

					// Aya added this sanity check
					assert(it != succ->CntrlOrderPreds->end());

                    int succ_ind = distance(succ->CntrlOrderPreds->begin(), it) + 1;

                    memstr += ", from = \"out" + to_string(end_ind) + "\"";
                    memstr += ", to = \"in" + to_string(succ_ind) + "\"";
                    memstr += "];\n";
                }
            }
        }
        if (enode->type == MC_ ) {
            for (auto& pred : *(enode->CntrlPreds)) {
                if (pred->type == LSQ_) {
                    memstr += printEdge(pred, enode);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", mem_address = \"true\"";
                    memstr += ", from = \"out" + to_string(getMemOutputCount(pred) + 2) + "\"";
                    memstr += ", to = \"in" + to_string(getMemInputCount(enode) + 1) + "\"";
                    memstr += "];\n";
                    memstr += printEdge(pred, enode);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", mem_address = \"true\"";
                    memstr += ", from = \"out" + to_string(getMemOutputCount(pred) + 3) + "\"";
                    memstr += ", to = \"in" + to_string(getMemInputCount(enode) + 2) + "\"";
                    memstr += "];\n";
                    memstr += printEdge(pred, enode);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", mem_address = \"false\"";
                    memstr += ", from = \"out" + to_string(getMemOutputCount(pred) + 4) + "\"";
                    memstr += ", to = \"in" + to_string(getMemInputCount(enode) + 3) + "\"";
                    memstr += "];\n";
                    memstr += printEdge(enode, pred);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", mem_address = \"false\"";
                    memstr += ", from = \"out" + to_string(getMemOutputCount(enode) + 1) + "\"";
                    memstr += ", to = \"in" + to_string(getMemInputCount(pred) + 1) + "\"";
                    memstr += "];\n";
                }
            }
        }

        if (enode->type != Inst_)
            continue;

        auto* I = enode->Instr;
        // enode->memPort = false;
        if ((isa<LoadInst>(I) || isa<StoreInst>(I)) && enode->memPort) {
            int mcidx;
            auto* MEnode = (ENode*)enode->Mem;

            mcidx                  = incount[(void*)MEnode];
            incount[(void*)MEnode] = mcidx + 1;

            int address_ind = getDotAdrIndex(enode, MEnode);
            int data_ind    = getDotDataIndex(enode, MEnode);

            memstr += printEdge(enode, MEnode);
            memstr += " [";
            memstr += printColor(enode, MEnode);
            memstr += ", mem_address = \"true\"";
            memstr += ", from = \"out2\"";
            memstr += ", to = \"in" + to_string(address_ind) + "\"";
            memstr += "];\n";

            if (isa<LoadInst>(I)) {
                memstr += printEdge(MEnode, enode);
                memstr += " [";
                memstr += printColor(MEnode, enode);
                memstr += ", mem_address = \"false\"";
                memstr += ", from = \"out" + to_string(data_ind) + "\"";
                memstr += ", to = \"in1\"";
            } else {
                memstr += printEdge(enode, MEnode);
                memstr += " [";
                memstr += printColor(enode, MEnode);
                memstr += ", mem_address = \"false\"";
                memstr += ", from = \"out1\"";
                memstr += ", to = \"in" + to_string(data_ind) + "\"";
            }
            memstr += "];\n";
        }
    }
    return memstr;
}

std::string printDataflowEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag) {
    string str = "";

    int count = 0;

    for (auto& bnd : *bbnode_dag) {
        llvm::BasicBlock* branchSucc1;
        bool unconditional = false;

		/*
        str += "\tsubgraph cluster_" + to_string(count) + " {\n"; // draw cluster around BB
        str += "\tcolor = \"darkgreen\";\n";
        str += "\t\tlabel = \"" + string(bnd->BB->getName().str().c_str()) + "\";\n";
		*/		

        for (auto& enode : *enode_dag) {
            if (enode->type != Branch_n && enode->BB == bnd->BB) {
                std::vector<ENode*>* pastSuccs = new std::vector<ENode*>;

                int enode_index = 1;

                for (auto& enode_succ : *(enode->CntrlSuccs)) {
                    if (enode_succ->type != LSQ_ && enode_succ->type != MC_) {

                        if (enode_succ->type == Branch_) { // redundant Branch nodes--do not print

                            if (enode_succ->CntrlSuccs->size() > 0) {
                                if (enode_succ->CntrlSuccs->front()->type == Fork_ ||
                                    enode_succ->CntrlSuccs->front()->type == Branch_)
                                    str += printEdge(
                                        enode,
                                        enode_succ->CntrlSuccs
                                            ->front()); // print successor of redundant  Branch

                                else // normal print
                                    str += printEdge(enode, enode_succ);

                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out" + to_string(enode_index) + "\"";
                                str += ", to = \"in1\""; // data Branch or Fork input
                                //str += printEdgeWidth(enode, enode_succ);
                                str += "];\n";
                            }
                            if (enode_succ->JustCntrlSuccs->size() > 0) {

                                if (enode_succ->JustCntrlSuccs->front()->type == Fork_c ||
                                    enode_succ->JustCntrlSuccs->front()->type == Branch_c)
                                    str += printEdge(
                                        enode,
                                        enode_succ->JustCntrlSuccs
                                            ->front()); // print successor of redundant  Branch

                                else                                     // normal print
                                    str += printEdge(enode, enode_succ); // print successor

                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out" + to_string(enode_index) + "\"";
                                str += ", to = \"in2\""; /// Branch condition
                                //str += printEdgeWidth(enode, enode_succ);
                                str += "];\n";
                            }
                        } else if (enode->type == Branch_ &&
                                   enode_succ->type ==
                                       Fork_) { // redundant Branch nodes--do not print

                            llvm::BranchInst* BI =
                                dyn_cast<llvm::BranchInst>(enode->Instr); // determine successor BBs

                            if (BI->isUnconditional()) {
                                unconditional = true;
                            } else {
                                branchSucc1 = BI->getSuccessor(1); // successor for condition false
                            }
                            if (enode->JustCntrlPreds->size() == 1) { // connected to ctrl fork

                                str += printEdge(enode, enode_succ);
                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out1\"";

                                if (enode_succ->type == Fork_)
                                    str += ", to = \"in1\"";
                                else if (enode_succ->type == Branch_)
                                    str += ", to = \"in2\"";

                                //str += printEdgeWidth(enode, enode_succ);
                                str += "];\n";
                            }
                        } else {
                            str += printEdge(enode, enode_succ);
                            str += " [";
                            str += printColor(enode, enode_succ);

                            str += ", from = \"out" +
                                   (enode->isCntrlMg ? "2" : to_string(enode_index)) + "\"";

                            if (enode->type == Fork_ &&
                                enode->CntrlPreds->front()->type == Branch_) { // Branch condition
                                str += ", to = \"in2\"";

                            } else if (enode->isCntrlMg ||
                                       enode->type == Fork_ &&
                                           enode->CntrlPreds->front()->isCntrlMg) {
                                str += ", to = \"in1\"";

                            } else { // every other enode except branch, check successor

                                if (enode_succ->type == Inst_) {

                                    if (contains(pastSuccs,
                                                 enode_succ)) { // when fork has multiple
                                                                // connections to same node
                                        int count = 1;
                                        for (auto& past : *pastSuccs)
                                            if (past == enode_succ)
                                                count++;

                                        str += ", to = \"in" + to_string(count) + "\"";

                                    } else if (enode_succ->Instr->getOpcode() ==
                                               Instruction::Call) {

                                        int toInd = indexOf(enode_succ->CntrlPreds, enode) + 1;
                                        str += ", to = \"in" + to_string(toInd) + "\"";
                                    } else {
                                        int toInd = getOperandIndex(
                                            enode, enode_succ); // check llvm instruction operands
                                        str += ", to = \"in" + to_string(toInd) + "\"";
                                    }

                                } else { // every other successor

                                    int toInd = (enode_succ->type == Branch_n)
                                                    ? 1
                                                    : indexOf(enode_succ->CntrlPreds, enode) + 1;
                                    toInd = (enode_succ->type == End_)
                                                ? enode_succ->JustCntrlPreds->size() + toInd
                                                : toInd;
                                    str += ", to = \"in" + to_string(toInd) + "\"";
                                }
                            }
                            //str += printEdgeWidth(enode, enode_succ);
                            str += "];\n";
                        }
                        enode_index++;
                    }
                    pastSuccs->push_back(enode_succ);
                    //enode_index++;
                }

                for (auto& enode_csucc : *(enode->JustCntrlSuccs)) {

                    if ((enode_csucc->BB == enode->BB || enode_csucc->type == Sink_) &&
                        enode->type != Branch_c && !skipNodePrint(enode)) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);

                        if (enode->type == Phi_c && enode->isCntrlMg)
                            str += ", from = \"out1\"";
                        else
                            str += ", from = \"out" + to_string(enode_index) + "\"";

                        int toInd = (enode->type == Fork_ || enode->type == Branch_) ? 2 : 1;
                        if (enode->type == Cst_ && enode_csucc->type == Inst_) {
                            assert (enode_csucc->Instr->getOpcode() == Instruction::GetElementPtr);
                            toInd = indexOf(enode_csucc->JustCntrlPreds, enode) + enode_csucc->CntrlPreds->size() + 1;
                        }
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        //str += printEdgeWidth(enode, enode_csucc);
                        str += "];\n";
                    }

                    if (enode_csucc->type == End_ || enode_csucc->type == LSQ_) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);
                        str += ", from = \"out" + to_string(enode_index) + "\"";
                        int toInd = indexOf(enode_csucc->JustCntrlPreds, enode) + 1;
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        //str += printEdgeWidth(enode, enode_csucc);
                        str += "];\n";
                    }

                    enode_index++;
                }
            }
        }

        str += "\t}\n";

        for (auto& enode : *enode_dag) {
            if (enode->BB == bnd->BB) {
                for (auto& enode_succ : *(enode->CntrlSuccs)) {

                    if (enode->type == Branch_n && enode_succ->type != Branch_ &&
                        enode_succ->type != Branch_n) {
                        str += printEdge(enode, enode_succ);
                        str += " [";
                        str += printColor(enode, enode_succ);

                        int toInd = indexOf(enode_succ->CntrlPreds, enode) + 1;

                        if (enode_succ->isMux &&
                            (enode_succ->type == Phi_ || enode_succ->type == Phi_n)) {
                            int fromInd;
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_succ->BB == branchSucc1 ? 2 : 1;

                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd + 1) + "\"";
                        } else if (enode_succ->type != Sink_) {
                            int fromInd;
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_succ->BB == branchSucc1 ? 2 : 1;
                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd) + "\"";

                        } else {
                            int fromInd;
                            if (unconditional)
                                fromInd = 2;
                            else {
                                for (auto& succ : *(enode->CntrlSuccs)) {
                                    if (succ != enode_succ) {
                                        fromInd = succ->BB == branchSucc1 ? 1 : 2;
                                        break;
                                    }
                                }
                            }
                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd) + "\"";
                        }
                        //str += printEdgeWidth(enode, enode_succ);
                        str += "];\n";
                    }
                }

                for (auto& enode_csucc : *(enode->JustCntrlSuccs)) {
                    if (enode->type == Branch_c && enode->BB != NULL) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);

                        int toInd = indexOf(enode_csucc->JustCntrlPreds, enode) + 1;

                        int fromInd;

                        if (enode_csucc->type != Sink_) {
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_csucc->BB == branchSucc1 ? 2 : 1;
                        } else {
                            if (unconditional)
                                fromInd = 2;
                            else
                                fromInd = enode_csucc->BB == branchSucc1 ? 1 : 2;
                        }
                        str += ", from = \"out" + to_string(fromInd) + "\"";
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        //str += printEdgeWidth(enode, enode_csucc);
                        str += "];\n";
                    }
                }
            }
        }

        count++;
    }
    return str;
}

/*
void printDotEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, bool full) {

    if (full)
        dotfile << printMemEdges(enode_dag);
    dotfile << printDataflowEdges(enode_dag, bbnode_dag);
}
void printDotDFG(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string name,
                 bool full, std::string serial_number) {

    std::string output_filename = name;

    dotfile.open(output_filename);

    std::string infoStr;

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(std::ctime(&t));

    infoStr = "Digraph G {\n\tsplines=spline;\n";

    // infoStr += "//Graph created: " + date;
    infoStr += "//DHLS version: " + DHLS_VERSION;
    infoStr += "\" [shape = \"none\" pos = \"20,20!\"]\n";

    dotfile << infoStr;

    printDotNodes(enode_dag, full, serial_number);

    printDotEdges(enode_dag, bbnode_dag, full);

    dotfile << "}";

    dotfile.close();
}
*/
