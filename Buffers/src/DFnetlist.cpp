#include <cassert>
#include "DFnetlist.h"

using namespace std;
using namespace Dataflow;

template<typename T1, typename T2>
static map<T2,T1> reverseMap(const map<T1,T2>& M12)
{
    map<T2,T1> M21;
    for (auto& it: M12) M21.emplace(it.second, it.first);
    return M21;
}

// Lana 26.03.19 Added sink, source, mux, cntrl mg
map<BlockType,string> DFnetlist_Impl::BlockType2String = {
    {ELASTIC_BUFFER,"Buffer"},
    {FORK, "Fork"},
    {BRANCH, "Branch"},
    {DEMUX, "Demux"},
    {MERGE, "Merge"},
    {SELECT, "Select"},
    {OPERATOR, "Operator"},
    {CONSTANT, "Constant"},
    {FUNC_ENTRY, "Entry"},
    {FUNC_EXIT, "Exit"},
    {SOURCE, "Source"},
    {SINK, "Sink"}, 
    {CNTRL_MG, "CntrlMerge"},
    {MUX, "Mux"}, 
    {LSQ, "LSQ"},
    {MC, "MC"},
	{SELECTOR, "Selector"},
	{DISTRIBUTOR, "Distributor"}
};

map<PortType,string> DFnetlist_Impl::PortType2String = {
    {GENERIC_PORT, "generic"},
    {SELECTION_PORT, "selection"},
    {TRUE_PORT, "true_sel"},
    {FALSE_PORT, "false_sel"}
};

map<string,BlockType> DFnetlist_Impl::String2BlockType = reverseMap<BlockType,string>(DFnetlist_Impl::BlockType2String);
map<string,PortType> DFnetlist_Impl::String2PortType = reverseMap<PortType,string>(DFnetlist_Impl::PortType2String);

void DFnetlist_Impl::init()
{
    nBlocks = nChannels = nPorts = 0;
    freeBlock = freePort = freeChannel = -1;
    nextFree = -1;
    entryControl = invalidDataflowID;
    exitControl.clear();
    setMilpSolver();
}

void DFnetlist_Impl::setMilpSolver(const std::string& solver)
{
    milpSolver = solver;
}

DFnetlist_Impl::DFnetlist_Impl()
{
    init();
}

DFnetlist_Impl::DFnetlist_Impl(const string& name)
{
    init();
    if (not readDataflowDot(name)) return;
    check();
}

DFnetlist_Impl::DFnetlist_Impl(FILE* file)
{
    init();
    if (not readDataflowDot(file)) return;
    check();
}

DFnetlist_Impl::DFnetlist_Impl(const std::string &name, const std::string &name_bb) {
    init();
    if (not readDataflowDot(name)) return;
    check();
    if (not readDataflowDotBB(name_bb)) return;
    //SHAB_note: do error checking
}

const string& DFnetlist_Impl::getName() const
{
    return net_name;
}

string DFnetlist_Impl::genBlockName(BlockType type) const
{
    string prefix = "_" + BlockType2String[type] + "_";
    int idx = 1;
    while (getBlock(prefix + to_string(idx)) != invalidDataflowID) ++idx;
    return prefix + to_string(idx);
}

blockID DFnetlist_Impl::createBlock(BlockType type, const string& name)
{
    string gname = name;

    if (getBlock(name) != invalidDataflowID) {
        setError("Duplicated block name: " + name + ".");
        return invalidDataflowID;
    }

    if (gname.empty()) gname = genBlockName(type);

    nBlocks++;
    // Check if there is some available slot
    blockID idx;
    if (freeBlock != invalidDataflowID) {
        idx = freeBlock;
        freeBlock = blocks[idx].nextFree;
    } else {
        idx = blocks.size();
        blocks.push_back(Block {});
    }
    name2block[gname] = idx;
    allBlocks.insert(idx);

    // Init the block
    Block& B = blocks[idx];
    B.id = idx;
    B.name = gname;
    B.type = type;
    B.value = 0;
    B.boolValue = false;
    B.nextFree = invalidDataflowID;
    B.basicBlock = invalidDataflowID;
    B.DFSorder = -1;
    B.delay = 0;
    B.latency = 0;
    B.II = 0;
    B.data = B.portCond = B.portFalse = B.portTrue = B.srcCond = invalidDataflowID;
    B.slots = 0;
    B.transparent = true;
    B.freq = 0.0;

    return idx;
}

