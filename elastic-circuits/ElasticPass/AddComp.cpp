#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

#define BRANCH_SUCC_COUNT 2
#define STORE_SUCC_COUNT 1

// Initializing static class variables
int CircuitGenerator::ssa        = 1;
int CircuitGenerator::fork_id    = 0;
int CircuitGenerator::phi_id     = 0;
int CircuitGenerator::buff_id    = 0;
int CircuitGenerator::branch_id  = 0;
int CircuitGenerator::store_id   = 0;
int CircuitGenerator::memcont_id = 0;
int CircuitGenerator::start_id   = 0;
int CircuitGenerator::fifo_id    = 0;
int CircuitGenerator::idt        = 0;
int CircuitGenerator::ret_id     = 0;
int CircuitGenerator::call_id    = 0;
int CircuitGenerator::switch_id  = 0;
int CircuitGenerator::select_id  = 0;
int CircuitGenerator::cst_id     = 0;
int CircuitGenerator::op_id      = 0;
int CircuitGenerator::sink_id    = 0;
int CircuitGenerator::source_id  = 0;

void CircuitGenerator::check() {

    for (auto& enode : *enode_dag) {
        BBNode* bbnode = findBB(enode);
        assert(contains(bbnode->Live_out, enode) == enode->is_live_out);
    }
    for (auto& bbnode : *bbnode_dag) {
        for (auto& li : *(bbnode->Live_in)) {
            assert(contains(bbnode->Live_in, li) == li->is_live_in);
        }
    }
}

void CircuitGenerator::buildDagEnodes(Function& F) {

    // Find arguments of function create enodes for them
    BasicBlock& BB = *F.begin();
    BasicBlock* BBref;
    BBref = &BB;

    int id = 0; // Leonardo inserted line
    for (auto ai = F.arg_begin(), ae = F.arg_end(); ai != ae; ++ai) {
        Argument& A = *ai;
        Argument* Aref;
        Aref               = &A;
        ENode* enode2      = new ENode(Argument_, Aref->getName().str().c_str(), Aref, BBref);
        enode2->argName    = Aref->getName().str(); // Lana 13.08.19
        enode2->id         = id++;                  // Leonardo inserted line
        enode2->is_live_in = true;
        enode_dag->push_back(enode2);
    }

    // Iterate over BBs, name them, and add them to bbnode_dag
    for (auto IIT = F.begin(), IE = F.end(); IIT != IE; ++IIT) {

        BasicBlock& BB = *IIT;
        BasicBlock* BBref;
        BBref = &BB;

        std::string name   = "block";
        std::string BBname = name + std::to_string(idt + 1);
        BB.setName(BBname.c_str());

        BBNode* bbnode1 = new BBNode(BBref, idt);

        bbnode_dag->push_back(bbnode1); // Add BBs as BBnodes
        idt++;
    }

    // Iterate over BBNodes and form the loops
    for (auto& bbnode : *bbnode_dag) {
        for (auto& bbnode1 : *bbnode_dag) {
            for (pred_iterator PI = pred_begin(bbnode->BB), E = pred_end(bbnode->BB); PI != E;
                 ++PI) {
                BasicBlock* Pred = *PI;
                if (bbnode1->BB == Pred) {
                    bbnode->CntrlPreds->push_back(bbnode1);
                    bbnode1->CntrlSuccs->push_back(bbnode);
                }
            }
        }
    }

    // Iterate over ever BB's instructions and form ENodes and form connections
    // b/w instructions and their pred instructions/input BBs (for branches)/
    // function arguments/immediates & predicates
    for (auto IIT = F.begin(), IE = F.end(); IIT != IE; ++IIT) {

        BasicBlock& BB = *IIT;
        BasicBlock* BBref;
        BBref = &BB;

        for (auto IIIT = BB.begin(), IIE = BB.end(); IIIT != IIE; ++IIIT) {

            Instruction& Inst = *IIIT;
            Instruction* Instref;
            Instref = &Inst;

            unsigned opcode = Inst.getOpcode();
            ENode* enode;
            if (opcode == Instruction::Br)
                enode = new ENode(Branch_, Inst.getOpcodeName(), Instref, BBref);
            else if (opcode == Instruction::PHI)
                enode = new ENode(Phi_, Inst.getOpcodeName(), Instref, BBref);
            else
                enode = new ENode(Inst_, Inst.getOpcodeName(), Instref, BBref);

            if (opcode == Instruction::Store)
                enode->id = store_id++;
            else if (opcode == Instruction::Ret)
                enode->id = ret_id++;
            else if (opcode == Instruction::Call)
                enode->id = call_id++;
            else if (opcode == Instruction::Switch)
                enode->id = switch_id++;
            else if (opcode == Instruction::Select)
                enode->id = select_id++;
            else
                enode->id = op_id++;

            std::string name    = "";
            std::string ssaname = name + std::to_string(ssa);
            if (Inst.getName() == "" && opcode != Instruction::Store && opcode != Instruction::Br &&
                opcode != Instruction::Switch && opcode != Instruction::Ret &&
                opcode != Instruction::Call) {
                Inst.setName(ssaname.c_str());
                ssa++;
            }

            if (opcode == Instruction::Br || opcode == Instruction::Switch)
                ssa++;

            for (size_t i = 0; i < Inst.getNumOperands(); i++) {
                if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Inst.getOperand(i))) {
                    ENode* cstNode =
                        new ENode(Cst_, std::to_string(CI->getSExtValue()).c_str(), BBref);
                    cstNode->CI = CI; // dyn_cast<llvm::ConstantInt>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);
                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                } else if (llvm::ConstantFP* CF = dyn_cast<llvm::ConstantFP>(Inst.getOperand(i))) {
                    ENode* cstNode = new ENode(Cst_, BBref);
                    cstNode->CF    = CF; // dyn_cast<llvm::ConstantFP>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);
                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                } else if (dyn_cast<llvm::UndefValue>(Inst.getOperand(i))) {
                    ENode* cstNode = new ENode(Cst_, BBref);
                    cstNode->CI    = 0; // dyn_cast<llvm::ConstantFP>(Inst.getOperand(i));
                    if (enode->type == Phi_) {
                        llvm::PHINode* PI = dyn_cast<llvm::PHINode>(&Inst);
                        assert(PI != NULL);
                        cstNode->BB          = PI->getIncomingBlock(i);
                        cstNode->is_live_out = true;
                    }
                    cstNode->id = cst_id++;
                    enode->CntrlPreds->push_back(cstNode);
                    cstNode->CntrlSuccs->push_back(enode);
                    enode_dag->push_back(cstNode);
                }

                for (auto& nd2 : *enode_dag) {
                    if (nd2->type == Inst_ || nd2->type == Phi_ || nd2->type == Branch_) {
                        if (nd2->Instr == Inst.getOperand(i)) {
                            if (opcode != Instruction::PHI) {
                                enode->CntrlPreds->push_back(nd2);
                                nd2->CntrlSuccs->push_back(enode);
                            }
                        }
                    } else if (nd2->type == Argument_) {
                        if (nd2->A == Inst.getOperand(i)) {
                            if (enode->Instr->getOpcode() != Instruction::PHI) {
                                enode->CntrlPreds->push_back(nd2);
                                nd2->CntrlSuccs->push_back(enode);
                            }
                        }
                    }
                }
            }

            enode_dag->push_back(enode);
        }
    }

    for (BBNode* bbnode : *bbnode_dag) {
        BBMap->insert(std::make_pair(bbnode->BB, bbnode));
    }
}

