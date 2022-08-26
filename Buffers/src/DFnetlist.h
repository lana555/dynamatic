#ifndef DATAFLOW_IMPL_H
#define DATAFLOW_IMPL_H

#include <cassert>
#include <deque>
#include <list>
#include <map>
#include <string>
#include "Dataflow.h"
#include "ErrorManager.h"
#include "FileUtil.h"
#include "MILP_Model.h"

// Some useful macros
#define ForAllBlocks(b)         for (blockID b: allBlocks)
#define ForAllChannels(c)       for (channelID c: allChannels)
#define ForAllPorts(b,p)        for (portID p: getPorts(b, ALL_PORTS))
#define ForAllInputPorts(b,p)   for (portID p: getPorts(b, INPUT_PORTS))
#define ForAllOutputPorts(b,p)  for (portID p: getPorts(b, OUTPUT_PORTS))
#define ForAllBasicBlocks(bb)   for (bbID bb = 0; bb < numBasicBlocks(); ++bb)

namespace Dataflow
{
struct milpVarsEB; // Defined locally in DFnetlist_buffers.cpp
struct milpVarsMG; // Defined locally in DFnetlist_MG.cpp

using bbID = int;       // Identifiers for Basic Blocks
using bbArcID = int;    // Identifiers for Basic Block arcs
using vecBBs = std::vector<bbID>;
using setBBs = std::set<bbID>;
using vecBBArcs = std::vector<bbArcID>;
using setBBArcs = std::set<bbArcID>;

using longValueType = long long;    // Internal representation for constant values

/**
 * @class BasicBlockGraph
 * @author Jordi Cortadella
 * @date 30/08/18
 * @file DFnetlist.h
 * @brief Class to represent Basic Block Graphs (BBGs). The blocks of the DF
 * netlist are annotated with a BB identifier of the BBG. Each BBG also includes
 * a set of BB cycles that cover the most frequently executed BBs.
 */
class BasicBlockGraph
{
public:

    /**
     * @brief Constructor
     */
    BasicBlockGraph() {
        clear();
    }

    /**
     * @brief Initializes the BB graph (empty).
     */
    void clear() {
        BBs.clear();
        Arcs.clear();
        entryBB = invalidDataflowID;
    }

    /**
     * @return The number of Basic Blocks
     */
    int numBasicBlocks() const {
        return BBs.size();
    }

    /**
     * @return The number of Arcs
     */
    int numArcs() const {
        return Arcs.size();
    }

    /**
     * @return True if the BB graph is empty, and false otherwise.
     */
    bool empty() const {
        return numBasicBlocks() == 0;
    }

    /**
     * @brief Adds a new BB to the BB graph.
     * @return The id of the BB.
     * @note the IDs start from 1, blocks that don't belong in any basic block have ID 0.
     */
    bbID createBasicBlock() {
        bbID id = BBs.size() + 1;

        // Block with freq = -1 and exec = 1
        BasicBlock bb;
        bb.exec = 1;
        bb.freq = -1;
        bb.id = id;

        BBs.push_back(bb);
        return id;
    }

    /**
     * @brief Defines the entry BB.
     * @param bb Id of the entry BB.
     */
    void setEntryBasicBlock(bbID bb) {
        entryBB = bb;
    }

    /**
     * @return The entry Basic Block
     */
    bbID getEntryBasicBlock() const {
        return entryBB;
    }

    setBBs& getExitBasicBlocks() {
        return exitBBs;
    }

    void addExitBasicBlock(bbID bb) {
        exitBBs.insert(bb);
    }

    bool isEntryBasicBlock(bbID bb) {
        return entryBB == bb;
    }

    bool isExitBasicBlock(bbID bb) {
        return exitBBs.count(bb) > 0;
    }

    /**
    * @brief Returns the arc between two BBs.
    * @param src Id of the first block.
    * @param dst Id of the second block.
    * @return Id of the arc. In case it does not exist,
    * it returns invalidDataflowID.
    */
    bbArcID findArc(bbID src, bbID dst) const;

    /**
     * @brief Creates an arc between two BBs (if it did not exist)
     * @param src Id of the first block.
     * @param dst Id of the second block.
     * @return Id of the arc.
     */
    bbArcID findOrAddArc(bbID src, bbID dst, double freq = 0);

    /**
     * @brief Returns the src BB of an arc.
     * @param arc Id of the arc.
     * @return The id of the BB.
     */
    bbID getSrcBB(bbArcID arc) const {
        return Arcs[arc].src;
    }

    /**
     * @brief Returns the dst BB of an arc.
     * @param arc Id of the arc.
     * @return The id of the BB.
     */
    bbID getDstBB(bbArcID arc) const {
        return Arcs[arc].dst;
    }

    /**
     * @brief Defines the probability of an arc.
     * @param arc Id of the arc.
     * @param prob Probability of the arc.
     */
    void setProbability(bbArcID arc, double prob) {
        Arcs[arc].prob = prob;
    }

    /**
     * @brief Returns the probability of an arc.
     * @param arc Id of the arc.
     * @return The probability of the arc.
     */
    double getProbability(bbArcID arc) const {
        return Arcs[arc].prob;
    }

    void setFrequencyArc(bbArcID arc, double freq) {
        Arcs[arc].freq = freq;
    }

    double getFrequencyArc(bbArcID arc) const {
        return Arcs[arc].freq;
    }

    /**
     * @brief Calculates the back edges using a DFS traversal
     * from the entry basic block.
     * @return True if successful, and false otherwise (e.g., entry
     * not defined).
     */
    bool calculateBackArcs();

    /**
     * @brief Defines whether an arc is forward or backward.
     * @param arc Id of the arc.
     * @param back True if backward, false if forward.
     */
    void setBackArc(bbArcID arc, bool back = true) {
        Arcs[arc].back = back;
    }

    /**
     * @brief Checks whether an arc is forward or backward.
     * @param arc Id of the arc.
     * @return True if backward, false if forward.
     */
    bool isBackArc(bbArcID arc) const {
        return Arcs[arc].back;
    }

    /**
     * @brief Returns the set of predecessor arcs.
     * @param bb Id of the BB.
     * @return The set of predecessor arcs.
     */
    setBBArcs& predecessors(bbID bb) {
        return BBs[bb - 1].pred;
    }

    /**
     * @brief Returns the set of successor arcs.
     * @param bb Id of the BB.
     * @return The set of successor arcs.
     */
    setBBArcs& successors(bbID bb) {
        return BBs[bb - 1].succ;
    }

    /**
     * @brief Sets de execution frequency of a BB.
     * @param bb Id of the BB.
     * @param f Execution frequency.
     */
    void setFrequency(bbID bb, double f) {
        BBs[bb - 1].freq = f;
    }

    /**
     * @brief Returns the execution frequency of a BB.
     * @param bb The id of the BB.
     * @return The execution frequency of the BB.
     */
    double getFrequency(bbID bb) const {
        return BBs[bb - 1].freq;
    }

    /**
     * @brief Sets the execution time of a BB.
     * @param bb The id of the BB.
     * @param e The execution time.
     */
    void setExecTime(bbID bb, double t) {
        BBs[bb - 1].exec = t;
    }

    /**
     * @brief Retgurns the execution time of a BB.
     * @param bb The id of the BB.
     * @return The execution time.
     */
    double getExecTime(bbID bb) const {
        return BBs[bb - 1].exec;
    }

    double isEntryBB(bbID bb) {
        return entryBB == bb;
    }

    double getDFSorder(bbID bb) {
        return BBs[bb - 1].DFS_order;
    }

    void setDFSorder(bbID bb, int order) {
        BBs[bb - 1].DFS_order = order;
    }

    double getSCCnumber(bbID bb) {
        return BBs[bb - 1].SCC_number;
    }

    void setSCCnumber(bbID bb, int number) {
        BBs[bb - 1].SCC_number = number;
    }


    /**
     * @brief If the BB frequencies are already define, nothing is done.
     * Otherwise, the BB frequencies are calculated treating the graph
     * as a Markov chain. It considers that the frequency of the
     * entry point is 1. If the probabilities of the arcs are not defined,
     * some default values are set, considering that back arcs have a
     * higher probability.
     * @param back_prob The default probablity of back arcs in case the
     * probabilities of the arcs are not defined.
     * @return True if successful, and false if some error occurred.
     */
    bool calculateBasicBlockFrequencies(double back_prob = 0.9);

    /**
     * @brief Extracts a set of Basic Block cycles that maximizes the covered
     * execution time.
     * @param coverage Target coverage for the cycles (0 <= coverage <= 1).
     * If coverage is negative, only one cycle is extracted (the one with
     * max execution time).
     * @return The ratio of covered execution time (between 0 and 1).
     */
     //SHAB_note: check this :-?
    double extractBasicBlockCycles(double coverage = -1.0);