void DFnetlist_Impl::removeBlock(blockID id)
{
    assert(validBlock(id));
    Block& B = blocks[id];

    while (not B.allPorts.empty()) removePort(*(B.allPorts.begin()));

    name2block.erase(B.name);

    B = Block {invalidDataflowID};
    nBlocks--;
    B.nextFree = freeBlock;
    freeBlock = id;
    allBlocks.erase(id);
}

blockID DFnetlist_Impl::getBlock(const string& name) const
{
    const auto& it = name2block.find(name);
    if (it == name2block.end()) return invalidDataflowID;
    return it->second;
}

const string& DFnetlist_Impl::getBlockName(blockID id) const
{
    assert (validBlock(id));
    return blocks[id].name;
}

BlockType DFnetlist_Impl::getBlockType(blockID id) const
{
    assert (validBlock(id));
    return blocks[id].type;
}

std::string DFnetlist_Impl::printBlockType(BlockType type) const {
    switch (type) {
        case ELASTIC_BUFFER:
            return "ELASTIC_BUFFER";
        case FORK:
            return "FORK";
        case BRANCH:
            return "BRANCH";
        case MERGE:
            return "MERGE";
        case DEMUX:
            return "DEMUX";
        case SELECT:
            return "SELECT";
        case CONSTANT:
            return "CONSTANT";
        case FUNC_ENTRY:
            return "FUNC_ENTRY";
        case FUNC_EXIT:
            return "FUNC_EXIT";
        case SOURCE:
            return "SOURCE";
        case SINK:
            return "SINK";
        case MUX:
            return "MUX";
        case CNTRL_MG:
            return "CNTRL_MG";
        case LSQ:
            return "LSQ";
        case MC:
            return "MC";
        case UNKNOWN:
            return "UNKNOWN";
        case OPERATOR:
            return "OPERATOR";
    }
}

bool DFnetlist_Impl::getChannelMerge(channelID c){ return channels[c].channelMerge;} //Carmine 09.03.22 these two functions are used to preserve 
void DFnetlist_Impl::setChannelMerge(channelID c){ channels[c].channelMerge = true;} //Carmine 09.03.22 the information for writing the dot after reduceMerges function

void DFnetlist_Impl::printChannelInfo(channelID c, int slots, int transparent) {
    cout << "Adding buffer in " << getChannelName(c);
    cout << " | ";
    cout << "slots: " << slots << ", trans: " << transparent;
    cout << " | ";
//    cout << printBlockType(getBlockType(getSrcBlock(c))) << "->";
//    cout << printBlockType(getBlockType(getDstBlock(c)));
//    cout << " | ";
    cout << "BB" << getBasicBlock(getSrcBlock(c)) << " -> ";
    cout << "BB" << getBasicBlock(getDstBlock(c));
    if (isInnerChannel(c)) cout << " (inner)" << endl;
    else cout << endl;
}

bool DFnetlist_Impl::isInnerChannel(channelID c) {
    BlockType src_type = getBlockType(getSrcBlock(c));
    BlockType dst_type = getBlockType(getDstBlock(c));
    if (src_type == BRANCH && (dst_type == MUX || dst_type == MERGE || dst_type == CNTRL_MG)) return false;

    bbID src_bb = getBasicBlock(getSrcBlock(c));
    bbID dst_bb = getBasicBlock(getDstBlock(c));
    assert((src_bb == dst_bb) || (src_bb == 0) || (dst_bb == 0));

    if (src_bb == 0 || dst_bb == 0) return false;
    return true;
}

void DFnetlist_Impl::setBasicBlock(blockID id, bbID bb)
{
    assert (validBlock(id));
    blocks[id].basicBlock = bb;
}

bbID DFnetlist_Impl::getBasicBlock(blockID id) const
{
    assert (validBlock(id));
    return blocks[id].basicBlock;
}

