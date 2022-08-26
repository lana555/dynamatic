#include <cassert>
#include <iterator>
#include <sstream>
#include <graphviz/cgraph.h>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

static double delays [250][8];

static string allowedChars = "_.";

/**
 * @brief Indicates whether the string is a good identifier.
 * @param DF The dataflow netlist.
 * @param name The identifier.
 * @return True if it is valid and false otherwise.
 * @note If not valid, it stores an error message in the netlist.
 */
static bool goodIdentifier(DFnetlist_Impl& DF, const string& name)
{
    assert(not name.empty());

    if (name[0] != '_' and not isalpha(name[0])) {
        DF.setError("Invalid identifier: " + name + ".");
        return false;
    }

    for (char c: name) {
        if (isalnum(c) or allowedChars.find(c) != string::npos) continue;
        DF.setError("Invalid identifier: " + name + ".");
        return false;
    }
    return true;
}

/**
 * @brief Returns a positive integer extracted from s.
 * If the conversion is wrong, it returns a negative number.
 * @param s The string.
 * @return The extracted value (-1 if an error occurs).
 */
static int getPositiveInteger(const string& s)
{
    try {
        size_t p;
        long value = stol(s, &p);
        if (p != s.size()) return -1;
        return value;
    } catch(...) {
        return -1;
    }
}

/**
 * @brief Returns a positive double extracted from s.
 * If the conversion is wrong, it returns a negative number.
 * @param s The string.
 * @return The extracted value (-1 if an error occurs).
 */
static double getPositiveDouble(const string& s)
{
    try {
        size_t p;
        double value = stod(s, &p);
        if (p != s.size()) return -1.0;
        return value;
    } catch(...) {
        return -1.0;
    }
}

/**
 * @brief Extracts a suffix with an integer value. The id is supposed to
 * have the form id:value. In case the suffix does not exist, the default value
 * is returned. The original string is modified with the suffix removed.
 * @param id The identifier with the suffix.
 * @param value The returned value (initialized with the default value).
 * @return True if successful, and false otherwise.
 */
static bool extractIntegerSuffix(string& id, int& value)
{
    value = -1;
    size_t p = id.find_last_of(':');
    // Lana 30.06.19 When MC or LSQ, remove string part with extra port info
    size_t t = id.find_last_of('*');
    if (p == string::npos) return true;
    string s;
    if (t == string::npos)
    	s = id.substr(p+1, string::npos);
    else 
    	s = id.substr(p+1, t-p-1);
    int v = getPositiveInteger(s);
    if (v < 0) return false;
    else id = id.substr(0, p);
    value = v;
    return true;
}

/**
 * @brief Extracts a suffix with a double value. The id is supposed to
 * have the form id:value. In case the suffix does not exist, the default value
 * is returned. The original string is modified with the suffix removed.
 * @param id The identifier with the suffix.
 * @param value The returned value (initialized with the default value).
 * @return True if successful, and false otherwise.
 */
static bool extractDoubleSuffix(string& id, double& value)
{
    size_t p = id.find_last_of(':');
    if (p == string::npos) return true;
    string s = id.substr(p+1, string::npos);
    double v = getPositiveDouble(s);
    if (v < 0) return false;
    else id = id.substr(0, p);
    value = v;
    return true;
}

/**
 * @brief Treats a port string.
 * @param N The netlist.
 * @param name Name of the ports (including the suffixes). After the call to
 * the function, the suffixes have been removed.
 * @param width Used to return the width of the port. It is initialized with
 * the default width of the channels.
 * @param type Used to return the type of the port.
 * @return True if the specification is correct, and false otherwise.
 * @note The acceptable suffixes are:
 *   "?": selection port (for branch and select)
 *   "+" and "-": True and false branches.
 * If no suffix is specified, the port is generic.
 */
static bool readPortDeclaration(DFnetlist_Impl& DF, string& name, int &width, PortType& type)
{
    if (not extractIntegerSuffix(name, width)) {
        DF.setError("Incorrect port declaration: " + name + ".");
        return false;
    }

    char suffix = name.back();

    if (suffix == '?') {
        type = SELECTION_PORT;
        name.pop_back();
    } else if (suffix == '+') {
        type = TRUE_PORT;
        name.pop_back();
    } else if (suffix == '-') {
        type = FALSE_PORT;
        name.pop_back();
    } else {
        type = GENERIC_PORT;
    }

    return goodIdentifier(DF, name);
}