    void DFS();

    void computeSCC();

    void setDSUnumberArc(bbArcID arc, int number) {
        assert (arc >= 0 && arc < numArcs());
        Arcs[arc].DSU_number = number;
    }

    void addMGnumber(bbArcID arc, int number) {
        assert (arc >= 0 && arc < numArcs());
        Arcs[arc].MG_numbers.push_back(number);
    }

    int getDSUnumberArc(bbArcID arc) {
        return Arcs[arc].DSU_number;
    }

    vector<int>& getMGnumbers(bbArcID arc) {
        return Arcs[arc].MG_numbers;
    }

    std::vector<bbID>& getBlocksInCycle(int cycle_number){
        return cycles[cycle_number].cycle;
    }

    int getCycleNum(){
        return cycles.size();
    }

private:
    // Structure to represent a Basic Block.
    struct BasicBlock {
        double freq;            // Execution frequency of the BB
        double exec;            // Execution time of the BB
        setBBArcs pred;         // Prededessor arcs
        setBBArcs succ;         // Successor arcs
        double residual_freq;   // Residual frequency (used to compute frequent cycles)
        bbID id;
        int DFS_order;
        int SCC_number;
    };

    // Arc connecting two BBs in the Basic Block Graph
    struct BB_Arc {
        bbID src;       // Src BB
        bbID dst;       // Dst BB
        bool back;      // Is it a back edge?
        double prob;    // Probability of taking the arc (-1 if undefined)
        bbArcID id;
        double freq;    // Execution frequency of the arc

        vector<int> MG_numbers;
        int DSU_number;
    };

    // Structure to represent a cycle in the Basic Block Graph.
    struct BasicBlockCycle {
        double freq;                // Execution frequency of the cycle
        double exec;                // Execution time of the cycle
        std::vector<bbID> cycle;    // Sequence of BBs (the last BB is connected to the first)
        std::vector<bool> executed; // A flag to denote which block is executed
    };

    std::vector<BasicBlock> BBs;    // List of basic blocks
    std::vector<BB_Arc> Arcs;       // List of arcs
    bbID entryBB;                   // Entry Basic Block
    setBBs exitBBs;                 // set of exit Basic Blocks
    std::vector<bbID> DFSorder;     // DFS order to get SCC of MGs

    std::vector<BasicBlockCycle> cycles;    // List of critical cycles

    /**
     * @brief Defines the default probabilities for the arcs.
     * All the output arcs are equiprobable, except for the
     * case in which there is one back edge, that takes the
     * default probability, whereas the remaining arcs get
     * the remaining (distributed) probability.
     * @param back_prob The default probablity for back edges.
     */
    void setDefaultProbabilities(double back_prob = 0.9);

    /**
     * @brief Extracts one basic block cycle using the residual frequencies.
     * @return The basic block cycle.
     */
    BasicBlockCycle extractBasicBlockCycle();
};

/**
 * @class DFnetlist_Impl
 * @author Jordi Cortadella
 * @date 30/08/18
 * @file DFnetlist.h
 * @brief Implementation of the Dataflow netlist
 */
class DFnetlist_Impl
{

public:
    /**
     * @brief Default constructor.
     */
    DFnetlist_Impl();

    /**
    * @brief Constructor. It reads a netlist from a file.
    * @param name File name of the input description.
    */
    DFnetlist_Impl(const std::string& name);

    /**
    * @brief Constructor. It creates a netlist from a file descriptor.
    * The file is assumed to be opened.
    * @param file File descriptor.
    */
    DFnetlist_Impl(FILE* file);

    /**
     * Shabnam:
    * @brief Constructor. It creates a netlist from two files.
     * first file consists of blocks.
     * second file consists of basic blocks.
    * @param file File name of the input descriptions.
    */
    DFnetlist_Impl(const std::string& name, const std::string& name_bb);

    /**
     * @brief Checks that the netlist is well-formed.
     * @return True if it is well-formed and false otherwise.
     * @note If not well-formed, an error message is generated.
     */
    bool check();

    /**
     * @return The name of the netlist.
     */
    const std::string& getName() const;

    /**
     * @brief Creates a new block in the netlist.
     * @param type Type of the block.
     * @param name Name of the block.
     * @return The block id. It returns an invalid block if an error occurs.
     */
    blockID createBlock(BlockType type, const std::string& name = "");

    /**
     * @brief Returns a block id of the netlist.
     * @param name Name of the block.
     * @return The id of the block (invalidDataflowID if not found).
     */
    blockID getBlock(const std::string& name) const;

    /**
     * @brief Returns the name of a block.
     * @param id Identifier of the block.
     * @return The name of the block.
     */
    const std::string& getBlockName(blockID id) const;

    /**
     * @brief Returns the type of block.
     * @param id Identifier of the block.
     * @return The type of the block.
     */
    BlockType getBlockType(blockID id) const;

    std::string printBlockType(BlockType type) const;

    void printChannelInfo(channelID c, int slots, int transparent);

    bool isInnerChannel(channelID c);

    bool getChannelMerge(channelID c); //Carmine 09.03.22 these two functions are used to preserve 
    void setChannelMerge(channelID c); //Carmine 09.03.22 the information for writing the dot after reduceMerges function


    /**
     * @brief Returns the delay of a block.
     * @param id Identifier of the block.
     * @return The delay of the block.
     */
    double getBlockDelay(blockID id, int indx) const;


    /**
     * @brief Defines the basic block of a block.
     * @param id Identifer of the block
     */
    void setBasicBlock(blockID id, bbID bb = invalidDataflowID);

    /**
     * @brief Returns the basic block of a block.
     * @param id Identifier of the block.
     * @return The basic block of the block.
     */
    bbID getBasicBlock(blockID id) const;

    /**
     * @brief Sets the delay of a block.
     * @param id Identifier of the block.
     * @param d Delay of the block.
     */
    void setBlockDelay(blockID id, double d, int indx);

    /**
     * @brief Returns the latency of a block.
     * @param id Identifier of the block.
     * @return The latency of the block.
     */
    int getLatency(blockID id) const;

    /**
     * @brief Sets the latency of a block.
     * @param id Identifier of the block.
     * @param lat Latency of the block.
     */
    void setLatency(blockID id, int lat);


    double getBlockRetimingDiff(blockID id) const;


    void setBlockRetimingDiff(blockID id, double diff);

    /**
    * @brief Returns the initiation interval of a block.
    * @param id Identifier of the block.
    * @return The initiation interval of the block.
    */
    int getInitiationInterval(blockID id) const;

    /**
     * @brief Sets the initiation interval of a block.
     * @param id Identifier of the block.
     * @param ii Initiation interval of the block.
     */
    void setInitiationInterval(blockID id, int ii);

    /**
     * @brief Sets the execution frequency of a block.
     * @param id Identifier of the block.
     * @param freq Execution frequency of the block.
     */
    void setExecutionFrequency(blockID id, double freq);

    /**
    * @brief Returns the execution frequency of a block.
    * @param id Identifier of the block.
    * @return The execution frequency of the block.
    */
    int getExecutionFrequency(blockID id) const;


    /**
     * @brief Sets the true/false exe. fraction of a select op.
     * @param id Identifier of the block.
     * @param freq Execution frequency of the block.
     */
    void setTrueFrac(blockID id, double freq);

    /**
    * @brief Returns the the true/false exe. fraction of a select op.
    * @param id Identifier of the block.
    * @return The execution frequency of the block.
    */
    double getTrueFrac(blockID id) const;

    /**
     * @brief Defines the value of a constant block.
     * @param id Identifier of the block.
     * @param value Value of the constant.
     */
    void setValue(blockID id, longValueType value);

    /** Lana 07.03.19 
     * @brief Defines the operation (instruction) of operators.
     * @param id Identifier of the block.
     * @param op Operation.
     */
    void setOperation(blockID id, std::string op);

    // Lana 02.07.19 
    const std::string& getOperation(blockID id) const;

    // Lana 03.07.19 
    void setMemPortID(blockID id, int memPortID);

    int getMemPortID(blockID id) const;

    void setMemOffset(blockID id, int memOffset);

    int getMemOffset(blockID id) const;

    void setMemBBCount(blockID id, int count);

    int getMemBBCount(blockID id) const;

    void setMemLdCount(blockID id, int count);

    int getMemLdCount(blockID id) const;

    void setMemStCount(blockID id, int count);

    int getMemStCount(blockID id) const;

    void setMemName(blockID id, std::string name);

    const std::string& getMemName(blockID id) const;

    void setMemPortSuffix(blockID id, std::string op);

