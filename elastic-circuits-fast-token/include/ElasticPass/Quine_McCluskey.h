#pragma once
#include <set>
#include <bitset>
#include <vector>

//#include <sstream>
#include <fstream>

//-------------------------------------------------------//

//using namespace llvm;
using namespace std;

// Following the algorithm in: https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
		// https://github.com/mohdomama/Quine-McCluskey/blob/master/tabulation.cpp
	// step 1: Find the prime implicants
	// step 2: Find the essential prime implicants


class McCluskey_Utils {
	/*
		A class that contains utility functions.
	*/
public:
	int get1s(string x) {
		// returns the number of 1s in a binary string
		int count = 0;
		for (int i = 0; i < x.size(); ++i){
			if (x[i] == '1')
				count++;
		}
		return count;
	}

	bool checkEmpty(vector< vector< string> > table){
		for (int i = 0; i < table.size(); ++i){
			if (table[i].size()!=0) {
				return false;
			}
		}
		return true;
	}

	bool primeIncludes(string imp, string minTerm){
		// check if a prime implicant satisfies a minterm or not
		for (int i = 0; i < imp.size(); ++i){
			if (imp[i]!='-'){
				if(imp[i]!=minTerm[i]){
					return false;
				}
			}
		}
		return true;
	}

	bool compare(string a, string b) {
		// checks if two string differ at exactly one location or not
		int count = 0;
		for(int i = 0; i < a.length(); i++) {
			if (a[i]!=b[i])
				count++;
		}

		if(count == 1)
			return true;

		return false;
	}

	string getDiff(string a, string b) {
		// returs a string that replaces the differ location of two strings with '-'
		for(int i = 0; i < a.length(); i++) {
			if (a[i]!=b[i])
				a[i]='-';
		}

		return a;
	}

	int getVar(set<int> comb, vector<string> primeImp){
		// returns the number of variables in a petrick method combination
		int count = 0;
		set<int> :: iterator itr;
		for (itr = comb.begin(); itr != comb.end(); ++itr){
			int imp = *itr;
			for (int i = 0; i < primeImp[imp].size(); ++i){
				if (primeImp[imp][i]!='-')
					count ++;
			}
		}
		return count;

	}

private:

};


class Quine_McCluskey {

public:
	void initialize(int number_of_bits, int number_of_minterms, int number_of_dont_cares, vector<string>* minterms_only_in_binary, vector<string>* minterms_dont_cares_in_binary);
	void solve(std::ofstream& dbg_file, bool print_flag = true);
	vector<string> getEssentialPrimeImpl();
	void displayFunctions(std::ofstream& dbg_file, std::string bool_function_name);


private:
	//std::vector<int> minInt; // minterms in decimal (i.e., m4 is 100 etc.)
	McCluskey_Utils util;

 	vector<string> minPlusDontCaresBin; // minterms and don't-cares in binary
	vector<string> minOnlyBin; // minterms only in binary
	
 	int nBits; // number of bits
 	int nMin;  // number of minterms (i.e., products in the sum of products)
	int nDontCares; // number of don't-cares
	int nMinPlusDontCares;
 	vector<vector<string>> table; 
 	vector<string> primeImp;
 	vector<set<int>> functions;

	vector<vector<string>> combinePairs(vector< vector< string> > table, set<string>& primeImpTemp);
	void createTable(std::ofstream& dbg_file, bool print_flag = true);
	void setPrimeImp(std::ofstream& dbg_file, bool print_flag = true);
	void getPosComb(vector< set<int> > &patLogic, int level, set<int> prod, set< set<int> > &posComb);
	void minimize(std::ofstream& dbg_file, bool print_flag = true);
	
};