// Lana 04/07/19 Extract memory suffix (from * till end)
// For MC and LSQ nodes
string extractMemorySuffix(string& port)
{
    size_t t = port.find_last_of('*');

    if (t != string::npos) {
        return port.substr(t, string::npos);
    }
    return "";
}

// Reads the ports of a block
static bool readPorts(DFnetlist_Impl& DF, blockID id, Agnode_t* v, bool input)
{
    char* in_out = (char *) string(input ? "in" : "out").c_str();
    const char* attr = agget(v, in_out);

    if (DF.getBlockType(id) == FUNC_EXIT & input == false)
        return true;

    if ((attr = agget(v, in_out)) != nullptr) {
        istringstream iss(attr);
        vector<string> tokens {istream_iterator<string>{iss},
                               istream_iterator<string>{}
                              };
        for (auto& port: tokens) {
            PortType type;
            int width;

            // Lana 04/07/19 Extract memory suffix
            string s = extractMemorySuffix(port);
            
            if (not readPortDeclaration(DF, port, width, type)) return false;
            portID p = DF.createPort(id, input, port, width, type);
            if (p == invalidDataflowID) return false;

            // Lana 04/07/19 Extract memory suffix
            if (s != "") DF.setMemPortSuffix(p, s);

        }
    }

    return true;
}

// Reads the delays of a block
static bool readDelays(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    const char* attr = agget(v, (char *) "delay");
    if (attr == nullptr) return true;

    string block_name = DF.getBlockName(id);
    istringstream iss(attr);
    vector<string> tokens {istream_iterator<string>{iss},
                           istream_iterator<string>{}
                          };
	int i;

	i = 0;
    for (auto& port: tokens) {
        // Check whether it is the delay of the block (no port specification)
        if (isdigit(port[0])) {
		//printf("Andrea Get value of block %s\n\r", block_name);
            double v = getPositiveDouble(port);
		//cout << "Andrea Get value of block " << block_name << " v " << v << " port " << port << endl;
            //if (v >= 0) {
                DF.setBlockDelay(id, v, i);
		delays [id][i] = v; 
		//cout << block_name << " id " << id << " i " << i << " delays [id][i] " << delays [id][i] << endl;  
		i++;
                continue;
            //} else {
            //    DF.setError("Wrong delay specification for block " + block_name + ".");
            //    return false;
            //}
        }

        double delay = -1;
        if (not extractDoubleSuffix(port, delay) or delay < 0) {
            DF.setError("Wrong delay specification for block " + block_name + ".");
            return false;
        }

        portID p = DF.getPort(id, port);
        if (not DF.validPort(p)) {
            DF.setError ("Block " + block_name + ": unknown port " + port + ".");
            return false;
        }
        DF.setPortDelay(p, delay);
    }

    return true;
}

// Reads the latency and initiation interval of a block
static bool readLatencyII(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "latency");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != OPERATOR) {
            DF.setError("Block " + block_name + ": latency can only be defined for operators.");
            return false;
        }
        int lat = getPositiveInteger(attr);
        if (lat < 0) {
            DF.setError("Block " + block_name + ": wrong value for latency.");
            return false;
        }
        DF.setLatency(id, lat);
    }

    attr = agget(v, (char *) "II");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != OPERATOR) {
            DF.setError("Block " + block_name + ": II can only be defined for operators.");
            return false;
        }
        int ii = getPositiveInteger(attr);
        if (ii <= 0) {
            DF.setError("Block " + block_name + ": wrong value for II.");
            return false;
        }
        DF.setInitiationInterval(id, ii);
    }

    return true;
}

// Reads the execution frequency of a block
static bool readExecFrequency(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "freq");
    if (attr != nullptr and strlen(attr) > 0) {
        double freq = getPositiveDouble(attr);
        if (freq < 0) {
            DF.setError("Block " + block_name + ": wrong value for execution frequency.");
            return false;
        }
        DF.setExecutionFrequency(id, freq);
    }

    return true;
}