    const std::string& getMemPortSuffix(blockID id) const;

    void setFuncName(blockID id, std::string op);

    const std::string& getFuncName(blockID id) const;

    void setOrderings(blockID id, map<bbID, vector<int>> value);

    // Lana 04/10/19

    void setLSQDepth(blockID id, int depth);

    int getLSQDepth(blockID id) const;

    void setNumLoads(blockID id, std::string name);

    const std::string& getNumLoads(blockID id) const;

    void setNumStores(blockID id, std::string name);

    const std::string& getNumStores(blockID id) const;

    void setLoadOffsets(blockID id, std::string name);

    const std::string& getLoadOffsets(blockID id) const;

    void setStoreOffsets(blockID id, std::string name);

    const std::string& getStoreOffsets(blockID id) const;

    void setLoadPorts(blockID id, std::string name);

    const std::string& getLoadPorts(blockID id) const;

    void setStorePorts(blockID id, std::string name);

    const std::string& getStorePorts(blockID id) const;

    void setGetPtrConst(blockID id, int c);

    int getGetPtrConst(blockID id) const;

    /**
     * @brief Defines the value of a constant block.
     * @param id Identifier of the block.
     * @param value Value of the constant.
     */
    longValueType getValue(blockID id);

    /**
     * @brief Retruns the boolean value of a constant.
     * @param id Identifier of the block.
     * @return The Boolean value of the constant.
     */
    bool getBoolValue(blockID id) const;

    /**
    * @brief Returns the number of slots of an elastic buffer.
    * @param id Identifier of the block.
    * @return The number of slots of the buffer.
    */
    int getBufferSize(blockID id) const;

    /**
     * @brief Sets the number of slots of an elastic buffer.
     * @param id Identifier of the block.
     * @param slots Number of slots of the buffer.
     */
    void setBufferSize(blockID id, int slots);

    /**
     * @brief Checks whether the buffer is transparent.
     * @param id Identifier of the block.
     * @return True if transparent, and false if opaque.
     */
    bool isBufferTransparent(blockID id) const;

    /**
      * @brief Sets the transparency of an elastic buffer.
      * @param id Identifier of the block.
      * @param value True if transparent, and false if opaque.
      */
    void setBufferTransparency(blockID id, bool value);

    /**
     * @param p Port ID.
     * @return The block owner of the port.
     */
    blockID getBlockFromPort(portID p) const;

    /**
     * @brief Removes a block and all the associated channels from the netlist.
     * @param block Identifier of the block.
     */
    void removeBlock(blockID block);

    /**
     * @brief Adds a new port to a block.
     * @param block id of the block.
     * @param isInput Direction of the port.
     * @param name Name of the port.
     * @param width The width of the port.
     * @param type Type of the port (generic, select, true, false).
     * @return The port id. It returns an invalidPort in case of error.
     * @note If the name is empty, a new fresh name is generated.
     */
    portID createPort(blockID block, bool isInput, const std::string& name="", int width = -1, PortType type = GENERIC_PORT);

    /**
     * @brief Removes a port and the associated channels from the netlist.
     * @param port Identifier of the port.
     */
    void removePort(portID port);

    /**
     * @brief Returns the id of a port from a given block and pin name.
     * @param block Id of the block.
     * @param name Name of the pin
     * @return the port id (invalidePort if not found).
     */
    portID getPort(blockID block, const std::string& name) const;

    /**
     * @brief Returns the name of a port.
     * @param port Identifier of the port.
     * @param full If asserted, the full name (block:port) is returned.
     * @return A string with the name of the port.
     */
    const std::string& getPortName(portID port, bool full=true) const;

    /**
     * @brief Returns the type of a port.
     * @param port The id of the port.
     * @return The type of the port.
     */
    PortType getPortType(portID port) const;

    /**
     * @brief Indicates whether a port is input.
     * @param port The id of the port.
     * @return True if it is an input port, and false if it is output.
     */
    bool isInputPort(portID port) const;

    /**
    * @brief Indicates whether a port is output.
    * @param port The id of the port.
    * @return True if it is an output port, and false if it is input.
    */
    bool isOutputPort(portID port) const;

    /**
     * @brief Defines the width of a port.
     * @param port The id of the port.
     * @param width The width of the port.
     */
    void setPortWidth(portID port, int width);

    /**
     * @brief Returns the width of a port.
     * @param port The id of the port.
     * @return The width of the port.
     */
    int getPortWidth(portID port) const;

    /**
     * @brief Checks whether a port is Boolean.
     * @param port The id of the port.
     * @return True if it is boolean, and false otherwise.
     */
    bool isBooleanPort(portID port) const;

    /**
     * @brief Checks whether a port is a control port.
     * @param port The id of the port.
     * @return True if it is a control port, and false otherwise.
     */
    bool isControlPort(portID port) const;

    /**
     * @brief Indicates whether the port has a constant boolean value.
     * @param port Identifier of the port.
     * @param value Value of the constant in case the port has a constant value.
     * @return True if it has a Boolean constant value, and false otherwise.
     */
    bool isBooleanConstant(portID port, bool& value) const;

    /**
     * @brief Returns the delay of a port.
     * @param port Identifier of the port.
     * @return The delay of the port.
     */
    double getPortDelay(portID port) const;

    /**
     * @brief Sets the delay of a port.
     * @param port Identifier of the port.
     * @param d The delay.
     */
    void setPortDelay(portID port, double d);

    /**
     * @brief Returns the combinational delay between an input and and output
     * port of a block. The ports are assumed to belong to the same blocks and
     * the latency of the block is assumed to be zero.
     * @param inp The input port.
     * @param outp The output port.
     * @return The combinational delay between inp and outp.
     */
    double getCombinationalDelay(portID inp, portID outp) const;

    /**
     * @brief Returns a set of ports with a certain direction.
     * @param id Block id (the owner of the ports).
     * @param dir Direction of the ports (input/output/all).
     * @return The set of ports.
     */
    const setPorts& getPorts(blockID id, PortDirection dir) const;

    /**
     * @brief Returns the set of ports that define values for a port.
     * @param p The identifier of the port.
     * @return The set of ports that define values that reach port p.
     */
    const setPorts& getDefinitions(portID p) const;

    /**
     * @param id Block id.
     * @return The conditional port of a block (invalidDataflowID if it does not exists).
     */
    portID getConditionalPort(blockID id) const;

    /**
     * @param id Block id.
     * @return The True port of a block (invalidDataflowID if it does not exists).
     */
    portID getTruePort(blockID id) const;

    /**
     * @param id Block id.
     * @return The False port of a block (invalidDataflowID if it does not exists).
     */
    portID getFalsePort(blockID id) const;

    /**
     * @param id Block id.
     * @return The data port of a branch or demux.
     */
    portID getDataPort(blockID id) const;

    /**
     * @brief Returns the complementary port associated to one of
     * the ports of a demux. If the parameter is an input control port,
     * it returns the corresponding output data port, and vice versa.
     * @param port Port of the demux.
     * @return The complementary port.
     */
    portID getDemuxComplementaryPort(portID port) const;

    /**
     * @brief Returns the channel associated to a port.
     * @param port The port from which we want to obtain the channel.
     * @return The channel id..
     */
    channelID getConnectedChannel(portID port) const;

    /**
     * @brief Checks whether a port is connected.
     * @param port The port id.
     * @return True if the port is connected, and False otherwise.
     */
    bool isPortConnected(portID port) const;

    /**
     * @brief Returns the opposite port connected to the same channel.
     * @param port The id of the port.
     * @return The port ID of the opposite port.
     */
    portID getConnectedPort(portID port) const;

    /**
     * @brief Creates a channel between two ports.
     * @param src The source port.
     * @param dst The destination port.
     * @param slots Size of the elastic buffer (0 if no buffer).
     * @param transparent Indicates that the elastic buffer is transparent.
     * @return The channel id. It returns invalidChannel in case of an error.
     */
    channelID createChannel(portID src, portID dst, int slots = 0, bool transparent = true);

    /**
     * @brief Removes a channel.
     * @param id Id of the channel.
     */
    void removeChannel(channelID id);

    /**
     * @brief Returns the source port of a channel.
     * @param id Id of the channel.
     * @return The source port. It returns invalidPort in case of error.
     */
    portID getSrcPort(channelID id) const;

    /**
     * @brief Returns the destination port of a channel.
     * @param id Id of the channel.
     * @return The destination port. It returns invalidPort in case of error.
     */
    portID getDstPort(channelID id) const;

    /**
     * @brief Returns the source block of a channel.
     * @param id Id of the channel.
     * @return The source block. It returns invalidBlock in case of error.
     */
    blockID getSrcBlock(channelID id) const;