// traverses list of enodes and list of bbnodes to classify each BBs list of
// live-ins and live-outs
void CircuitGenerator::formLiveBBnodes() {
    for (auto enode : *enode_dag) { // an argument is a live in of the first BB
        if (enode->type == Argument_) {
            BBNode* bbnode = bbnode_dag->front();
            bbnode->Live_in->push_back(enode);
            enode->is_live_in = true;
        } else if (enode->type == Cst_ && enode->is_live_out == true) {
            // a live-out constant implies its an input to some Phi node in a
            // different BB
            BBNode* bbnode = findBB(enode);
            bbnode->Live_out->push_back(enode);
        }
        // a branch is a live out of its own BB
        else if (enode->type == Branch_) {
            BBNode* bbnode = findBB(enode);
            bbnode->Live_out->push_back(enode);
            enode->is_live_out = true;
        }

        for (auto pred : *(enode->CntrlPreds)) {

            if (pred->BB != enode->BB) { // if a predecessor belongs to a different BB ...

                for (auto bbnode : *bbnode_dag) { // looking for the bbnode of pred->BB
                    if (bbnode->BB == pred->BB) {
                        // it is a live out of the block it comes from ...

                        // Lana 20.10.2018.
                        if (!contains(bbnode->Live_out, pred))
                            bbnode->Live_out->push_back(pred);
                        pred->is_live_out = true;
                    }
                    if (bbnode->BB == enode->BB) {
                        // and it is a live in of the block it goes to

                        // Lana 20.10.2018.
                        if (!contains(bbnode->Live_in, pred))
                            bbnode->Live_in->push_back(pred);
                        pred->is_live_in = true;
                    }
                }
            } else {
                // LLVM Phi nodes are the sole exception to the rule that a live-in to a BB
                // cannot originate from the same BB.

                if (enode->type == Phi_) {
                    BBNode* bbnode = findBB(pred);

                    if (!contains(bbnode->Live_in, pred))
                        bbnode->Live_in->push_back(pred); // 20.10.2018 Lana
                    // bbnode->Live_in->push_back(pred);
                    pred->is_live_in = true;

                    if (!contains(bbnode->Live_out, pred))
                        bbnode->Live_out->push_back(pred); // 20.10.2018 Lana
                    // bbnode->Live_out->push_back(pred);
                    pred->is_live_out = true;

                    // break; 20.10.2018 Lana removed because other phi preds were not getting added
                    // to liveout list
                }
            }
        }
    }
}

// enode_dag is not complete - it has no loops at the moment -- this function adds the loops and
// produces the correct enode_dag
void CircuitGenerator::fixBuildDagEnodes() {
    for (auto& enode_0 : *enode_dag) {         // for each enode
        if (enode_0->type == Phi_) {           // if it is a phi, there is a loop
            for (auto& enode_1 : *enode_dag) { // go through enode dag again, for each enode
                for (size_t i = 0; i < enode_0->Instr->getNumOperands(); i++) { // for each operand
                    if (enode_1->type == Phi_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->Instr) { // if the phi is connected to that enode
                            // Lana 18.10.2018. Make sure there are no double edges between phis
                            //if (!contains(enode_0->CntrlPreds, enode_1))
                                enode_0->CntrlPreds->push_back(enode_1); // add connection
                            //if (!contains(enode_1->CntrlSuccs, enode_0))
                                enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }
                    if (enode_1->type == Inst_ || enode_1->type == Branch_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->Instr) { // if the phi is connected to that enode
                            // Lana 18.10.2018. Make sure there are no double edges between phis
                            enode_0->CntrlPreds->push_back(enode_1); // add connection
                            enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }

                    if (enode_1->type == Argument_) {
                        if (enode_0->Instr->getOperand(i) ==
                            enode_1->A) { // if the phi is connected to that enode
                            enode_0->CntrlPreds->push_back(enode_1); // add connection
                            enode_1->CntrlSuccs->push_back(enode_0); // add connection
                        }
                    }
                }
            }
        }
    }
}

