/*
 * Authors: Ayatallah Elakhras
 * 			
 */
#include "ElasticPass/CircuitGenerator.h"
#include<iterator>
#include<algorithm>

/**
 * @brief pass a Phi_ enode to this function to calculate its SEL predicate in sel_always
 * @param 
 * @note 
 */
void CircuitGenerator::calc_MUX_SEL(std::ofstream& dbg_file, ENode* enode, std::vector<std::vector<pierCondition>>& sel_always) {
	dbg_file << "\n\n---------------------------------------------------\n";
	dbg_file << "Node name is " << getNodeDotNameNew(enode) << " of type " <<  getNodeDotTypeNew(enode) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->BB)) << " has " << enode->CntrlPreds->size() << " predecessors\n";

	std::vector<std::vector<std::vector<pierCondition>>> merged_paths_functions;
	std::vector<std::vector<pierCondition>> sum_of_products;
	std::vector<pierCondition> one_product;

	BBNode* startBB = bbnode_dag->at(0);
	BBNode* endBB = bbnode_dag->at(bbnode_dag->size()-1);

	for(int i = 0; i < enode->CntrlPreds->size(); i++) {
		dbg_file << "\nConsidering producer node name " << getNodeDotNameNew(enode->CntrlPreds->at(i)) << " of type " <<  getNodeDotTypeNew(enode->CntrlPreds->at(i)) << " in BasicBlock number " << aya_getNodeDotbbID(BBMap->at(enode->CntrlPreds->at(i)->BB)) << "\n";
		BBNode* virtualBB = new BBNode(nullptr , bbnode_dag->size());
		bbnode_dag->push_back(virtualBB);
		BasicBlock *llvm_predBB = (dyn_cast<PHINode>(enode->Instr))->getIncomingBlock(i);
		// insert a virtualBB at the edge corresponding to this producer
		insertVirtualBB(enode, virtualBB, llvm_predBB);
		// find f_i which is the conditions of pierBBs in the paths between the START and the virtualBB we just inserted, 
		////////////////////////// clearing the datastructures 
		sum_of_products.clear();
		// find all the paths between the startBB and the virtualBB
		/////////////////// variables needed inside enumerate_paths
		bool *visited = new bool[bbnode_dag->size()];
		int *path = new int[bbnode_dag->size()];
		int path_index = 0; 
		// needed to hold the enumerated paths 
		std::vector<std::vector<int>>* final_paths = new std::vector<std::vector<int>>;
		// 2nd enumerate all the paths between the current producer and this
		// reseting the data structures for a new prod_con pair
		for(int i = 0; i < bbnode_dag->size(); i++){
			visited[i] = false;
		}
		final_paths->clear();
		/////////////////////////// end of clearing the datastructures
		enumerate_paths_dfs_start_to_end(startBB, virtualBB, visited, path, path_index, final_paths);
		dbg_file << "Found " << final_paths->size() << " paths\n";
		// loop over the final_paths and identify the pierBBs with their conditions
		for(int kk = 0; kk < final_paths->size(); kk++) {
			dbg_file << "Printing path number " << kk << " details\n";
			for(int hh = 0; hh < final_paths->at(kk).size(); hh++) {
				dbg_file << final_paths->at(kk).at(hh) + 1 << " -> ";
			}
			dbg_file << "\n";

			// identifying the pierBBs in 1 path
			std::vector<int>* pierBBs = new std::vector<int>;
			find_path_piers(final_paths->at(kk), pierBBs, virtualBB, endBB);

			///// Debugging to check the pierBBs 
			dbg_file << "\n Printing all the piers of that path!\n";
			for(int u = 0; u < pierBBs->size(); u++) {
				dbg_file << pierBBs->at(u) + 1 << ", ";
			}	
			dbg_file << "\n\n";
			/////////////////////////////////////
			// loop on piers of that path to construct a product out of them, then push this product to the sum_of_products before moving to a new path
			one_product.clear();
			for(int u = 0; u < pierBBs->size(); u++) {
				pierCondition cond;
				cond.pierBB_index = pierBBs->at(u);

				int next_pierBB_index;
				if(u == pierBBs->size()-1) {
					next_pierBB_index = BBMap->at(llvm_predBB)->Idx;
				} else {
					next_pierBB_index = pierBBs->at(u+1);
				} 
				if(is_negated_cond(pierBBs->at(u), next_pierBB_index, enode->BB ,dbg_file)) {
					cond.is_negated = true;
				} else {
					cond.is_negated = false;
				}
				one_product.push_back(cond);
			}
			sum_of_products.push_back(one_product);

		}  // end of loop on all paths between START and the virtualBB

		// we should have 1 product per path!
		assert(sum_of_products.size() == final_paths->size());

		/// PRINTING the sum_of_products of all paths to that virtualBB
		dbg_file << "\nPrinting the sum of products from START to that virtualBB!\n";
		for(int u = 0; u < sum_of_products.size(); u++) {
			for(int v = 0; v < sum_of_products.at(u).size(); v++) {
				if(sum_of_products.at(u).at(v).is_negated)
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << sum_of_products.at(u).at(v).pierBB_index +1 << " & ";
			}
			dbg_file << " + ";
		}
		dbg_file << "\n\n";

		std::vector<std::vector<pierCondition>>* sum_of_products_ptr = &sum_of_products;
		delete_repeated_paths(sum_of_products_ptr);

		removeVirtualBB(enode, virtualBB, llvm_predBB); 	
		merged_paths_functions.push_back(sum_of_products);

	}  // end of the enode->CntrlPreds->size() loops

	getSOP(dbg_file, merged_paths_functions, sel_always);
}

