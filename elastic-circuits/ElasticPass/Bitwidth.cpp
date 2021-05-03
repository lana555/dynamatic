#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

#define CONTROL_SIZE 0
#define DATA_SIZE 32


//---------------------------------------------
// defined in Utils.h :

int getSuccIdxInPred(const ENode* node, const ENode* pred) {
    // search for node in predecessor's ctrlSuccs
    int idx = indexOf(pred->CntrlSuccs, node);
    if (idx != -1)
        return idx;
    
    idx = indexOf(pred->JustCntrlSuccs, node);
    if (idx != -1)
        return pred->CntrlSuccs->size() + idx;

    return -1;
}
int getPredIdxInSucc(const ENode* node, const ENode* succ) {
    int idx = indexOf(succ->CntrlPreds, node);
    if (idx != -1)
        return idx;
    
    idx = indexOf(succ->JustCntrlPreds, node);
    if (idx != -1)
        return succ->CntrlPreds->size() + idx;
    
    return -1;
}

// I try to identify patterns that occur when an edge carries a condition
//eg. a Branch with predecessor a CmpInst (or a Fork with predecessor a CmpInst) 

bool isBranchConditionEdge(const ENode* from, const ENode* branch) {
    assert (branch->type == Branch_n || branch->type == Branch_c);

    // original idea : use fixed indices as displayed in the .dot output
    //however, this indexing is not respected in node generation

    // int idxInBranch = getPredIdxInSucc(from, branch);
    // return idxInBranch == BRANCH_CONDITION_IN;

    switch (from->type)
    {
    case Fork_:
        // cheat : as if from == fork.preds[0]
        return isBranchConditionEdge(from->CntrlPreds->front(), branch);

    case Branch_:
        return true;

    case Inst_: {
        unsigned int opcode = from->Instr->getOpcode();
        return opcode == Instruction::ICmp || opcode == Instruction::FCmp;
    }

    case Fork_c:
    default:
        return false;
    }
}
bool isMuxConditionEdge(const ENode* from, const ENode* mux) {
    assert (mux->type == Phi_ || mux->type == Phi_n);
    assert (mux->isMux);

    // int idxInMux = getPredIdxInSucc(from, mux);
    // return idxInMux == MUX_CONDITION_IN;

    switch (from->type)
    {
    case Fork_:
        return isMuxConditionEdge(from->CntrlPreds->front(), mux);

    case Phi_c: // TODO probably miss a condition here
        return from->isCntrlMg;
    
    default:
        return false;
    }

}
bool isCMergeConditionEdge(const ENode* cmerge, const ENode* to) {
    assert (cmerge->type == Phi_c);
    assert (cmerge->isCntrlMg);

    // int idxInCMerge = getSuccIdxInPred(to, cmerge);
    // return idxInCMerge == CMERGE_CONDITION_OUT;

    switch (to->type)
    {
    case Fork_:
        // like for branches/muxes, Forks are transparent here
        //if any successor is a condition, this output must be a condition
        for (const ENode* succ : *to->CntrlSuccs)
            if (isCMergeConditionEdge(cmerge, succ))
                return true;
        return false;

    case Phi_:
    case Phi_n:
        return to->isMux;
    
    default:
        return false;
    }
}


// returns the Value* held by the ENode, if not null (one in Instr, Arg, CI, CF)
static Value* getOutput(const ENode* e);

static unsigned getInputSize(const OptimizeBitwidth& OB, const ENode* node, const ENode* pred);
static unsigned getOutputSize(const OptimizeBitwidth& OB, const ENode* node, const ENode* succ);