void CircuitGenerator::InsertNode(ENode* enode, ENode* after) {
    // removing all connections between enode and its successors
    for (auto& succ : *(enode->CntrlSuccs)) { // for all enode's successors
        removeNode(succ->CntrlPreds, enode);

        after->CntrlSuccs->push_back(succ); // add connection between after and enode's successors
        succ->CntrlPreds->push_back(after); // add connection between after and enode's successors
    }
    enode->CntrlSuccs = new std::vector<ENode*>; // all successors of enode removed

    after->CntrlPreds->push_back(enode); // add connection between after and enode
    enode->CntrlSuccs->push_back(after); // add connection between after and enode

    if (after->type == Fork_) {
        if (enode->Instr != nullptr) {
            if (isa<BranchInst>(enode->Instr) && (enode->Instr->getNumOperands() > 1)) {
                // this is conditional branch, unconditional branches will be processed after
                ENode* pr = enode->CntrlPreds->at(0);

                // Lana 19.10. generalizing
                if (pr->Instr != nullptr)
                    after->Instr = pr->Instr;
                else if (pr->A != nullptr)
                    after->A = pr->A;
                else if (pr->CI != nullptr)
                    after->CI = pr->CI;
                else if (pr->CF != nullptr)
                    after->CF = pr->CF;

            } else {
                after->Instr = enode->Instr;
            }
        } else if (enode->A != nullptr) {
            after->A = enode->A;
        } else if (enode->CI != nullptr) {
            after->CI = enode->CI;
        } else if (enode->CF != nullptr) {
            after->CF = enode->CF;
        }
    }
}

// Lana 20.10.2018. New function to insert Branches
void CircuitGenerator::InsertBranchNew(ENode* enode, BBNode* bbnode) {
    assert(contains(bbnode->Live_out, enode) && enode->is_live_out);

    int branch_count = 0;
    // iterate successors in succ bbs--the one with the max nb of phis determines how many branches
    // we need to place
    for (auto& succ_bb : *(bbnode->CntrlSuccs)) {
        int count = 0;
        for (auto& succ_node : *(enode->CntrlSuccs)) {
            if (succ_node->BB == succ_bb->BB &&
                (succ_node->type == Phi_n || succ_node->type == Phi_))
                count++;
        }
        if (count > branch_count)
            branch_count = count;
    }

    // instantiate branches and reconnect nodes accordingly
    for (int i = 0; i < branch_count; i++) {
        ENode* branch = new ENode(Branch_n, "branch", enode->BB);
        branch->id    = branch_id++;
        setBBIndex(branch, bbnode);

        for (auto& succ_bb : *(bbnode->CntrlSuccs)) {
            for (auto it = enode->CntrlSuccs->begin(); it != enode->CntrlSuccs->end();) {
                ENode* succ_node = *it;
                if (succ_node->BB == succ_bb->BB &&
                    (succ_node->type == Phi_n || succ_node->type == Phi_)) {

                    // remove connection between enode and its successors
                    it = removeNode(enode->CntrlSuccs, succ_node);
                    // remove connection between enode and its successors
                    removeNode(succ_node->CntrlPreds, enode);

                    // add connection between after and enode's successors
                    branch->CntrlSuccs->push_back(succ_node);
                    // add connection between after and enode's successors
                    succ_node->CntrlPreds->push_back(branch);

                    break;
                } else {
                    ++it;
                }
            }
        }

        branch->CntrlPreds->push_back(enode);

        enode->CntrlSuccs->push_back(branch);
        bbnode->Live_out->insert(bbnode->Live_out->begin(), branch);
        branch->is_live_out = true;

        // add branch to list of nodes
        enode_dag->push_back(branch);

        if (enode->Instr != nullptr)
            branch->Instr = enode->Instr;
        else if (enode->A != nullptr)
            branch->A = enode->A;
        else if (enode->CI != nullptr)
            branch->CI = enode->CI;
        else if (enode->CF != nullptr)
            branch->CF = enode->CF;
    }
}

// used to add forks in the appropriate places in a circuit BEFORE branches
void CircuitGenerator::addFork() {

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];

        if (enode->CntrlSuccs->size() > 1) {
            // Lana 28.04.19 Put fork always, not just when succ in same block
            // (if multiple succs in different bbs, we will connect fork to multiple brs)
            // if we have at least one successor in the same block, we need a fork
            // bool inBB = false;
            // for (auto &succ : *(enode->CntrlSuccs)) {
            // if (succ->BB == enode->BB) {
            // inBB = true;
            //	break;
            //}
            //}
            // If it's already a fork, dont add another
            // if (inBB == true && enode->type != Fork_) {
            if (enode->type != Fork_) {
                ENode* fork = new ENode(Fork_, "fork", enode->BB);
                fork->id    = fork_id++;

                // insert fork between enode and its successors
                InsertNode(enode, fork);

                // Update Live outs
                if (enode->is_live_out == true) {
                    BBNode* bbnode = findBB(enode);

                    removeNode(bbnode->Live_out, enode);
                    enode->is_live_out = false;
                    bbnode->Live_out->push_back(fork);
                    fork->is_live_out = true;
                }
                // add fork to list of nodes
                enode_dag->push_back(fork);
            }
        }
    }
}

