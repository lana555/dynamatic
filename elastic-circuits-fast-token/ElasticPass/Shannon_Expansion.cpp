#include "ElasticPass/Shannon_Expansion.h"

/**
 * @brief  
 * @param 
 * @return 
 */
void Shannon_Expansion::printMUX(std::ofstream& dbg_file, MUXrep mux) {
	dbg_file << "\n************ About to print the details of a MUX ***********\n";
	// first check if any of the inputs of the MUX are MUXes themselves
	if(mux.in0_preds_muxes.size() == 0 && mux.in1_preds_muxes.size() == 0) {
		dbg_file << "\tThe MUX has non-MUX inputs!!!\n";
	} else {
		dbg_file << "\tThe MUX has MUX inputs!!!\n";
	}

	dbg_file << "\tThe SEL of the MUX is: " << mux.cond_var_index + 1 << "\n\n";

	//assert(mux.in0.size() == 1 && mux.in1.size() == 1);
	dbg_file << "\tThe size of in0 of the MUX is: " << mux.in0.size() << " and the expressions are \n\n";
	for(int i = 0; i < mux.in0.size(); i++) {
		for(int j = 0; j < mux.in0.at(i).size(); j++) {
			if(mux.in0.at(i).at(j).is_const) {
				dbg_file << mux.in0.at(i).at(j).const_val << " & ";
			} else {
				if(mux.in0.at(i).at(j).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << mux.in0.at(i).at(j).var_index +1 << " & ";
			}
		}
		dbg_file << " + "; 
	}
	dbg_file << "\n\n";

	dbg_file << "\tThe size of in1 of the MUX is: " << mux.in1.size() << " and the expressions are \n\n";
	for(int i = 0; i < mux.in1.size(); i++) {
		for(int j = 0; j < mux.in1.at(i).size(); j++) {
			if(mux.in1.at(i).at(j).is_const) {
				dbg_file << mux.in1.at(i).at(j).const_val << " & ";
			} else {
				if(mux.in1.at(i).at(j).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << mux.in1.at(i).at(j).var_index +1 << " & ";
			}
		}
		dbg_file << " + "; 
	}
	dbg_file << "\n\n";


}

int Shannon_Expansion::myFind(std::vector<boolVariable> vec, int var_index) {
	// returns -1 if not found and the index if found!
	for(int i = 0; i < vec.size(); i++) {
		if(vec.at(i).var_index == var_index)
			return i;
	} 

	return -1;
} 

std::vector<std::vector<Shannon_Expansion::boolVariable>> Shannon_Expansion::getNegCofactor(int var_index, std::vector<std::vector<boolVariable>> bool_funct) {
	
	std::vector<std::vector<boolVariable>> in0;
	for(int i = 0; i < bool_funct.size(); i++) {
		// looping on 1 product
		int idx = myFind(bool_funct.at(i), var_index);
		if(idx != -1) {
			// the var_index is found in this product, so check if it's negated or not
			if(!bool_funct.at(i).at(idx).is_negated) {
				// so passing var_index as 0 would evaluate the expression as a 0!
				boolVariable const_var;
				const_var.is_const = true;
				const_var.const_val = 0;

				std::vector<boolVariable> one_product;
				one_product.push_back(const_var);

				in0.push_back(one_product);

			} else {
				// so passing var_index as 0 would put a 1 in the product
				if(bool_funct.at(i).size() == 1) {
					boolVariable const_var_;
					const_var_.is_const = true;
					const_var_.const_val = 1;

					std::vector<boolVariable> one_product_;
					one_product_.push_back(const_var_);

					in0.push_back(one_product_);

				} else {
					assert(bool_funct.at(i).size() > 1);
					// erase that variable from the product
					auto pos = idx + bool_funct.at(i).begin();
					bool_funct.at(i).erase(pos); 
					// push the product
					in0.push_back(bool_funct.at(i));
				} 

			}

		} else {
			// var_index is not part of the product, so pass as is
			in0.push_back(bool_funct.at(i));
		} 
	}  

	return in0;
}

std::vector<std::vector<Shannon_Expansion::boolVariable>> Shannon_Expansion::getPosCofactor(int var_index, std::vector<std::vector<boolVariable>> bool_funct) {
	
	std::vector<std::vector<boolVariable>> in1;
	for(int i = 0; i < bool_funct.size(); i++) {
		// looping on 1 product
		int idx = myFind(bool_funct.at(i), var_index);
		if(idx != -1) {
			// the var_index is found in this product, so check if it's negated or not
			if(bool_funct.at(i).at(idx).is_negated) {
				// so passing var_index as 1 would evaluate the expression as a 0!
				boolVariable const_var;
				const_var.is_const = true;
				const_var.const_val = 0;

				std::vector<boolVariable> one_product;
				one_product.push_back(const_var);

				in1.push_back(one_product);

			} else {
				// so passing var_index as 1 would put a 1 in the product
				if(bool_funct.at(i).size() == 1) {
					boolVariable const_var_;
					const_var_.is_const = true;
					const_var_.const_val = 1;

					std::vector<boolVariable> one_product_;
					one_product_.push_back(const_var_);

					in1.push_back(one_product_);

				} else {
					assert(bool_funct.at(i).size() > 1);
					// erase that variable from the product
					auto pos = idx + bool_funct.at(i).begin();
					bool_funct.at(i).erase(pos); 
					// push the product
					in1.push_back(bool_funct.at(i));
				} 

			}

		} else {
			// var_index is not part of the product, so pass as is
			in1.push_back(bool_funct.at(i));
		} 
	}  

	return in1;
}

std::vector<std::vector<Shannon_Expansion::boolVariable>> Shannon_Expansion::simplify_sop(std::vector<std::vector<boolVariable>> funct) {
	// you need to simplify only if you have more than one product!
	assert(funct.size() > 0);
	if(funct.size() < 2)
		return funct;

	// loop over the different products
	for(int i = 0; i < funct.size(); i++) {
		if(funct.at(i).size() == 1) {
			// if one of the products has just a single variable, check if this variable is a constant
			if(funct.at(i).at(0).is_const) {
				if(funct.at(i).at(0).const_val == 0) {
					// erase this product
					funct.at(i).erase(funct.at(i).begin());
				} else {
					assert(funct.at(i).at(0).const_val == 1);
					// construct a new function made up of ONLY constant 1!
					boolVariable const_var_;
					const_var_.is_const = true;
					const_var_.const_val = 1;

					std::vector<boolVariable> one_product_;
					one_product_.push_back(const_var_);
					std::vector<std::vector<boolVariable>> new_funct;
					new_funct.push_back(one_product_);
					// stop looping, return!!
					return new_funct;
				}
			} 
		} 
		// Otherwise, if there are multiple variables within a product, do nothing!
	} 

	// final clearance
	for(int i = 0; i < funct.size(); i++) {
		if(funct.at(i).size() == 0) {
			funct.erase(funct.begin() + i);
		}
	}

	return funct;
}

Shannon_Expansion::MUXrep Shannon_Expansion::getCofactor_wrapper(int var_index, std::vector<std::vector<boolVariable>> bool_funct, std::vector<int>* condition_variables) {

	MUXrep resulting_mux;

	// to get in0, for every product that contains the var_index, it should either be replaced with a 0, or with a 1 (if the product contains no other variables) or to the other variable in the product. The product should remain as is if it doesn't contain the var_index

	// constructing in0
	std::vector<std::vector<boolVariable>> in0 = getNegCofactor(var_index, bool_funct);
	// constructing in1
	std::vector<std::vector<boolVariable>> in1 = getPosCofactor(var_index, bool_funct);

	// e.g., if 1 product is 1, then the whole expression should be 1! If 1 product is 0, and there are other products, clear that 0 product!!
	std::vector<std::vector<boolVariable>> simplified_in0 = simplify_sop(in0);
	std::vector<std::vector<boolVariable>> simplified_in1 = simplify_sop(in1);

	resulting_mux.in0 = simplified_in0;
	resulting_mux.in1 = simplified_in1;
	resulting_mux.cond_var_index = var_index; // it is the correct index of the condition of a specific BB

	resulting_mux.is_valid = true;
	// return the multiplexer resulting from the cofactor of the bool_funct w.r.t the var_index
	return resulting_mux;

} 

std::vector<std::vector<Shannon_Expansion::boolVariable>>Shannon_Expansion::string_to_vec_funct(std::vector<std::string> bool_funct, std::vector<int>* condition_variables) {
	std::vector<std::vector<boolVariable>> funct_vec;
	std::vector<boolVariable> one_product;

	for(int i = 0; i < bool_funct.size(); i++) {
		one_product.clear();

		// sanity check
		assert(bool_funct.at(i).length() == condition_variables->size());

		for(int j = 0; j < bool_funct.at(i).length(); j++) {
			// skip if there is a '-' in place of that condition!
			if(bool_funct.at(i).at(j) == '-' || bool_funct.at(i).at(j) == 'x') {  // 14/06/2022: added an extra condition to skip if there is x too!!
				continue;
			} else {
				boolVariable cond;
				cond.var_index = condition_variables->at(j);
				if(bool_funct.at(i).at(j) == '0') {
					cond.is_negated = true;
				} else {
					cond.is_negated = false;
				}
			
				// I assume that they come out of quineMcCluskey as variables not constants
				cond.is_const = false;
				cond.const_val = -1;
				one_product.push_back(cond);
			}

		} 
		funct_vec.push_back(one_product);
	}
	
	return funct_vec;
}

bool Shannon_Expansion::isCondinFunct(int var_index, std::vector<std::vector<boolVariable>> bool_funct) {
	bool found = false;
	for(int i = 0; i < bool_funct.size(); i++) {
		// looping on 1 product
		for(int j = 0; j < bool_funct.at(i).size(); j++) {
			if(bool_funct.at(i).at(j).var_index == var_index) {	
				found = true;
				break;  // found the variable in this product so stop looping
			}
		}
		if(found == true)
			break;
	}
	return found;
} 

// I assume that the variable you do co-factor w.r.t is always the variable with the smallest index! So, this function checks all the variables in the passed sum of products and returns the variable with the smallest index
int Shannon_Expansion::getLeastVarIdx(std::vector<std::vector<boolVariable>> funct_vec) {
	std::vector<int> var_indices;
	for(int i = 0; i < funct_vec.size(); i++) {
		for(int j = 0; j < funct_vec.at(i).size(); j++) {
			var_indices.push_back(funct_vec.at(i).at(j).var_index);
		}
	}

	// sort the var_indices to get the smallest possible index
		// note that some of the indices might be -1 (corresponding to a constant product), therefore, take care to consider only indices >= 0!!
	sort(var_indices.begin(), var_indices.end());
	for(int i = 0; i < var_indices.size(); i++) {
		if(var_indices.at(i) >= 0)
			return var_indices.at(i);
	}

	return -1;  // if the sum of products is all constants!!

}

bool Shannon_Expansion::hasMultipleVariables(std::vector<std::vector<boolVariable>>funct_vec, std::ofstream& dbg_file) {
	dbg_file << "\n\nInside hasMultipleVariables!\n";
	dbg_file << "\nThe list of var_index of these variables is: \n";
	std::vector<int> present_variables;
	// count only the variables (not the constants), so ignore if var_index = -1
	for(int i = 0; i < funct_vec.size(); i++) {
		for(int j = 0; j < funct_vec.at(i).size(); j++) {
			dbg_file << funct_vec.at(i).at(j).var_index << ", ";
			if(funct_vec.at(i).at(j).var_index != -1) {
				auto pos = std::find(present_variables.begin(), present_variables.end(), funct_vec.at(i).at(j).var_index);
				if(pos == present_variables.end()) 
					present_variables.push_back(funct_vec.at(i).at(j).var_index);
			}
		} 
		dbg_file << "\n";
	}

	dbg_file << "The size of present_variables is: " << present_variables.size() << "\n";
	dbg_file << "Leaving hasMultipleVariables!\n\n";
	return(present_variables.size() > 1);
}

// TODO I have a constraining assumption that I need to work on later:
	// I can have ONLY up to 2 levels of Muxes (i.e., 1 mux fed by another mux at either or both of the 2 inputs; that's the worst case I can handle!)
Shannon_Expansion::MUXrep Shannon_Expansion::apply_shannon(std::vector<std::string> essential_sel_always, std::vector<int>* condition_variables, std::ofstream& dbg_file) {

	std::vector<std::vector<boolVariable>> funct_vec = string_to_vec_funct(essential_sel_always, condition_variables);

	/////////// TEMPORARILY FOR DEBUGGING!!!
	dbg_file << "\n**********************************************\n";
	dbg_file << "\n\nPrinting the funct_vec of the expression initially\n";
	for(int i = 0; i < funct_vec.size(); i++) {
		for(int j = 0; j < funct_vec.at(i).size(); j++) {
			if(funct_vec.at(i).at(j).is_const) {
				dbg_file << funct_vec.at(i).at(j).const_val << " & ";
			} else {
				if(funct_vec.at(i).at(j).is_negated) 
					dbg_file << "not C";
				else
					dbg_file << "C";

				dbg_file << funct_vec.at(i).at(j).var_index +1 << " & ";
			}

		} 
		dbg_file << " + "; 
	}
	dbg_file << "\n\n";
	/////////////////////////////////////////////////

	MUXrep resulting_mux, resulting_mux_in0, resulting_mux_in1;

	// 1st check that there are more than one distinct variables!
	if(hasMultipleVariables(funct_vec, dbg_file)) {
		
		////////////////////////////////////////////////////////////////////
		dbg_file << "\nThe initial expression has multiple variables! \n";

		// TODO: this is where I should better choose the order that I do co-factor w.r.t
		int var_index = getLeastVarIdx(funct_vec);

		dbg_file << "\nThe least var index of the initial expression is: " << var_index << "\n";
		if(var_index >= 0) {
			resulting_mux = getCofactor_wrapper(var_index, funct_vec, condition_variables);
				
			/////////// TEMPORARILY FOR DEBUGGING
			dbg_file << "\nPrinting the in0 details of the resulting MUX\n";
			for(int i = 0; i < resulting_mux.in0.size(); i++) {
				for(int j = 0; j < resulting_mux.in0.at(i).size(); j++) {
				if(resulting_mux.in0.at(i).at(j).is_const) {
					dbg_file << resulting_mux.in0.at(i).at(j).const_val << " & ";
				} else {
					if(resulting_mux.in0.at(i).at(j).is_negated) 
						dbg_file << "not C";
					else
						dbg_file << "C";

					dbg_file << resulting_mux.in0.at(i).at(j).var_index +1 << " & ";
				}

				} 
				dbg_file << " + "; 
			} 
			dbg_file << "\n\n";

			dbg_file << "\nPrinting the in1 details of the resulting MUX\n";
			for(int i = 0; i < resulting_mux.in1.size(); i++) {
				for(int j = 0; j < resulting_mux.in1.at(i).size(); j++) {
					if(resulting_mux.in1.at(i).at(j).is_const) {
						dbg_file << resulting_mux.in1.at(i).at(j).const_val << " & ";
					} else {
						if(resulting_mux.in1.at(i).at(j).is_negated) 
							dbg_file << "not C";
						else
							dbg_file << "C";

						dbg_file << resulting_mux.in1.at(i).at(j).var_index +1 << " & ";
					}

				} 
				dbg_file << " + "; 
			} 
			dbg_file << "\n\n";
			//////////////////////////////////////////////////////////////////////////////////////////////

			// TODO: the following needs to be done in a cleaner way to be generic enough to support any number of MUX levels 
			dbg_file << "\nCheck if you need to apply co-factor again on in0\n";
			// apply cofactor again just once on in0
			if(hasMultipleVariables(resulting_mux.in0, dbg_file)) {
				int var_index_in0 = getLeastVarIdx(resulting_mux.in0);
				if(var_index_in0 >= 0) {
					resulting_mux_in0 = getCofactor_wrapper(var_index_in0, resulting_mux.in0, condition_variables);
					resulting_mux.in0_preds_muxes.push_back(resulting_mux_in0);
					resulting_mux_in0.succs_muxes.push_back(resulting_mux);
				}

			} else {
				resulting_mux_in0.is_valid = false;
				dbg_file << "Not many variables in in0 to do Shannon's!\n";
			}
			// apply cofactor again just once on in1
			dbg_file << "\nCheck if you need to apply co-factor again on in1\n";
			if(hasMultipleVariables(resulting_mux.in1, dbg_file)) {
				int var_index_in1 = getLeastVarIdx(resulting_mux.in1);
				if(var_index_in1 >= 0) {
					resulting_mux_in1 = getCofactor_wrapper(var_index_in1, resulting_mux.in1, condition_variables);
					resulting_mux.in1_preds_muxes.push_back(resulting_mux_in1);
					resulting_mux_in1.succs_muxes.push_back(resulting_mux);
				}
			} else {
				resulting_mux_in1.is_valid = false;
				dbg_file << "Not many variables in in1 to do Shannon's!\n";
			} 

		}
	} else {
		resulting_mux.is_valid = false;
		dbg_file << "Not many variables to do Shannon's!\n";
	} 

	return resulting_mux; // I should detect in the main outside that there are chances that the inputs of this Mux are also Muxes, themselves!
}