static bool readTrueFrac(DFnetlist_Impl& DF, blockID id, Agnode_t* v) {
    const char* attr = agget(v, (char *) "trueFrac");
    if (attr != nullptr and strlen(attr) > 0) {
        //if (DF.getOperation(id) != "select_op")
            //DF.setError("True/false execution fraction can be specified only for select op.");
        double frac = getPositiveDouble(attr);
        if (frac < 0) {
            DF.setError("Select " + to_string(id) + ": wrong value for execution fraction.");
        }
        DF.setTrueFrac(id, frac);
    }
    return true;
}

// Lana: reads ops of operators
static bool readOperation(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "op");

    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != OPERATOR) {
            DF.setError("Block " + block_name + ": operation can only be defined for operators.");
            return false;
        }

        DF.setOperation(id, std::string(attr));
    }
  
    return true;
}

// Lana: reads function name of function call
static bool readFuncName(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "function");

    if (DF.getBlockType(id) == OPERATOR && DF.getOperation(id) == "call_op")
        if (attr != nullptr and strlen(attr) > 0) {
            DF.setFuncName(id, std::string(attr));
        }
        else {
            DF.setError("Call " + block_name + ": function name is not set.");
            return false;
        }
  
    return true;
}

// Lana: reads basic block id
// SHAB_note: changed this so all blocks can have bbID tag.
static bool readBasicBlock(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "bbID");

//    if (DF.getBlockType(id) == BRANCH
//        || (DF.getBlockType(id) == OPERATOR && DF.getOperation(id) == "lsq_load_op")
//        || (DF.getBlockType(id) == OPERATOR && DF.getOperation(id) == "lsq_store_op")
//        || (DF.getBlockType(id) == OPERATOR && DF.getOperation(id) == "mc_load_op")
//        || (DF.getBlockType(id) == OPERATOR && DF.getOperation(id) == "mc_store_op")) {

        if (attr != nullptr and strlen(attr) > 0) {
            int bbID = getPositiveInteger(attr);
            
            DF.setBasicBlock(id, bbID);
        }
        else {
            DF.setError("Branch " + block_name + ": bbID is not set.");
            return false;
        }
//    }

  
    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemPortID(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "portId");
    if (attr != nullptr and strlen(attr) > 0) {
        int t = getPositiveInteger(attr);
        if (t < 0) {
            DF.setError("Block " + block_name + ": wrong value for port id.");
            return false;
        }
        DF.setMemPortID(id, t);
    }

    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemOffset(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "offset");
    if (attr != nullptr and strlen(attr) > 0) {
        int t = getPositiveInteger(attr);
        if (t < 0) {
            DF.setError("Block " + block_name + ": wrong value for offset.");
            return false;
        }
        DF.setMemOffset(id, t);
    }

    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemBBCount(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "bbcount");
    if (attr != nullptr and strlen(attr) > 0) {
        int t = getPositiveInteger(attr);
        if (t < 0) {
            DF.setError("Block " + block_name + ": wrong value for bb count.");
            return false;
        }
        DF.setMemBBCount(id, t);
    }

    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemLdCount(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "ldcount");
    if (attr != nullptr and strlen(attr) > 0) {
        int t = getPositiveInteger(attr);
        if (t < 0) {
            DF.setError("Block " + block_name + ": wrong value for load count.");
            return false;
        }
        DF.setMemLdCount(id, t);
    }

    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemStCount(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "stcount");
    if (attr != nullptr and strlen(attr) > 0) {
        int t = getPositiveInteger(attr);
        if (t < 0) {
            DF.setError("Block " + block_name + ": wrong value for store count.");
            return false;
        }
        DF.setMemStCount(id, t);
    }

    return true;
}

// Lana 03/07/19 read memory port parameters to connect to MC/LSQ
static bool readMemName(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "memory");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != MC && DF.getBlockType(id) != LSQ) {
            DF.setError("Block " + block_name + ": memory name can only be defined for MC/LSQ.");
            return false;
        }

        DF.setMemName(id, std::string(attr));
    }

    return true;
}

