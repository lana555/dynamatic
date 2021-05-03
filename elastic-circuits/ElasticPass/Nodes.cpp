#include "ElasticPass/Nodes.h"
#include "Head.h"
#include "Utils.h"
//----------------------------------------------------//

void ENode::commonInit(const node_type nd, const char* name, Instruction* inst, Argument* a,
                       BasicBlock* bb) {
    type = nd;
    Name = name;

    parameterNodesReferences[0] = new std::vector<ENode*>();
    parameterNodesReferences[1] = new std::vector<ENode*>();
    parameterNodesReferences[2] = new std::vector<ENode*>();

    CntrlPreds     = new std::vector<ENode*>;
    CntrlSuccs     = new std::vector<ENode*>;
    JustCntrlPreds = new std::vector<ENode*>;
    JustCntrlSuccs = new std::vector<ENode*>;
    //sizes          = new std::vector<unsigned>;

    Instr = inst;
    BB    = bb;
    A     = a;
}

ENode::ENode(node_type nd, const char* name, Instruction* inst, BasicBlock* bb) {
    commonInit(nd, name, inst, nullptr, bb);
}

ENode::ENode(node_type nd, BasicBlock* bb) { commonInit(nd, "NA", nullptr, nullptr, bb); }

ENode::ENode(node_type nd, const char* name, Argument* a, BasicBlock* bb) {
    commonInit(nd, name, nullptr, a, bb);
}

ENode::ENode(node_type nd, const char* name, BasicBlock* bb) {
    commonInit(nd, name, nullptr, nullptr, bb);
}

ENode::ENode(node_type nd, const char* name) { commonInit(nd, name, nullptr, nullptr, nullptr); }

bool ENode::isLoadOrStore() const {
    if (type != Inst_)
        return false;
    return isa<LoadInst>(Instr) || isa<StoreInst>(Instr);
}

//----------------------------------------------------//

BBNode::BBNode(BasicBlock* bb, int idx, const char* name) { 
    this->name  = name;
    BB          = bb;
    Idx         = idx;
    CntrlPreds  = new std::vector<BBNode*>;
    CntrlSuccs  = new std::vector<BBNode*>;
    Live_in     = new std::vector<ENode*>;
    Live_out    = new std::vector<ENode*>;
    Cntrl_nodes = new std::vector<ENode*>;
    counter     = 0;
}

bool BBNode::valid_successor(std::string succ_name) {
    for (auto& succ : *(this->CntrlSuccs)) {
        if (succ->BB->getName() == succ_name) {
            return true;
        }
    }
    return false;
}

void BBNode::set_succ_freq(std::string succ_name, double freq) {
    assert(freq >= 0);
    assert(valid_successor(succ_name));
    this->succ_freqs[succ_name] = freq;
}

double BBNode::get_succ_freq(std::string succ_name) {
    assert(valid_successor(succ_name));
    return this->succ_freqs[succ_name];
}

//----------------------------------------------------//