    /**
    * @brief Returns the destination block of a channel.
    * @param id Id of the channel.
    * @return The destination block. It returns invalidBlock in case of error.
    */
    blockID getDstBlock(channelID id) const;

    /**
     * @brief Returns the name of the channel.
     * @param id Identifier of the channel.
     * @param full If asserted, a block:port -> block:port description
     * is generated. Otherwise, a block -> block description is generated.
     * @return The string representing the name of the port.
     */
    string getChannelName(channelID id, bool full=true) const;

    /**
     * @brief Returns the channel ID.
     * @param id Identifier of the port.
     * @return The ID representing the channel connected to that port
     */
    channelID getChannelID(portID id); //Carmine 09.03.22 new function


    /**
    * @brief Returns the number of slots of the elastic buffer in the channel.
    * @param id Identifier of the channel.
    * @return The number of slots of the buffer.
    */
    int getChannelBufferSize(channelID id) const;

    /**
     * @brief Sets the number of slots of the elastic buffer in the channel.
     * @param id Identifier of the channel.
     * @param slots Number of slots of the buffer.
     */
    void setChannelBufferSize(blockID id, int slots);

    /**
     * @brief Checks whether the buffer of the channel is transparent.
     * @param id Identifier of the channel.
     * @return True if transparent, and false if opaque.
     */
    bool isChannelTransparent(channelID id) const;

    /**
      * @brief Sets the transparency of the elastic buffer in the channel.
      * @param id Identifier of the channel.
      * @param value True if transparent, and false if opaque.
      */
    void setChannelTransparency(channelID id, bool value);


    /** //Carmine 21.02.22
      * @brief Sets the presence of an EB on the channel
      * @param id Identifier of the channel.
      */
    void setChannelEB(channelID id);

    /** //Carmine 21.02.22
      * @brief Gets the presence of an EB on the channel
      * @param id Identifier of the channel.
      */
    bool getChannelEB(channelID id);

    void setChannelFrequency(channelID id, double value);

    double getChannelFrequency(channelID id);

    /**
     * @brief Checks whether a channel has an elastic buffer (slots > 0).
     * @param id Identifier of the channel.
     * @return True if it has a buffer, and false otherwise.
     */
    bool hasBuffer(channelID id) const;

    /**
     * @brief Inserts a buffer in a channel. The insertion removes
     * the previous channel and creates two new channels.
     * @param c The channel identifier.
     * @param slots Number of slots of the buffer.
     * @param transparent True if the channel must be transparent and false if opaque.
     * @return The block identifier.
     */
    blockID insertBuffer(channelID c, int slots, bool transparent);

    /**
     * @brief Inserts a buffer in a channel. The insertion removes
     * the previous channel and creates two new channels.
     * @param c The channel identifier.
     * @param slots Number of slots of the buffer.
     * @param transparent True if the channel must be transparent and false if opaque.
     * @param EB True if an elastic buffer needs to be placed.
     * @return The block identifier.
     */
    blockID insertBuffer(channelID c, int slots, bool transparent, bool EB);

    /**
     * @brief Removes a buffer and reconnects the input and output channels.
     * @param buf Identifier of the buffer.
     * @return The identifier of the new channel.
     */
    channelID removeBuffer(blockID buf);

    /**
     * @brief Sets an error message for the netlist.
     * @param err Error message.
     */
    void setError(const std::string& err);

    /**
     * @return The error message associated to the netlist.
     */
    const std::string& getError() const;

    /**
     * @return The error message associated to the netlist.
     */
    std::string& getError();

    /**
     * @brief Erases the error.
     */
    void clearError();

    /**
     * @brief Indicates whether there is an error.
     * @return True if there is an error and false otherwise.
     */
    bool hasError() const;

    /**
     * @return The number of blocks of the netlist.
     */
    int numBlocks() const;

    /**
     * @return The number of channels of the netlist.
     */
    int numChannels() const;

    /**
     * @return The number of ports of the netlist.
     */
    int numPorts() const;

    /**
     * @param p The port id.
     * @return True if it is valid and false otherwise.
     */
    bool validPort(portID p) const;

    /**
     * @brief Indicates whether a block id is valid.
     * @param id The id of the block.
     * @return True if the id is valid and false otherwise.
     */
    bool validBlock(blockID id) const;

    /**
    * @brief Indicates whether a channel id is valid.
    * @param id The id of the channel.
    * @return True if the id is valid and false otherwise.
    */
    bool validChannel(channelID id) const;

    /**
     * @brief Reduce the number of merges since 1 input merges can be reduced to a wire.
     */
    void reduceMerges(); //Carmine 09.03.22 new function

    void execute_reduction(blockID b);  //Carmine 09.03.22 function to eliminate  a block

    /**
     * @brief Writes the dataflow netlist in dot format.
     * @param filename The name of the file. In case the filename is empty,
     * it is written into cout.
     * @return true if successful, and false otherwise.
     */
    bool writeDot(const std::string& filename = "");

    /**
     * @brief Writes the dataflow netlist in dot format.
     * @param s The output stream.
     * @return True if successful, and false otherwise.
     */
    bool writeDot(std::ostream& of);

    bool writeDotMG(const std::string& filename = "");
    bool writeDotMG(std::ostream& s);

    bool writeDotBB(const std::string& filename = "");
    bool writeDotBB(std::ostream& of);

    /**
     * @brief Writes the basic blocks of a dataflow netlist in dot format.
     * @param filename The name of the file. In case the filename is empty,
     * it is written into cout.
     * @return true if successful, and false otherwise.
     */
    bool writeBasicBlockDot(const std::string& filename = "");

    /**
     * @brief Writes the basic blocks of a dataflow netlist in dot format.
     * @param s The output stream.
     * @return True if successful, and false otherwise.
     */
    bool writeBasicBlockDot(std::ostream& s);

    /**
     * @brief Adds elastic buffers to meet a certain cycle period and
     * guarantee elasticity in the system.
     * @param Period Target cycle Period. If Period <= 0, then no constraints on the period are assumed.
     * @param BufferDelay CLK-to-Q delay of the Elastic Buffer.
     * @param If asserted, throughput is maximized.
     * @return True if no error, and false otherwise.
     * @note In case of throughput maximization, it is assumed that the set of critical basic blocks has
     * been extracted.
     */
    bool addElasticBuffers(double Period = 0, double BufferDelay = 0, bool MaxThroughput = false, double coverage = 0);

    /**
     * @brief Adds elastic buffers to meet a certain cycle period and
     * guarantee elasticity in the system by looking at a Basic Block level.
     * @param Period Target cycle Period. If Period <= 0, then no constraints on the period are assumed.
     * @param BufferDelay CLK-to-Q delay of the Elastic Buffer.
     * @param If asserted, throughput is maximized.
     * @return True if no error, and false otherwise.
     * @note In case of throughput maximization, it is assumed that the set of critical basic blocks has
     * been extracted.
     */
    bool addElasticBuffersBB(double Period = 0, double BufferDelay = 0, bool MaxThroughput = false, double coverage = 0, int timeout = -1, bool first_MG = false);
    bool addElasticBuffersBB_sc(double Period = 0, double BufferDelay = 0, bool MaxThroughput = false, double coverage = 0, int timeout = -1, bool first_MG = false, const std::string& model_mode = "default", const std::string& lib_path="");

    void addBorderBuffers();
    void findMCLSQ_load_channels();
    /**
     * @brief Creates buffers for all those channels annotated with buffers.
     * The information in the channels is deleted.
     */
    void instantiateElasticBuffers();

    /**
     * @brief Creates buffers for all those channels annotated inside input file.  // Carmine 09.02.22
     */
    void instantiateAdditionalElasticBuffers(const std::string& filename);


    /**
     * @brief Removes the elastic buffers and transfers the information to the channels.
     */
    void hideElasticBuffers();

    /**
     * @brief Removes all the elastic buffers from the netlist.
     */
    void cleanElasticBuffers();

    /**
     * @brief Sets the solver for MILP optimization problems.
     * @param solver Name of the solver (usually cbc or glpsol).
     */
    void setMilpSolver(const std::string& solver="cbc");

    /**
     * @brief Removes all non-SCC blocks and channels.
     * @return The netlist with only the SCCs.
     * @note The method recalculates the DFS order of all components.
     */
    void eraseNonSCC();

    /**
     * @brief Calculates the graph of basic blocks.
     * @return True if successful, and False otherwise.
     * @note The function assumes that the reachable definitions
     * have been previously computed.
     */
    bool calculateBasicBlocks();

    /**
     * @brief Performs different types of optimizations to reduce
     * the complexity of the dataflow netlist. It also calculates the
     * rechable definitions for every block.
     * @return True if changes have been produced, and false otherwise.
     */
    bool optimize();