// Lana 04/10/19 read LSQ params for json
static bool readLSQParams(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);

    if (DF.getBlockType(id) == LSQ) {

        const char* attr = agget(v, (char *) "fifoDepth");
        if (attr != nullptr and strlen(attr) > 0) {
            int t = getPositiveInteger(attr);
            if (t < 0) {
                DF.setError("Block " + block_name + ": wrong value for LSQ depth.");
                return false;
            }

             DF.setLSQDepth(id, t);
        }

        attr = agget(v, (char *) "numLoads");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setNumLoads(id, std::string(attr));

        attr = agget(v, (char *) "numStores");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setNumStores(id, std::string(attr));

        attr = agget(v, (char *) "loadOffsets");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setLoadOffsets(id, std::string(attr));

        attr = agget(v, (char *) "storeOffsets");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setStoreOffsets(id, std::string(attr));

        attr = agget(v, (char *) "loadPorts");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setLoadPorts(id, std::string(attr));

        attr = agget(v, (char *) "storePorts");

        if (attr != nullptr and strlen(attr) > 0) 
            DF.setStorePorts(id, std::string(attr));
    }

    return true;
}

// Lana 04/10/19 read LSQ params for json
static bool readGetPtrConst(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);

    if (DF.getBlockType(id) == OPERATOR) {

        const char* attr = agget(v, (char *) "constants");
        if (attr != nullptr and strlen(attr) > 0) {
            int t = getPositiveInteger(attr);
            if (t < 0) {
                DF.setError("Block " + block_name + ": wrong value for GetPtr constants.");
                return false;
            }

             DF.setGetPtrConst(id, t);
        }

    }

    return true;
}

static bool readValue(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    string block_name = DF.getBlockName(id);
    const char* attr = agget(v, (char *) "value");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != CONSTANT) {
            DF.setError("Block " + block_name + ": value can only be defined for constants.");
            return false;
        }

        string str_attr(attr);
        size_t sz;
        // Lana 9.6.2021. Axel resource sharing update
        unsigned long long MSB = (1LL << sizeof(unsigned long long) * 8 -1);
        unsigned long long unsigned_v;
        longValueType v;

        std::stringstream ss;
        ss << std::hex << str_attr;
        ss >> unsigned_v;

        if(unsigned_v & MSB){
            v = (unsigned_v & ~MSB) - MSB;
        }else{
            v = unsigned_v;
        }

        DF.setValue(id, v);
        return true;
    }

    if (DF.getBlockType(id) == CONSTANT) {
        DF.setError("Block " + block_name + ": missing value for constant.");
        return false;
    }

    return true;
}

// Reads the attributes of elastic buffers (slots and transparency)
static bool readBufferAttributes(DFnetlist_Impl& DF, blockID id, Agnode_t* v)
{
    if (DF.getBlockType(id) != ELASTIC_BUFFER) return true;


    string block_name = DF.getBlockName(id);
    //cout << "setting buffer attributes for " << block_name << endl;

    const char* attr = agget(v, (char *) "slots");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != ELASTIC_BUFFER) {
            DF.setError("Block " + block_name + ": slots can only be defined for elastic buffers.");
            return false;
        }
        //cout << "SHAB: attr for buffer " << DF.getBlockName(id) << ": " << attr << endl;
        int slots = getPositiveInteger(attr);
        //cout << "SHAB: slots for buffer " << DF.getBlockName(id) << ": " << slots << endl;
        if (slots <= 0) {
            DF.setError("Block " + block_name + ": wrong value for slots.");
            return false;
        }
        DF.setBufferSize(id, slots);
    }

    attr = agget(v, (char *) "transparent");
    if (attr != nullptr and strlen(attr) > 0) {
        if (DF.getBlockType(id) != ELASTIC_BUFFER) {
            DF.setError("Block " + block_name + ": transparency can only be defined for elastic buffers.");
            return false;
        }

        string str_attr(attr);
        if (str_attr == "true") DF.setBufferTransparency(id, true);
        else if (str_attr == "false") DF.setBufferTransparency(id, false);
        else {
            DF.setError("Block " + block_name + ": wrong value for transparency (should be true or false.");
            return false;
        }
    }
    return true;
}

