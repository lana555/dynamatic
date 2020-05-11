#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Memory.h"
#include "MemElemInfo/MemUtils.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

/**
 * @brief connectEnodeWithMCEnode
 * Performs the necessary connections between a memory instruction
 * enode and a memory controller/LSQ enode once it has been determined
 * that the two should connect.
 */
void connectEnodeWithMCEnode(ENode* enode, ENode* mcEnode, BBNode* bb) {
    enode->Mem = mcEnode;
    enode->CntrlSuccs->push_back(mcEnode);
    mcEnode->CntrlPreds->push_back(enode);
    enode->memPort = true;
    setBBIndex(enode, bb);
    setMemPortIndex(enode, mcEnode);
    setBBOffset(enode, mcEnode);
}

/**
 * @brief getOrCreateMCEnode
 * @param base: base address of the array associated with the MC
 * @returns pointer to MC Enode associate with the @p base
 */
ENode* CircuitGenerator::getOrCreateMCEnode(const Value* base,
                                            std::map<const Value*, ENode*>& baseToMCEnode) {
    auto iter     = baseToMCEnode.find(base);
    auto* mcEnode = iter->second;
    if (iter == baseToMCEnode.end()) {
        mcEnode             = new ENode(MC_, base->getName().str().c_str());
        baseToMCEnode[base] = mcEnode;
        // Add new MC to enode_dag
        enode_dag->push_back(mcEnode);
    }
    return mcEnode;
}

/**
 * @brief CircuitGenerator::addMCForEnode
 * For a given load or store instruction (which has not previously
 * been connected to an LSQ), associate it with an MC.
 * @param enode
 * @param baseToMCEnode
 */
void CircuitGenerator::addMCForEnode(ENode* enode, std::map<const Value*, ENode*>& baseToMCEnode) {
    Instruction* I = enode->Instr;
    auto* mcEnode  = getOrCreateMCEnode(findBase(I), baseToMCEnode);
    connectEnodeWithMCEnode(enode, mcEnode, findBB(enode));
    updateMCConstant(enode, mcEnode);
}

void CircuitGenerator::constructLSQNodes(std::map<const Value*, ENode*>& LSQnodes) {
    /* Create an LSQ enode for each of the LSQs reported
     * by MemElemInfo, and create a mapping between the unique
     * bases of the LSQs and the corresponding LSQ enodes */
    auto LSQlist = MEI.getLSQList();
    for (auto lsq : LSQlist) {
        auto* base     = lsq->base;
        auto* e        = new ENode(LSQ_, base->getName().str().c_str());
        LSQnodes[base] = e;
    }

    /* Append all the LSQ ENodes to the enode_dag */
    for (const auto& it : LSQnodes)
        enode_dag->push_back(it.second);
}

void CircuitGenerator::updateMCConstant(ENode* enode, ENode* mcEnode) {
    /* Each memory controller contains a notion of how many
     * store operations will be executed for a given BB, when
     * entering said BB. This constant is used to ensure that
     * all pending stores are completed, before the program is
     * allowed to terminate.
     * If present, locate this constant node and increment the
     * count, else create a new constant node.
     */

    if (enode->Instr->getOpcode() == Instruction::Store) {
        bool found = false;
        for (auto& pred : *mcEnode->JustCntrlPreds)
            if (pred->BB == enode->BB && pred->type == Cst_) {
                pred->cstValue++;
                found = true;
             }
        if (!found) {
            ENode* cstNode    = new ENode(Cst_, enode->BB);
            cstNode->cstValue = 1;
            cstNode->JustCntrlSuccs->push_back(mcEnode);
            cstNode->id = cst_id++;
            mcEnode->JustCntrlPreds->push_back(cstNode);

            // Add new constant to enode_dag
            enode_dag->push_back(cstNode);
        }
    }
}

/**
 * @brief CircuitGenerator::addMemoryInterfaces
 * This pass is responsible for emitting all connections between
 * memory accessing instructions and associated memory controllers.
 * If @p useLSQ, LSQs will be emitted for all instructions determined
 * to have possible data dependencies by the MemElemInfo pass.
 */
