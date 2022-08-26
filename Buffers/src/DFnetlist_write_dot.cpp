#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

static map<BlockType,string> blockShapes = {
    {ELASTIC_BUFFER," shape=box, style=filled, fillcolor=darkolivegreen3, height = 0.4"},
    {FORK, " shape=oval"},
    {BRANCH, " shape=oval"},
    {MERGE, " shape=oval"},
    {MUX, " shape=oval"},
    {CNTRL_MG, " shape=oval"},
    {SELECT, " shape=oval"},
    {OPERATOR, " shape=oval"},
    //{CONSTANT, "shape=circle, label=\"\", fixedsize=true, width=0.2, style=filled, fillcolor=white"},
    {CONSTANT, " shape=oval"},
    {FUNC_ENTRY, " shape=oval"},
    {FUNC_EXIT, " shape=oval"},
    {SINK, " shape=oval"},
    {SOURCE, " shape=oval"},
    {SELECTOR, "shape=oval" },
    {DISTRIBUTOR, "shape=oval" }
};

static map<PortType,string> channelDstAttr = {
    {SELECTION_PORT, ", arrowhead=obox"},
    {GENERIC_PORT, ", arrowhead=normal"},
    {TRUE_PORT, ", arrowhead=dot"},
    {FALSE_PORT, ", arrowhead=odot"}
};

static map<PortType,string> channelSrcAttr = {
    {TRUE_PORT, ", dir=both, arrowtail=dot"},
    {FALSE_PORT, ", dir=both, arrowtail=odot"}
};

bool DFnetlist_Impl::writeBasicBlockDot(const string& filename)
{
    ostringstream of;
    if (not writeBasicBlockDot(of)) return false;
    return FileUtil::write(of.str(), filename, getError());
}

bool DFnetlist_Impl::writeBasicBlockDot(std::ostream& of)
{
    if (BBG.empty()) calculateBasicBlocks();
    of << "Digraph ";
    const string& name = getName();
    if (name.empty()) of << "BasicBlocks";
    else of << name << "_BasicBlocks";
    of << " {" << endl;

    for (bbID bb = 0; bb < BBG.numBasicBlocks(); ++bb) {
        of << "  BB" << bb << " [label=\"BB #" << bb << "\";];" << endl;
    }

    of << endl;
    for (bbID bb_src = 0; bb_src < BBG.numBasicBlocks(); ++bb_src) {
        for (bbArcID arc: BBG.successors(bb_src)) {
            bbID bb_dst = BBG.getDstBB(arc);
            of << "  BB" << bb_src << " -> BB" << bb_dst;
            if (BBG.isBackArc(arc)) of << " [color=red;]";
            of << ';' << endl;
        }
    }

    of << "}" << endl;
    return true;
}

bool DFnetlist_Impl::writeDot(const string& filename)
{
    ostringstream of;
    if (not writeDot(of)) return false;
    return FileUtil::write(of.str(), filename, getError());
}

bool DFnetlist_Impl::writeDot(std::ostream& of)
{
    of << "// Number of blocks: " << numBlocks() << endl;
    of << "// Number of channels: " << numChannels() << endl;
    of << "Digraph ";
    const string& name = getName();
    if (name.empty()) of << "DataflowNetlist";
    else of << name;
    of << " {" << endl;

    of << endl << "  // Blocks" << endl;
    
    // Lana 03/07/19
    /*
    setBlocks b_inBBs;
    vector<vecBlocks> bbBlocks(BBG.numBasicBlocks());
    ForAllBlocks(b) {
        bbID bb = getBasicBlock(b);
        if (bb != invalidDataflowID) bbBlocks[bb].push_back(b);
    }

    for (bbID bb = 0; bb < BBG.numBasicBlocks(); ++bb) {
        of << "  subgraph cluster_" << bb << " {" << endl;
        of << "    label=\"BB #" << bb << "\";" << endl;
        of << "    style=filled;" << endl;
        of << "    color=lightgrey;" << endl;
        // Write and rank the blocks
        vecBlocks minrank, maxrank;
        for (blockID b: bbBlocks[bb]) {
            of << "  ";
            writeBlockDot(of, b);
            b_inBBs.insert(b);
            BlockType type = getBlockType(b);
            if (type == MERGE or type == FUNC_ENTRY) minrank.push_back(b);
            else if (type == BRANCH or type == FUNC_EXIT) maxrank.push_back(b);
        }

        if (not minrank.empty()) {
            of << "    {rank=min;";
            for (blockID b: minrank) of << ' ' << getBlockName(b);
            of << ";}" << endl;
        }

        if (not maxrank.empty()) {
            of << "    {rank=max;";
            for (blockID b: maxrank) of << ' ' << getBlockName(b);
            of << ";}" << endl;
        }
        of << "  }" << endl;
    }*/

// Print nodes in BB clusters
for (bbID i = 1; i <= BBG.numBasicBlocks(); i++) {
    of <<  "subgraph cluster_" + to_string(i) + " {\n";
    of << "color = \"darkgreen\"\n";
	of << "label = \"block"+ to_string(i) +"\"\n";
    	
    ForAllBlocks(b) {
    	bbID bb_src = getBasicBlock(b);
  
        if (bb_src == i) 
            writeBlockDot(of, b);  
    }

    of << "}\n";
}
// Print remaining nodes (bbID = 0)
    ForAllBlocks(b) {
        bbID bb_src = getBasicBlock(b);
        
        if (bb_src == 0) 
            writeBlockDot(of, b);
    }


    of << endl << "  // Channels" << endl;


    ForAllChannels(c) writeChannelDot(of, c);

    of << "}" << endl;
    return true;
}

