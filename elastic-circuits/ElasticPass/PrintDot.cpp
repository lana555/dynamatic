#include "ElasticPass/ComponentsTiming.h"
#include "ElasticPass/Memory.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Pragmas.h"

using namespace std;

std::string NEW_LINE("\n");
std::string DHLS_VERSION("0.1.1");

#define BB_MINLEN 3
#define COND_SIZE 1
#define CONTROL_SIZE 0

std::ofstream dotfile;

// type of int comparison, obtained from llvm instruction
std::string getIntCmpType(ENode* enode) {

    int instr_type = dyn_cast<CmpInst>(enode->Instr)->getPredicate();
    std::string s  = "";

    switch (instr_type) {
        case CmpInst::ICMP_EQ:
            s = "icmp_eq";
            break;

        case CmpInst::ICMP_NE:
            s = "icmp_ne";
            break;

        case CmpInst::ICMP_UGT:
            s = "icmp_ugt";
            break;

        case CmpInst::ICMP_UGE:
            s = "icmp_uge";
            break;

        case CmpInst::ICMP_ULT:
            s = "icmp_ult";
            break;

        case CmpInst::ICMP_ULE:
            s = "icmp_ule";
            break;

        case CmpInst::ICMP_SGT:
            s = "icmp_sgt";
            break;

        case CmpInst::ICMP_SGE:
            s = "icmp_sge";
            break;

        case CmpInst::ICMP_SLT:
            s = "icmp_slt";
            break;

        case CmpInst::ICMP_SLE:
            s = "icmp_sle";
            break;

        default:
            s = "icmp_arch";
            break;
    }
    return s;
}

// type of float comparison, obtained from llvm instruction
std::string getFloatCmpType(ENode* enode) {

    int instr_type = dyn_cast<CmpInst>(enode->Instr)->getPredicate();
    std::string s  = "";

    switch (instr_type) {

        case CmpInst::FCMP_OEQ:
            s = "fcmp_oeq";
            break;

        case CmpInst::FCMP_OGT:
            s = "fcmp_ogt";
            break;

        case CmpInst::FCMP_OGE:
            s = "fcmp_oge";
            break;

        case CmpInst::FCMP_OLT:
            s = "fcmp_olt";
            break;

        case CmpInst::FCMP_OLE:
            s = "fcmp_ole";
            break;

        case CmpInst::FCMP_ONE:
            s = "fcmp_one";
            break;

        case CmpInst::FCMP_ORD:
            s = "fcmp_ord";
            break;

        case CmpInst::FCMP_UNO:
            s = "fcmp_uno";
            break;

        case CmpInst::FCMP_UEQ:
            s = "fcmp_ueq";
            break;

        case CmpInst::FCMP_UGE:
            s = "fcmp_uge";
            break;

        case CmpInst::FCMP_ULT:
            s = "fcmp_ult";
            break;

        case CmpInst::FCMP_ULE:
            s = "fcmp_ule";
            break;

        case CmpInst::FCMP_UNE:
            s = "fcmp_une";
            break;

        case CmpInst::FCMP_UGT:
            s = "fcmp_ugt";
            break;

        default:
            s = "fcmp_arch";
            break;
    }

    return s;
}

bool isLSQport(ENode* enode) {
    for (auto& succ : *(enode->CntrlSuccs))
        if (succ->type == LSQ_)
            return true;
    return false;
}

// check llvm instruction operands to connect enode with instruction successor
int getOperandIndex(ENode* enode, ENode* enode_succ) {
    const Value* V;

    // force load address from predecessor to port 2
    if (enode_succ->Instr->getOpcode() == Instruction::Load && enode->type != MC_ &&
        enode->type != LSQ_)
        return 2;

    // purposefully not in if-else because of issues with dyn_cast
    if (enode->Instr != NULL)
        V = dyn_cast<Value>(enode->Instr);
    if (enode->A != NULL)
        V = dyn_cast<Value>(enode->A);
    if (enode->CI != NULL)
        V = dyn_cast<Value>(enode->CI);
    if (enode->CF != NULL)
        V = dyn_cast<Value>(enode->CF);

    // for getelementptr, use preds ordering because operands can have identical values
    if (enode_succ->Instr->getOpcode() == Instruction::GetElementPtr)
        return indexOf(enode_succ->CntrlPreds, enode) + 1;

    // if both operands are constant, values could be the same
    // return index in predecessor array
    if (enode->CI != NULL || enode->CF != NULL) {
        if (enode_succ->CntrlPreds->size() >= 2)
            if (enode_succ->Instr->getOperand(0) == enode_succ->Instr->getOperand(1))
                return indexOf(enode_succ->CntrlPreds, enode) + 1;
    }

    for (int i = 0; i < (int)enode_succ->CntrlPreds->size(); i++)
        if (V == enode_succ->Instr->getOperand(i)) {
            return i + 1;
        }

    // special case for store which predecessor not found
    if (enode_succ->Instr->getOpcode() == Instruction::Store)
        return 2;

    return 1;
}