double DFnetlist_Impl::getBlockDelay(blockID id, int indx) const
{
    assert(validBlock(id));
    
    if ( indx < 0 ) 
        return blocks[id].delay;
    else
        return blocks[id].delays[indx];
}


void DFnetlist_Impl::setBlockDelay(blockID id, double d, int indx)
{
    assert(validBlock(id));
    if ( indx == 0 )
        blocks[id].delay = d;
    blocks[id].delays[indx] = d;
}

double DFnetlist_Impl::getBlockRetimingDiff(blockID id) const
{
	assert (validBlock(id));
	return blocks[id].retimingDiff;
}

void DFnetlist_Impl::setBlockRetimingDiff(blockID id, double diff)
{
	assert (validBlock(id));
	blocks[id].retimingDiff = diff;
}

int DFnetlist_Impl::getLatency(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].latency;
}

void DFnetlist_Impl::setLatency(blockID id, int lat)
{
    assert(validBlock(id));
    blocks[id].latency = lat;
}

int DFnetlist_Impl::getInitiationInterval(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].II;
}

void DFnetlist_Impl::setInitiationInterval(blockID id, int ii)
{
    assert(validBlock(id));
    blocks[id].II = ii;
}

int DFnetlist_Impl::getExecutionFrequency(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].freq;
}

void DFnetlist_Impl::setExecutionFrequency(blockID id, double freq)
{
    assert(validBlock(id));
    blocks[id].freq = freq;
    //cout << getBlockName(id) << " " << freq << endl;
}


void DFnetlist_Impl::setTrueFrac(blockID id, double freq)
{
    assert(validBlock(id));
    blocks[id].frac = freq;
}

double DFnetlist_Impl::getTrueFrac(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].frac;
}

void DFnetlist_Impl::setValue(blockID id, long long value)
{
    assert(validBlock(id));
    blocks[id].value = value;
    blocks[id].boolValue = value != 0;
}

//Lana 07.03.19 
void DFnetlist_Impl::setOperation(blockID id, std::string op)
{
    assert(validBlock(id));
    blocks[id].operation = op;
}

// Lana 02/07/19
const string& DFnetlist_Impl::getOperation(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].operation;
}

void DFnetlist_Impl::setFuncName(blockID id, std::string func)
{
    assert(validBlock(id));
    blocks[id].funcName = func;
}

// Lana 02/07/19
const string& DFnetlist_Impl::getFuncName(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].funcName;
}

// Lana 03.07.19 
void DFnetlist_Impl::setMemPortID(blockID id, int memPortID)
{
    assert(validBlock(id));
    blocks[id].memPortID = memPortID;

}

int DFnetlist_Impl::getMemPortID(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memPortID;
}

void DFnetlist_Impl::setMemOffset(blockID id, int memOffset)
{
    assert(validBlock(id));
    blocks[id].memOffset = memOffset;

}

int DFnetlist_Impl::getMemOffset(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memOffset;
}

void DFnetlist_Impl::setMemBBCount(blockID id, int count)
{
    assert(validBlock(id));
    blocks[id].memBBCount = count;

}

int DFnetlist_Impl::getMemBBCount(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memBBCount;
}

void DFnetlist_Impl::setMemLdCount(blockID id, int count)
{
    assert(validBlock(id));
    blocks[id].memLdCount = count;

}

int DFnetlist_Impl::getMemLdCount(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memLdCount;
}

void DFnetlist_Impl::setMemStCount(blockID id, int count)
{
    assert(validBlock(id));
    blocks[id].memStCount = count;

}

int DFnetlist_Impl::getMemStCount(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memStCount;
}

void DFnetlist_Impl::setMemName(blockID id, std::string name)
{
    assert(validBlock(id));
    blocks[id].memName = name;
}

const string& DFnetlist_Impl::getMemName(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].memName;
}

void DFnetlist_Impl::setMemPortSuffix(portID port, std::string op)
{
    assert(validPort(port));
    ports[port].memPortSuffix = op;

}

const string& DFnetlist_Impl::getMemPortSuffix(portID port) const
{
    assert(validPort(port));
    return ports[port].memPortSuffix;
}

