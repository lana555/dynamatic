#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

// temporary approach for adding buffers 
void CircuitGenerator::addBuffersSimple() {
	std::vector<ENode*> nodeList;

	for (auto& enode : *enode_dag) {
		if (enode->type == Phi_ || enode->type == Phi_n) {
			if(enode->is_init_token_merge) 
				continue;  // skip the Merges that are added to mimic the initialization token

			// consider only the Merges at the loop header...
			if( BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode->CntrlPreds->at(1)->BB)->Idx) ) {
				ENode* new_node = NULL;

				if (enode->Instr != NULL) {
					new_node = new ENode(Bufferi_, "buffI", enode->Instr, NULL);
				} else if (enode->A != NULL) {
					new_node = new ENode(Buffera_, "buffA", enode->A, NULL);
				} else
					new_node = new ENode(Buffera_, "buffA");

				new_node->id = CircuitGenerator::buff_id++;


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

				nodeList.push_back(new_node);

			} 

		}
		// STILL need to do the same for the PHI_C nodes that are added for connecting the constants (I no longer have the ordered control network!)
		else if(enode->type == Phi_c) {
			if(BBMap->at(enode->BB)->is_loop_header && (BBMap->at(enode->BB)->Idx <= BBMap->at(enode->JustCntrlPreds->at(0)->BB)->Idx || BBMap->at(enode->BB)->Idx <= BBMap->at(enode->JustCntrlPreds->at(1)->BB)->Idx)) {
				ENode* new_node = NULL;

				if (enode->Instr != NULL) {
					new_node = new ENode(Bufferi_, "buffI", enode->Instr, NULL);
				} else if (enode->A != NULL) {
					new_node = new ENode(Buffera_, "buffA", enode->A, NULL);
				} else
					new_node = new ENode(Buffera_, "buffA");

				new_node->id = CircuitGenerator::buff_id++;


				assert(enode->type == Phi_c);
				assert(enode->JustCntrlSuccs->size() > 0);
				// buffer should be the predecessor of the original enode's successor in place of the enode
				auto pos = std::find(enode->JustCntrlSuccs->at(0)->JustCntrlPreds->begin(), enode->JustCntrlSuccs->at(0)->JustCntrlPreds->end(), enode);

				assert(pos != enode->JustCntrlSuccs->at(0)->JustCntrlPreds->end());
				int index = pos - enode->JustCntrlSuccs->at(0)->JustCntrlPreds->begin();
				enode->JustCntrlSuccs->at(0)->JustCntrlPreds->at(index) = new_node;

				new_node->JustCntrlSuccs->push_back(enode->JustCntrlSuccs->at(0));

				// buffer should replace the original enode successor .at(0)
				enode->JustCntrlSuccs->at(0) = new_node;
				new_node->JustCntrlPreds->push_back(enode);

				nodeList.push_back(new_node);

			}
		}
	}

	for (auto node : nodeList)
		enode_dag->push_back(node);
}

/*
	Aya: we need to insert the buffer between the Phi and its successors in a way that doesn't violate the indices of the successor in the Phi successor array and also the index of the Phi in the predecessor array of a specific successor of Phi

	Predecessor of the new_node(buffer) should be the enode and the successor of the new_node(buffer) should be the original successor of the enode
*/ 