void CircuitGenerator::setSizes() {

    if (!OptimizeBitwidth::isEnabled())
        return;

    for (ENode* node : *enode_dag) {
        //std::cout << node->Name << " (type=" << node->type << ") - " 
        //    << node->CntrlPreds->size() << ", " << node->JustCntrlPreds->size() << " - " 
        //    << node->CntrlSuccs->size() << ", " << node->JustCntrlSuccs->size() << std::endl;

        // add inputs one by one
        for (const ENode* pred : *node->CntrlPreds)
            node->sizes_preds.push_back(getInputSize(*OB, node, pred));
        for (const ENode* pred : *node->JustCntrlPreds)
            node->sizes_preds.push_back(getInputSize(*OB, node, pred));

        // add outputs one by ones
        for (const ENode* succ : *node->CntrlSuccs)
            node->sizes_succs.push_back(getOutputSize(*OB, node, succ));
        for (const ENode* succ : *node->JustCntrlSuccs)
            node->sizes_succs.push_back(getOutputSize(*OB, node, succ));

        // add at least one I/O to the nodes
        //eg. even for an End_ node with no successor,
        //an output ("out1:0") is still expected to be printed
        if (node->sizes_preds.empty())
            node->sizes_preds.push_back(CONTROL_SIZE);
        if (node->sizes_succs.empty())
            node->sizes_succs.push_back(CONTROL_SIZE);
    }
}


static unsigned getInputSize(const OptimizeBitwidth& OB, const ENode* node, const ENode* pred) {

    // for special nodes, (that may not have a corresponding value)
    //we just call getOutput of predecessor
    switch (node->type) {
    case Branch_n:
    case Branch_c:
        // bits needed for condition input : log2(#outs)
        if (isBranchConditionEdge(pred, node)) 
            return op::ceil_log2(node->CntrlSuccs->size() + node->JustCntrlSuccs->size());
        // else, same as Fork
    case Fork_:
    case Fork_c: // input_size = output_size of pred
        return getOutputSize(OB, pred, node);

    case Phi_:
    case Phi_n:
    case Phi_c:
        // if node is Mux, bits needed for condition is log2(#ins - 1) 
        //-1 because the condition itself is an input, and shouldn't be counted
        if (node->isMux && isMuxConditionEdge(pred, node))
            return op::ceil_log2(node->CntrlPreds->size() + node->JustCntrlPreds->size() - 1);
        return getOutputSize(OB, pred, node);

    case Start_:
    case Source_:
        return CONTROL_SIZE; // no inputs for these nodes

    case End_:
    case Sink_:
        return getOutputSize(OB, pred, node);

    case Branch_:
        return 1;

    case Cst_: // constant nodes only relay its value : input_size == output_size
        return getOutputSize(OB, node, nullptr);

    case Argument_:
    case Buffera_:
    case Bufferi_:
    case Fifoa_:
    case Fifoi_: {
        const Value* out = getOutput(node);
        return out != nullptr ? OB.getSize(out) : getOutputSize(OB, pred, node);
    }

    case Inst_:
        if (getOutput(pred) != nullptr)
            return OB.getSize(getOutput(pred));
        return CONTROL_SIZE;

    case MC_:
    case LSQ_:
    default:
        return contains(node->JustCntrlPreds, pred) ? CONTROL_SIZE : DATA_SIZE;
    }
}
static unsigned getOutputSize(const OptimizeBitwidth& OB, const ENode* node, const ENode* succ) {

    // if we have some output (any Value* somehow inherited), use it
    const Value* out = getOutput(node);

    switch (node->type)
    {
    case Fork_:
    case Fork_c:
        if (!node->CntrlPreds->empty())
            return getInputSize(OB, node, node->CntrlPreds->front());
        else
            return getInputSize(OB, node, node->JustCntrlPreds->front());

    case Branch_n: // output_size = input_size (not the condition, though!)
    case Branch_c: {
        for (const ENode* pred : *node->CntrlPreds)
            if (!isBranchConditionEdge(pred, node))
                return getInputSize(OB, node, pred);

        for (const ENode* pred : *node->JustCntrlPreds)
            if (!isBranchConditionEdge(pred, node))
                return getInputSize(OB, node, pred);

        assert (false && "Illegal state : Branch should have a non-condition input");
    }

    case Phi_:
    case Phi_n:
    case Phi_c:
        if (node->isCntrlMg && isCMergeConditionEdge(node, succ))
            return op::ceil_log2(node->CntrlPreds->size() + node->JustCntrlPreds->size());

        if (out != nullptr)
            return OB.getSize(out);
        else 
            return node->type == Phi_c ? CONTROL_SIZE : DATA_SIZE;
            
    case Start_:
        return CONTROL_SIZE;
    case Source_:
        if (node->CntrlSuccs->empty())
            return CONTROL_SIZE;
        else if (node->CntrlSuccs->front()->type == Cst_)
            return getInputSize(OB, node->CntrlSuccs->front(), node);
        else
            return out != nullptr ? OB.getSize(out) : DATA_SIZE;
        
    case End_:
    case Sink_:
        if (node->CntrlSuccs->empty() || node->CntrlPreds->empty())
            return CONTROL_SIZE;
        else
            return getInputSize(OB, node, node->CntrlPreds->front());

    case Branch_:
        return 1;
    case Cst_:
        return out != nullptr ? OB.getSize(out) : (op::findLeftMostBit(node->cstValue) + 1);

    case Argument_:
    case Buffera_:
    case Bufferi_:
    case Fifoa_:
    case Fifoi_:
        if (out != nullptr)
            return OB.getSize(out);
        else if (!node->CntrlPreds->empty())
            return getInputSize(OB, node, node->CntrlPreds->front());
        else
            return getInputSize(OB, node, node->JustCntrlPreds->front());

    case Inst_:
        if (node->Instr->getOpcode() == Instruction::Ret) {
            if (node->Instr->getNumOperands() != 0)
                return OB.getSize(node->Instr->getOperand(0));
            else
                return CONTROL_SIZE;
        }
        else
            return OB.getSize(out);

    case MC_:
    case LSQ_:
    default:
        return contains(node->JustCntrlSuccs, succ) ? CONTROL_SIZE : DATA_SIZE;
    }
}