bool skipNodePrint(ENode* enode) {
    if (enode->type == Argument_ && enode->CntrlSuccs->size() == 0)
        return true;
    if ((enode->type == Branch_n) || (enode->type == Branch_c))
        if (enode->CntrlSuccs->size() == 1)
            if (enode->CntrlSuccs->front()->type == Fork_ ||
                enode->CntrlSuccs->front()->type == Branch_ ||
                enode->CntrlSuccs->front()->type == Branch_n)
                return true;
    if (enode->type == Branch_ && enode->JustCntrlPreds->size() != 1)
        return true;
    if (enode->type == MC_) // enode->memPort
        return false;
    return false;
}

int getInPortSize(ENode* enode, int index) {

    if (enode->type == Argument_) {
        return DATA_SIZE;
    } else if (enode->type == Branch_n) {
        return (index == 0) ? DATA_SIZE : COND_SIZE;
    } else if (enode->type == Branch_c) {
        return (index == 0) ? CONTROL_SIZE : COND_SIZE;
    } else if (enode->type == Branch_) {
        return COND_SIZE;

    } else if (enode->type == Inst_) {
        if (enode->Instr->getOpcode() == Instruction::Select && index == 0)
            return COND_SIZE;
        else
            return (enode->JustCntrlPreds->size() > 0) ? CONTROL_SIZE : DATA_SIZE;
    } else if (enode->type == Fork_) {
        return (enode->CntrlPreds->front()->type == Branch_ ||
                (enode->CntrlPreds->front()->isCntrlMg &&
                 enode->CntrlPreds->front()->type == Phi_c))
                   ? COND_SIZE
                   : DATA_SIZE;

    } else if (enode->type == Fork_c) {
        return CONTROL_SIZE;
    } else if (enode->type == Buffera_ || enode->type == Bufferi_ || enode->type == Fifoa_ ||
               enode->type == Fifoi_) {
        return enode->CntrlPreds->size() > 0 ? DATA_SIZE : CONTROL_SIZE;

    } else if (enode->type == Phi_ || enode->type == Phi_n) {
        return (index == 0 && enode->isMux) ? COND_SIZE : DATA_SIZE;
    } else if (enode->type == Phi_c) {
        return CONTROL_SIZE;
    } else if (enode->type == Cst_) {
        return DATA_SIZE;
    } else if (enode->type == Start_) {
        return CONTROL_SIZE;
    } else if (enode->type == Sink_) {
        return enode->CntrlPreds->size() > 0 ? DATA_SIZE : CONTROL_SIZE;
    }
}

int getOutPortSize(ENode* enode, int index) {
    if (enode->type == Inst_) {
        if (enode->Instr->getOpcode() == Instruction::ICmp ||
            enode->Instr->getOpcode() == Instruction::FCmp)
            return COND_SIZE;
        else
            return (enode->JustCntrlPreds->size() > 0) ? CONTROL_SIZE : DATA_SIZE;
    } else if (enode->type == Source_) {
        return (enode->CntrlSuccs->size() > 0 ? DATA_SIZE : CONTROL_SIZE);
    } else if (enode->type == Phi_ || enode->type == Phi_n) {
        return DATA_SIZE;
    } else {
        return getInPortSize(enode, 0);
    }
}
unsigned int getConstantValue(ENode* enode) {

    if (enode->CI != NULL)
        return enode->CI->getSExtValue();
    else if (enode->CF != NULL) {
        ConstantFP* constfp_gv = llvm::dyn_cast<llvm::ConstantFP>(enode->CF);
        float gv_fpval         = (constfp_gv->getValueAPF()).convertToFloat();
        return *(unsigned int*)&gv_fpval;
    } else
        return enode->cstValue;
}

void printDotCFG(std::vector<BBNode*>* bbnode_dag, std::string name) {

    std::string output_filename = name;

    dotfile.open(output_filename);

    std::string s;

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(std::ctime(&t));

    s = "Digraph G {\n\tsplines=spline;\n";

    // infoStr += "//Graph created: " + date;
    s += "//DHLS version: " + DHLS_VERSION;
    s += "\" [shape = \"none\" pos = \"20,20!\"]\n";

    for (auto& bnd : *bbnode_dag) {
        s += "\t\t\"";
        s += bnd->BB->getName().str().c_str();
        s += "\";\n";
    }

    for (auto& bnd : *bbnode_dag) {
        for (auto& bnd_succ : *(bnd->CntrlSuccs)) {
            s += "\t\t\"";
            s += bnd->BB->getName().str().c_str();
            s += "\" -> \"";
            s += bnd_succ->BB->getName().str().c_str();
            s += "\" [color = \"";
            // back edge if BB succ has id smaller or equal (self-edge) to bb
            s += (bnd_succ->Idx <= bnd->Idx) ? "red" : "blue";
            s += "\", freq = ";
            // freq extracted through profiling
            int freq = bnd->get_succ_freq(bnd_succ->BB->getName());
            s += to_string(freq);
            s += "];\n";
        }
    }

    dotfile << s;
    dotfile << "}";

    dotfile.close();
}

