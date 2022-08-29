#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

//////////////////////////////////////////////// OLD connect_prods_cons()
/*void CircuitGenerator::ConnectProds_Cons() {
	// Temporary!!! For Debugging
    //std::ofstream dbg_file_paths;
	//dbg_file_paths.open("paths_check.txt");

	//std::ofstream dbg_file_bridges;
	//dbg_file_bridges.open("bridges_check.txt");

	//dbg_file.open("bridges_indices.txt");

	// variables needed inside enumerate_paths
	int cfg_size = bbnode_dag->size();
	bool *visited = new bool[cfg_size];

	int *path = new int[cfg_size];
	int path_index = 0; 

	// Data structures that will hold the results of different intermediate analysis
	
	// vector of all the possible paths between the producer and consumer
	std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;

	// vector of indices of bbs in the bbnode_dag that are bridges in any of the paths
	std::vector<int>* bridges_indices = new std::vector<int>;

	// For each enode operation, loop over its producers
	for (auto& enode : *enode_dag) {
		for (auto& pred : *enode->CntrlPreds) {
			dbg_file_paths << "\nChecking paths between: enode= " << enode->Name << " which is in BB number: " << (BBMap->at(enode->BB))->Idx  << " and pred= " << pred->Name << " which is in BB number: " << (BBMap->at(pred->BB))->Idx << "::\n"; 

			dbg_file_bridges << "\nChecking bridges between: enode= " << enode->Name << " which is in BB number: " << (BBMap->at(enode->BB))->Idx  << " and pred= " << pred->Name << " which is in BB number: " << (BBMap->at(pred->BB))->Idx << "::\n"; 

// Temporary for Debugging!!!!
			// dbg_file << "\n\nProducer operation name is " << pred->Name << " and in Block number " << (BBMap->at(pred->BB))->Idx + 1 << ".. COnsumer opertaion name is " << enode->Name << " and in Block number " << (BBMap->at(enode->BB))->Idx + 1 << "\n";
			
			// we access BBNodes through the BBMap since an ENode object doesn't hold anything in the BBNode object that is inside it
			// if ((BBMap->at(enode->BB))->Idx != (BBMap->at(pred->BB))->Idx){


			if (enode->BB != pred->BB) {
			// reseting the data structures for a new prod_con pair
				for(int i = 0; i < cfg_size; i++){
					visited[i] = false;
				}
				path_index = 0; 

				final_paths->clear();
				bridges_indices->clear();

			// calling different analysis methods
				enumerate_paths_dfs(BBMap->at(enode->BB), BBMap->at(pred->BB), visited, path, path_index, final_paths);

				// temporary for debugging!!
				
				for(int dd = 0; dd < final_paths->size(); dd++) {
					if(final_paths->at(dd).size() == 0) {	
						dbg_file_paths << "\nNO PATH!!\n";
					}
					for(int de = 0; de < final_paths->at(dd).size(); de++) {
						dbg_file_paths << final_paths->at(dd).at(de) << ", ";
					}
					dbg_file_paths << "\n";
				}

			// loop over all_paths and returns indices of bridges
				FindBridgesHelper(BBMap->at(enode->BB), final_paths, bridges_indices);
				if(bridges_indices->size() == 0) {
					dbg_file_bridges <<  " No bridges exist!! \n"; 
				}
				for(int hh = 0; hh < bridges_indices->size(); hh++) {
					dbg_file_bridges << bridges_indices->at(hh) << ", ";
				} 
				dbg_file_bridges << "\n"; 

			// Print the indices to file to check
				//PrintBridgesIndices(BBMap->at(pred->BB), BBMap->at(enode->BB), bridges_indices);

			// transform the enode_dag to account for the Bridges
				AddBridges(pred, enode, bridges_indices);
				
			}
		}
	}

	delete[] visited;
	delete[] path;

	delete final_paths;
	delete bridges_indices;

	// Temporary for debugging!!!
	dbg_file.close();
}
*/