// Lana 04/10/19 LSQ params
void DFnetlist_Impl::setLSQDepth(blockID id, int depth)
{
    assert(validBlock(id));
    blocks[id].fifoDepth = depth;

}

int DFnetlist_Impl::getLSQDepth(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].fifoDepth;
}

void DFnetlist_Impl::setNumLoads(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].numLoads = s;

}

const string& DFnetlist_Impl::getNumLoads(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].numLoads;
}

void DFnetlist_Impl::setNumStores(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].numStores = s;

}

const string& DFnetlist_Impl::getNumStores(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].numStores;
}

void DFnetlist_Impl::setLoadOffsets(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].loadOffsets = s;

}

const string& DFnetlist_Impl::getLoadOffsets(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].loadOffsets;
}

void DFnetlist_Impl::setStoreOffsets(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].storeOffsets = s;

}

const string& DFnetlist_Impl::getStoreOffsets(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].storeOffsets;
}

void DFnetlist_Impl::setLoadPorts(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].loadPorts = s;

}

const string& DFnetlist_Impl::getLoadPorts(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].loadPorts;
}

void DFnetlist_Impl::setStorePorts(blockID id, std::string s)
{
    assert(validPort(id));
    blocks[id].storePorts = s;

}

const string& DFnetlist_Impl::getStorePorts(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].storePorts;
}

// Getelementptr array dimensions
void DFnetlist_Impl::setGetPtrConst(blockID id, int c)
{
    assert(validBlock(id));
    blocks[id].getptrc = c;

}

void DFnetlist_Impl::setOrderings(blockID id, map<bbID, vector<int>> value){
    assert (validBlock(id));
    blocks[id].orderings = value;
}

//---------------------------

long long DFnetlist_Impl::getValue(blockID id)
{
    assert(validBlock(id));
    return blocks[id].value;
}

bool DFnetlist_Impl::getBoolValue(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].boolValue;
}

int DFnetlist_Impl::getBufferSize(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].slots;
}

void DFnetlist_Impl::setBufferSize(blockID id, int slots)
{
    assert(validBlock(id));
    blocks[id].slots = slots;
}

bool DFnetlist_Impl::isBufferTransparent(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].transparent;
}

void DFnetlist_Impl::setBufferTransparency(blockID id, bool value)
{
    assert(validBlock(id));
    blocks[id].transparent = value;
}

blockID DFnetlist_Impl::getBlockFromPort(portID p) const
{
    assert(validPort(p));
    return ports[p].block;
}

// Getelementptr array dimensions
int DFnetlist_Impl::getGetPtrConst(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].getptrc;
}

// Generates a fresh port name for a given block name. isInput indicates whether the port
// must be input or output (prefix "in" or "out"). name2port contains all full names
// for the ports (to check that not an existing name is generated).
static string genFreshPortName(const string& blockName, bool isInput, map<string, portID>& name2port)
{
    string prefix = isInput ? "in" : "out";
    for (int suffix = 1; ; ++suffix) {
        string name = prefix + to_string(suffix);
        if (name2port.count(blockName + ":" + name) == 0) return name;
    }
}

portID DFnetlist_Impl::createPort(blockID block, bool isInput, const string& name, int width, PortType type)
{
    assert(validBlock(block));

    string localname = name.empty() ? genFreshPortName(getBlockName(block), isInput, name2port) : name;
    string fullname = getBlockName(block) + ":" + localname;

    // Check for duplication
    if (name2port.count(fullname) > 0) {
        setError("Duplicated port name (" + fullname + ").");
        return invalidDataflowID;
    }

    // Add a new port to the list of ports
    int pid;
    if (freePort == invalidDataflowID) {
        pid = ports.size();
        ports.push_back(Port {});
    } else {
        pid = freePort;
        freePort = ports[pid].nextFree;
    }

    nPorts++;
    name2port.emplace(fullname, pid);

    Block& B = blocks[block];
    B.allPorts.insert(pid);
    if (isInput) B.inPorts.insert(pid);
    else B.outPorts.insert(pid);

    // Special treatment for demux: the input and output ports must be paired.
    // The input ports are declared with the following order: s1,...,sn, data.
    // The output ports are declared as: d1,...,dn. The ports must be paired
    // as (c1,d1),...,(cn,dn).
    // The input ports are stored in listPorts. Afterwards, the output ports
    // are paired in order of declaration. The remaining input port is the
    // data port
    if (B.type == DEMUX) {
        if (isInput) B.listPorts.push_back(pid);
        else {
            assert(not B.listPorts.empty());
            portID inp = B.listPorts.front();
            B.listPorts.pop_front();
            B.demuxPairs.emplace(pid, inp);
            B.demuxPairs.emplace(inp, pid);

            // The last input port is data
            if (B.listPorts.size() == 1) {
                B.data = B.listPorts.front();
                B.listPorts.pop_front();
            }
        }
    }

    // Init the port
    Port& P = ports[pid];
    P.id = pid;
    P.block = block;
    P.short_name = localname;
    P.full_name = fullname;
    P.isInput = isInput;
    P.width = width;
    P.delay = 0;
    P.type = type;
    P.channel = invalidDataflowID;
    allPorts.insert(pid);

    return pid;
}