// used to add a branch node after every live out and forks if needed
void CircuitGenerator::addBranch() {
    for (auto& bbnode : *bbnode_dag) {
        // branch condition node
        ENode* Br = NULL;

        // for every live-out
        for (auto it = bbnode->Live_out->begin(); it != bbnode->Live_out->end();) {
            ENode* lo = *it;

            if (lo->type != Branch_ && lo->type != Branch_n) {

                InsertBranchNew(lo, bbnode);

                // update live-outs
                it              = removeNode(bbnode->Live_out, lo);
                lo->is_live_out = false;

            } else {
                if (lo->type == Branch_) { // find branch condition
                    Br = lo;
                }

                ++it;
            }
        }

        // create fork for branch condition
        ENode* fork = new ENode(Fork_, "fork", bbnode->BB);
        fork->id    = fork_id++;

        for (auto& lo : *(bbnode->Live_out)) {
            if (lo->type != Branch_) {
                assert(Br != NULL);

                // connect branch condition to branch nodes
                Br->CntrlSuccs->push_back(lo);
                lo->CntrlPreds->push_back(Br);
            }
        }

        // add fork between branch condition and branches (if needed)
        if (Br != NULL && Br->CntrlSuccs->size() > 1) {
            // std::cout << "inserting fork: " << getNodeDotName(fork) <<"\n";
            // std::cout << "before branch: " << getNodeDotName(Br) <<"\n";
            // for (auto &nn : *(Br->CntrlPreds)) {
            // std::cout << "branch preds: " << getNodeDotName(nn) <<"\n";
            // std::cout << Br->Instr->getNumOperands()<<"\n";
            //}

            InsertNode(Br, fork);
            enode_dag->push_back(fork);
        }
    }
}

// Sink node for missing Branch and Store outputs
// Store needs to be adjusted to LSQ analysis
void CircuitGenerator::addSink() {
    std::vector<ENode*> sinkList;

    for (auto& enode : *enode_dag) {

        if (enode->type == Branch_n) {
            assert(enode->CntrlSuccs->size() > 0); // has to have at least one successor

            if (enode->CntrlSuccs->size() < BRANCH_SUCC_COUNT) {
                ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;

                enode->CntrlSuccs->push_back(sink_node);
                sink_node->CntrlPreds->push_back(enode);

                sinkList.push_back(sink_node);
            }
        }

        else if (enode->type == Branch_c) {
            assert(enode->JustCntrlSuccs->size() > 0); // has to have at least one successor

            if (enode->JustCntrlSuccs->size() < BRANCH_SUCC_COUNT) {
                ENode* sink_node = new ENode(Sink_, "sink"); //, enode->BB);
                sink_node->id    = sink_id++;

                enode->JustCntrlSuccs->push_back(sink_node);
                sink_node->JustCntrlPreds->push_back(enode);

                sinkList.push_back(sink_node);
            }
        }

        /*if (enode->type == Inst_ ) {
            if  (enode->Instr->getOpcode() == Instruction::Store) {

                if (enode->CntrlSuccs->size() < STORE_SUCC_COUNT) {
                    ENode * sink_node = new ENode(Sink_, "sink", enode->BB);
                    sink_node->id = sink_id++;

                    enode->CntrlSuccs->push_back(sink_node);
                    sink_node->CntrlPreds->push_back(enode);

                    sinkList.push_back(sink_node);

                }
            }
        }*/
    }

    for (auto sink_node : sinkList)
        enode_dag->push_back(sink_node);
}

// Source node for constants whenever their successor is not a branch
// If successor is branch, then we need control fork as input to trigger the computation
// Cntr fork case handled in addControl()
void CircuitGenerator::addSource() {
    std::vector<ENode*> sourceList;

    for (auto& enode : *enode_dag) {
        if (enode->type == Cst_) {

            if (enode->CntrlSuccs->size() > 0)
                if (enode->CntrlSuccs->front()->type != Branch_n &&
                    enode->CntrlSuccs->front()->type != Branch_ &&
                    enode->JustCntrlPreds->size() == 0) {
                    ENode* source_node = new ENode(Source_, "source", enode->BB);
                    source_node->id    = source_id++;

                    enode->CntrlPreds->push_back(source_node);
                    source_node->CntrlSuccs->push_back(enode);

                    sourceList.push_back(source_node);
                }
            if (enode->JustCntrlSuccs->size() > 0) {
                if (enode->JustCntrlSuccs->front()->type == Inst_)
                    if (enode->JustCntrlSuccs->front()->Instr->getOpcode() ==
                        Instruction::GetElementPtr) {
                        ENode* source_node = new ENode(Source_, "source", enode->BB);
                        source_node->id    = source_id++;

                        enode->CntrlPreds->push_back(source_node);
                        source_node->CntrlSuccs->push_back(enode);

                        sourceList.push_back(source_node);
                    }
            }
        }
    }
    for (auto source_node : sourceList)
        enode_dag->push_back(source_node);
}