// Reads the attributes of elastic buffers (slots and transparency)
static bool readChannelBufferAttributes(DFnetlist_Impl& DF, channelID c, Agnode_t* v)
{
    //cout << "setting channel buffer attributes for " << DF.getChannelName(c) << endl;
    const char* attr = agget(v, (char *) "slots");
    int slots = 0;
    if (attr != nullptr and strlen(attr) > 0) {
        slots = getPositiveInteger(attr);
        if (slots < 0) {
            DF.setError("Channel " + DF.getChannelName(c) + ": wrong value for slots.");
            return false;
        }
        DF.setChannelBufferSize(c, slots);
    }
    DF.setChannelBufferSize(c, slots);

    attr = agget(v, (char *) "transparent");
    bool transparent = slots <= 1;
    if (attr != nullptr and strlen(attr) > 0) {
        string str_attr(attr);
        if (str_attr == "true") transparent = true;
        else if (str_attr == "false") transparent = false;
        else {
            DF.setError("Channel " + DF.getChannelName(c) + ": wrong value for transparency (should be true or false.");
            return false;
        }
    }

    DF.setChannelTransparency(c, transparent);
    return true;
}

bool DFnetlist_Impl::readDataflowDot(FILE *f)
{
    agseterr(AGMAX);

    Agraph_t *g = agread(f, nullptr);

    if (agerrors() > 0) {
        string errmsg(aglasterr());
        setError("Read netlist: " + errmsg);
        return false;
    }

    if (g == nullptr) return false; // No graph has been read.

    if (not agisdirected(g)) {
        setError("It is not a directed graph.");
        return false;
    }

    // Name of the graph
    net_name = string(agnameof(g));
    if (not goodIdentifier(*this, net_name)) return false;

    // Get the default width of the ports (originally defined as 32 bits)
    default_width = -1;
    const char* attr = agget(g, (char *) "channel_width");
    if (attr != nullptr) {
        // Default channel width defined
        int v = getPositiveInteger(attr);
        if (v < 0) {
            setError("Incorrect value for channel_width: " + string(attr));
            return false;
        }
        default_width = v;
    }

    // Traverse the set of nodes
    for (Agnode_t* v = agfstnode(g); v; v = agnxtnode(g,v)) {
        string node_name = string(agnameof(v));
//        cout << "currently traversing node " << node_name << endl;

        if (not goodIdentifier(*this, node_name)) {
            setError("Block " + node_name + ": invalid identifier.");
            return false;
        }

        const char* attr = agget(v, (char *) "type");
  //      cout << "\ttype is " << attr << endl;
        if (attr == nullptr) {
            setError("Block " + node_name + ": type not defined.");
            return false;
        }

        string type_str = string(attr);
        auto it = String2BlockType.find(type_str);
        if (it == String2BlockType.end()) {
            setError("Block " + node_name + ": unknown type " + type_str);
            return false;
        }

        BlockType type = String2BlockType[type_str];
        blockID id = createBlock(type, node_name);
        if (not validBlock(id)) return false;

        // Reading input and output ports
        if (not readPorts(*this, id, v, true)) return false;
        if (not readPorts(*this, id, v, false)) return false;

        // Reading delays
        if (not readDelays(*this, id, v)) return false;

        // Reading latency and initiation interval
        if (not readLatencyII(*this, id, v)) return false;

        // Reading execution frequency
        if (not readExecFrequency(*this, id, v)) return false;

        // Reading select input fraction
        if (not readTrueFrac(*this, id, v)) return false;

        // Reading the value for constants
        if (not readValue(*this, id, v)) return false;

        // Reading the operation for operators
        if (not readOperation(*this, id, v)) return false;

        // Reading the BB id
        if (not readBasicBlock(*this, id, v)) return false;

        // Reading mem port param
        if (not readMemPortID(*this, id, v)) return false;

        // Reading mem port param
        if (not readMemOffset(*this, id, v)) return false;

        // Reading mem port param
        if (not readMemBBCount(*this, id, v)) return false;

        // Reading mem port param
        if (not readMemLdCount(*this, id, v)) return false;

        // Reading mem port param
        if (not readMemStCount(*this, id, v)) return false;

        if (not readMemName(*this, id, v)) return false;

        if (not readFuncName(*this, id, v)) return false;

        if (not readLSQParams(*this, id, v)) return false;

        // Reading getelementptr array dimensions
        if (not readGetPtrConst(*this, id, v)) return false;

        // SHAB: to support naive buffer placement
        if (not readBufferAttributes(*this, id, v)) return false;
    }

    // Traverse the set of edges
    for (Agnode_t* v = agfstnode(g); v; v = agnxtnode(g,v)) {
        for (Agedge_t* e = agfstout(g,v); e; e = agnxtout(g,e)) {
            string src_name = agnameof(agtail(e));
            string dst_name = agnameof(aghead(e));
            blockID src = getBlock(src_name);

            if (not validBlock(src)) {
                setError ("Unknown block " + src_name + ".");
                return false;
            }

            blockID dst = getBlock(dst_name);
            if (not validBlock(dst)) {
                setError ("Unknown block " + dst_name + ".");
                return false;
            }

            const char* attr = agget(e, (char *) "from");
            if (attr == nullptr) {
                setError("Missing port for channel " + src_name + " -> " + dst_name + ".");
                return false;
            }

            portID psrc = getPort(src, string(attr));
            if (not validPort(psrc)) {
                setError ("Block " + getBlockName(src) + ": unknown port " + string(attr) + ".");
                return false;
            }


            attr = agget(e, (char *) "to");
            if (attr == nullptr) {
                setError("Missing port for channel " + src_name + " -> " + dst_name + ".");
                return false;
            }

            portID pdst = getPort(dst, string(attr));
            if (not validPort(pdst)) {
                setError ("Block " + getBlockName(dst) + ": unknown port " + string(attr) + ".");
                return false;
            }

            channelID c = createChannel(psrc, pdst);
            if (not validChannel(c)) {
                setError("Error when creating channel " + src_name + " -> " + dst_name + ".");
                return false;
            }

            // Checking attributes for elastic buffers (slots and transparency)
            if (not readChannelBufferAttributes(*this, c, v)) return false;
        }
    }

    //bbCount = 0;

    ///for(Agraph_t* h = agfstsubg(g); h; h = agnxtsubg(h)) {
    //    bbCount++;
    //}

    return true;
}