bool DFnetlist_Impl::writeDotBB(const std::string &filename) {
    ostringstream of;
    if (not writeDotBB(of)) return false;
    return FileUtil::write(of.str(), filename, getError());
}

bool DFnetlist_Impl::writeDotBB(std::ostream& of) {
    of << "Digraph ";
    const string& name = getName();
    if (name.empty()) of << "DataflowNetlist";
    else of << name;
    of << " {" << endl;

    of << "\tsplines=spline;" << endl;
    for (bbID i = 1; i < BBG.numBasicBlocks(); i++) {
        of << "\t\t\"block" << to_string(i) << "\";" << endl;
    }

    for (bbArcID i = 0; i < BBG.numArcs(); i++) {
        bbID src = BBG.getSrcBB(i);
        bbID dst = BBG.getDstBB(i);
        string color = BBG.isBackArc(i) ? "\"red\"" : "\"blue\"";
        int freq = BBG.getFrequencyArc(i);
        int DSU = BBG.getDSUnumberArc(i);
        string MG_numbers = "\"";
        vector<int> tmp = BBG.getMGnumbers(i);
        for (int j = 0; j < tmp.size(); j++) {
            MG_numbers += to_string(tmp[j]);
            if (j != tmp.size() - 1) MG_numbers += ", ";
        }
        MG_numbers += "\"";

        of << "\t\t";
        of << "\"block" << to_string(src) << "\"";
        of << " -> ";
        of << "\"block" << to_string(dst) << "\"";
        of << " [";
        of << "color = " << color;
        of << ", freq = " << freq;
        of << ", DSU = " << DSU;
        of << ", MG = " << MG_numbers;
        of << "];" << endl;
    }

    of << "}";
    return true;
}

bool DFnetlist_Impl::writeDotMG(const std::string &filename) {
    ostringstream of;
    if (not writeDotMG(of)) return false;
    return FileUtil::write(of.str(), filename, getError());
}

bool DFnetlist_Impl::writeDotMG(std::ostream &s) {

    for (auto curr: MG) {
        for (auto b: curr.getBlocks()) {
            s << "block " << getBlockName(b) << endl;
        }

        for (auto c: curr.getChannels()) {
            s << "channel " << getBlockName(getSrcBlock(c)) << " -> " << getBlockName(getDstBlock(c)) << endl;
        }
        s << "minimum frequency is " << MGfreq[0] << endl;
        cout << "writing shit" << endl;
    }
}