std::string getNodeDotNameNew(ENode* enode) {
    string name = "\"";
    switch (enode->type) {
        case Branch_: // treat llvm branch as constant
            name += "brCst_";
            name += enode->BB->getName().str().c_str();
            break;
        case LSQ_:
            name += "LSQ_";
            name += enode->Name;
            break;
        case MC_:
            name += "MC_";
            name += enode->Name;
            break;
        case Cst_:
            name += "cst_";
            name += to_string(enode->id);
            break;
        case Argument_:
            name += enode->argName.c_str();
            break;
        case Phi_n:
            name += enode->Name;
            name += "_n";
            name += to_string(enode->id);
            break;
        default:
            name += enode->Name;
            name += "_";
            name += to_string(enode->id);
            break;
    }
    name += "\"";
    return name;
}

std::string getNodeDotTypeNew(ENode* enode) {
    string name = "type = \"";

    switch (enode->type) {
        case Inst_:
            name += "Operator";
            break;
        case Phi_:
        case Phi_n:
            if (enode->isMux)
                name += "Mux";
            else
                name += "Merge";
            break;
        case Phi_c:
            if (enode->isCntrlMg)
                name += "CntrlMerge";
            else
                name += "Merge";
            break;
        case Fork_:
        case Fork_c:
            name += "Fork";
            break;
        case Buffera_:
        case Bufferi_:
            name += "Buffer";
            break;
        case Fifoa_:
        case Fifoi_:
            name += "Fifo";
            break;
        case Start_:
            name += "Entry\", control= \"true";
            break;
        case Source_:
            name += "Source";
            break;
        case Sink_:
            name += "Sink";
            break;
        case End_:
            name += "Exit";
            break;
        case LSQ_:
            name += "LSQ";
            break;
        case MC_:
            name += "MC";
            break;
        case Cst_:
            name += "Constant";
            break;
        case Argument_:
            name += "Entry";
            break;
        case Branch_n:
        case Branch_c:
            name += "Branch";
            break;
        case Branch_: // treat llvm branch as constant
            name += "Constant";
            break;
        default:
            break;
    }

    name += "\", ";
    return name;
}

std::string getNodeDotbbID(ENode* enode) {
    string name = "bbID= ";
    name += to_string(getBBIndex(enode));
    return name;
}

std::string getFuncName(ENode* enode) {
    Instruction* I  = enode->Instr;
    StringRef fname = cast<CallInst>(I)->getCalledFunction()->getName();
    return fname.str();
}

std::string getNodeDotOp(ENode* enode) {
    string name = ", op = \"";
    switch (enode->Instr->getOpcode()) {
        case Instruction::ICmp:
            name += getIntCmpType(enode);
            name += "_op\"";
            break;
        case Instruction::FCmp:
            name += getFloatCmpType(enode);
            name += "_op\"";
            break;
        case Instruction::Load:
            name += (isLSQport(enode) ? "lsq_load" : "mc_load");
            name += "_op\", ";
            name += "bbID= " + to_string(getBBIndex(enode));
            name += ", portId= " + to_string(getMemPortIndex(enode));
            // name += ", offset= " + to_string(getBBOffset(enode));
            break;
        case Instruction::Store:
            name += (isLSQport(enode) ? "lsq_store" : "mc_store");
            name += "_op\", ";
            name += "bbID= " + to_string(getBBIndex(enode));
            name += ", portId= " + to_string(getMemPortIndex(enode));
            // name += ", offset= " + to_string(getBBOffset(enode));
            break;
        case Instruction::Call:
            name += enode->Name;
            name += "_op\", ";
            name += "function = \"" + getFuncName(enode) + "\" ";
            break;
        default:
            name += enode->Name;
            name += "_op\"";
            break;
    }

    return name;
}

