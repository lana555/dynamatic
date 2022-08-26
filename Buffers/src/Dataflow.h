#ifndef DATAFLOW_H
#define DATAFLOW_H

#include <list>
#include <set>
#include <string>
#include <vector>

namespace Dataflow
{

// Identifiers for the main objects of a dataflow system.
// Functions, blocks, channels, ports and basic blocks are represented by a unique identifier.
// (integers starting from 0). Each type has a different space of identifiers.
// Eventually, the range of id's can be sparse, i.e., when functions, blocks,
// channels and ports are removed, the original id's do not change.

const int invalidDataflowID = -1;

using funcID = int;
using blockID = int;
using channelID = int;
using portID = int;

// Collections of dataflow objects

using vecFuncs = std::vector<funcID>;
using setFuncs = std::set<funcID>;
using listFuncs = std::list<funcID>;
using vecBlocks = std::vector<blockID>;
using setBlocks = std::set<blockID>;
using listBlocks = std::list<blockID>;
using vecPorts = std::vector<portID>;
using setPorts = std::set<portID>;
using listPorts = std::list<portID>;
using vecChannels = std::vector<channelID>;
using setChannels = std::set<channelID>;
using listChannels = std::list<channelID>;

/// Types of blocks.
/// This is an enumerated type with all types of dataflow components.
/// The last component should never be used in a dataflow system.
enum BlockType {OPERATOR,
                ELASTIC_BUFFER,
                FORK,
                BRANCH,
                MERGE,
                DEMUX,
                SELECT,
                CONSTANT,
                FUNC_ENTRY,
                FUNC_EXIT,
                SOURCE,
                SINK,
        		MUX,
        		CNTRL_MG,
        		LSQ, 
        		MC,
				DISTRIBUTOR,
				SELECTOR,
                UNKNOWN
               };

/// Type of port. Most ports will be generic. The other types are used for
/// branch and select blocks. A SELECTION_PORT must be boolean and is used
/// to select one of the branches (true or false). TRUE_PORT and FALSE_PORT
/// are the ports selected by the SELECTION_PORT.
enum PortType {GENERIC_PORT,    // Most ports will be generic
               SELECTION_PORT,  // Selection port for branch/select/demux blocks
               TRUE_PORT,       // True port for branch and select blocks
               FALSE_PORT       // False port for branch and select blocks
              };

/// Direction of a list of ports (mostly used for iterators)
enum PortDirection {INPUT_PORTS,
                    OUTPUT_PORTS,
                    ALL_PORTS
                   };

class DFnetlist_Impl;
class DFlib_Impl;

class DFnetlist
{

public:
	  DFnetlist_Impl* DFI;    /// Implementation of DFnet;

    /**
     * @brief Default constructor.
     */
    DFnetlist();

    /**
      * @brief Default destructor.
      */
    ~DFnetlist();

    /**
    * @brief Constructor. It reads a netlist from a file.
    * @param name Filename of the input description.
    */
    DFnetlist(const std::string& name);

    /**
    * @brief Constructor. It creates a netlist from a file descriptor.
    * The file is assumed to be opened.
    * @param file File descriptor.
    */
    DFnetlist(FILE* file);

    /**
   * Shabnam:
  * @brief Constructor. It creates a netlist from two files.
   * first file consists of blocks.
   * second file consists of basic blocks.
  * @param file File name of the input descriptions.
  */
    DFnetlist(const std::string& name, const std::string& name_bb);

    /**
     * @brief Copy constructor.
     */
    DFnetlist(const DFnetlist& other);

    /**
     * @brief Sets the solver for MILP optimization problems.
     * @param solver Name of the solver (usually cbc or glpsol).
     */
    void setMilpSolver(const std::string& solver = "cbc");

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
     * @brief Returns the delay of a block.
     * @param id Identifier of the block.
     * @return The delay of the block.
     */
    double getBlockDelay(blockID id, int indx) const;

    /**
     * @brief Sets the delay of a block.
     * @param id Identifier of the block.
     * @param d Delay of the block.
     */
    void setBlockDelay(blockID id, double d, int indx);

    double getBlockRetimingDiff(blockID id) const;

    void setBlockRetimingDiff(blockID id, double retimingDiff);


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

    double getTrueFrac(blockID id) const;

    /**
            * @brief Returns the number of slots of an elastic buffer.
    * @param id Identifier of the block.
    * @return The number of slots of the buffer.
    */
    int getBufferSize(blockID id) const;

    /**
     * @brief Sets the number of slots of an elastic buffer.
     * @param id Identifier of the block.
     * @param ii Number of slots of the buffer.
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
     * @param name Name of the port.
     * @param isInput Direction of the port.
     * @param width The width of the port.
     * @param type Type of the port (generic, select, true, false).
     * @return The port id. It returns an invalidPort in case of error.
     * @note If the name is empty, a new fresh name is created.
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
     * @brief Returns the set of ports with a certain direction.
     * @param id Block id (the owner of the ports).
     * @param dir Direction of the ports (input/output/all).
     * @return The set of ports.
     */
    const setPorts& getPorts(blockID id, PortDirection dir) const;

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
     * @return The data port of a demux.
     */
    portID getDemuxDataPort(blockID id) const;

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
     * @brief Returns the block type associated to a block
     * @param block The block from which we want to obtain the type
     * @return The block type
     */
    BlockType getBlockType(blockID block) const;

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
     * @return The channel id. It returns invalidChannel in case of an error.
     */
    channelID createChannel(portID src, portID dst);

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
     * @brief Removes a channel.
     * @param id Id of the channel.
     */
    void removeChannel(channelID id);