////// oLD FUNCTION
/* void CircuitGenerator::arrangePredsSuccss(std::vector<ENode*> branches) {
	for(int j = 1; j < branches.size()-1; j++) {
		// the data should be always placed at CntrlPreds.at(0)
		branches.at(j)->CntrlPreds->push_back(branches.at(j-1));	

// Need to search for LLVM branch in the same BB to extract the condition from it
		for(auto& enode : *enode_dag) {
			if(enode->type == Branch_ && enode->BB == branches.at(j)->BB) { 
				// placing the condition at CntrlPreds.at(1)
				branches.at(j)->CntrlPreds->push_back(enode->CntrlPreds->at(1));

				// access the predecessor that acts as the condition of the existing branch and push the new branch to its successors.
				enode->CntrlPreds->at(1)->CntrlSuccs->push_back(branches.at(j));

				// deciding if branches.at(j+1) should be in the true or false branch.. Check if branches.at(j+1) is post-dominating llvm's branch succs.at(0) or succs.at(1)
					IMP Alert!! This assumes that the post-dominator tree counts a BB to be post-dominating itself (i.e. in case the two enodes happened to be in the same BB) 
				
				if(my_post_dom_tree->properlyDominates(branches.at(j+1)->BB, 
						enode->CntrlSuccs->at(0)->BB)) {
					branches.at(j)->CntrlSuccs->push_back(branches.at(j+1));
					// add a sink node in .at(1)
					ENode* sink_node = new ENode(Sink_, "sink");
					sink_node->id = sink_id++;
					sink_node->CntrlPreds->push_back(branches.at(j));	
					branches.at(j)->CntrlSuccs->push_back(sink_node);
					enode_dag->push_back(sink_node);
				} else {
					// It must then be post-dominating the 2nd successor of the LLVm branch, but will do an assert just in case anything is going wrong
					assert(my_post_dom_tree->properlyDominates(branches.at(j+1)->BB, enode->CntrlSuccs->at(1)->BB));

					// add sink 1st in .at(0)
					ENode* sink_node = new ENode(Sink_, "sink");
					sink_node->id = sink_id++;
					sink_node->CntrlPreds->push_back(branches.at(j));	
					branches.at(j)->CntrlSuccs->push_back(sink_node);
					enode_dag->push_back(sink_node);

					// then put the next branch in .at(1)
					branches.at(j)->CntrlSuccs->push_back(branches.at(j+1));
				}
					
			}


		}

	}
} */

///////////////////////////////// Adding bridges in the data-less control path
void CircuitGenerator::AddBridges_Cntrl(ENode* producer, ENode* consumer, std::vector<int>* bridges_indices) {
	std::vector<ENode*> branches;

	// if there is actually a Bridge
	if(bridges_indices->size() > 0) {
		// removing the direct connection between the producer and consumer
		removeNode(producer->JustCntrlSuccs, consumer);
		removeNode(consumer->JustCntrlPreds, producer);
		
		branches.push_back(producer);
		// create all the needed branches and put in a vector
		for(int i = bridges_indices->size()-1; i >= 0; i--) {
			ENode* branchC = new ENode(Branch_c, "branchC", bbnode_dag->at(bridges_indices->at(i))->BB);
			branchC->id = branch_id++;
			branches.push_back(branchC);
			enode_dag->push_back(branchC);

			// adding this branchC to the vector of control enodes in its BBnode
			bbnode_dag->at(bridges_indices->at(i))->Cntrl_nodes->push_back(branchC);
		}
		branches.push_back(consumer);
		
		producer->JustCntrlSuccs->push_back(branches.at(1));

		for(int j = 1; j < branches.size()-1; j++) {
			// the data should be always placed at CntrlPreds.at(0)
			branches.at(j)->JustCntrlPreds->push_back(branches.at(j-1));	

// 1st need to search for LLVM branch in the same BB
			for(auto& enode : *enode_dag) {
				if(enode->type == Branch_ && enode->BB == branches.at(j)->BB) 				{   // if branch braces
			// THINK AGAIN: we push the enode's CntrlPreds not the JustCntrlPreds, right?? Because it's not part of the control path, rather we just use it to get the condition
					branches.at(j)->JustCntrlPreds->push_back(enode->CntrlPreds->at(1));
					enode->CntrlPreds->at(1)->JustCntrlSuccs->push_back(branches.at(j));

			// deciding if branches.at(j+1) should be in the true or false branch.. Check if branches.at(j+1) is post-dominating llvm's succs.at(0) or succs.at(1)
					if(my_post_dom_tree->properlyDominates(branches.at(j+1)->BB, enode->CntrlSuccs->at(0)->BB)) {

						branches.at(j)->JustCntrlSuccs->push_back(branches.at(j+1));

						// add a sink node in .at(1)
						ENode* sink_node = new ENode(Sink_, "sink");
						sink_node->id = sink_id++;
						sink_node->JustCntrlPreds->push_back(branches.at(j));	
						branches.at(j)->JustCntrlSuccs->push_back(sink_node);
						enode_dag->push_back(sink_node);
					} else {
						// It must then be post-dominating the 2nd successor of the LLVm branch, but will do an assert just in case anything is going wrong
						assert(my_post_dom_tree->properlyDominates(branches.at(j+1)->BB, enode->CntrlSuccs->at(1)->BB));

						// add sink 1st in .at(0)
						ENode* sink_node = new ENode(Sink_, "sink");
						sink_node->id = sink_id++;
						sink_node->JustCntrlPreds->push_back(branches.at(j));	
						branches.at(j)->JustCntrlSuccs->push_back(sink_node);
						enode_dag->push_back(sink_node);

						// then put the next branch in .at(1)
						branches.at(j)->JustCntrlSuccs->push_back(branches.at(j+1));

					}
					
				}


			}

		}

		consumer->JustCntrlPreds->push_back(branches.at(branches.size()-2));
	}
}