std::string getNodeDotInputs(ENode* enode) {
    string name = "";

    switch (enode->type) {
        case Argument_:
        case Branch_:
        case Fork_:
        case Fork_c:
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Start_:
        case Sink_:
            name += ", in = \"in1:" + to_string(getInPortSize(enode, 0)) + "\"";
            break;
        case Branch_n:
        case Branch_c:
            name += ",  in = \"in1:" + to_string(getInPortSize(enode, 0));
            name += " in2?:" + to_string(getInPortSize(enode, 1)) + "\"";
            break;

        case Inst_:
        case Phi_:
        case Phi_n:
        case Phi_c:
            name += ", in = \"";

            for (int i = 0; i < (int)enode->CntrlPreds->size() + (int)enode->JustCntrlPreds->size();
                 i++) {
                name += "in" + to_string(i + 1);
                name += ((i == 0 && enode->isMux) ? "?:" : ":");
                name += to_string(getInPortSize(enode, i)) + " ";
            }
            if (enode->type == Inst_)
                if (enode->Instr->getOpcode() == Instruction::Load)
                    name += "in2:32";
            name += "\"";
            break;

        case LSQ_:
        case MC_:
            name += ", in = \"";

            for (int i = 0; i < (int)enode->JustCntrlPreds->size(); i++) {
                name += "in" + to_string(i + 1) + ":" +
                        to_string(enode->type == MC_ ? DATA_SIZE : CONTROL_SIZE);
                name += "*c" + to_string(i) + " ";
            }

            for (auto& pred : *enode->CntrlPreds) {
                if (pred->type != Cst_) {

                    name +=
                        "in" + to_string(getDotAdrIndex(pred, enode)) + ":" + to_string(ADDR_SIZE);
                    name += (pred->Instr->getOpcode() == Instruction::Load) ? "*l" : "*s";
                    name += to_string(pred->memPortId) + "a ";

                    if (pred->Instr->getOpcode() == Instruction::Store) {
                        name += "in" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                to_string(DATA_SIZE);
                        name += "*s" + to_string(pred->memPortId) + "d ";
                    }
                }
            }
            name += "\"";
            break;
        case End_:

            name += ", in = \"";
            for (auto& pred : *enode->JustCntrlPreds) {
                auto it =
                    std::find(enode->JustCntrlPreds->begin(), enode->JustCntrlPreds->end(), pred);
                int ind = distance(enode->JustCntrlPreds->begin(), it);
                name += "in" + to_string(ind + 1) + ":" + to_string(CONTROL_SIZE);
                // if its not memory, it is the single control port
                name += (pred->type == MC_ || pred->type == LSQ_) ? "*e " : "";
            }

            for (auto i = enode->JustCntrlPreds->size();
                 i < (enode->JustCntrlPreds->size() + enode->CntrlPreds->size());
                 i++) // if data ports, count from zero
                name += "in" + to_string(i + 1) + ":" + to_string(DATA_SIZE) + " ";
            name += "\"";
            break;
        default:
            break;
    }

    return name;
}

std::string getNodeDotOutputs(ENode* enode) {
    string name = "";

    switch (enode->type) {
        case Argument_:
        case Buffera_:
        case Bufferi_:
        case Cst_:
        case Start_:
        case Branch_:
        case Phi_:
        case Phi_n:
        case Source_:
            name += ", out = \"out1:" + to_string(getOutPortSize(enode, 0)) + "\"";
            break;
        case Branch_n:
        case Branch_c:
            name += ", out = \"out1+:" + to_string(getInPortSize(enode, 0));
            name += " out2-:" + to_string(getInPortSize(enode, 0)) + "\"";
            break;
        case Fork_:
        case Fork_c:
        case Inst_:

            if (enode->CntrlSuccs->size() > 0 || enode->JustCntrlSuccs->size() > 0) {
                name += ", out = \"";
                for (int i = 0;
                     i < (int)enode->CntrlSuccs->size() + (int)enode->JustCntrlSuccs->size(); i++) {
                    name += "out" + to_string(i + 1) + ":";
                    name += to_string(getOutPortSize(enode, 0)) + " ";
                }
            }
            if (enode->type == Inst_)
                if (enode->Instr->getOpcode() == Instruction::Store)
                    name += "out2:32";

            name += "\"";
            break;

        case Phi_c:
            name += ", out = \"out1:" + to_string(CONTROL_SIZE);
            if (enode->isCntrlMg)
                name += " out2?:" + to_string(COND_SIZE);
            name += "\"";
            break;

        case LSQ_:
        case MC_:
            name += ", out = \"";

            if (getMemOutputCount(enode) > 0) {
                for (auto& pred : *enode->CntrlPreds)
                    if (pred->type != Cst_ && pred->Instr->getOpcode() == Instruction::Load) {
                        name += "out" + to_string(getDotDataIndex(pred, enode)) + ":" +
                                to_string(DATA_SIZE);
                        name += "*l" + to_string(pred->memPortId) + "d ";
                    }
            }

            name += "out" + to_string(getMemOutputCount(enode) + 1) + ":" +
                    to_string(CONTROL_SIZE) + "*e ";
            name += "\"";
            break;
        case End_:
            name += ", out = \"out1:";

            if (enode->CntrlPreds->size() > 0)
                name += to_string(DATA_SIZE);
            else 
                name += to_string(CONTROL_SIZE);
            name += "\"";
            break;
        default:
            break;
    }

    return name;
}