// Lana 20.10.2018. Removing some nodes that we don't need before we start modifying the DFG
void CircuitGenerator::removeRedundantBeforeElastic(std::vector<BBNode*>* bbnode_dag,
                                                    std::vector<ENode*>* enode_dag) {

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            // remove operand 0 from getelementptr, no need to connect in circuit
            if (enode->Instr->getOpcode() == Instruction::GetElementPtr) {

                ENode* predToRemove = NULL;
                for (auto& pred : *(enode->CntrlPreds)) {
                    if (pred->Instr != NULL) {
                        if (dyn_cast<Value>(pred->Instr) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }
                    } else if (pred->A != NULL) {
                        if (dyn_cast<Value>(pred->A) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }

                    } else if (pred->CI != NULL) {
                        if (dyn_cast<Value>(pred->CI) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }

                    } else if (pred->CF != NULL) {
                        if (dyn_cast<Value>(pred->CF) == enode->Instr->getOperand(0)) {
                            predToRemove = pred;
                            break;
                        }
                    }
                }

                predToRemove->CntrlSuccs->erase(std::remove(predToRemove->CntrlSuccs->begin(),
                                                            predToRemove->CntrlSuccs->end(), enode),
                                                predToRemove->CntrlSuccs->end());
                enode->CntrlPreds->erase(
                    std::remove(enode->CntrlPreds->begin(), enode->CntrlPreds->end(), predToRemove),
                    enode->CntrlPreds->end());
            }
        }
    }

    std::vector<ENode*> tmpNodes;

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            // remove getelementptr when only one predecessor (can be directly propagated to load,
            // no extra computation)
            if ((enode->Instr->getOpcode() == Instruction::GetElementPtr &&
                 enode->CntrlPreds->size() == 1)) {
                tmpNodes.push_back(enode);
            }

            // we don't need alloca, all memories already 'allocated'
            if ((enode->Instr->getOpcode() == Instruction::Alloca)) {
                tmpNodes.push_back(enode);
            }

            // remove zext and sext if they are in front of alloca or getelementptr that we are
            // removing if getelementptr stays, keep zext/sext so that we can identify which ports
            // each pred is connected to
            if (enode->Instr->getOpcode() == Instruction::SExt ||
                enode->Instr->getOpcode() == Instruction::ZExt) {
                ENode* succ = enode->CntrlSuccs->front();
                if (succ->type == Inst_) {
                    if ((succ->Instr->getOpcode() == Instruction::Alloca)) {
                        tmpNodes.push_back(enode);
                    }
                    if ((succ->Instr->getOpcode() == Instruction::GetElementPtr &&
                         succ->CntrlPreds->size() == 1)) {
                        tmpNodes.push_back(enode);
                    }
                }
            }

        } else if (enode->type == Argument_) {
            // if argument is connected directly to store address (without getptr), address is zero
            // (there is no offset calculation)
            // removing arg to avoid unnecessary argument connections through circuit

            std::vector<ENode*> nodeList;
            for (auto& succ : *enode->CntrlSuccs) {
                if (succ->type == Inst_) {
                	bool add_const = false;
                    if (succ->Instr->getOpcode() == Instruction::Store) {
                        if (succ->Instr->getOperand(1) == dyn_cast<Value>(enode->A)) 
                        	add_const = true;                    
                    }
                    if (succ->Instr->getOpcode() == Instruction::Load) {
                        if (succ->Instr->getOperand(0) == dyn_cast<Value>(enode->A)) 
                        	add_const = true;
                    }

                    if (add_const) {
                    	succ->CntrlPreds->erase(std::remove(succ->CntrlPreds->begin(),
                                                                succ->CntrlPreds->end(), enode),
                                                    succ->CntrlPreds->end());

                        ENode* cstNode = new ENode(Cst_, std::to_string(0).c_str(), succ->BB);

                        cstNode->id = cst_id++;
                        succ->CntrlPreds->push_back(cstNode);
                        cstNode->CntrlSuccs->push_back(succ);
                        enode_dag->push_back(cstNode);
                        nodeList.push_back(succ);
                    }
                }
            }

            for (auto node : nodeList)
                enode->CntrlSuccs->erase(
                    std::remove(enode->CntrlSuccs->begin(), enode->CntrlSuccs->end(), node),
                    enode->CntrlSuccs->end());

            if (enode->CntrlSuccs->size() == 0)
                tmpNodes.push_back(enode);
        }
    }
    for (auto enode : tmpNodes) {

        if (enode->type != Argument_) {
            ENode* pred = enode->CntrlPreds->front();
            pred->CntrlSuccs->erase(
                std::remove(pred->CntrlSuccs->begin(), pred->CntrlSuccs->end(), enode),
                pred->CntrlSuccs->end());

            // ENode* succ = enode->CntrlSuccs->front();

            for (auto& succ : *enode->CntrlSuccs) {
                succ->CntrlPreds->erase(
                    std::remove(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode),
                    succ->CntrlPreds->end());

                pred->CntrlSuccs->push_back(succ);
                succ->CntrlPreds->push_back(pred);
            }
        }
        for (auto bbnode : *bbnode_dag) {
            if (contains(bbnode->Live_in, enode)) {
                bbnode->Live_in->erase(
                    std::remove(bbnode->Live_in->begin(), bbnode->Live_in->end(), enode),
                    bbnode->Live_in->end());
            }
            if (contains(bbnode->Live_out, enode)) {
                bbnode->Live_out->erase(
                    std::remove(bbnode->Live_out->begin(), bbnode->Live_out->end(), enode),
                    bbnode->Live_out->end());
            }
        }

        delete enode;
        enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode),
                         enode_dag->end());
    }
}

