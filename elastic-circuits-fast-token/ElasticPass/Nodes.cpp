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

	// AYA 23/02/2022
	is_negated_input = new std::vector<bool>;
	is_advanced_component = false;

	// AYA 15/03/2022
	is_init_token_const = false;

	// AYA 19/03/2022
	is_init_token_merge = false;

    CntrlPreds     = new std::vector<ENode*>;
    CntrlSuccs     = new std::vector<ENode*>;
    JustCntrlPreds = new std::vector<ENode*>;
    JustCntrlSuccs = new std::vector<ENode*>;

	// Aya: introducing the following to assure correct order is maintained
	CntrlOrderPreds = new std::vector<ENode*>;
	CntrlOrderSuccs = new std::vector<ENode*>;

	isBackwardPreds_Cntrl = new std::vector<bool>;
	isBackwardPreds = new std::vector<bool>;
	
    //sizes          = new std::vector<unsigned>;

	// Aya: added this to avoid adding redundant branches for consumers in the same BB having the same producer
	/*done_bridges_flag = new std::vector<bool>;
	done_bridges_flag_Ctrl = new std::vector<bool>;
	twin_consumers = new std::vector<std::vector<ENode*>*>;
	twin_consumers_Ctrl = new std::vector<std::vector<ENode*>*>;
	*/

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

	//////////// Aya: newly added fields for keeping loop information
	is_loop_latch = false; 
	is_loop_exiting = false; 
	is_loop_header = false;

	// Aya: 25/02/2022: intialized the loop to a nullptr
	loop = nullptr;

    // Aya: 23/05/2022
    BB_deps = new std::vector<int>;

	//BasicBlock* loop_header_bb = new BasicBlock;
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