std::string getFloatValue(float x) {
#define nodeDotNameSIZE 100
    char* nodeDotName = new char[nodeDotNameSIZE];
    snprintf(nodeDotName, nodeDotNameSIZE, "%2.3f", x);
    return string(nodeDotName);
}

std::string getHexValue(int x) {
#define nodeDotNameSIZE 100
    char* nodeDotName = new char[nodeDotNameSIZE];
    snprintf(nodeDotName, nodeDotNameSIZE, "%08X", x);
    return string(nodeDotName);
}

std::string getNodeDotParams(ENode* enode) {
    string name = "";
    switch (enode->type) {
        case Branch_:
            name += ", value = \"0x1\"";
            break;
        case Inst_:
            name += ", delay=" + getFloatValue(get_component_delay(enode->Name, DATA_SIZE));
            
            if (isLSQport(enode))
                name += ", latency=" + to_string(get_component_latency(("lsq_" + enode->Name), DATA_SIZE));
            else
                name += ", latency=" + to_string(get_component_latency((enode->Name), DATA_SIZE));
            name += ", II=1";
            break;
        case Phi_:
        case Phi_n:
            name += ", delay=";
            if (enode->CntrlPreds->size() == 1)
                name += getFloatValue(get_component_delay(ZDC_NAME, DATA_SIZE));
            else
                name += getFloatValue(get_component_delay(enode->Name, DATA_SIZE));
            break;
        case Phi_c:
            name += ", delay=" + getFloatValue(get_component_delay(enode->Name, DATA_SIZE));
            break;
        case Cst_:
            name += ", value = \"0x" + getHexValue(getConstantValue(enode)) + "\"";
        default:
            break;
    }

    return name;
}

std::string getLSQJsonParams(ENode* memnode) {
    // Additional LSQ configuration params for json file

    string fifoDepth = ", fifoDepth = ";
    string numLd     = ", numLoads = \"{";
    string numSt     = "numStores = \"{";
    string ldOff     = "loadOffsets = \"{";
    string stOff     = "storeOffsets = \"{";
    string ldPts     = "loadPorts = \"{";
    string stPts     = "storePorts = \"{";

    int count = 0;
    int depth = getLSQDepth(memnode);
    fifoDepth += to_string(depth);

    for (auto& enode : *memnode->JustCntrlPreds) {
        BBNode* bb = enode->bbNode;

        int ld_count = getBBLdCount(enode, memnode);
        int st_count = getBBStCount(enode, memnode);

        numLd += to_string(ld_count);
        numLd += count == (memnode->JustCntrlPreds->size() - 1) ? "" : "; ";
        numSt += to_string(st_count);
        numSt += count == (memnode->JustCntrlPreds->size() - 1) ? "" : "; ";

        ldOff += "{";
        stOff += "{";
        ldPts += "{";
        stPts += "{";

        for (auto& node : *memnode->CntrlPreds) {

            if (node->type != Cst_) {
                auto* I = node->Instr;
                if (isa<LoadInst>(I) && node->BB == enode->BB) {
                    string d = (ldOff.find_last_of("{") == ldOff.size() - 1) ? "" : ";";
                    ldOff += d;
                    ldOff += to_string(getBBOffset(node));
                    ldPts += d;
                    ldPts += to_string(getMemPortIndex(node));

                } else if (isa<StoreInst>(I) && node->BB == enode->BB) {
                    string d = (stOff.find_last_of("{") == stOff.size() - 1) ? "" : ";";
                    stOff += d;
                    stOff += to_string(getBBOffset(node));
                    stPts += d;
                    stPts += to_string(getMemPortIndex(node));
                }
            }
        }

        // zeroes up to lsq depth needed for json config
        for (int i = ld_count; i < depth; i++) {
            string d = (i == 0) ? "" : ";";
            ldOff += d;
            ldOff += "0";
            ldPts += d;
            ldPts += "0";
        }

        for (int i = st_count; i < depth; i++) {
            string d = (i == 0) ? "" : ";";
            stOff += d;
            stOff += "0";
            stPts += d;
            stPts += "0";
        }
        ldOff += "}";
        stOff += "}";
        ldPts += "}";
        stPts += "}";
        count++;
    }

    numLd += "}\", ";
    numSt += "}\", ";
    ldOff += "}\", ";
    stOff += "}\", ";
    ldPts += "}\", ";
    stPts += "}\"";

    return fifoDepth + numLd + numSt + ldOff + stOff + ldPts + stPts;
}

std::string getNodeDotMemParams(ENode* enode) {
    string name = ", memory = \"" + string(enode->Name) + "\", ";
    name += "bbcount = " + to_string(getMemBBCount(enode)) + ", ";
    name += "ldcount = " + to_string(getMemLoadCount(enode)) + ", ";
    name += "stcount = " + to_string(getMemStoreCount(enode));
    if (enode->type == LSQ_)
        name += getLSQJsonParams(enode);
    return name;
}