void DFnetlist_Impl::removePort(portID p)
{
    assert(validPort(p));
    Port& P = ports[p];
    Block& B = blocks[P.block];

    B.allPorts.erase(p);
    if (P.isInput) B.inPorts.erase(p);
    else B.outPorts.erase(p);

    if (validChannel(P.channel)) removeChannel(P.channel);
    name2port.erase(getPortName(p));
    P = Port {};
    P.nextFree = freePort;
    freePort = p;
    allPorts.erase(p);
}

portID DFnetlist_Impl::getPort(blockID block, const string& name) const
{
    assert(validBlock(block));
    string fullname = getBlockName(block) + ":" + name;
    const auto& it = name2port.find(fullname);
    if (it == name2port.end()) return invalidDataflowID;
    return it->second;
}

const string& DFnetlist_Impl::getPortName(portID port, bool full) const
{
    assert(validPort(port));
    return full ? ports[port].full_name : ports[port].short_name;
}

PortType DFnetlist_Impl::getPortType(portID port) const
{
    assert(validPort(port));
    return ports[port].type;
}

bool DFnetlist_Impl::isInputPort(portID port) const
{
    assert(validPort(port));
    return ports[port].isInput;
}

bool DFnetlist_Impl::isOutputPort(portID port) const
{
    assert(validPort(port));
    return not ports[port].isInput;
}

void DFnetlist_Impl::setPortWidth(portID port, int width)
{
    assert(validPort(port));
    ports[port].width = width;

}

int DFnetlist_Impl::getPortWidth(portID port) const
{
    assert(validPort(port));
    return ports[port].width;
}

bool DFnetlist_Impl::isBooleanPort(portID port) const
{
    assert(validPort(port));
    return ports[port].width == 1;
}

bool DFnetlist_Impl::isControlPort(portID port) const
{
    assert(validPort(port));
    return ports[port].width == 0;
}

double DFnetlist_Impl::getPortDelay(portID port) const
{
    assert(validPort(port));
    return ports[port].delay;
}

void DFnetlist_Impl::setPortDelay(portID port, double d)
{
    assert(validPort(port));
    ports[port].delay = d;
}

double DFnetlist_Impl::getCombinationalDelay(portID inp, portID outp) const
{
    assert(validPort(inp) and validPort(outp));
    assert(isInputPort(inp) and isOutputPort(outp));
    blockID b = getBlockFromPort(inp);
    assert (b == getBlockFromPort(outp));
    assert (getLatency(b) == 0);
    return getPortDelay(inp) + getPortDelay(outp) + getBlockDelay(b, -1);
}

const setPorts& DFnetlist_Impl::getPorts(blockID id, PortDirection dir) const
{
    assert(validBlock(id));
    if (dir == INPUT_PORTS) return blocks[id].inPorts;
    if (dir == OUTPUT_PORTS) return blocks[id].outPorts;
    return blocks[id].allPorts;
}

const setPorts& DFnetlist_Impl::getDefinitions(portID p) const
{
    assert(validPort(p));
    return ports[p].defs;
}

portID DFnetlist_Impl::getConditionalPort(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].portCond;
}