// Lana 20.10. 2018. Removing nodes after elastic modifications
// later we could extend to remove 1-input merges etc.
void CircuitGenerator::removeRedundantAfterElastic(std::vector<ENode*>* enode_dag) {

    std::vector<ENode*> tmpNodes;

    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        // remove forks with only one successor
        if ((enode->type == Fork_ || enode->type == Fork_c) &&
            (enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size()) == 1) {

            tmpNodes.push_back(enode);
        }

        // remove constants which are never read
        if ((enode->type == Cst_) &&
            (enode->CntrlSuccs->size() + enode->JustCntrlSuccs->size()) == 0) {

            tmpNodes.push_back(enode);
        }
    }
    for (auto enode : tmpNodes) {

        if (enode->type == Fork_) {
            ENode* pred = enode->CntrlPreds->front();
            ENode* succ = enode->CntrlSuccs->front();

            pred->CntrlSuccs->erase(
                std::remove(pred->CntrlSuccs->begin(), pred->CntrlSuccs->end(), enode),
                pred->CntrlSuccs->end());
            succ->CntrlPreds->erase(
                std::remove(succ->CntrlPreds->begin(), succ->CntrlPreds->end(), enode),
                succ->CntrlPreds->end());

            pred->CntrlSuccs->push_back(succ);
            succ->CntrlPreds->push_back(pred);
        } else if (enode->type == Fork_c) {
            ENode* pred = enode->JustCntrlPreds->front();
            ENode* succ = enode->JustCntrlSuccs->front();

            pred->JustCntrlSuccs->erase(
                std::remove(pred->JustCntrlSuccs->begin(), pred->JustCntrlSuccs->end(), enode),
                pred->JustCntrlSuccs->end());
            succ->JustCntrlPreds->erase(
                std::remove(succ->JustCntrlPreds->begin(), succ->JustCntrlPreds->end(), enode),
                succ->JustCntrlPreds->end());

            pred->JustCntrlSuccs->push_back(succ);
            succ->JustCntrlPreds->push_back(pred);
        }

        delete enode;
        enode_dag->erase(std::remove(enode_dag->begin(), enode_dag->end(), enode),
                         enode_dag->end());
    }
}

void CircuitGenerator::addBuffersSimple() {
    std::vector<ENode*> nodeList;

    for (auto& enode : *enode_dag) {
        if (enode->type == Phi_ || enode->type == Phi_n || enode->type == Phi_c) {
            if (enode->CntrlPreds->size() > 1 || enode->JustCntrlPreds->size() > 1) {

                ENode* new_node = NULL;

                if (enode->Instr != NULL) {
                    new_node = new ENode(Bufferi_, "buffI", enode->Instr, NULL);
                } else if (enode->A != NULL) {
                    new_node = new ENode(Buffera_, "buffA", enode->A, NULL);
                } else
                    new_node = new ENode(Buffera_, "buffA");

                new_node->id = CircuitGenerator::buff_id++;

                if (!enode->is_live_out) {
                    new_node->BB = enode->BB;
                }

                if (enode->type == Phi_c) {

                    ENode* e_succ = enode->JustCntrlSuccs->front();

                    new_node->JustCntrlPreds->push_back(enode);
                    new_node->JustCntrlSuccs->push_back(e_succ);

                    removeNode(e_succ->JustCntrlPreds, enode);
                    removeNode(enode->JustCntrlSuccs, e_succ);

                    enode->JustCntrlSuccs->push_back(new_node);
                    e_succ->JustCntrlPreds->push_back(new_node);

                } else {

                    ENode* e_succ = enode->CntrlSuccs->front();

                    new_node->CntrlPreds->push_back(enode);
                    new_node->CntrlSuccs->push_back(e_succ);

                    removeNode(e_succ->CntrlPreds, enode);
                    removeNode(enode->CntrlSuccs, e_succ);

                    enode->CntrlSuccs->push_back(new_node);
                    e_succ->CntrlPreds->push_back(new_node);
                }

                nodeList.push_back(new_node);

                // enode_dag->push_back(new_node);
            }
        }
    }

    for (auto node : nodeList)
        enode_dag->push_back(node);
}

void CircuitGenerator::setMuxes() {
    std::vector<ENode*> nodeList;

    for (auto& enode : *enode_dag) {
        enode->isMux     = false;
        enode->isCntrlMg = false;
        if (enode->type == Phi_c) {
            if (enode->JustCntrlPreds->size() > 1) {

                // std::cout << "+++++++++++++++++++++++++";
                ENode* fork     = new ENode(Fork_, "fork", enode->BB);
                bool phi_found  = false;
                fork->isCntrlMg = false;

                for (auto& phi_node : *enode_dag) {
                    if (phi_node->type == Phi_)
                        if (enode->BB == phi_node->BB) {
                            phi_found       = true;
                            phi_node->isMux = true;
                            phi_node->CntrlPreds->push_back(fork);
                            fork->CntrlSuccs->push_back(phi_node);
                        }
                }
                if (phi_found) { // implement changes only if phi exists
                    enode->isCntrlMg = true;
                    fork->id         = fork_id++;
                    nodeList.push_back(fork);
                    fork->CntrlPreds->push_back(enode);
                    enode->CntrlSuccs->push_back(fork);
                }

            } else
                enode->isCntrlMg = false;
        }
    }

    for (auto node : nodeList)
        enode_dag->push_back(node);
}

void CircuitGenerator::setBBIds() {
    for (auto& enode : *enode_dag) {
        bool found = false;
        for (auto& bbnode : *bbnode_dag) {
            if (bbnode->BB == enode->BB) {
                setBBIndex(enode, bbnode);
                found = true;
            }
        }
        if (!found)
            enode->bbId = 0;
    }

}