void printDotNodes(std::vector<ENode*>* enode_dag, bool full) {

    for (auto& enode : *enode_dag) {
        if (!skipNodePrint(enode)) {
            // enode->memPort = false; //temporary, disable lsq connections
            string dotline = "\t\t";

            dotline += getNodeDotNameNew(enode);

            if (full) {

                dotline += " [";

                dotline += getNodeDotTypeNew(enode);
                dotline += getNodeDotbbID(enode);

                if (enode->type == Inst_)
                    dotline += getNodeDotOp(enode);

                dotline += getNodeDotInputs(enode);
                dotline += getNodeDotOutputs(enode);
                dotline += getNodeDotParams(enode);
                if (enode->type == MC_ || enode->type == LSQ_)
                    dotline += getNodeDotMemParams(enode);

                dotline += "];\n";
            }

            dotfile << dotline;
        }
    }
}

std::string printEdge(ENode* from, ENode* to) {
    string s = "\t\t";
    s += getNodeDotNameNew(from);
    s += " -> ";
    s += getNodeDotNameNew(to);
    return s;
}

std::string printColor(ENode* from, ENode* to) {
    string s = "color = \"";

    if (from->type == Phi_c && (to->type == Fork_ || to->type == Phi_ || to->type == Phi_n))
        s += "green"; // connection from CntrlMg to Muxes
    else if (from->type == Phi_c || from->type == Branch_c || from->type == Fork_c ||
             to->type == Phi_c || to->type == Branch_c || to->type == Fork_c ||
             from->type == Start_ || (to->type == End_ && from->JustCntrlSuccs->size() > 0))
        s += "gold3"; // control-only
    else if (from->type == MC_ || from->type == LSQ_ || to->type == MC_ || to->type == LSQ_)
        s += "darkgreen"; // memory
    else if (from->type == Branch_n)
        s += "blue"; // outside BB
    else if (from->type == Branch_ || to->type == Branch_)
        s += "magenta"; // condition
    else
        s += "red"; // inside BB

    s += "\"";
    if (from->type == Branch_n || from->type == Branch_c)
        s += ", minlen = " + to_string(BB_MINLEN);

    return s;
}

std::string printMemEdges(std::vector<ENode*>* enode_dag) {
    string memstr = "";
    std::map<void*, int> incount;

    // print lsq/mc connections
    for (auto& enode : *enode_dag) {
        if (enode->type == MC_ || enode->type == LSQ_) {
            int index = 1;
            for (auto& pred : *(enode->JustCntrlPreds)) {
                if (pred->type == Cst_) {
                    memstr += printEdge(pred, enode);
                    memstr += " [";
                    memstr += printColor(pred, enode);
                    memstr += ", from = \"out1\"";
                    memstr += ", to = \"in" + to_string(index) + "\"";
                    memstr += "];\n";
                    index++;
                }
            }
            int end_ind = getMemOutputCount(enode) + 1;
            for (auto& succ : *(enode->JustCntrlSuccs)) {
                if (succ->type == End_) {
                    memstr += printEdge(enode, succ);
                    memstr += " [";
                    memstr += printColor(enode, succ);

                    auto it = std::find(succ->JustCntrlPreds->begin(), succ->JustCntrlPreds->end(),
                                        enode);
                    int succ_ind = distance(succ->JustCntrlPreds->begin(), it) + 1;

                    memstr += ", from = \"out" + to_string(end_ind) + "\"";
                    memstr += ", to = \"in" + to_string(succ_ind) + "\"";
                    memstr += "];\n";
                }
            }
        }

        if (enode->type != Inst_)
            continue;

        auto* I = enode->Instr;
        // enode->memPort = false;
        if ((isa<LoadInst>(I) || isa<StoreInst>(I)) && enode->memPort) {
            int mcidx;
            auto* MEnode = (ENode*)enode->Mem;

            mcidx                  = incount[(void*)MEnode];
            incount[(void*)MEnode] = mcidx + 1;

            int address_ind = getDotAdrIndex(enode, MEnode);
            int data_ind    = getDotDataIndex(enode, MEnode);

            memstr += printEdge(enode, MEnode);
            memstr += " [";
            memstr += printColor(enode, MEnode);
            memstr += ", mem_address = \"true\"";
            memstr += ", from = \"out2\"";
            memstr += ", to = \"in" + to_string(address_ind) + "\"";
            memstr += "];\n";

            if (isa<LoadInst>(I)) {
                memstr += printEdge(MEnode, enode);
                memstr += " [";
                memstr += printColor(MEnode, enode);
                memstr += ", mem_address = \"false\"";
                memstr += ", from = \"out" + to_string(data_ind) + "\"";
                memstr += ", to = \"in1\"";
            } else {
                memstr += printEdge(enode, MEnode);
                memstr += " [";
                memstr += printColor(enode, MEnode);
                memstr += ", mem_address = \"false\"";
                memstr += ", from = \"out1\"";
                memstr += ", to = \"in" + to_string(data_ind) + "\"";
            }
            memstr += "];\n";
        }
    }
    return memstr;
}