    /**
     * @brief Returns the name of the channel.
     * @param id Identifier of the channel.
     * @param full If asserted, a block:port -> block:port description
     * is generated. Otherwise, a block -> block description is generated.
     * @return The string representing the name of the port.
     */
    std::string getChannelName(channelID id, bool full=true) const;

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
     */
    void removeBuffer(blockID buf);

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
    bool writeDot(std::ostream& s);

    bool writeDotMG(const std::string& filename = "");
    bool writeDotMG(std::ostream& s);

    bool writeDotBB(std::ostream& s);
    bool writeDotBB(const std::string& filename = "");

    /**
     * @brief Writes the Basic Blocks of the dataflow netlist in dot format.
     * @param filename The name of the file. In case the filename is empty,
     * it is written into cout.
     * @return true if successful, and false otherwise.
     */
    bool writeBasicBlockDot(const std::string& filename = "");

    /**
     * @brief Writes the Basic Blocks of the dataflow netlist in dot format.
     * @param s The output stream.
     * @return True if successful, and false otherwise.
     */
    bool writeBasicBlockDot(std::ostream& s);

    /**
     * @brief Adds elastic buffers to meet a certain cycle period and
     * have elasticity in the system.
     * @param Period Target cycle Period (ignored if Period <= 0).
     * @param BufferDelay CLK-to-Q delay of the Elastic Buffer.
     * @param maxThroughput If asserted, it maximizes the throghput of the system.
     * @return True if successful, and false otherwise.
     */
    bool addElasticBuffers(double Period = 0, double BufferDelay = 0, bool maxThroughput = false, double coverage = 0);

    bool addElasticBuffersBB(double Period = 0, double BufferDelay = 0, bool maxThroughput = false, double coverage = 0, int timeout = -1, bool first_MG = false);

    bool addElasticBuffersBB_sc(double Period = 0, double BufferDelay = 0, bool maxThroughput = false, double coverage = 0, int timeout = -1, bool first_MG = false, const std::string& model_mode = "default", const std::string& lib_path="");

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
     * @brief Removes all non-SCC blocks and channels.
     * @return The netlist with only the SCCs.
     */
    void eraseNonSCC();

    void computeSCC(bool onlyMarked);

    void printBlockSCCs();

    /**
     * @brief Calculates the Basic Blocks of the netlist.
     * @return True if no error occurred, and false otherwise.
     */
    bool calculateBasicBlocks();

    /**
    * @brief Performs different types of optimizations to reduce
    * the complexity of the dataflow netlist.
    * @return True if changes have been produced, and false otherwise.
    */
    bool optimize();

    /**
     * @brief Remove the control (synchronization) blocks of the dataflow netlist.
     * @return True if changes have been produced, and false otherwise.
     */
    bool removeControl();

    /**
     * @brief Extracts a set of marked graphs from the netlist
     * until a certain coverage of execution frequency is achieved.
     * @param coverage The target coverage.
     * @return The achieved coverage.
     */
    double extractMarkedGraphs(double coverage);
};

class DFlib
{

    DFlib_Impl* DFLI;    /// Implementation of DFlib;

public:
    /**
     * @brief Default constructor.
     */
    DFlib();

    /**
      * @brief Deafult destructor.
      */
    ~DFlib();

    /**
    * @brief Constructor. It creates dataflow library from a file (dot format).
    * @param name Name of the file.
    */
    DFlib(const std::string& name);


    /**
     * @brief Adds new functions to the library.
     * @param filename Name of the file (dot format).
     * @return True if successful, and false if an error is produced.
     */
    bool add(const std::string& filename);

    /**
     * @brief Writes the dataflow library in dot format.
     * @param filename The name of the file. In case the filename is empty,
     * it is written into cout.
     * @return true if successful, and false otherwise.
     */
    bool writeDot(const std::string& filename = "");

    /**
     * @brief Sets an error message for the library
     * @param err The error message
     */
    void setError(const std::string& err);

    /**
    * @return The error message associated to the netlist.
    */
    const std::string& getError() const;

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
     * @return The number of functions of the library.
     */
    int numFuncs() const;

    /**
     * @brief Gets a funcID from a name function.
     * @param fname Name of the function.
     * @return The funcID (invalidDataflowID if it does not exist).
     */
    funcID getFuncID(const std::string& fname) const;

    /**
     * @brief Returns a reference to the dataflow netlist of a function.
     * @param fname Name of the function.
     * @return A reference to the dataflow netlist of the function.
     */
    DFnetlist& operator[](const std::string& fname);

    /**
     * @brief Returns a const reference to the dataflow netlist of a function.
     * @param fname Name of the function.
     * @return A const reference to the dataflow netlist of the function.
     */
    const DFnetlist& operator[](const std::string& fname) const;

    /**
     * @brief Returns a reference to the dataflow netlist of a function.
     * @param id Id of the function.
     * @return A reference to the dataflow netlist of the function.
     */
    DFnetlist& operator[](funcID id);

    /**
     * @brief Returns a const reference to the dataflow netlist of a function.
     * @param id Id of the function.
     * @return A const reference to the dataflow netlist of the function.
     */
    const DFnetlist& operator[](funcID id) const;
};

}
#endif // DATAFLOW_H