void DFnetlist_Impl::writeBlockDot(ostream& s, blockID b)
{
    s << "  " << getBlockName(b) << " [type=" << BlockType2String[getBlockType(b)];

    // Delays
    ostringstream delay;

    bool only_block_delay = true;
    bool first_delay = true;
    if (getBlockDelay(b, -1) > 0) {
        delay << getBlockDelay(b, -1);
        first_delay = false;
    }

    int num_control = 0, num_noncontrol = 0;

    Block& B = blocks[b];
    // Ports
    string inp, outp;

    // Lana 24.03.19 Adding missing exit/entry port
    int tmpPortWidth = 0;

    // Axel: Sort the ports by name
    map<std::string, int> sorted_ports;
    for (auto p : B.allPorts) {
        std::string name = getPortName(p, false);
        int index = name.find_last_not_of("0123456789");
        int portIndex = stoi(name.substr(index + 1));
        string portDesc = name.substr(0, index + 1);
        string newPortIndex =
                portIndex < 10 ?
                        "0" + to_string(portIndex) : to_string(portIndex);
        string newPortName = portDesc + newPortIndex;
        sorted_ports.emplace(newPortName, p);
        //sorted_ports.
    }

    //for (auto& p: B.allPorts) {
    for (auto it = sorted_ports.begin(); it != sorted_ports.end(); it++) {
        auto &p = it->second;

        double d = getPortDelay(p);
        if (d > 0) {
            if (first_delay) first_delay = false;
            else delay << ' ';
            delay << getPortName(p, false) << ':' << d;
            only_block_delay = false;
        }

        if (isControlPort(p)) num_control++;
        else num_noncontrol++;
        if (getBlockType(b) != DEMUX) {
            string& str = isInputPort(p) ? inp : outp;
            str += getPortName(p, false);
            switch (getPortType(p)) {
            case SELECTION_PORT:
                str += "?";
                break;
            case TRUE_PORT:
                str += "+";
                break;
            case FALSE_PORT:
                str += "-";
                break;
            case GENERIC_PORT:
                break;
            }
            str += ":";
            str += to_string(getPortWidth(p));
            str += getMemPortSuffix(p);
            str += " ";

            // Lana 24.03.19 Adding missing exit/entry port
            tmpPortWidth = getPortWidth(p);
        }
    }

    // Special case for demux: start from scratch using the demux pairs.
    // The list of input and output ports are listed in order, with the
    // data port at the end of the input ports.
    if (getBlockType(b) == DEMUX) {
        for(auto& pair: B.demuxPairs) {
            if (not isControlPort(pair.first)) continue;
            inp += getPortName(pair.first, false) + ":0 ";
            outp += getPortName(pair.first, false) + ":0 ";
        }
        inp += getPortName(B.data, false) + ":" + to_string(getPortWidth(B.data)) + " ";
    }

    // Lana 25.03.19 Ignore constant here (input size is incorrect)
    if (not inp.empty() && B.type != CONSTANT) {
        inp.pop_back();
        s << ", in=\"" << inp << "\"";
    }

    // Lana 24.03.19 Adding missing exit/entry port and fizing constant input size
    if (getBlockType(b) == FUNC_ENTRY || getBlockType(b) == CONSTANT) {
        s << ", in=\"in1:" << to_string(tmpPortWidth) << "\"";
    }

    if (not outp.empty()) {
        outp.pop_back();
        s << ", out=\"" << outp << "\"";
    }

    // Lana 24.03.19 Adding missing exit/entry port
    if (getBlockType(b) == FUNC_EXIT) {
        s << ", out=\"out1:" << to_string(tmpPortWidth) << "\"";
    }

    // Lana 07.03.19. Printing out operation type of operators
    if (B.type == OPERATOR and not B.operation.empty()) {
        s << ", op = \"" << B.operation << "\"";
    }

    if (not delay.str().empty()) {
        s << ", delay=";
        if (not only_block_delay) s << "\"";
        s << delay.str();
        if (not only_block_delay) s << "\"";
    }

    if (B.type == OPERATOR and B.latency > 0) {
        s << ", latency=" << B.latency;
        if (B.II > 1) s << ", II=" << B.II;
    }


    if (B.type == OPERATOR and (B.operation == "getelementptr_op")) {
        s << ", constants=" << getGetPtrConst(b);
    }

    s << ", bbID = " << getBasicBlock(b);
    // Lana 02/07/19
    if (B.type == OPERATOR and (B.operation == "mc_load_op" 
        || B.operation == "mc_store_op" 
        || B.operation == "lsq_load_op"
        || B.operation == "lsq_store_op")) {
        
        //s << ", bbID = " << getBasicBlock(b);
        s << ", portID = " << getMemPortID(b);
        s << ", offset = " << getMemOffset(b);

    }

    if (B.type == OPERATOR and (B.operation == "call_op")) {
        
        s << ", function = \"" << getFuncName(b) << "\"";

    }

    if (B.type == BRANCH) {
        
        //s << ", bbID = " << getBasicBlock(b);

    }

    if (B.type == MC || B.type == LSQ) {

        s << ", memory = \"" << getMemName(b)<< "\"";
        s << ", bbcount = " << getMemBBCount(b);
        s << ", ldcount = " << getMemLdCount(b);
        s << ", stcount = " << getMemStCount(b);
     
    }

    if (B.type == LSQ) {

        s << ", fifoDepth = " << getLSQDepth(b);
        s << ", numLoads = \"" << getNumLoads(b) << "\"";
        s << ", numStores = \"" << getNumStores(b) << "\"";
        s << ", loadOffsets = \"" << getLoadOffsets(b) << "\"";
        s << ", storeOffsets = \"" << getStoreOffsets(b) << "\"";
        s << ", loadPorts = \"" << getLoadPorts(b) << "\"";
        s << ", storePorts = \"" << getStorePorts(b) << "\"";
     
    }

    if (B.type == ELASTIC_BUFFER) {
        s << ", slots=" << B.slots << ", transparent=" << (B.transparent ? "true" : "false");
        //s << ", fillcolor=" << (B.transparent ? "gray" : "black, fontcolor=white");
        s << ", label=\"" << getBlockName(b) << " [" << B.slots;
        if (B.transparent) s << 't';
        s << "]\"";
    }

    // Lana 25.03.19 Printing out constants in hex always
    if (B.type == CONSTANT) {
        string k;
        int width = getPortWidth(getOutPort(b));
        if (width == 1) k = (B.value == 0) ? "\"0x0\"" : "\"0x1\"";
        else if (B.value < 1000 and B.value > -1000) {
            std::stringstream ss;
            ss << hex << (B.value);
            k = "\"0x" + ss.str() + "\"";
        }
        else {
            long long mask = ((long long) 1 << width) - 1;
            std::stringstream ss;
            ss << hex << (B.value & mask);
            k = "\"0x" + ss.str() + "\"";
        }
        s << ", value=" << k << ", label=";
        if (width == 1) {
            s << (B.value == 0 ? "false" : "true");
        } else   s << k;
    }

    //Axel
    if (B.retimingDiff > 0) {
        s << ", retimingDiff=" << B.retimingDiff;
    }

    s << ", " << blockShapes[getBlockType(b)];

    // Special color for control blocks
   // if (B.type != ELASTIC_BUFFER and num_control > num_noncontrol) s << ", color=green, fillcolor=greenyellow";

    if (B.type == SELECTOR) {
        s << ", orderings=\"";
        for (auto mappings : B.orderings) {
            for (auto index_it = mappings.second.begin();
                    index_it != mappings.second.end(); ++index_it) {
                if (index_it != (mappings.second.end() - 1)) {
                    s << to_string(*index_it) + "|";
                } else {
                    s << to_string(*index_it) + " ";
                }
            }
        }
        s << "\"";
        s << ", color=pink, fillcolor=pink";
    }
    
    s << "];" << endl;
}