static Value* getOutput(const ENode* e) {
    Value* res = nullptr;

    if (e->CI != nullptr)
        res = e->CI;
    else if (e->CF != nullptr)
        res = e->CF;
    else if (e->A != nullptr)
        res = e->A;
    else if (e->Instr != nullptr)
        res = e->Instr;
        
    return res;
}


/*
switch (node->type)
{
case Fork_:
case Fork_c:
case Branch_n:
case Branch_c:
case Phi_:
case Phi_n:
case Phi_c:

case Inst_:
case Argument_:
case Cst_:
case Branch_:

case Start_:
case End_:
case Source_:
case Sink_:

case Bufferi_:
case Buffera_:

case LSQ_:
case MC_:
default:
    break;
}

*/

/* JustCntrl <-> size == 0 IS FALSE : 
eg. Fork_->Branch_C node uses JustCntrlPred to communicate the condition input

ie. THIS DOESN'T HOLD
for (ENode* node : *enode_dag) {
    for (unsigned i = 0 ; i < node->JustCntrlPreds->size() ; ++i)
        if (node->sizes_preds.at(node->CntrlPreds->size() + i) != 0) {
            std::cout << "node " << node->type << ", " << node->Name << ", idx=" << i
                << "(#CntrlPreds=" << node->CntrlPreds->size() << ") ; "
                << "from=" << node->JustCntrlPreds->at(i)->Name << std::endl;
            assert (false);
        }

    for (unsigned i = 0 ; i < node->JustCntrlSuccs->size() ; ++i)
        if (node->sizes_succs.at(node->CntrlSuccs->size() + i) != 0) {
            std::cout << "node " << node->type << ", " << node->Name << ", idx=" << i
                << "(#CntrlSuccs=" << node->CntrlSuccs->size() << ") ; " 
                << "to=" << node->JustCntrlSuccs->at(i)->Name << std::endl;
            assert (false);
        }
}
*/