bool DFnetlist_Impl::readDataflowDot(const string& filename)
{
    FILE* f = fopen(filename.c_str(), "r");
    if (f == nullptr) {
        setError("File " + filename + " could not be opened.");
        return false;
    }

    bool status = readDataflowDot(f);
    fclose(f);
    return status;
}

bool DFnetlist_Impl::determineEntryExitBlock() {
    int entry_count = 0, exit_count = 0;
    for (int bb = 1; bb <= BBG.numBasicBlocks(); bb++) {
        if (BBG.predecessors(bb).empty()) {
            BBG.setEntryBasicBlock(bb);
            entry_count++;
        }
        if (BBG.successors(bb).empty()) {
            BBG.addExitBasicBlock(bb);
            exit_count++;
        }
    }

    assert(entry_count == 1 && exit_count > 0);
    return true;
}

bool DFnetlist_Impl::setAllBBFrequencies() {
    for (bbID bb = 1; bb <= BBG.numBasicBlocks(); bb++) {
        double succ_freq = 0, pred_freq = 0;

        for (bbArcID succ: BBG.successors(bb))
            succ_freq += BBG.getFrequencyArc(succ);

        for (bbArcID pred: BBG.predecessors(bb))
            pred_freq += BBG.getFrequencyArc(pred);

        if (BBG.isEntryBB(bb)) {
            assert(pred_freq == 0);
            BBG.setFrequency(bb, succ_freq);
        } else if (BBG.isExitBasicBlock(bb)) {
            assert(succ_freq == 0);
            BBG.setFrequency(bb, pred_freq);
        } else {
            assert (succ_freq == pred_freq);
            BBG.setFrequency(bb, succ_freq);
        }
    }
    return true;
}

void DFnetlist_Impl::computeBlockFrequencies() {
    computeChannelFrequencies();
    ForAllBlocks(b) {
        BlockType type = getBlockType(b);
        double freq = 0;
        if (type == FUNC_ENTRY) {
            ForAllOutputPorts(b, p) {
                freq += getChannelFrequency(getConnectedChannel(p));
            }
        } else {
            ForAllInputPorts(b, p) {
                freq += getChannelFrequency(getConnectedChannel(p));
            }
        }
        setExecutionFrequency(b, freq);
    }
}
void DFnetlist_Impl::computeChannelFrequencies() {
    ForAllChannels(c) {
        double freq = 0;

        bbID src = getBasicBlock(getSrcBlock(c));
        bbID dst = getBasicBlock(getDstBlock(c));
        BlockType src_type = getBlockType(getSrcBlock(c));
        BlockType dst_type = getBlockType(getDstBlock(c));

        if (src == 0 || dst == 0) {
//            setChannelFrequency(c, 1);
            continue;
        }

        // inner/outer channel
        if (src_type == BRANCH && (dst_type == MUX || dst_type == MERGE || dst_type == CNTRL_MG)) {
            bbArcID connection = BBG.findArc(src, dst);
            assert(connection != invalidDataflowID);
            freq = BBG.getFrequencyArc(connection);
        } else {
            assert(src == dst);
            freq = BBG.getFrequency(src);
        }

        setChannelFrequency(c, freq);
    }
}