/**
 * @brief fills the sel_always structure which represents the SEL of the MUX!!
 * @param 
 * @note 
 */
void CircuitGenerator::getSOP(std::ofstream& dbg_file, const std::vector<std::vector<std::vector<pierCondition>>>& merged_paths_functions, std::vector<std::vector<pierCondition>>& sel_always) {
	// DEBUGGING.. check the 2 functions (f0 and f1) of this Merge component!
	dbg_file << "The current Merge node the following functions: \n\t";
	for(int iter = 0; iter < merged_paths_functions.size(); iter++) {
		dbg_file << "f" << iter << " = ";
		for(int y = 0; y < merged_paths_functions.at(iter).size(); y++) {
			// take the conditions from a product by product
			for(int z = 0; z <  merged_paths_functions.at(iter).at(y).size(); z++) {
				if(merged_paths_functions.at(iter).at(y).at(z).is_negated)
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << merged_paths_functions.at(iter).at(y).at(z).pierBB_index +1 << " & ";
			}
			dbg_file << " + ";
		}
		dbg_file << "\n\t";
	}	
	/////////////////////// end of debugging f0 and f1 of that phi enode	
	
	// extract the sum of minterms (i.e., f1 + don't-cares) in a binary string form
	std::vector<std::string>* minterms_only_in_binary = new std::vector<std::string>;
	std::vector<std::string>* minterms_plus_dont_cares_in_binary = new std::vector<std::string>;
	std::vector<int>* condition_variables = new std::vector<int>;

	int number_of_dont_cares = get_binary_sop(merged_paths_functions, minterms_only_in_binary, minterms_plus_dont_cares_in_binary, condition_variables, dbg_file);
	// we should have at least a single minterm!
	assert(minterms_plus_dont_cares_in_binary->size() > 0);

	// call the initialization object of Quine-McCLuskey
	Quine_McCluskey boolean_simplifier;
	int number_of_bits = minterms_plus_dont_cares_in_binary->at(0).length();
	int number_of_minterms = minterms_plus_dont_cares_in_binary->size() - number_of_dont_cares;
	boolean_simplifier.initialize(number_of_bits, number_of_minterms, number_of_dont_cares, minterms_only_in_binary, minterms_plus_dont_cares_in_binary);

	boolean_simplifier.solve(dbg_file);
	std::string bool_expression_name = "SEL_ALWAYS";
	boolean_simplifier.displayFunctions(dbg_file, bool_expression_name);
	std::vector<std::string> essential_sel_always =
			boolean_simplifier.getEssentialPrimeImpl();
	

	sel_always = essential_impl_to_pierCondition(essential_sel_always);

	///////// debugging to check the sel_always expression
	dbg_file << "\n\nSEL_ALWAYS = ";
	for(int u = 0; u < sel_always.size(); u++) {
		for(int v = 0; v < sel_always.at(u).size(); v++) {
			if(sel_always.at(u).at(v).is_negated) 
				dbg_file << "not C";
			else
				dbg_file << "C";

			dbg_file << sel_always.at(u).at(v).pierBB_index +1 << " & ";
		}
		dbg_file << " + "; 
	} 
	dbg_file << "\n\n";
}