portID DFnetlist_Impl::getTruePort(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].portTrue;
}

portID DFnetlist_Impl::getFalsePort(blockID id) const
{
    assert(validBlock(id));
    return blocks[id].portFalse;
}

portID DFnetlist_Impl::getDataPort(blockID id) const
{
    assert(validBlock(id));
    auto t = getBlockType(id);
    assert(t == BRANCH or t == DEMUX);
    return blocks[id].data;
}

portID DFnetlist_Impl::getDemuxComplementaryPort(portID port) const
{
    assert(validPort(port));
    const Block& B = blocks[ports[port].block];
    const auto& it = B.demuxPairs.find(port);
    if (it != B.demuxPairs.end()) return it->second;
    return invalidDataflowID;
}

channelID DFnetlist_Impl::getConnectedChannel(portID port) const
{
    assert(validPort(port));
    return ports[port].channel;
}

bool DFnetlist_Impl::isPortConnected(portID port) const
{
    return getConnectedChannel(port) != invalidDataflowID;
}

portID DFnetlist_Impl::getConnectedPort(portID port) const
{
    assert(validPort(port));
    if (isInputPort(port)) return getSrcPort(getConnectedChannel(port));
    return getDstPort(getConnectedChannel(port));
}

channelID DFnetlist_Impl::createChannel(portID src, portID dst, int slots, bool transparent)
{
    assert(validPort(src) and validPort(dst));
    assert(isOutputPort(src) and isInputPort(dst));
    nChannels++;
    channelID id;
    if (freeChannel != invalidDataflowID) {
        id = freeChannel;
        freeChannel = channels[id].nextFree;
    } else {
        id = channels.size();
        channels.push_back(Channel {});
    }

    Channel& C = channels[id];
    C.id = id;
    C.src = src;
    C.dst = dst;
    C.slots = slots;
    C.transparent = transparent;
    C.backEdge = false;
    C.nextFree = invalidDataflowID;
    allChannels.insert(id);

    ports[src].channel = ports[dst].channel = id;
    return id;
}

void DFnetlist_Impl::removeChannel(channelID id)
{
    assert(validChannel(id));
    Channel& C = channels[id];
    C.id = invalidDataflowID;
    C.nextFree = freeChannel;
    freeChannel = id;
    ports[C.src].channel = ports[C.dst].channel = invalidDataflowID;
    allChannels.erase(id);
}

portID DFnetlist_Impl::getSrcPort(channelID id) const
{
    assert(validChannel(id));
    return channels[id].src;
}

portID DFnetlist_Impl::getDstPort(channelID id) const
{
    assert(validChannel(id));
    return channels[id].dst;
}

blockID DFnetlist_Impl::getSrcBlock(channelID id) const
{
    return getBlockFromPort(getSrcPort(id));
}

blockID DFnetlist_Impl::getDstBlock(channelID id) const
{
    return getBlockFromPort(getDstPort(id));
}

channelID DFnetlist_Impl::getChannelID(portID id){ //Carmine 09.03.22 added a new function to retrieve channel ID from port ID
    assert(validPort(id));
    return ports[id].channel;
}

string DFnetlist_Impl::getChannelName(channelID id, bool full) const
{
    assert(validChannel(id));
    portID src = getSrcPort(id);
    portID dst = getDstPort(id);
    if (full) return getPortName(src, true) + " -> " + getPortName(dst, true);
    blockID bsrc = getBlockFromPort(src);
    blockID bdst = getBlockFromPort(dst);
    return getBlockName(bsrc) + " -> " + getBlockName(bdst);
}

int DFnetlist_Impl::getChannelBufferSize(channelID id) const
{
    assert(validChannel(id));
    return channels[id].slots;
}

void DFnetlist_Impl::setChannelBufferSize(blockID id, int slots)
{
    assert(validChannel(id));
    channels[id].slots = slots;
}

bool DFnetlist_Impl::isChannelTransparent(channelID id) const
{
    assert(validChannel(id));
    return channels[id].transparent;
}

void DFnetlist_Impl::setChannelTransparency(channelID id, bool value)
{
    assert(validChannel(id));
    channels[id].transparent = value;
}