//SHAB_note: add error checking.
bool DFnetlist_Impl::readDataflowDotBB(FILE *f) {

    cout << "===================" << endl;
    cout << "READING BB DOT FILE" << endl;
    cout << "===================" << endl;

    agseterr(AGMAX);

    Agraph_t *g = agread(f, nullptr);

    cout << "Reading graph name..." << endl;
    if (agerrors() > 0) {
        string errmsg(aglasterr());
        setError("Read netlist: " + errmsg);
        return false;
    }

    if (g == nullptr) return false; // No graph has been read.

    if (not agisdirected(g)) {
        setError("It is not a directed graph.");
        return false;
    }

    // Name of the graph
    if (not goodIdentifier(*this, string(agnameof(g)))) return false;

    BBG.clear();
    map<string, bbID> name2bbID;

    cout << "Reading set of nodes..." << endl;
    // Traverse the set of nodes
    for (Agnode_t* v = agfstnode(g); v; v = agnxtnode(g,v)) {
        string node_name = string(agnameof(v));

        if (not goodIdentifier(*this, node_name)) {
            setError("Block " + node_name + ": invalid identifier.");
            return false;
        }

        if (name2bbID.find(node_name) != name2bbID.end()) {
            setError("Block " + node_name + ": already defined.");
            return false;
        }

        bbID node_bbID = BBG.createBasicBlock();
        name2bbID[node_name] = node_bbID;
    }

    cout << "Reading set of edges between nodes..." << endl;
    // Traverse the set of edges
    for (Agnode_t* v = agfstnode(g); v; v = agnxtnode(g,v)) {
        for (Agedge_t* e = agfstout(g,v); e; e = agnxtout(g,e)) {
            string src_name = agnameof(agtail(e));
            string dst_name = agnameof(aghead(e));

            auto it_src = name2bbID.find(src_name);
            if (it_src == name2bbID.end()) {
                setError("Invalid source Basic Block: " + src_name + " to " + dst_name);
                return false;
            }

            auto it_dst = name2bbID.find(dst_name);
            if (it_dst == name2bbID.end()) {
                setError("Invalid destination Basic Block: " + src_name + " to " + dst_name);
                return false;
            }

            bbID src = it_src->second;
            bbID dst = it_dst->second;

            if (BBG.findArc(src, dst) != -1) {
                setError(src_name + "->" + dst_name + " arc: already defined");
                return false;
            }

            const char* attr = agget(e, (char *) "freq");
            double freq = 0;
            if (attr != nullptr and strlen(attr) > 0) {
                freq = getPositiveDouble(attr);
                if (freq < 0) {
                    setError(src_name + "->" + dst_name + " arc: negative execution frequency.");
                    return false;
                }
            }

            bbArcID arc = BBG.findOrAddArc(src, dst, freq);
        }
    }

    // set the entryBB. it is assumed that the entry/exit BB has no input/output edges.
    cout << "Setting entry and exit BB..." << endl;
    determineEntryExitBlock();
    cout << "\tEntry: BB" << BBG.getEntryBasicBlock() << ", Exit:";
    for (auto bb: BBG.getExitBasicBlocks())
        cout << " BB" << bb;
    cout << endl;

    // for each basic block, sum of input and output must match.
    cout << "Setting BB frequencies..." << endl;
    setAllBBFrequencies();
    for (int i = 1; i <= BBG.numBasicBlocks(); i++) {
        cout << "BB" << i << " : " << BBG.getFrequency(i) << endl;
    }
    cout << endl;

    return true;
}

bool DFnetlist_Impl::readDataflowDotBB(const std::string &filename) {
    FILE* f = fopen(filename.c_str(), "r");
    if (f == nullptr){
        setError("File " + filename + " could not be opened.");
        return false;
    }

    bool status = readDataflowDotBB(f);
    fclose(f);
    return status;
}