void CircuitGenerator::addMemoryInterfaces(const bool useLSQ) {
    std::map<const Value*, ENode*> LSQnodes;

    if (useLSQ) {
        constructLSQNodes(LSQnodes);
    }

    // for convenience, we store a mapping between the base arrays in a program,
    // and their associated memory interface ENode (MC or @todo LSQ)
    std::map<const Value*, ENode*> baseToMCEnode;

    /* For each memory instruction present in the enode_dag, connect it
     * to a previously created LSQ if MEI has determined so, else, connect
     * it to a regular MC.
     * Iterate using index, given that iterators are invalidated once a
     * container is modified (which happens during addMCForEnode).
     */
    for (unsigned int i = 0; i < enode_dag->size(); i++) {
        auto* enode = enode_dag->at(i);
        if (!enode->isLoadOrStore())
            continue;

        if (useLSQ && MEI.needsLSQ(enode->Instr)) {
            const LSQset& lsq = MEI.getInstLSQ(enode->Instr);
            connectEnodeWithMCEnode(enode, LSQnodes[lsq.base], findBB(enode));
            continue;
        }

        addMCForEnode(enode, baseToMCEnode);
    }

    // Check if LSQ and MC are targetting the same memory
    for (auto lsq : LSQnodes) {
        auto* base     = lsq.first;
        if (baseToMCEnode[base] != NULL) {
            auto *lsqNode = lsq.second;
            lsqNode->lsqToMC = true;
            auto *mcEnode = baseToMCEnode[base];
            mcEnode->lsqToMC = true;

            // LSQ will connect as a predecessor of MC
            // MC should keep track of stores arriving from LSQ
            for (auto& enode : *lsqNode ->CntrlPreds) 
               updateMCConstant(enode, mcEnode);
            
            lsqNode->CntrlSuccs->push_back(mcEnode);
            mcEnode->CntrlPreds->push_back(lsqNode);
            //setBBIndex(lsqNode, mcEnode->bbNode);
            setLSQMemPortIndex(lsqNode, mcEnode);
            setBBOffset(lsqNode, mcEnode);
        }
    }
}

// set index of BasicBlock in load/store connected to MC or LSQ
void setBBIndex(ENode* enode, BBNode* bbnode) { enode->bbId = bbnode->Idx + 1; }

// load/store port index for LSQ-MC connection
void setLSQMemPortIndex(ENode* enode, ENode* memnode) {

    assert (enode->type == LSQ_);
    int loads = 0;
    int stores = 0;
    for (auto& node : *memnode->CntrlPreds) {
	    if (enode == node)
	        break;
	    if (node->type == LSQ_){
	        loads++;
	        stores++;
	    }
	    else {
	        auto* I = node->Instr;
	        if (isa<LoadInst>(I))
	        	loads++;
	        else {
	        	assert (isa<StoreInst>(I));
	        	stores++;
	        }
	    }
	}
	enode->lsqMCLoadId = loads; 
    enode->lsqMCStoreId = stores;
}

// load/store port index for LSQ or MC
void setMemPortIndex(ENode* enode, ENode* memnode) {
    int offset = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (enode == node)
            break;
        if (!compareMemInst(enode, node))
            offset++;
    }
    enode->memPortId = offset;

    if (enode->type == LSQ_){
    	int loads = 0;
    	int stores = 0;
    	for (auto& node : *memnode->CntrlPreds) {
	        if (enode == node)
	            break;
	        if (node->type == LSQ_){
	            loads++;
	            stores++;
	        }
	        else {
	        	auto* I = node->Instr;
	        	if (isa<LoadInst>(I))
	        		loads++;
	        	else {
	        		assert (isa<StoreInst>(I));
	        		stores++;
	        	}
	        }
	    }
	    enode->lsqMCLoadId = loads; 
        enode->lsqMCStoreId = stores;
	}
}

// ld/st offset within bb (for LSQ or MC grop allocator)
void setBBOffset(ENode* enode, ENode* memnode) {
    int offset = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (enode == node)
            break;
        if (compareMemInst(enode, node) && enode->BB == node->BB)
            offset++;
    }
    enode->bbOffset = offset;
}

int getBBIndex(ENode* enode) { return enode->bbId; }