void CircuitGenerator::ConnectProds_Cons_Cntrl() {
	// Temporary!!! For Debugging
	//dbg_file.open("bridges_indices.txt");

	// variables needed inside enumerate_paths
	int cfg_size = bbnode_dag->size();
	bool *visited = new bool[cfg_size];

	int *path = new int[cfg_size];
	int path_index = 0; 

	// Data structures that will hold the results of different intermediate analysis
	std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
	std::vector<int>* bridges_indices = new std::vector<int>;

	// For each enode operation, loop over its producers in the Control Path
	for (auto& enode : *enode_dag) {
		for (auto& pred : *enode->JustCntrlPreds) {
			// we access BBNodes through the BBMap since an ENode object doesn't hold anything in the BBNode object that is inside it
			// if ((BBMap->at(enode->BB))->Idx != (BBMap->at(pred->BB))->Idx){
			if (enode->BB != pred->BB) {
			// reseting the data structures for a new prod_con pair
				for(int i = 0; i < cfg_size; i++){
					visited[i] = false;
				}
				path_index = 0; 

				final_paths->clear();
				bridges_indices->clear();

			// calling different analysis methods
				enumerate_paths_dfs(BBMap->at(enode->BB), BBMap->at(pred->BB), visited, path, path_index, final_paths);

				// for debugging purposes, to print the paths enumerated
				////////////////////// For debugging, checking the final paths 
				if(pred->BB != nullptr && enode->BB != nullptr)
					dbg_file_paths << "Producer is in BB: " << (BBMap->at(pred->BB))->Idx + 1 << ", Consumer is in BB: " << (BBMap->at(enode->BB))->Idx + 1 << "\n";
				else 	
					if(enode->BB == nullptr) {
						dbg_file_paths << "Producer is in BB: " << (BBMap->at(pred->BB))->Idx + 1 << ", Consumer is in BB: NULL" << "\n";
					}

				for(int hehe = 0; hehe < final_paths->size(); hehe++) {
					dbg_file_paths << "Path number: " << hehe << "{ ";
					for(int gg = 0; gg < final_paths->at(hehe).size(); gg++) {
						dbg_file_paths << final_paths->at(hehe).at(gg) + 1 << ", ";
					} 
					dbg_file_paths << "}\n";
				}  

				/////////////////////////

			// loop over all_paths and returns indices of bridges
				FindBridgesHelper(BBMap->at(enode->BB), final_paths, bridges_indices);

			// Print the indices to file to check
				//PrintBridgesIndices_Cntrl(BBMap->at(pred->BB), BBMap->at(enode->BB), bridges_indices);

			// transform the enode_dag to account for the Bridges
				AddBridges_Cntrl(pred, enode, bridges_indices);
				
			}
		}
	}

	delete[] visited;
	delete[] path;

	delete final_paths;
	delete bridges_indices;

	// Temporary for debugging!!!
	//dbg_file.close();
}


