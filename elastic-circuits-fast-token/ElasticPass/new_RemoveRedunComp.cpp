#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

void CircuitGenerator::FixMergeSuccs(ENode* redun_merge, ENode* merge_to_keep, std::ofstream& dbg_file) {
	
	assert(redun_merge->CntrlSuccs->size() == 1);

	// search for the redun_merge in the predecessors of the single successor 
	auto pos_enode_dag = std::find(redun_merge->CntrlSuccs->at(0)->CntrlPreds->begin(), redun_merge->CntrlSuccs->at(0)->CntrlPreds->end(), redun_merge);

	assert(pos_enode_dag != redun_merge->CntrlSuccs->at(0)->CntrlPreds->end());

	int index = pos_enode_dag - redun_merge->CntrlSuccs->at(0)->CntrlPreds->begin();

	redun_merge->CntrlSuccs->at(0)->CntrlPreds->at(index) = merge_to_keep;
	merge_to_keep->CntrlSuccs->push_back(redun_merge->CntrlSuccs->at(0));

}

void CircuitGenerator::cutMergePreds(ENode* redun_merge, std::ofstream& dbg_file) {
	// Some sanity check
	assert(redun_merge->CntrlPreds->size() == 2);

	// delete for the current redun_merge from the successors of its 1st predecessor
	auto pos_enode_dag = std::find(redun_merge->CntrlPreds->at(0)->CntrlSuccs->begin(), redun_merge->CntrlPreds->at(0)->CntrlSuccs->end(), redun_merge);

	assert(pos_enode_dag != redun_merge->CntrlPreds->at(0)->CntrlSuccs->end());

	if (pos_enode_dag != redun_merge->CntrlPreds->at(0)->CntrlSuccs->end()) {
		redun_merge->CntrlPreds->at(0)->CntrlSuccs->erase(pos_enode_dag);
	} 

	// since I'm sure that the 1st predecessor of those Merges is a constant, after deleting the Merge, this constant should have no Succs!
	assert(redun_merge->CntrlPreds->at(0)->CntrlSuccs->size() == 0);

	// delete for the current redun_merge from the successors of its 1st predecessor
	auto pos_enode_dag_ = std::find(redun_merge->CntrlPreds->at(1)->CntrlSuccs->begin(), redun_merge->CntrlPreds->at(1)->CntrlSuccs->end(), redun_merge);

	assert(pos_enode_dag_ != redun_merge->CntrlPreds->at(1)->CntrlSuccs->end());

	if (pos_enode_dag_ != redun_merge->CntrlPreds->at(1)->CntrlSuccs->end()) {
		redun_merge->CntrlPreds->at(1)->CntrlSuccs->erase(pos_enode_dag_);
	} 
	
}


// Please note that the predecessors of this Merges I loop over here are always on the CntrlSuccs
void CircuitGenerator::removeRedunLoopSelMerges(std::vector<int>* redun_merges_indices, std::ofstream& dbg_file) {
	// loop on all Merges and identify all those having the same two inputs 
	int j;
	for(int i = 0; i < enode_dag->size(); i++) {
		ENode* enode = enode_dag->at(i);
		if(enode->type == Phi_n && !enode->isMux) {
			redun_merges_indices->clear();
			// I need ONLY Merges not MUXes!!
			// loop over all other enodes searching for enodes that are identical to enode!
			redun_merges_indices->push_back(j);
			j = i + 1;
			while (j < enode_dag->size()) {
				ENode* enode_2 = enode_dag->at(j);
				if(enode_2->type == Phi_n && !enode_2->isMux) {
					// I only care if they have the same inputs
						// Specifically, the same constant value and the same loop condition (second input!)
					// I'm sure the 1st input of these Merges is a constant, so sanity check!
					assert(enode->CntrlPreds->at(0)->type == Cst_ && enode_2->CntrlPreds->at(0)->type == Cst_);
					if(enode->CntrlPreds->at(0)->cstValue == enode_2->CntrlPreds->at(0)->cstValue && enode->CntrlPreds->at(1) == enode_2->CntrlPreds->at(1)) {
						// these two Merges should become 1, so push them in the vector 
						redun_merges_indices->push_back(j);
					} 
				}

				j++; 
			}

		}
		
		// Before starting a new iteration with a new enode in the enode_dag, erase the redundant Merges with their connections and replace with the connections with only one of those redundant Merges
		for(int r = 1; r < redun_merges_indices->size(); r++) {
			cutMergePreds(enode_dag->at(redun_merges_indices->at(r)), dbg_file);
			// connect the successor of all the redundant Merges to the single Merge we will keep
			FixMergeSuccs(enode_dag->at(redun_merges_indices->at(r)), enode_dag->at(redun_merges_indices->at(0)), dbg_file);
		}

	}
}

void CircuitGenerator::cleanRedunMerges(std::vector<int>* redun_merges_indices) {
	for(int i = 1; i < redun_merges_indices->size(); i++) {
		auto pos_ = std::find(enode_dag->begin(), enode_dag->end(), enode_dag->at(redun_merges_indices->at(i)) );
		assert(pos_ != enode_dag->end());
		enode_dag->erase(pos_);
	} 
}

void CircuitGenerator::cleanConstants() {
	std::vector<ENode*>* consts_to_delete = new std::vector<ENode*>;
	// loop on all constants and delete those that don't have succs!
	for(auto& enode: *enode_dag) {
		if(enode->type == Cst_) {
			if(enode->CntrlSuccs->size() == 0) {
				consts_to_delete->push_back(enode);

				// erase it from the succs of its single preds in the control network
				assert(enode->JustCntrlPreds->size() == 1);
				assert(enode->CntrlPreds->size() == 0);

				auto pos = std::find(enode->JustCntrlPreds->at(0)->CntrlSuccs->begin(), enode->JustCntrlPreds->at(0)->CntrlSuccs->end(), enode);

				assert(pos != enode->JustCntrlPreds->at(0)->CntrlSuccs->end());
				enode->JustCntrlPreds->at(0)->CntrlSuccs->erase(pos);
				
			} 
		} 
	} 

	// delete all the constant enodes in consts_to_delete
	for(int i = 0; i < consts_to_delete->size(); i++) {
		auto pos_ = std::find(enode_dag->begin(), enode_dag->end(), consts_to_delete->at(i));
		assert(pos_ != enode_dag->end());
		enode_dag->erase(pos_);
	} 
}

void CircuitGenerator::removeRedunLoopSelMerges_wrapper(std::ofstream& dbg_file) {
	std::vector<int>* redun_merge_indices = new std::vector<int>;
	removeRedunLoopSelMerges(redun_merge_indices, dbg_file);
	cleanRedunMerges(redun_merge_indices);
	cleanConstants();
}

/*void CircuitGenerator::removeRedunConstants() {


}*/


