#pragma once
#include <set>
#include <bitset>
#include <vector>

#include <bits/stdc++.h>


#include <fstream>

#include <cassert>

//-------------------------------------------------------//

using namespace std;

class Shannon_Expansion {

public:
	struct boolVariable{
		int var_index;
		bool is_negated;

		bool is_const;
		int const_val;

		bool operator ==(const boolVariable& rhs) const {
			return(rhs.var_index == var_index && rhs.is_negated == is_negated && rhs.is_const == is_const && rhs.const_val == const_val);
		}

		bool operator ==(const int& rhs_int) const {
			return(rhs_int == var_index);
		}

		// constructor to initialize the variables 
		boolVariable() {
			var_index = -1;
			is_negated = false;
			is_const = false;
			const_val = -1;
		}
	};

	struct MUXrep{
		std::vector<std::vector<boolVariable>> in0;
		std::vector<std::vector<boolVariable>> in1;
		int cond_var_index;

		bool is_valid; // this just checks if this object actually holds a valid MUX or not

		std::vector<MUXrep> in0_preds_muxes; // in case one of the inputs of the mux ends up being a mux itself
		std::vector<MUXrep> in1_preds_muxes; // in case one of the inputs of the mux ends up being a mux itself

		// NOTE THAT I'm currently not making any use of the following field!!
		std::vector<MUXrep> succs_muxes; // in case this mux ends up being an input to another mux
	};

	// a boolean expression is a sum of products, where one product is a vector of boolVariable, therefore, a boolean expression is represented as std::vector<std::vector<boolVariable>>

	// this is what we will call from outside!
	MUXrep apply_shannon(std::vector<std::string> essential_sel_always, std::vector<int>* condition_variables, std::ofstream& dbg_file);

	// takes a MUXrep object and prints it!
	void printMUX(std::ofstream& dbg_file, MUXrep mux);
	
private:
	// We represent a MUX as a struct at in0, in1 and cond as boolVariable 
	Shannon_Expansion::MUXrep getCofactor_wrapper(int var_index, std::vector<std::vector<boolVariable>> bool_funct, std::vector<int>* condition_variables);
	std::vector<std::vector<Shannon_Expansion::boolVariable>> getNegCofactor(int var_index,std::vector<std::vector<Shannon_Expansion::boolVariable>> bool_funct);
	std::vector<std::vector<Shannon_Expansion::boolVariable>> getPosCofactor(int var_index,std::vector<std::vector<boolVariable>> bool_funct);

	std::vector<std::vector<Shannon_Expansion::boolVariable>> simplify_sop(std::vector<std::vector<boolVariable>> funct);
	int myFind(std::vector<boolVariable> vec, int var_index);
	std::vector<std::vector<Shannon_Expansion::boolVariable>> string_to_vec_funct(std::vector<std::string> bool_funct, std::vector<int>* condition_variables); 

	int getLeastVarIdx(std::vector<std::vector<boolVariable>> funct_vec);
	bool hasMultipleVariables(std::vector<std::vector<boolVariable>>funct_vec, std::ofstream& dbg_file);

	// i'm never calling any of the following functions!
	bool isCondinFunct(int var_index, std::vector<std::vector<boolVariable>> bool_funct);

		
};


