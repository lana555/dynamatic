#include "ElasticPass/Quine_McCluskey.h"


void Quine_McCluskey::initialize(int number_of_bits, int number_of_minterms, int number_of_dont_cares, vector<string>* minterms_only_in_binary, vector<string>* minterms_dont_cares_in_binary) {

	nBits = number_of_bits;
	nMin = number_of_minterms;
	nDontCares = number_of_dont_cares;
	nMinPlusDontCares = nMin + nDontCares;
	
	for (int i = 0; i < nMin; ++i){
		minOnlyBin.push_back(minterms_only_in_binary->at(i));
	}

	for (int i = 0; i < nMinPlusDontCares; ++i){
		minPlusDontCaresBin.push_back(minterms_dont_cares_in_binary->at(i));
	}

	// initializing the table of prime implicants 
	table = vector< vector< string> >(nBits+1);

}

vector< vector< string> > Quine_McCluskey::combinePairs(vector< vector< string> > table, set<string>& primeImpTemp) {
	bool checked[table.size()][nMinPlusDontCares] = {false};
	vector< vector< string> > newTable(table.size()-1);

	for (int i = 0; i < table.size() -1; ++i){
		for (int j = 0; j < table[i].size(); ++j){
			for (int k = 0; k < table[i+1].size(); k++){
				if (util.compare(table[i][j], table[i+1][k])){
					newTable[i].push_back(util.getDiff(table[i][j], table[i+1][k]));
					checked[i][j] = true;
					checked[i+1][k] = true;
				}
			}
		}
	}

	for (int i = 0; i < table.size(); ++i){
		for (int j = 0; j < table[i].size(); ++j){
			if (!checked[i][j]) {
				primeImpTemp.insert(table[i][j]);
			}
		}
	}

	return newTable;
}


void Quine_McCluskey::createTable(std::ofstream& dbg_file, bool print_flag) {
	for (int i = 0; i < nMinPlusDontCares; ++i){
		int num1s = util.get1s(minPlusDontCaresBin[i]);
		table[num1s].push_back(minPlusDontCaresBin[i]);
	}

	if(print_flag) {
		dbg_file << "\nPrinting the prime implicants table in its first state:\n";
		for (int i = 0; i < nBits+1; ++i){
			dbg_file << i << ")  ";
			for (int j = 0; j < table[i].size(); ++j){
				dbg_file << table[i][j] << ", ";
			}
			dbg_file << endl;
		}	
		dbg_file << "\n";
	}
	
}

void Quine_McCluskey::setPrimeImp(std::ofstream& dbg_file, bool print_flag) {
	set<string> primeImpTemp;
	createTable(dbg_file, print_flag);
	// cout << "\nGetting Prime Implicants.." << endl;

	// Combine consecutive terms in the table until its empty
	while (!util.checkEmpty(table)){
		table = combinePairs(table, primeImpTemp);
	}
	
	set<string > :: iterator itr;  // convert set to vector
	 for (itr = primeImpTemp.begin(); itr != primeImpTemp.end(); ++itr){
	 	string x = *itr;
    	primeImp.push_back(x);
	}

	if(print_flag) {
		dbg_file << "\nPrinting the combined prime implicants:\n";
		for (int i = 0; i < primeImp.size(); ++i){
			dbg_file << i << ")  " << primeImp[i];
			dbg_file << endl;
		}	
		dbg_file << "\n";
	}
	

	/*cout << "\nThe Prime Implicants are:" << endl;
	for (int i = 0; i < primeImp.size(); ++i){
		cout  << i << ") "<< util.binToString(primeImp[i]) << endl;
	}*/
}

void Quine_McCluskey::getPosComb(vector< set<int> > &patLogic, int level, set<int> prod, set< set<int> > &posComb) {
	// a recursive function to multiple elements of set patLogic and store it in set posComb
	if (level == patLogic.size()){
		set<int> x = prod;
		posComb.insert(x);
		return;
	}
	else{
		set<int > :: iterator itr;  
		for (itr = patLogic[level].begin(); itr != patLogic[level].end(); ++itr){

			int x = *itr;
        	bool inserted = prod.insert(x).second;
        	getPosComb(patLogic, level+1, prod, posComb);
        	if (inserted){
        		prod.erase(x);
        	}
    	}
	}
}