void CircuitGenerator::setFreqs(const std::string& function_name) {
    // frequency information should be in a file called "<function_name>_freq.txt"
    std::string file_name = function_name + "_freq.txt";
    std::ifstream file(file_name.c_str());
    if (!file.is_open()) {
        std::cout << "couldn't open file " << function_name + "_freq.txt" << std::endl;
        return;
    }

    // Reading the frequencies from the file
    std::string line;
    std::map<std::pair<std::string, std::string>, int> frequencies;
    while (getline(file, line)) {
        std::stringstream iss(line);
        std::string src_bb, dst_bb;
        int freq;
        iss >> src_bb >> dst_bb >> freq;

        assert(frequencies.find({src_bb, dst_bb}) == frequencies.end());
        assert(freq >= 0);

        frequencies[{src_bb, dst_bb}] = freq;
    }

    // setting the frequencies in the BBNodes
    for (auto& bnd : *bbnode_dag) {
        std::string src_bb = bnd->BB->getName();
        for (auto& bnd_succ : *(bnd->CntrlSuccs)) {
            std::string dst_bb = bnd_succ->BB->getName();

            assert(frequencies.find({src_bb, dst_bb}) != frequencies.end());

            bnd->set_succ_freq(dst_bb, frequencies[{src_bb, dst_bb}]);
        }
    }
}

// finds control phi in `bbnode`
ENode* CircuitGenerator::findPhiC(BBNode* bbnode) {
    for (auto& enode : *enode_dag) {
        if ((enode->type == Phi_c || enode->type == Start_) && enode->BB == bbnode->BB) {
            return enode;
        }
    }
    return NULL;
}