    /**
     * @brief Remove the control (synchronization) blocks of the dataflow netlist.
     * @return True if changes have been produced, and false otherwise.
     */
    bool removeControl();

    /**
     * @brief Extract a set of marked graphs to achieve a certain
     * execution frequency coverage. The extraction is stopped when
     * the coverage is achieved or no more marked graphs are found.
     * @param coverage The target coverage.
     * @return The achieved coverage.
     */
    double extractMarkedGraphs(double coverage);

    /**
     * @brief Extract a set of marked graphs consisting of Basic Blocks to achieve a certain
     * execution frequency coverage. The extraction is stopped when
     * the coverage is achieved or no more marked graphs are found.
     * @param coverage The target coverage.
     * @return The achieved coverage.
     */
    double extractMarkedGraphsBB(double coverage);

    void printBlockSCCs();
    void computeSCCpublic(bool onlyMarked) {
        computeSCC(onlyMarked);
    }

    setBlocks allBlocks;       // Set of blocks (for iterators)
    setPorts allPorts;         // Set of ports (for iterators)
    setChannels allChannels;   // Set of channels (for iterators)

    BasicBlockGraph BBG;        // Graph of Basic Blocks


private:

    struct Block {
        blockID id;                 // Id of the block (redundant, but useful)
        std::string name;           // Name of the block
        BlockType type;             // Type of block
        longValueType value;        // Value (only used for constants)
        bool boolValue;             // Boolean value (only used for constants)
        blockID nextFree;           // Next free block in the vector of blocks
        bbID basicBlock;            // Basic block to which the block belongs to
        double delay;               // Delay of the block
        double delays[8];           // Delays of the block //Andrea
        int latency;                // Latency of the block (only for Operators)
        int II;                     // Initiation interval of the block (only for Operators)
        int slots;                  // Number of slots (only for EBs)
        bool transparent;           // Is the buffer transparent? (only for EBs)
        setPorts inPorts;           // Set of input ports (id's)
        setPorts outPorts;          // Set of output ports (id's)
        setPorts allPorts;          // All ports of the block
        portID portCond;            // Port for condition (for branch/select)
        portID portTrue, portFalse; // True and false ports (for branch/select)
        portID data;                // Port for data (input for branch/demux, output for select)
        portID srcCond;             // Port that generates the condition for the branch (beyond forks)
        double freq;                // Execution frequency (obtained from profiling)
        double frac;                // True/false fraction of select inputs (obtained from profiling)
        double retimingDiff;        // Axel
        bool mark;                  // Flag used for traversals
        int scc_number;             // SCC number
        int DFSorder;               // Post-visit number during DFS traversal
        std::deque<portID> listPorts;       // Temporary list of input ports (for demux pairing)
        std::map<portID,portID> demuxPairs; // Pairs of ports in a demux. The pairs are in both directions.
        blockID bbParent;           // Disjoint set parent for BB calculation
        int bbRank;                 // Disjoint set rank for BB calculation
        std::string operation;      // Lana: arithmetic/memory operation (instruction)
        int memPortID;              // Lana: used to connect load/store to MC/LSQ
        int memOffset;              // Lana: used to connect load/store to MC/LSQ
        int memBBCount;             // Lana: number of BBs connected to MC/LSQ
        int memLdCount;             // Lana: number of loads connected to MC/LSQ
        int memStCount;             // Lana: number of stores connected to MC/LSQ
        std::string memName;        // LSQ/MC name
        std::string funcName;       // Lana: name of called function (for call instruction)
        int fifoDepth;              // Lana: LSQ depth
        // Lana: LSQ json configuration info
        std::string numLoads;       
        std::string numStores;
        std::string loadOffsets;
        std::string storeOffsets;
        std::string loadPorts;
        std::string storePorts;
        int getptrc; // Lana: constant for getelementpointer dimensions
        map<bbID, vector<int>> orderings; //Axel : used by selector to know order of execution
    };

    struct Port {
        portID id;              // Identifier of the port
        std::string short_name; // Name of the port
        std::string full_name;  // Full name of the port (block:port)
        blockID block;          // Owner of the port
        portID nextFree;        // Next free slot in the vector of ports
        bool isInput;           // Direction of the port
        int width;              // Width of the port (0: control, 1: boolean, >1: data)
        double delay;           // Delay associated to the port
        PortType type;          // Type: generic, selection, sel_true or sel_false
        channelID channel;      // Channel connected to the port
        setPorts defs;          // Set of definitions (ports) reaching this port
        std::string memPortSuffix;  // Lana: MC/LSQ port suffix, to distinguish port types of MC/LSQ
    };

    struct Channel {
        channelID id;               // Identifier of the channel
        portID src;                 // Source port (output port)
        portID dst;                 // Destination port (input port)
        int slots;                  // Number of slots (used to insert Elastic Buffers)
        bool transparent;           // Is the buffer transparent? (used to insert Elastic Buffers)
        bool backEdge;              // Indicates whether it is a back edge (after calculation of BBs)
        channelID nextFree;         // Next free slot in the vector of channels
        bool mark;                  // Flag used for traversals
        double freq;                // number of times this channels is executed
        bool EB = false;            // boolean to set the presence of an elastic buffer on the channel // Carmine 21.02.22 
        bool channelMerge = false;  // boolean to set the presence of merge after channel during reduceMerges function // Carmine 09.03.22
    };

    // Structure to represent a fragment of a netlist.
    // It is used for extracting SCCs and Marked Graphs.
    struct subNetlist {
        setBlocks blocks;       // Set of blocks in the subnetlist
        setChannels channels;   // Set of channels in the subnetlist

        // Constructor
        subNetlist() {}

        // Copy
        subNetlist(subNetlist const& other) :
            blocks(other.blocks), channels(other.channels) {}

        // Assignment
        subNetlist& operator=(subNetlist other) {
            other.swap(*this);
            return *this;
        }

        // Swap
        void swap(subNetlist& other) {
            using std::swap;
            swap(blocks, other.blocks);
            swap(channels, other.channels);
        }

        // Resets the subnetlist
        void clear() {
            blocks.clear();
            channels.clear();
        }

        // Is it empty?
        bool empty() const {
            return channels.empty() and blocks.empty();
        }

        // Number of blocks of the subnetlist
        int numBlocks() const {
            return blocks.size();
        }

        // Number of channels of the subnetlist
        int numChannels() const {
            return channels.size();
        }

        // Returns the set of blocks (not modifiable)
        const setBlocks& getBlocks() const {
            return blocks;
        }

        // Returns the set of channels
        const setChannels& getChannels() const {
            return channels;
        }

        // Checks whether a block is in the subnetlist
        bool hasBlock(blockID b) const {
            return blocks.count(b) > 0;
        }

        // Checks whether a channel is in the subnetlist
        bool hasChannel(channelID c) const {
            return channels.count(c) > 0;
        }

        // Inserts a block into the subnetlist
        void insertBlock(blockID b) {
            blocks.insert(b);
        }

        // Inserts a channel into the subnetlist.
        // The blocks connected to the channel are also inserted.
        void insertChannel(const DFnetlist_Impl& dfn, channelID c) {
            channels.insert(c);
            insertBlock(dfn.getBlockFromPort(dfn.getSrcPort(c)));
            insertBlock(dfn.getBlockFromPort(dfn.getDstPort(c)));
        }
    };

    struct subNetlistBB {
        set <bbID> BasicBlocks;
        set <bbArcID> BasicBlockArcs;

        int SCC_no;
        int min_freq; //todo: for now we're ignoring this

        int DSU_no;
        int DSU_parent;
        int DSU_rank;

        subNetlistBB(){
            BasicBlocks = {};
            BasicBlockArcs = {};
            clear();
        }

        void clear(){
            BasicBlocks.clear();
            BasicBlockArcs.clear();
            SCC_no = 0;
            min_freq = 0;
            DSU_rank = -1;
            DSU_parent = -1;
            DSU_no = -1;
        }

        bool empty(){
            return BasicBlocks.empty() and BasicBlockArcs.empty();
        }

        int numBasicBlocks(){
            return BasicBlocks.size();
        }

        int numBasicBlockArcs() {
            return BasicBlockArcs.size();
        }

        const set<bbID>& getBasicBlocks() {
            return BasicBlocks;
        }

        const set<bbArcID>& getBasicBlockArcs() {
            return BasicBlockArcs;
        }

        bool hasBasicBlock(bbID b) {
            return BasicBlocks.count(b) > 0;
        }

        bool hasBasicBlockArc(bbArcID a) {
            return BasicBlockArcs.count(a) > 0;
        }