int getMemPortIndex(ENode* enode) { return enode->memPortId; }

int getBBOffset(ENode* enode) { return enode->bbOffset; }

// check if two mem instructions are different type (for offset calculation)
bool compareMemInst(ENode* enode1, ENode* enode2) {
	// When MC connected to LSQ
	// LSQ has both load and store port connected to MC 
	// Need to increase offset of both load and store
    if (enode1->type == LSQ_ || enode2->type == LSQ_)
        return false;

    auto* I1 = enode1->Instr;
    auto* I2 = enode2->Instr;

    return ((isa<LoadInst>(I1) && isa<StoreInst>(I2)) || (isa<LoadInst>(I2) && isa<StoreInst>(I1)));
}

// total count of loads connected to lsq
int getMemLoadCount(ENode* memnode) {
    int ld_count = 0;
    for (auto& enode : *memnode->CntrlPreds) {
        if (enode->type != Cst_ && enode->type != LSQ_) {
            auto* I = enode->Instr;
            if (isa<LoadInst>(I))
                ld_count++;
        }
    }
    return ld_count;
}

// total count of stores connected to lsd
int getMemStoreCount(ENode* memnode) {
    int st_count = 0;
    for (auto& enode : *memnode->CntrlPreds) {
        if (enode->type != Cst_ && enode->type != LSQ_) {
            auto* I = enode->Instr;
            if (isa<StoreInst>(I))
                st_count++;
        }
    }
    return st_count;
}

// total count of bbs connected to lsq (corresponds to control fork connection
// count)
int getMemBBCount(ENode* memnode) { return memnode->JustCntrlPreds->size(); }

// total input count to MC/LSQ
int getMemInputCount(ENode* memnode) {
    return getMemBBCount(memnode) + 2 * getMemStoreCount(memnode) + getMemLoadCount(memnode);
}

// total output count to MC/LSQ
int getMemOutputCount(ENode* memnode) { return getMemLoadCount(memnode); }

// adress index for dot (fork connections go first)
int getDotAdrIndex(ENode* enode, ENode* memnode) {
    int count = 0;
    for (auto& node : *memnode->CntrlPreds) {
        if (node == enode)
            break;
        auto* I = node->Instr;
        if (isa<StoreInst>(I))
            count += 2;
        else
            count++;
    }

    return getMemBBCount(memnode) + count + 1;
}

// data index for dot
int getDotDataIndex(ENode* enode, ENode* memnode) {
    auto* I = enode->Instr;
    if (isa<StoreInst>(I))
        return getDotAdrIndex(enode, memnode) + 1;
    else {
        int count = 0;

        for (auto& node : *memnode->CntrlPreds) {
            if (node == enode)
                break;
            auto* I = node->Instr;
            if (isa<LoadInst>(I))
                count++;
        }

        return count + 1;
    }
}

int getBBLdCount(ENode* enode, ENode* memnode) {
    int ld_count = 0;

    for (auto& node : *memnode->CntrlPreds) {

        if (node->type != Cst_) {
            auto* I = node->Instr;
            if (isa<LoadInst>(I) && node->BB == enode->BB)
                ld_count++;
        }
    }

    return ld_count;
}

int getBBStCount(ENode* enode, ENode* memnode) {
    int st_count = 0;

    for (auto& node : *memnode->CntrlPreds) {

        if (node->type != Cst_) {
            auto* I = node->Instr;
            if (isa<StoreInst>(I) && node->BB == enode->BB)
                st_count++;
        }
    }

    return st_count;
}

int getLSQDepth(ENode* memnode) {
    // Temporary: determine LSQ depth based on the BB with largest ld/st count
    // Does not guarantee maximal throughput!

    int d = 0;

    for (auto& enode : *memnode->JustCntrlPreds) {
        BBNode* bb = enode->bbNode;

        int ld_count = getBBLdCount(enode, memnode);
        int st_count = getBBStCount(enode, memnode);

        d = (d < ld_count) ? ld_count : d;
        d = (d < st_count) ? st_count : d;
    }

    int power = 16;

    // round up to power of two
    while (power < d)
        power *= 2;

    // increase size for performance (not optimal!)
    // power *= 2;

    return power;
}