void CircuitGenerator::addControl() {

    //-------------------------------------------------//

    ENode* retOr_node = new ENode(End_, "end", NULL);
    retOr_node->id    = 0;
    enode_dag->push_back(retOr_node);

    // We first add the Merge-Fork-Branch combo to every
    // BB in `bbnode_dag`
    for (auto& bbnode : *bbnode_dag) {
        ENode* phiC_node = NULL;

        if (bbnode == bbnode_dag->front()) {
            phiC_node     = new ENode(Start_, "start", bbnode->BB);
            phiC_node->id = start_id++;
        } else {
            // create new `phi_node` node
            phiC_node            = new ENode(Phi_c, "phiC", bbnode->BB);
            phiC_node->id        = phi_id++;
            phiC_node->isCntrlMg = false;
        }
        bbnode->Cntrl_nodes->push_back(phiC_node);
        enode_dag->push_back(phiC_node);

        ENode* forkC_node = new ENode(Fork_c, "forkC", bbnode->BB);
        forkC_node->id    = fork_id++;
        bbnode->Cntrl_nodes->push_back(forkC_node);
        enode_dag->push_back(forkC_node);

        phiC_node->JustCntrlSuccs->push_back(forkC_node);
        forkC_node->JustCntrlPreds->push_back(phiC_node);

        // Lana 23.10.2018: Connecting csts to control fork
        for (auto& enode : *enode_dag) {

            if (enode->type == Cst_ && enode->BB == bbnode->BB) {
                if (enode->CntrlSuccs->size() > 0) {
                    if (enode->CntrlSuccs->front()->type == Branch_n ||
                        enode->CntrlSuccs->front()->type == Branch_) {
                        forkC_node->JustCntrlSuccs->push_back(enode);
                        enode->JustCntrlPreds->push_back(forkC_node);
                    }

                    if (enode->CntrlSuccs->front()->type == Inst_) {
                        if (enode->CntrlSuccs->front()->Instr->getOpcode() ==
                            Instruction::GetElementPtr ||
                            enode->CntrlSuccs->front()->Instr->getOpcode() ==
                            Instruction::Load ||
                            enode->CntrlSuccs->front()->Instr->getOpcode() ==
                            Instruction::Store ) {
                            forkC_node->JustCntrlSuccs->push_back(enode);
                            enode->JustCntrlPreds->push_back(forkC_node);
                        }
                    }
                } else if (enode->JustCntrlSuccs->size() > 0) {
                    if (enode->JustCntrlSuccs->front()->type == MC_) {
                        forkC_node->JustCntrlSuccs->push_back(enode);
                        enode->JustCntrlPreds->push_back(forkC_node);
                    }
                }
            }
            if (enode->type == LSQ_) {
                for (auto& pred : *enode->CntrlPreds)
                    if (pred->BB == forkC_node->BB)
                        if (!contains(forkC_node->JustCntrlSuccs, enode)) {
                            forkC_node->JustCntrlSuccs->push_back(enode);
                            enode->JustCntrlPreds->push_back(forkC_node);
                        }
            }
            if (enode->type == MC_ || enode->type == LSQ_) {
                if (!contains(enode->JustCntrlSuccs, retOr_node)) {
                    enode->JustCntrlSuccs->push_back(retOr_node);
                    retOr_node->JustCntrlPreds->push_back(enode);
                }
            }
        }

        if (bbnode->CntrlSuccs->size() != 0) { // BB branches/jumps to some other BB
            ENode* branchC_node = new ENode(Branch_c, "branchC", bbnode->BB);
            branchC_node->id    = branch_id++;
            bbnode->Live_out->push_back(branchC_node);
            branchC_node->is_live_out = true;
            bbnode->Cntrl_nodes->push_back(branchC_node);
            setBBIndex(branchC_node, bbnode);
            enode_dag->push_back(branchC_node);

            forkC_node->JustCntrlSuccs->push_back(branchC_node);
            branchC_node->JustCntrlPreds->push_back(forkC_node);

            // finding LLVM br instruction for current BB

            ENode* llvmBrNode = NULL;
            for (auto& enode : *enode_dag) {
                if (enode->type == Branch_ && enode->BB == bbnode->BB) {
                    llvmBrNode = enode;
                    break;
                }
            }
            assert(llvmBrNode != NULL);

            ENode* llvmBrFork = NULL;
            if (llvmBrNode->CntrlSuccs->size() == 0) {
                llvmBrFork = llvmBrNode;
            } else {
                llvmBrFork = llvmBrNode->CntrlSuccs->front();
                if (llvmBrFork->type != Fork_) {
                    // this means we need to create a new Fork node to
                    // fork the llvm branch condition and connect it
                    // to our control branch node.
                    llvmBrFork        = new ENode(Fork_, "fork", bbnode->BB);
                    llvmBrFork->id    = fork_id++;
                    llvmBrFork->Instr = llvmBrNode->Instr; // assign the Br instr to the new Fork
                    enode_dag->push_back(llvmBrFork);

                    // we connect the old successor of `llvmBrNode` to
                    // our new `llvmBrFork`, and disconnect it from
                    // `llvmBrNode`.
                    ENode* oldSucc = llvmBrNode->CntrlSuccs->front();
                    removeNode(llvmBrNode->CntrlSuccs, oldSucc);
                    removeNode(oldSucc->CntrlPreds, llvmBrNode);

                    llvmBrNode->CntrlSuccs->push_back(llvmBrFork);
                    llvmBrFork->CntrlPreds->push_back(llvmBrNode);

                    llvmBrFork->CntrlSuccs->push_back(oldSucc);
                    oldSucc->CntrlPreds->push_back(llvmBrFork);
                }
            }

            // LLVM br instr becomes condition for elastic branch
            llvmBrFork->JustCntrlSuccs->push_back(branchC_node);
            branchC_node->JustCntrlPreds->push_back(llvmBrFork);

            if (llvmBrNode->CntrlPreds->size() == 0) {
                forkC_node->JustCntrlSuccs->push_back(llvmBrNode);
                llvmBrNode->JustCntrlPreds->push_back(forkC_node);
            }
        } else { // BB never ends with a branch or a jump, but with a ret, hence has no successor BB
            ENode* llvmRetNode = NULL;
            for (auto& enode : *enode_dag) {
                if (enode->type == Inst_ && enode->Name.compare("ret") == 0 &&
                    enode->BB == bbnode->BB) {
                    llvmRetNode = enode;
                    break;
                }
            }
            assert(llvmRetNode != NULL);

            if (llvmRetNode->CntrlPreds->size() ==
                0) { // connect to existing Ret only when ret has no preds
                forkC_node->JustCntrlSuccs->push_back(llvmRetNode);
                llvmRetNode->JustCntrlPreds->push_back(forkC_node);

                llvmRetNode->JustCntrlSuccs->push_back(retOr_node);
                retOr_node->JustCntrlPreds->push_back(llvmRetNode);

            } else {
                ENode* exitNode = new ENode(Sink_, "sink"); //, bbnode->BB);
                enode_dag->push_back(exitNode);
                forkC_node->JustCntrlSuccs->push_back(exitNode);
                exitNode->JustCntrlPreds->push_back(forkC_node);
                exitNode->id = sink_id++;

                llvmRetNode->CntrlSuccs->push_back(retOr_node);
                retOr_node->CntrlPreds->push_back(llvmRetNode);
            }
        }
    }

    //-------------------------------------------------//

    for (auto& bbnode : *bbnode_dag) {
        // current BB jumps/branches to a different BB and has successors; no ret
        if (bbnode->CntrlSuccs->size() != 0) {
            // finding control branch in BB
            ENode* branchCBB = NULL;
            for (auto& enode : *enode_dag) {
                if (enode->type == Branch_c && enode->BB == bbnode->BB) {
                    branchCBB = enode;
                }
            }
            assert(branchCBB != NULL);

            for (auto& bbnode_succ : *(bbnode->CntrlSuccs)) {
                ENode* phiCBBSucc = findPhiC(bbnode_succ);

                branchCBB->JustCntrlSuccs->push_back(phiCBBSucc);
                phiCBBSucc->JustCntrlPreds->push_back(branchCBB);
            }
        }
    }
}

void CircuitGenerator::setGetelementPtrConsts(std::vector<ENode*>* enode_dag) {
    // add constantsa to getelementptr which describe array dimensions
    // used to appropriately handle multi-dimensional arrays
    // (i.e., to convert multi-dim into single-dim address/index)

    std::vector<ENode*> nodeList;
    for (size_t idx = 0; idx < enode_dag->size(); idx++) {
        auto& enode = (*enode_dag)[idx];
        if (enode->type == Inst_) {

            if (enode->Instr->getOpcode() == Instruction::GetElementPtr) {

                Instruction* I = enode->Instr;

                int cst_count = 0; // I->getNumOperands() - 2;

                // extract dimensions from getelementptr instruction
                for (gep_type_iterator In = gep_type_begin(I), E = gep_type_end(I); In != E; ++In) {
                    if (In.isBoundedSequential()) {
                        ENode* cstNode = new ENode(Cst_, enode->BB);
                        // save num elements in array dimension as constant value
                        // will be used in VHDL to calculate address
                        cstNode->cstValue = In.getSequentialNumElements();
                        cstNode->JustCntrlSuccs->push_back(enode);
                        cstNode->id = cst_id++;
                        enode->JustCntrlPreds->push_back(cstNode);
                        nodeList.push_back(cstNode);
                        cst_count++;
                    }
                }
            }
        }
    }
    for (auto node : nodeList)
        enode_dag->push_back(node);
}