void DFnetlist_Impl::setChannelEB(channelID id) //Carmine 21.02.22 in case both transparent and non-trans are present on the same channel
{
    assert(validChannel(id));
    channels[id].EB = true;
}

bool DFnetlist_Impl::getChannelEB(channelID id) //Carmine 21.02.22 in case both transparent and non-trans are present on the same channel
{
    assert(validChannel(id));
    return channels[id].EB;
}

void DFnetlist_Impl::setChannelFrequency(channelID id, double value) {
    assert(validChannel(id));
    channels[id].freq = value;
    //cout << getChannelName(id) << " " << value << endl;
}

double DFnetlist_Impl::getChannelFrequency(channelID id) {
    assert(validChannel(id));
    return channels[id].freq;
}

bool DFnetlist_Impl::hasBuffer(channelID id) const
{
    assert(validChannel(id));
    return channels[id].slots > 0;
}

void DFnetlist_Impl::setError(const string& err)
{
    error.set(err);
}

const string& DFnetlist_Impl::getError() const
{
    return error.get();
}

string& DFnetlist_Impl::getError()
{
    return error.get();
}

void DFnetlist_Impl::clearError()
{
    error.clear();
}

bool DFnetlist_Impl::hasError() const
{
    return error.exists();
}

int DFnetlist_Impl::numBlocks() const
{
    return nBlocks;
}

int DFnetlist_Impl::numChannels() const
{
    return nChannels;
}

int DFnetlist_Impl::numPorts() const
{
    return nPorts;
}

bool DFnetlist_Impl::validPort(portID p) const
{
    return p >= 0 and p < ports.size() and ports[p].id == p;
}

bool DFnetlist_Impl::validBlock(blockID id) const
{
    return id >= 0 and id < blocks.size() and blocks[id].id == id;
}

bool DFnetlist_Impl::validChannel(channelID id) const
{
    return id >= 0 and id < channels.size() and channels[id].id == id;
}


bool DFnetlist_Impl::channelIsInMGs(channelID c) {
    assert(validChannel(c));
    return channels_in_MGs.count(c) > 0;
}

bool DFnetlist_Impl::blockIsInMGs(blockID b) {
    assert(validBlock(b));
    return blocks_in_MGs.count(b) > 0;
}

bool DFnetlist_Impl::channelIsInBorders(channelID c) {
    assert(validChannel(c));
    return channels_in_borders.count(c) > 0;
}

bool DFnetlist_Impl::blockIsInBorders(blockID b) {
    assert(validBlock(b));
    return blocks_in_borders.count(b) > 0;
}

bool DFnetlist_Impl::channelIsInMCLSQ(channelID c) {
    assert(validChannel(c));
    return channels_in_MC_LSQ.count(c) > 0;
}

bool DFnetlist_Impl::blockIsInMCLSQ(blockID b) {
    assert(validBlock(b));
    return blocks_in_MC_LSQ.count(b) > 0;
}

bool DFnetlist_Impl::channelIsCovered(channelID c, bool MG, bool borders, bool MC_LSQ) {
    bool state_MG = MG ? channelIsInMGs(c) : false;
    bool state_borders = borders ? channelIsInBorders(c) : false;
    bool state_MC_LSQ = MC_LSQ ? channelIsInMCLSQ(c) : false;
    return state_MG || state_borders || state_MC_LSQ;
}

bool DFnetlist_Impl::blockIsCovered(blockID b, bool MG, bool borders, bool MC_LSQ) {
    bool state_MG = MG ? blockIsInMGs(b) : false;
    bool state_borders = borders ? blockIsInBorders(b) : false;
    bool state_MC_LSQ = MC_LSQ ? blockIsInMCLSQ(b) : false;
    return state_MG || state_borders || state_MC_LSQ;
}

void DFnetlist_Impl::makeMGsfromCFDFCs() {
    MG_disjoint.clear();

    cout << "Adding Marked Graphs from CFDFCs..." << endl;
    for (int i = 0; i < CFDFC_disjoint.size(); i++) {
        MG_disjoint.push_back(getMGfromCFDFC(CFDFC_disjoint[i]));
    }
    cout << endl;
}
