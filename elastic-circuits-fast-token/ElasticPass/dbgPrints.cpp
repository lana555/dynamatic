#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"


// In case of debugging to check the post-dominator tree
void CircuitGenerator::printPostDomTree(Function& F) {
	std::ofstream post_dom_file;
	post_dom_file.open("post_dom_tree.txt");
	for(auto& bbnode_1 : *bbnode_dag) {
		for(auto& bbnode_2 : *bbnode_dag) {
			//if(bbnode_1 != bbnode_2){
				if(!my_post_dom_tree->properlyDominates(bbnode_2->BB, bbnode_1->BB) ) {
					post_dom_file << "BB_" << bbnode_2->Idx + 1 << " doesn't post dominate " << "BB_" << bbnode_1->Idx + 1<< "\n";
				}
			//} 
		} 
	} 

	post_dom_file.close();

}

void CircuitGenerator::PrintLoopHeaders(Function& F, LoopInfo& LI, bool cntrl_path_flag) {
	std::ofstream t_file;
	std::string file_name = (cntrl_path_flag) ? "aya_loops_control.txt": "aya_loops_data.txt";
	t_file.open(file_name.c_str());
	Loop* cons_loop;
	std::vector<ENode*>* enode_succs; 
	
	for (auto& enode : *enode_dag) {
		// loop over its consumers (successor nodes)
		enode_succs = (cntrl_path_flag) ? enode->JustCntrlSuccs : enode->CntrlSuccs;
		for (auto& succ : *enode_succs) {
			// they must be in different BBs
			if (enode->BB != succ->BB) {
				//////////////////////////////////////////////////////////
				cons_loop = LI.getLoopFor(succ->BB);

				while(cons_loop!=nullptr) {
					if(!cons_loop->contains(enode->BB)) {
						BasicBlock *loop_header = cons_loop->getHeader();
						t_file << "\nFor Producer enode name " << getNodeDotNameNew(enode) << ", with type " << getNodeDotTypeNew(enode) << ", in BBnode " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << "\n";
						t_file << "And Consumer enode name " << getNodeDotNameNew(succ) << ", with type " << getNodeDotTypeNew(succ) << ", in BBnode " << aya_getNodeDotbbID(BBMap->at(succ->BB)) << "\n";
						t_file << "Consumer is in a loop that the producer is not in and the Loop Header is BBNode number: " << loop_header->getName().str().c_str() << "\n";

						// THINK OF WHERE TO PLACE THIS STATEMENT?????
						cons_loop = cons_loop->getParentLoop(); 
						// assert(cons_loop == nullptr);

						if(loop_header != succ->BB || (succ->type != Phi_ && succ->type != Phi_n && succ->type != Phi_c)){
							t_file << "\nA Phi should be inserted in the Loop Header!!\n";
						}
					} else {
						cons_loop = nullptr;
					} 

				}
	//////////////////////////////////////////////////////////
			}
		}

	}
} 


void CircuitGenerator::PrintBridgesIndices(BBNode* producer_BB, BBNode* consumer_BB, std::vector<int>* bridges_indices) {
    std::ofstream dbg_file;
    dbg_file.open("NOW_bridges_indices.txt");
    
    dbg_file << "\n-------------------------------------\n";
    dbg_file << "Bridges for (Producer,Consumer) = (" <<            producer_BB->Idx + 1 << ", " << consumer_BB->Idx + 1 << "):  ";
    if(bridges_indices->size() == 0) {
        dbg_file << "No Bridges for this pair!!";
    }
    for(int k = 0; k < bridges_indices->size(); k++)
        dbg_file << bridges_indices->at(k) + 1 << ", ";

    dbg_file << "\n\n";

    // dbg_file.close();    
}


// print the entire enode_dag after transformations for debugging!!
void CircuitGenerator::dbgEnode_dag(std::string file_name) {
	std::ofstream dbg_7;
	dbg_7.open(file_name);

	// looping over the BBs and printing the enodes in each of them
	for (auto& bbnode : *bbnode_dag) {
		dbg_7 << "\n----------------------------------------\n";
		dbg_7 << "\n----------------------------------------\n";
		dbg_7 << "BBnode number " << bbnode->Idx + 1 << "\n";
		for (auto& enode : *enode_dag) {
			// check if this enode is inside the current bbnode
			if((BBMap->at(enode->BB)) == bbnode) {
				dbg_7 << "\n\n----------------------------------------\n";
				dbg_7 << "Operation Name: " << enode->Name << "\n"; 
				dbg_7 << "\n\nSuccessors are: ";
				for(auto& succ : *enode->CntrlSuccs) {
					dbg_7 << "Succ in BB number " << (BBMap->at(succ->BB))->Idx + 1 << " with Operation_Name: " << succ->Name << ", "; 
				}

				dbg_7 << "\n\nPredecessors are: ";
				for(auto& pred : *enode->CntrlPreds) {
					dbg_7 << "Pred in BB number " << (BBMap->at(pred->BB))->Idx + 1 << " with Operation_Name: " << pred->Name << ", "; 
				}
				dbg_7 << "\n\n";
			}
		}
	}

	dbg_7.close();
}