        void insertBasicBlock(bbID b) {
            BasicBlocks.insert(b);
        }

        void insertBasicBlockArc(bbArcID a) {
            BasicBlockArcs.insert(a);
        }

        void merge(subNetlistBB& other) {
            for (bbID bb: other.getBasicBlocks()) {
                insertBasicBlock(bb);
            }

            for (bbArcID bbArc: other.getBasicBlockArcs()) {
                insertBasicBlockArc(bbArc);
            }
        }

        bool sameSCC(subNetlistBB other) {
            return SCC_no == other.getSCCnumber();
        }

        int getSCCnumber() {
            return SCC_no;
        }

        int setSCCnumber(int number) {
            SCC_no = number;
        }

        int getMinimumFreq() {
            return min_freq;
        }

        void setMinimumFreq(int freq) {
            min_freq = freq;
        }

        int getDSUnumber() {
            return DSU_no;
        }

        void setDSUnumber(int number) {
            DSU_no = number;
        }

        bool haveCommonBasicBlock(subNetlistBB &other) {
            for (bbID bb: other.getBasicBlocks()) {
                if (hasBasicBlock(bb)) {
                    return true;
                }
            }
            return false;
        }

        int getDSUParent() {
            return DSU_parent;
        }

        void setDSUParent(int parent) {
            DSU_parent = parent;
        }

        int getDSURank() {
            return DSU_rank;
        }

        void setDSURank(int rank) {
            DSU_rank = rank;
        }

        void increaseDSURank() {
            DSU_rank++;
        }
    };

    std::string net_name;   // Name of the DF net
    int default_width;      // Default width for the channels
    ErrorMgr error;      // Error message
    int nBlocks;            // Number of blocks
    int nChannels;          // Number of channels
    int nPorts;             // Number of ports
    funcID nextFree;        // Next free slot in a library of functions

    std::vector<Block> blocks;       // List of blocks
    std::vector<Port> ports;         // List of ports
    std::vector<Channel> channels;   // List of channels

    double total_freq;

    //double delays [250][8];

    int freeBlock;      // Next free Block in the vector of blocks
    int freePort;       // Next free Port in the vector of ports
    int freeChannel;    // Next free Channel in the vector of channels

    //setBlocks allBlocks;       // Set of blocks (for iterators)
    //setPorts allPorts;         // Set of ports (for iterators)
    //setChannels allChannels;   // Set of channels (for iterators)

    setBlocks parameters;       // Set of paramenters of the dataflow netlist (entry)
    setBlocks results;          // Set of results of the dataflow netlist (exit)
    blockID entryControl;       // Entry block for control
    setBlocks exitControl;      // Exit blocks

    //BasicBlockGraph BBG;        // Graph of Basic Blocks
    bbID entryBB;               // Entry basic block

    std::string milpSolver;     // Name of the MILP solver

    std::map<std::string,blockID> name2block; // Map to obtain blocks from names
    std::map<std::string,portID> name2port;   // Map to obtain ports from names (string = "block:port")

    // Maps from/to blokcs/ports to strings
    static std::map<BlockType,std::string> BlockType2String;
    static std::map<std::string,BlockType> String2BlockType;
    static std::map<PortType,std::string> PortType2String;
    static std::map<std::string,PortType> String2PortType;

    std::vector<blockID> DFSorder;  // DFS order according to the post-visit ordering (tail is an entry node)
    std::vector<subNetlist> SCC;    // SCCs with at least one channel (in descending order of size)

    std::vector<subNetlist> MG;     // Extracted marked graphs in order of importance
    std::vector<double> MGfreq;     // Execution frequency of Marked Graphs

    setBlocks blocks_in_MGs;
    setChannels channels_in_MGs;

    setBlocks blocks_in_borders;
    setChannels channels_in_borders;

    setBlocks blocks_in_MC_LSQ;
    setChannels channels_in_MC_LSQ;

    std::vector<subNetlist> MG_disjoint; // All marked graphs that make the disjoint sets
    std::vector<double> MG_disjoint_freq;

    std::vector<subNetlistBB> CFDFC;
    std::vector<double> CFDFCfreq;

    // SHAB_note: should be change this to a map?
    std::vector<subNetlistBB> CFDFC_disjoint;
    std::vector<double> CFDFC_disjoint_freq;

    std::vector<vector<int> > components;  // each index corresponds to the CFDFCs the DSU_CFDFC is built of

    // Structure to store the MILP variables
    // Note: the out_retime variables are only used for pipelined units (latency > 0)
    struct milpVarsEB {
        vector<int> buffer_flop;        // Elastic buffer to cut combinational paths
        vector<int> buffer_flop_valid;        //Carmine 17.02.22 // Elastic buffer to cut combinational valid paths
        vector<int> buffer_flop_ready;        //Carmine 17.02.22 // Elastic buffer to cut combinational ready paths
        vector<int> buffer_slots;       // Number of slots of the elastic buffer
        vector<int> has_buffer;         // Boolean variable to indicate whether there is a buffer
        vector<int> time_path;          // Combinational arrival time for ports
        vector<int> time_valid_path;    //Carmine 16.02.22 // Combinational arrival time for valid ports
        vector<int> time_ready_path;    //Carmine 17.02.22 // Combinational arrival time for ready ports
        vector<int> time_elastic;       // Arrival time for elasticity (longest rigid fragment)
        vector<vector<int>> in_retime_tokens;  // Input retiming variables for tokens (indices: [MarkedGraph,block])
        vector<vector<int>> out_retime_tokens; // Output retiming variables for tokens (indices: [MarkedGraph,block]).
        vector<vector<int>> retime_bubbles;    // Retiming variables for bubbles (indices: [MarkedGraph,block])
        vector<vector<int>> th_tokens;  // Throughput associated to every channel for tokens (indices: [MargedGraph, channel])
        vector<vector<int>> th_bubbles; // Throughput associated to every channel for bubbles (indices: [MargedGraph, channel])
        vector<int> th_MG;              // Throughput variables (one for each marked graph)
    };

    /**
     * @brief Initializes the DF netlist
     */
    void init();

    /**
     * @return The size of the vector of blocks.
     */
    int vecBlocksSize() const {
        return blocks.size();
    }

    /**
     * @return The size of the vector of channels.
     */
    int vecChannelsSize() const {
        return channels.size();
    }

    /**
     * @return The size of the vector of ports.
     */
    int vecPortsSize() const {
        return ports.size();
    }

    /**
     * @brief Reads a dataflow netlist in dot format.
     * @param filename Name of the file.
     * @return True if no errors, and false otherwise.
     * @note In case a false is returned and there are no errors,
     * it indicates that there was no graph specification.
     */
    bool readDataflowDot(const std::string& filename);

    /**
     * @brief Reads a dataflow netlist in dot format.
     * @param f File descriptor.
     * @return True if no errors, and false otherwise.
     * @note In case a false is returned and there are no errors,
     * it indicates that there was no graph specification.
     */
    bool readDataflowDot(FILE* f);

    bool readDataflowDotBB(FILE *f);
    bool readDataflowDotBB(const std::string& filename);

    /**
     * @brief Generates a fresh name for a block.
     * @param type Type of the block.
     * @return A string with the name of the block.
     */
    std::string genBlockName(BlockType type) const;

    /**
     * @brief Check that the ports of a block are connected.
     * If the id is invalidDataflowID, all ports of the netlist are checked.
     * @param id Identifier of the block (eventually invalidDataflowID.
     * @return True if all ports connected, and false otherwise.
     * @note In case of error, an error message is generated.
     */
    bool checkPortsConnected(blockID id = invalidDataflowID);

    /**
     * @brief Returns the number of input ports of a block.
     * @param b Block identifier.
     * @return The number of input ports of the block.
     */
    int numInPorts(blockID b) const {
        assert(validBlock(b));
        return blocks[b].inPorts.size();
    }

    int getBlockSCCno(blockID b) const {
        assert(validBlock(b));
        return blocks[b].scc_number;
    }
    /**
     * @brief Returns the number of output ports of a block.
     * @param b Block identifier.
     * @return The number of output ports of the block.
     */
    int numOutPorts(blockID b) const {
        assert(validBlock(b));
        return blocks[b].outPorts.size();
    }

    /**
     * @brief Returns one of the input ports of the block.
     * @param b Block identifier.
     * @return The port ID or invalidPort if there are no input ports.
     */
    portID getInPort(blockID b) const {
        if (blocks[b].inPorts.empty()) return invalidDataflowID;
        return *(blocks[b].inPorts.begin());
    }