void DFnetlist_Impl::writeChannelDot(ostream& s, channelID id)
{
    portID src = getSrcPort(id);
    portID dst = getDstPort(id);

    s << "  " << getChannelName(id, false);

    s << " [from=" << getPortName(src, false) << ", to=" << getPortName(dst, false);
    s << channelSrcAttr[getPortType(src)] << channelDstAttr[getPortType(dst)];

    // Data, boolean, control


    blockID b_src = getSrcBlock(id);
    blockID b_dst = getDstBlock(id);

    bbID bb_src = getBasicBlock(b_src);
    bbID bb_dst = getBasicBlock(b_dst);

    string color;
    if (isBooleanPort(src)) color = "magenta";
    else if (isControlPort(src)) color = "gold3";
    else if (bb_src == bb_dst )
    	color = "red";
    else if (getBlockType(b_src) == BRANCH)
    	color = "blue";
    else 
        color = "darkgreen";
    s << ", color=" << color;

    // Is is a back-edge inside a basic block (branch->merge)? Do not constrain

    if (getBlockType(b_src) == BRANCH or getBlockType(b_dst) == MERGE or getChannelMerge(id)) {
        // If the same BB
        if (bb_src == bb_dst and bb_src >= 0) s << ", constraint=false";
    }

    // Information about buffers
    int slots = getChannelBufferSize(id);
    if (slots > 0) {
        bool transparent = isChannelTransparent(id);
        char cslot = transparent ? 'o' : '*';
        s << ", slots=" << slots << ", style=dotted, label=\"" << string(slots, cslot) << "\", transparent=";
        if (transparent) s << "true";
        else s << "false";
    }

    if (isBackEdge(id)) s << ", style=dashed, minlen = 3";

    s << "];" << endl;
}

bool DFlib_Impl::writeDot(const string& filename)
{
    ostringstream of;

    bool status = true;
    bool first = true;
    for (auto& df: funcs) {
        if (not first) of << endl;
        else first = false;
        status = df.writeDot(of);
        if (not status) break;
    }

    if (not status) return false;

    return FileUtil::write(of.str(), filename, getError());
}