std::string printDataflowEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag) {
    string str = "";

    int count = 0;

    for (auto& bnd : *bbnode_dag) {
        llvm::BasicBlock* branchSucc1;
        bool unconditional = false;

        str += "\tsubgraph cluster_" + to_string(count) + " {\n"; // draw cluster around BB
        str += "\tcolor = \"darkgreen\";\n";
        str += "\t\tlabel = \"" + string(bnd->BB->getName().str().c_str()) + "\";\n";

        for (auto& enode : *enode_dag) {
            if (enode->type != Branch_n && enode->BB == bnd->BB) {
                std::vector<ENode*>* pastSuccs = new std::vector<ENode*>;

                int enode_index = 1;

                for (auto& enode_succ : *(enode->CntrlSuccs)) {
                    if (enode_succ->type != LSQ_ && enode_succ->type != MC_) {

                        if (enode_succ->type == Branch_) { // redundant Branch nodes--do not print

                            if (enode_succ->CntrlSuccs->size() > 0) {
                                if (enode_succ->CntrlSuccs->front()->type == Fork_ ||
                                    enode_succ->CntrlSuccs->front()->type == Branch_)
                                    str += printEdge(
                                        enode,
                                        enode_succ->CntrlSuccs
                                            ->front()); // print successor of redundant  Branch

                                else // normal print
                                    str += printEdge(enode, enode_succ);

                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out" + to_string(enode_index) + "\"";
                                str += ", to = \"in1\""; // data Branch or Fork input
                                str += "];\n";
                            }
                            if (enode_succ->JustCntrlSuccs->size() > 0) {

                                if (enode_succ->JustCntrlSuccs->front()->type == Fork_c ||
                                    enode_succ->JustCntrlSuccs->front()->type == Branch_c)
                                    str += printEdge(
                                        enode,
                                        enode_succ->JustCntrlSuccs
                                            ->front()); // print successor of redundant  Branch

                                else                                     // normal print
                                    str += printEdge(enode, enode_succ); // print successor

                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out" + to_string(enode_index) + "\"";
                                str += ", to = \"in2\""; /// Branch condition
                                str += "];\n";
                            }
                        } else if (enode->type == Branch_ &&
                                   enode_succ->type ==
                                       Fork_) { // redundant Branch nodes--do not print

                            llvm::BranchInst* BI =
                                dyn_cast<llvm::BranchInst>(enode->Instr); // determine successor BBs

                            if (BI->isUnconditional()) {
                                unconditional = true;
                            } else {
                                branchSucc1 = BI->getSuccessor(1); // successor for condition false
                            }
                            if (enode->JustCntrlPreds->size() == 1) { // connected to ctrl fork

                                str += printEdge(enode, enode_succ);
                                str += " [";
                                str += printColor(enode, enode_succ);
                                str += ", from = \"out1\"";

                                if (enode_succ->type == Fork_)
                                    str += ", to = \"in1\"";
                                else if (enode_succ->type == Branch_)
                                    str += ", to = \"in2\"";

                                str += "];\n";
                            }
                        } else {
                            str += printEdge(enode, enode_succ);
                            str += " [";
                            str += printColor(enode, enode_succ);

                            str += ", from = \"out" +
                                   (enode->isCntrlMg ? "2" : to_string(enode_index)) + "\"";

                            if (enode->type == Fork_ &&
                                enode->CntrlPreds->front()->type == Branch_) { // Branch condition
                                str += ", to = \"in2\"";

                            } else if (enode->isCntrlMg ||
                                       enode->type == Fork_ &&
                                           enode->CntrlPreds->front()->isCntrlMg) {
                                str += ", to = \"in1\"";

                            } else { // every other enode except branch, check successor

                                if (enode_succ->type == Inst_) {

                                    if (contains(pastSuccs,
                                                 enode_succ)) { // when fork has multiple
                                                                // connections to same node
                                        int count = 1;
                                        for (auto& past : *pastSuccs)
                                            if (past == enode_succ)
                                                count++;

                                        str += ", to = \"in" + to_string(count) + "\"";

                                    } else if (enode_succ->Instr->getOpcode() ==
                                               Instruction::Call) {

                                        int toInd = indexOf(enode_succ->CntrlPreds, enode) + 1;
                                        str += ", to = \"in" + to_string(toInd) + "\"";
                                    } else {
                                        int toInd = getOperandIndex(
                                            enode, enode_succ); // check llvm instruction operands
                                        str += ", to = \"in" + to_string(toInd) + "\"";
                                    }

                                } else { // every other successor

                                    int toInd = (enode_succ->type == Branch_n)
                                                    ? 1
                                                    : indexOf(enode_succ->CntrlPreds, enode) + 1;
                                    toInd = (enode_succ->type == End_)
                                                ? enode_succ->JustCntrlPreds->size() + toInd
                                                : toInd;
                                    str += ", to = \"in" + to_string(toInd) + "\"";
                                }
                            }
                            str += "];\n";
                        }
                    }
                    pastSuccs->push_back(enode_succ);
                    enode_index++;
                }

                for (auto& enode_csucc : *(enode->JustCntrlSuccs)) {

                    if ((enode_csucc->BB == enode->BB || enode_csucc->type == Sink_) &&
                        enode->type != Branch_c && !skipNodePrint(enode)) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);

                        if (enode->type == Phi_c && enode->isCntrlMg)
                            str += ", from = \"out1\"";
                        else
                            str += ", from = \"out" + to_string(enode_index) + "\"";

                        int toInd = (enode->type == Fork_ || enode->type == Branch_) ? 2 : 1;
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        str += "];\n";
                    }

                    if (enode_csucc->type == End_ || enode_csucc->type == LSQ_) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);
                        str += ", from = \"out" + to_string(enode_index) + "\"";
                        int toInd = indexOf(enode_csucc->JustCntrlPreds, enode) + 1;
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        str += "];\n";
                    }

                    enode_index++;
                }
            }
        }

        str += "\t}\n";

        for (auto& enode : *enode_dag) {
            if (enode->BB == bnd->BB) {
                for (auto& enode_succ : *(enode->CntrlSuccs)) {

                    if (enode->type == Branch_n && enode_succ->type != Branch_ &&
                        enode_succ->type != Branch_n) {
                        str += printEdge(enode, enode_succ);
                        str += " [";
                        str += printColor(enode, enode_succ);

                        int toInd = indexOf(enode_succ->CntrlPreds, enode) + 1;

                        if (enode_succ->isMux &&
                            (enode_succ->type == Phi_ || enode_succ->type == Phi_n)) {
                            int fromInd;
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_succ->BB == branchSucc1 ? 2 : 1;

                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd + 1) + "\"";
                        } else if (enode_succ->type != Sink_) {
                            int fromInd;
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_succ->BB == branchSucc1 ? 2 : 1;
                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd) + "\"";

                        } else {
                            int fromInd;
                            if (unconditional)
                                fromInd = 2;
                            else {
                                for (auto& succ : *(enode->CntrlSuccs)) {
                                    if (succ != enode_succ) {
                                        fromInd = succ->BB == branchSucc1 ? 1 : 2;
                                        break;
                                    }
                                }
                            }
                            str += ", from = \"out" + to_string(fromInd) + "\"";
                            str += ", to = \"in" + to_string(toInd) + "\"";
                        }
                        str += "];\n";
                    }
                }

                for (auto& enode_csucc : *(enode->JustCntrlSuccs)) {
                    if (enode->type == Branch_c && enode->BB != NULL) {
                        str += printEdge(enode, enode_csucc);
                        str += " [";
                        str += printColor(enode, enode_csucc);

                        int toInd = indexOf(enode_csucc->JustCntrlPreds, enode) + 1;

                        int fromInd;

                        if (enode_csucc->type != Sink_) {
                            if (unconditional)
                                fromInd = 1;
                            else
                                fromInd = enode_csucc->BB == branchSucc1 ? 2 : 1;
                        } else {
                            if (unconditional)
                                fromInd = 2;
                            else
                                fromInd = enode_csucc->BB == branchSucc1 ? 1 : 2;
                        }
                        str += ", from = \"out" + to_string(fromInd) + "\"";
                        str += ", to = \"in" + to_string(toInd) + "\"";
                        str += "];\n";
                    }
                }
            }
        }

        count++;
    }
    return str;
}
void printDotEdges(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, bool full) {

    if (full)
        dotfile << printMemEdges(enode_dag);
    dotfile << printDataflowEdges(enode_dag, bbnode_dag);
}
void printDotDFG(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string name,
                 bool full) {

    std::string output_filename = name;

    dotfile.open(output_filename);

    std::string infoStr;

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(std::ctime(&t));

    infoStr = "Digraph G {\n\tsplines=spline;\n";

    // infoStr += "//Graph created: " + date;
    infoStr += "//DHLS version: " + DHLS_VERSION;
    infoStr += "\" [shape = \"none\" pos = \"20,20!\"]\n";

    dotfile << infoStr;

    printDotNodes(enode_dag, full);

    printDotEdges(enode_dag, bbnode_dag, full);

    dotfile << "}";

    dotfile.close();
}