    /**
    * @brief Returns one of the output ports of the block.
    * @param b Block identifier.
    * @return The port ID or invalidPort if there are no output ports.
    */
    portID getOutPort(blockID b) const {
        if (blocks[b].outPorts.empty()) return invalidDataflowID;
        return *(blocks[b].outPorts.begin());
    }

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkElasticBuffer(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkFork(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkBranch(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkMerge(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkSelect(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkFunc(blockID b);

    /**
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkConstant(blockID b);

    /**
     * @brief Checks that the Entry block is correct.
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkFuncEntry(blockID b);

    /**
     * @brief Checks that the Exit block is correct.
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkFuncExit(blockID b);

    /**
     * @brief Checks that a Demux block is correct.
     * @param b Block identifier.
     * @return True if the block is correct, and false otherwise.
     */
    bool checkDemux(blockID b);

    /**
     * @brief Checks that all input/output ports of the block are generic.
     * @param b Block identifier.
     * @return True if all generic, and false otherwise.
     */
    bool checkGenericPorts(blockID b);

    /**
     * @brief Infers the width of the channels.
     * @param default_width The default width for those that have undefined width.
     */
    void inferChannelWidth(int default_width);

    /**
     * @brief Checks whether two ports have the same width.
     * @param p1 First port.
     * @param p2 Second port.
     * @return True if they have the same width, and false otherwise.
     * @note An error is generated in case they have different width.
     */
    bool checkSamePortWidth(portID p1, portID p2);

    /**
     * @brief Checks whether all ports in a set have the same width.
     * @param P set of ports.
     * @return True if they have the same width, and false otherwise.
     * @note An error is generated in case they have different width.
     */
    bool checkSamePortWidth(const setPorts& P);

    /**
     * @brief Check that a block has a certain number of input/output ports.
     * If the value of the parameter is -1, no checking is done.
     * @param b Block identifier.
     * @param nin Number of input ports.
     * @param nout Number of output ports.
     * @return True if the number of ports is correct, and false otherwise.
     */
    bool checkNumPorts(blockID b, int nin, int nout);

    bool setAllBBFrequencies();
    bool determineEntryExitBlock();
    void computeChannelFrequencies();
    void computeBlockFrequencies();

    /**
    * @brief Writes the information of a block in dot format.
    * @param s The output stream.
    * @param id The block id.
    */
    void writeBlockDot(std::ostream& s, blockID id);

    /**
    * @brief Writes the information of a channel in dot format.
    * @param s The output stream.
    * @param id The channel id.
    */
    void writeChannelDot(std::ostream& s, channelID id);

    //SHAB_note: implement these
    void writeBasicBlockDot(std::ostream& s, bbID id);
    void writeArcDot(std::ostream& s, bbArcID id);

    /**
     * @brief Defines the mark of all blocks to a certain value.
     * @param value Value of the mark.
     */
    void markAllBlocks(bool value) {
        ForAllBlocks(b) blocks[b].mark = value;
    }

    /**
     * @brief Defines the mark of all channels to a certain value.
     * @param value Value of the mark.
     */
    void markAllChannels(bool value) {
        ForAllChannels(c) channels[c].mark = value;
    }

    /**
     * @brief Marks a block with a specific value.
     * @param b Id of the block.
     * @param value Value of the mark;
     */
    void markBlock(blockID b, bool value) {
        assert(validBlock(b));
        blocks[b].mark = value;
    }

    /**
     * @brief Defines the DFS order (post-visit) of the DFS traversal.
     * @param b Id of the block.
     * @param value Value of DFS order.
     */
    void setDFSorder(blockID b, int value) {
        assert(validBlock(b));
        blocks[b].DFSorder = value;
    }

    /**
     * @param b Id of the block.
     * @return the DFS order of the block (post-visit).
     */
    int getDFSorder(blockID b) const {
        assert(validBlock(b));
        return blocks[b].DFSorder;
    }

    /**
     * @brief Marks a channel with a specific value.
     * @param c Id of the channel.
     * @param value Value of the mark;
     */
    void markChannel(channelID c, bool value) {
        assert(validChannel(c));
        channels[c].mark = value;
    }

    /**
     * @brief Checks whether a block is marked.
     * @param b Id of the block.
     * @return True if the block is marked, and false otherwise.
     */
    bool isBlockMarked(blockID b) const {
        assert(validBlock(b));
        return blocks[b].mark;
    }

    /**
     * @brief Checks whether a channel is marked.
     * @param c Id of the channel.
     * @return True if the channel is marked, and false otherwise.
     */
    bool isChannelMarked(channelID c) const {
        assert(validChannel(c));
        return channels[c].mark;
    }

    /**
     * @brief Defines the back-edge property of the channel. This is defined
     * by the methods that calculate BBs.
     * @param c Id of the channel.
     * @param value True if the edge must be a back edge, and false otherwise.
     */
    void setBackEdge(channelID c, bool value = true) {
        assert(validChannel(c));
        channels[c].backEdge = value;
    }

    /**
     * @brief Check whether a channel is a back edge.
     * @param c Id of the channel.
     * @return True if it is a back edge, and false otherwise.
     */
    bool isBackEdge(channelID c) const {
        assert(validChannel(c));
        return channels[c].backEdge;
    }

    /**
     * @brief Executes a DFS traversal and defines DFSorder.
     * @param forward If asserted, DFS is executed in the forward direction,
     * otherwise DFS is executed in the backward direction.
     * @param onlyMarked If asserted, DFS only visits the blocks and channels
     * that are marked. Otherwise, all blocks and channels are visited.
     * @note Only the marked blocks and channels are visited.
     */
    void DFS(bool forward = true, bool onlyMarked = false);

    /**
     * @brief Caldulates the back edges of the dataflow netlist by doing
     * a DFS traversal from the entry nodes (based on Compiler's theory).
     */
    void calculateBackEdges();

    /**
      * @brief Computes the SCCs of the netlist. The SCCs with more than one block
      * are stored in the SCC vector in descending order of size.
      * @param onlyMarked If asserted, it only visits the blocks and channels
      * that are marked. Otherwise, all blocks and channels are visited.
      * @note The method recalculates the DFS order of all blocks.
      */
    void computeSCC(bool onlyMarked = false);

    /**
     * @return The name of the MILP solver used for optimization problems.
     */
    const std::string& getMilpSolver() const {
        return milpSolver;
    }

    /**
     * @brief Creates the variables of an MILP model for the insertion of buffers.
     * @param milp MILP model.
     * @param vars Structure storing the MILP variables.
     * @param max_throughput Indicates whether the variables for throughput must be created.
     */
    void createMilpVarsEB(Milp_Model& milp, milpVarsEB& vars, bool max_throughput, bool first_MG= false);
    void createMilpVarsEB_sc(Milp_Model& milp, milpVarsEB& vars, bool max_throughput, int mg, bool first_MG= false, string model_mode="default");
    void createMilpVars_remaining(Milp_Model& milp, milpVarsEB& vars, string model_mode);

    /**
     * @brief Creates the path constraints for the MILP model.b The constraints
     * ensure that no combinational path will be longer than the period.
     * @param milp MILP model.
     * @param Vars Structure that contains the MILP variables.
     * @param Period Cycle Period for the path constraints (ignored if Period <= 0).
     * @param BufferDelay Clk-Q delay of the elastic buffer.
     * @return True if successful, and false otherwise (e.g. unsatisfiable constraints).
     */
    bool createPathConstraints(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay);
    bool createPathConstraints_sc(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay, int mg);
    bool createPathConstraintsOthers_sc(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay, int mg, string model_mode, string lib_path);
    bool createPathConstraints_remaining(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay);
    bool createPathConstraintsOthers_remaining(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay, string model_mode, string lib_path);


    vector< pair<string, vector<double>>> read_lib_file(string lib_file_path); //Carmine 18.02.22 function to read block delays values from library
    /**
     * @brief Creates the elasticity constraints for the MILP model. The constraints
     * ensure that every cycle will have one elastic buffer at least (maybe transparent).
     * @param milp MILP model.
     * @param Vars Structure that contains the MILP variables.
     * @return True if successful, and false otherwise (e.g. unsatisfiable constraints).
     * @note This method must be called after createPathConstraints.
     */
    bool createElasticityConstraints(Milp_Model& milp, milpVarsEB& Vars);
    bool createElasticityConstraints_sc(Milp_Model& milp, milpVarsEB& Vars, int mg);
    bool createElasticityConstraints_remaining(Milp_Model& milp, milpVarsEB& Vars);

    /**
     * @brief Creates the throughput constraints for the MILP model.
     * @param milp MILP model.
     * @param Vars Structure that contains the MILP variables.
     * @return True if successful, and false otherwise (e.g. unsatisfiable constraints).
     * @note This method must be called after createPathConstraints and after extracting
     * the most frequent Marked Graphs of the netlist.
     */
    bool createThroughputConstraints(Milp_Model& milp, milpVarsEB& Vars, bool first_MG= false);
    bool createThroughputConstraints_sc(Milp_Model& milp, milpVarsEB& Vars, int mg, bool first_MG= false);

    bool channelIsInMGs(channelID c);
    bool blockIsInMGs(blockID b);

    bool channelIsInBorders(channelID c);
    bool blockIsInBorders(blockID b);

    bool channelIsInMCLSQ(channelID c);
    bool blockIsInMCLSQ(blockID b);

    bool channelIsCovered(channelID c, bool MG, bool borders, bool MC_LSQ);
    bool blockIsCovered(blockID b, bool MG, bool borders, bool MC_LSQ);
    /**
     * @brief Writes details of the MILP solution into cout (for debugging purposes).
     * @param milp The MILP model.
     * @param vars The set of variables of the MILP model.
     */
    void dumpMilpSolution(const Milp_Model& milp, const milpVarsEB& vars) const;

    void writeRetimingDiffs(const Milp_Model& milp, const milpVarsEB& vars);

    /**
     * @brief Makes some buffers non-transparent to cut combinational cycles.
     * @note This function should be rarely invoked. It is only necessary when
     * delays are not specified in the netlist and zero-delay cycles can occur
     * (the MILP model does not know how to handle them properly). This function
     * should be called as a sanity check in case some zero-delay cycle is present.
     */
    void makeNonTransparentBuffers();

    /**
     * @brief Removes those blocks that are unreachable. A block is unreachable
     * if it has no inputs (except for constants and entry blocks) or it has no outputs
     * (except for operators and exit blocks). The unused ports are also removed.
     * @return True if some change has been produced, and false otherwise.
     */
    bool removeUnreachableBlocksAndPorts();

    /**
     * @brief Remove branch and select blocks that have a constant condition.
     * @return True if some change has been produced, and false otherwise.
     */
    bool removeConstantBranchSelect();

    /**
     * @brief Removes 1-input/1-output merge and fork blocks. It also removes
     * demuxes with only one outpot port (it menas that it has only one input
     * sync port).
     * @return True if some changes have been produced, and false otherwise.
     */
    bool removeOneInOneOutMergeForkDemux();

    /**
     * @brief Collapses adjacent merge/fork blocks.
     * @return True if some changes have been produced, and false otherwise.
     */
    bool collapseMergeFork();

    /**
     * @brief Makes one set for the disjoint sets of BBs
     * @param b block identifier
     */
    void makeDisjointBB(blockID b);

    /**
     * @brief Find the representative of one block.
     * @param b Block identifier.
     * @return The representative of the block.
     */
    blockID findDisjointBB(blockID b);

    /**
     * @brief Makes the union of two disjoint sets for the calculation of BBs
     * @param b1 The first block.
     * @param b2 The second block.
     * @return True if a real union took place, and false if the blocks already
     * were in the same set.
     */
    bool unionDisjointBB(blockID b1, blockID b2);

    /**
     * @brief Invalidates the information about basic blocks.
     * This is required when the netlist is modified.
     */
    void invalidateBasicBlocks();

    /**
     * @brief Calculates the set of definitions reaching each port.
     * @note Each port receives the set of ports that define some data.
     * The defs are generated in operators, elastic buffers, function entries
     * and constants. The defs are propagated through merge, fork, branch,
     * demux and select blocks.
     */
    void calculateDefinitions();

    /**
     * @brief Extracts one marked graph from the dataflow netlist maximizing the
     * execution frequency of the components.
     * @param freq A map indicating the execution frequency of the blocks.
     * @return A set of channels that identify the extracted marked graph.
     */
    subNetlist extractMarkedGraph(const map<blockID, double>& freq);

    /**
     * @brief Extracts one marked graph from the control flow netlist maximizing the
     * execution frequency of the components.
     * @param freq A map indicating the execution frequency of the Basic Blocks.
     * @return A set of channels that identify the extracted marked graph.
     */
    subNetlistBB extractMarkedGraphBB(const map<bbID, double>& freq);

    /**
     * @brief Calculates the blocks and channels corresponding to a graph of Basic Blocks and Arcs.
     * @param BB_CFDFC A subNetlistBB of BBs and Arcs.
     * @return A subNetlist of blocks and channels between them.
     */
    subNetlist getMGfromCFDFC(subNetlistBB &BB_CFDFC);

    void calculateDisjointCFDFCs();
    int findCFDFC(int i);
    void unionCFDFC(int x, int y);

    void makeMGsfromCFDFCs();


    /**
     * @brief Sets a unit delay for each block.
     * @note This method should only be used for debugging purposes.
     */
    void setUnitDelays();

    bool removeFakeBranches();

public :
    //added for convenience, probably remove later
    std::vector<Block> & getBlocks(){
    	return blocks;       // List of blocks
    }
    std::vector<Port> & getPorts(){
    	return ports;// List of ports
    }
    std::vector<Channel> & getChannels(){
    	return channels;// List of channels
    }
};

class DFlib_Impl
{
public:

    /**
     * @brief Default constructor (empty library).
     */
    DFlib_Impl() {
        init();
    }

    /**
     * @brief Constructor from a file.
     * @param filename Name of the file (dot format).
     */
    DFlib_Impl(const string& filename);

    /**
     * @brief Adds new functions to the library.
     * @param filename Name of the file (dot format).
     * @return True if successful, and false if an error is produced.
     */
    bool add(const string& filename);

    /**
     * @brief Writes the dataflow library in dot format.
     * @param filename The name of the file. In case the filename is empty,
     * it is written into cout.
     * @return true if successful, and false otherwise.
     */
    bool writeDot(const std::string& filename = "");

    /**
     * @brief Writes the dataflow library in dot format.
     * @param s The output stream.
     * @return True if successful, and false otherwise.
     */
    bool writeDot(std::ostream& s);

    /**
     * @brief Sets an error message for the library
     * @param err The error message
     */
    void setError(const std::string& err) {
        error.set(err);
    }

    /**
    * @return The error message associated to the netlist.
    */
    const std::string& getError() const {
        return error.get();
    }

    /**
    * @return The error message associated to the netlist.
    */
    std::string& getError() {
        return error.get();
    }

    /**
     * @brief Erases the error.
     */
    void clearError() {
        error.clear();
    }

    /**
     * @brief Indicates whether there is an error.
     * @return True if there is an error and false otherwise.
     */
    bool hasError() const {
        return error.exists();
    }

    /**
     * @return The number of functions of the library.
     */
    int numFuncs() const {
        return nFuncs;
    }

    /**
     * @brief Gets a funcID from a name function.
     * @param fname Name of the function.
     * @return The funcID (invalidDataflowID if it does not exist).
     */
    funcID getFuncID(const string& fname) const {
        auto it = name2func.find(fname);
        if (it == name2func.end()) return invalidDataflowID;
        return it->second;
    }

    /**
     * @brief Returns a reference to the dataflow netlist of a function.
     * @param fname Name of the function.
     * @return A reference to the dataflow netlist of the function.
     */
    DFnetlist& operator[](const string& fname) {
        funcID id = getFuncID(fname);
        assert (id != invalidDataflowID);
        return funcs[id];
    }

    /**
     * @brief Returns a const reference to the dataflow netlist of a function.
     * @param fname Name of the function.
     * @return A const reference to the dataflow netlist of the function.
     */
    const DFnetlist& operator[](const string& fname) const {
        funcID id = getFuncID(fname);
        assert (id != invalidDataflowID);
        return funcs[id];
    }

    /**
     * @brief Returns a reference to the dataflow netlist of a function.
     * @param id Id of the function.
     * @return A reference to the dataflow netlist of the function.
     */
    DFnetlist& operator[](funcID id) {
        assert (allFuncs.count(id) > 0);
        return funcs[id];
    }

    /**
     * @brief Returns a const reference to the dataflow netlist of a function.
     * @param id Id of the function.
     * @return A const reference to the dataflow netlist of the function.
     */
    const DFnetlist& operator[](funcID id) const {
        assert (allFuncs.count(id) > 0);
        return funcs[id];
    }

private:
    std::string libName;    // Name of the DF library
    ErrorMgr error;         // Error manager
    int nFuncs;             // Number of functions (DF netlists)
    int freeDF;             // Next free slot in the vector of functions

    vector<DFnetlist> funcs;        // Vector of functions (index corresponds to funcID)
    map<string, funcID> name2func;  // Map from function name to funcID
    set<funcID> allFuncs;           // Set of all valid funcID's

    /**
     * @brief Initializes the library
     */
    void init();
};

}

#endif // DATAFLOW_IMPL_H