void Quine_McCluskey::minimize(std::ofstream& dbg_file, bool print_flag) {
	// prepare the primeImp chart to identify the essential prime implicants
	// NOte the number of columns of this chart is only nMin not nMinPlusDontCares!!
	bool primeImpChart[primeImp.size()][nMin] = {{false}};

	for (int i = 0; i < primeImp.size(); ++i){
		for (int j = 0; j < nMin; ++j){
			primeImpChart[i][j] = util.primeIncludes(primeImp[i], minOnlyBin[j]);
		}
	}

	if(print_flag) {
		dbg_file << "\nPrinting the prime implicant chart\n";
		// loop over the rows (each row is a combined prime implicant)
		for (int i = 0; i < primeImp.size(); ++i){
			for (int j = 0; j < nMin; ++j){
				if (primeImpChart[i][j] == true){
		 			dbg_file << "1\t";
		 		}
		 		else {
		 			dbg_file << "0\t";
		 		}
			}
			dbg_file << "\n";
		}
	}
	
	// petric logic
	vector< set<int> > patLogic;
	for (int j = 0; j < nMin; ++j){ // column iteration
		set<int> x;
		for (int i = 0; i < primeImp.size(); ++i){ // row iteration
			if (primeImpChart[i][j] == true) {
				x.insert(i);
			}
		}
		patLogic.push_back(x);
	}

	/*cout << "\nPetric logic is (row: minterms no., col: prime implicants no.): " << endl;
	for (int i = 0; i < patLogic.size(); ++i){
		set<int > :: iterator itr;  // convert set to vector
		for (itr = patLogic[i].begin(); itr != patLogic[i].end(); ++itr){
			int x = *itr;
        	cout << x << " ";
    	}
    	cout << endl;
	}*/

	// get all possible combinations
	set< set<int> > posComb;
	set<int> prod;
	getPosComb(patLogic, 0, prod, posComb); // recursively multiply set elements
	int min = 9999;

	//cout << "\nPossible combinations that satisfy all minterms:" << endl;
	set< set<int> > :: iterator itr1;
	for (itr1 = posComb.begin(); itr1 != posComb.end(); ++itr1){
		set<int> comb = *itr1;
		if (comb.size() < min){
			min = comb.size();
		}
		set<int > :: iterator itr;  
		/*for (itr = comb.begin(); itr != comb.end(); ++itr){
			int x = *itr;
        	cout << x << " ";
    	}
    	cout << endl;*/
	}

	//cout << "\nGetting the combinations with min terms and min variables ..." << endl;
	//Combinations with minimum terms
	vector< set<int> > minComb;
	set< set<int> > :: iterator itr;
	for (itr = posComb.begin(); itr != posComb.end(); ++itr){
		set<int> comb = *itr;
		if (comb.size() == min) {
			minComb.push_back(comb);
		}
	}

	//Combinations with minimum variables
	min = 9999;
	for (int i = 0; i < minComb.size(); ++i){
		if(util.getVar(minComb[i], primeImp) < min){
			min = util.getVar(minComb[i], primeImp);
		}
	}

	for (int i = 0; i < minComb.size(); ++i){
		if(util.getVar(minComb[i], primeImp) == min){
			functions.push_back(minComb[i]);
		}
	}
}



void Quine_McCluskey::solve(std::ofstream& dbg_file, bool print_flag) {
	if(print_flag)
		dbg_file << "\nHii from inside the Quine_McCluskey solver!\n";
	setPrimeImp(dbg_file, print_flag);
	minimize(dbg_file, print_flag);
	if(print_flag)
		dbg_file << "\nBye, leaving the Quine_McCluskey solver!\n";
}

vector<string> Quine_McCluskey::getEssentialPrimeImpl() {
	// returns a vector representing the sum of products of the EssentialPrimeImpl
	vector<string> essential_sum_of_products;

	for(int i = 0; i < functions.size(); i++) {
		set<int> function = functions[i];
		set<int> ::iterator itr;
		
		for(itr = function.begin(); itr != function.end(); ++itr) {
			int x = *itr;
			essential_sum_of_products.push_back(primeImp[x]);
		} 

	} 

	return essential_sum_of_products;
} 

void Quine_McCluskey::displayFunctions(std::ofstream& dbg_file, std::string bool_function_name) {
	dbg_file << "\n\n The possible functions for " << bool_function_name << ":\n";
	for(int i = 0; i < functions.size(); i++) {
		set<int> function = functions[i];
		set<int> ::iterator itr;
		
		for(itr = function.begin(); itr != function.end(); ++itr) {
			int x = *itr;
			dbg_file << (primeImp[x]) << " + ";
		} 

		dbg_file << "\n";
	} 

	dbg_file << "\n\n";
}


