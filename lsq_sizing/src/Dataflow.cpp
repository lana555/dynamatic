#include "Dataflow.h"
#include "DFnetlist.h"

using namespace std;
using namespace Dataflow;

// Constructors for the objects of DFnetlist
DFnetlist::DFnetlist() : DFI(new DFnetlist_Impl()) {}

DFnetlist::DFnetlist(const string& name) : DFI(new DFnetlist_Impl(name)) {}

DFnetlist::DFnetlist(FILE* file) : DFI(new DFnetlist_Impl(file)) {}

DFnetlist::DFnetlist(const string &name, const string &name_bb) : DFI(new DFnetlist_Impl(name, name_bb)) {}

DFnetlist::DFnetlist(const DFnetlist& other) : DFI(new DFnetlist_Impl(*(other.DFI))) {}

// Deconstructor
DFnetlist::~DFnetlist()
{
    delete DFI;
}

// Function definitions 
bool DFnetlist::check()
{
    return DFI->check();
}

const string& DFnetlist::getName() const
{
    return DFI->getName();
}

blockID DFnetlist::createBlock(BlockType type, const string& name)
{
    return DFI->createBlock(type, name);
}

blockID DFnetlist::getBlock(const string& name) const
{
    return DFI->getBlock(name);
}

const string& DFnetlist::getBlockName(blockID id) const
{
    return DFI->getBlockName(id);
}

double DFnetlist::getBlockDelay(blockID id) const
{
    return DFI->getBlockDelay(id);
}

void DFnetlist::setBlockDelay(blockID id, double d)
{
    DFI->setBlockDelay(id, d);
}

int DFnetlist::getLatency(blockID id) const
{
    return DFI->getLatency(id);
}

void DFnetlist::setLatency(blockID id, int lat)
{
    DFI->setLatency(id, lat);
}

int DFnetlist::getInitiationInterval(blockID id) const
{
    return DFI->getInitiationInterval(id);
}

void DFnetlist::setInitiationInterval(blockID id, int ii)
{
    DFI->setInitiationInterval(id, ii);
}

int DFnetlist::getExecutionFrequency(blockID id) const
{
    return DFI->getExecutionFrequency(id);
}

void DFnetlist::setExecutionFrequency(blockID id, double freq)
{
    DFI->setExecutionFrequency(id, freq);
}

void DFnetlist::setTrueFrac(blockID id, double freq)
{
    DFI->setTrueFrac(id, freq);
}

double DFnetlist::getTrueFrac(blockID id) const
{
    return DFI->getTrueFrac(id);
}

int DFnetlist::getBufferSize(blockID id) const
{
    return DFI->getBufferSize(id);
}

void DFnetlist::setBufferSize(blockID id, int slots)
{
    DFI->setBufferSize(id, slots);
}

bool DFnetlist::isBufferTransparent(blockID id) const
{
    return DFI->isBufferTransparent(id);
}

void DFnetlist::setBufferTransparency(blockID id, bool value)
{
    DFI->setBufferTransparency(id, value);
}

blockID DFnetlist::getBlockFromPort(portID p) const
{
    return DFI->getBlockFromPort(p);
}

void DFnetlist::removeBlock(blockID block)
{
    return DFI->removeBlock(block);
}

portID DFnetlist::createPort(blockID block, bool isInput, const string& name, int width, PortType type)
{
    return DFI->createPort(block, isInput, name, width, type);
}

void DFnetlist::removePort(portID port)
{
    DFI->removePort(port);
}

portID DFnetlist::getPort(blockID block, const string& name) const
{
    return DFI->getPort(block, name);
}

const string& DFnetlist::getPortName(portID port, bool full) const
{
    return DFI->getPortName(port, full);
}

PortType DFnetlist::getPortType(portID port) const
{
    return DFI->getPortType(port);
}

bool DFnetlist::isInputPort(portID port) const
{
    return DFI->isInputPort(port);
}

bool DFnetlist::isOutputPort(portID port) const
{
    return DFI->isOutputPort(port);
}

int DFnetlist::getPortWidth(portID port) const
{
    return DFI->getPortWidth(port);
}

bool DFnetlist::isBooleanPort(portID port) const
{
    return DFI->isBooleanPort(port);
}

bool DFnetlist::isControlPort(portID port) const
{
    return DFI->isControlPort(port);
}

double DFnetlist::getPortDelay(portID port) const
{
    return DFI->getPortDelay(port);
}

void DFnetlist::setPortDelay(portID port, double d)
{
    DFI->setPortDelay(port, d);
}

const setPorts& DFnetlist::getPorts(blockID id, PortDirection dir) const
{
    return DFI->getPorts(id, dir);
}

portID DFnetlist::getConditionalPort(blockID id) const
{
    return DFI->getConditionalPort(id);
}

portID DFnetlist::getTruePort(blockID id) const
{
    return DFI->getTruePort(id);
}

portID DFnetlist::getFalsePort(blockID id) const
{
    return DFI->getFalsePort(id);
}

portID DFnetlist::getDemuxDataPort(blockID id) const
{
    return DFI->getDataPort(id);
}

portID DFnetlist::getDemuxComplementaryPort(portID port) const
{
    return DFI->getDemuxComplementaryPort(port);
}

channelID DFnetlist::getConnectedChannel(portID port) const
{
    return DFI->getConnectedChannel(port);
}

BlockType DFnetlist::getBlockType(blockID block) const
{
	return DFI->getBlockType(block);
}

bool DFnetlist::isPortConnected(portID port) const
{
    return DFI->isPortConnected(port);
}

portID DFnetlist::getConnectedPort(portID port) const
{
    return DFI->getConnectedPort(port);
}

channelID DFnetlist::createChannel(portID src, portID dst)
{
    return DFI->createChannel(src, dst);
}

portID DFnetlist::getSrcPort(channelID id) const
{
    return DFI->getSrcPort(id);
}

portID DFnetlist::getDstPort(channelID id) const
{
    return DFI->getDstPort(id);
}

void DFnetlist::removeChannel(channelID id)
{
    return DFI->removeChannel(id);
}

string DFnetlist::getChannelName(channelID id, bool full) const
{
    return DFI->getChannelName(id, full);
}

int DFnetlist::getChannelBufferSize(channelID id) const
{
    return DFI->getChannelBufferSize(id);
}

void DFnetlist::setChannelBufferSize(blockID id, int slots)
{
    return DFI->setChannelBufferSize(id, slots);
}

bool DFnetlist::isChannelTransparent(channelID id) const
{
    return DFI->isChannelTransparent(id);
}


void DFnetlist::setChannelTransparency(channelID id, bool value)
{
    return DFI->setChannelTransparency(id, value);
}

void DFnetlist::setChannelFrequency(channelID id, double value) {
    return DFI->setChannelFrequency(id, value);
}

double DFnetlist::getChannelFrequency(channelID id) {
    return DFI->getChannelFrequency(id);
}

bool DFnetlist::hasBuffer(channelID id) const
{
    return DFI->hasBuffer(id);
}

void DFnetlist::setError(const string& err)
{
    DFI->setError(err);
}

const string& DFnetlist::getError() const
{
    return DFI->getError();
}

void DFnetlist::clearError()
{
    DFI->clearError();
}

bool DFnetlist::hasError() const
{
    return DFI->hasError();
}

int DFnetlist::numBlocks() const
{
    return DFI->numBlocks();
}

int DFnetlist::numChannels() const
{
    return DFI->numChannels();
}

int DFnetlist::numPorts() const
{
    return DFI->numPorts();
}

bool DFnetlist::validPort(portID p) const
{
    return DFI->validPort(p);
}

bool DFnetlist::validBlock(blockID id) const
{
    return DFI->validBlock(id);
}

bool DFnetlist::validChannel(channelID id) const
{
    return DFI->validChannel(id);
}

bool DFnetlist::writeDot(const string& filename)
{
    return DFI->writeDot(filename);
}

bool DFnetlist::writeDot(std::ostream& s)
{
    return DFI->writeDot(s);
}

bool DFnetlist::writeDotMG(const std::string &filename) {
    return DFI->writeDotMG(filename);
}

bool DFnetlist::writeDotMG(std::ostream &s) {
    return DFI->writeDotMG(s);
}

bool DFnetlist::writeDotBB(const std::string &filename) {
    return DFI->writeDotBB(filename);
}

bool DFnetlist::writeDotBB(std::ostream &s) {
    return DFI->writeDotBB(s);
}

bool DFnetlist::writeBasicBlockDot(const string& filename)
{
    return DFI->writeBasicBlockDot(filename);
}

bool DFnetlist::writeBasicBlockDot(std::ostream& s)
{
    return DFI->writeBasicBlockDot(s);
}

// Jiantao, Overall flow for LSQ parameter exploration
bool DFnetlist::buildAdjMatrix(double coverage, std::string& dot_name, DFnetlist& out_netlist, int selected_case)
{
    return DFI->buildAdjMatrix(coverage, dot_name, out_netlist, selected_case);
}

// Jiantao, 13/06/2022
bool DFnetlist::setSPLSQparams(blockID lsq_block_id, int load_depth, int store_depth)
{
    return DFI->setSPLSQparams(lsq_block_id, load_depth, store_depth);
}

// Jiantao, 13/06/2022
bool DFnetlist::changeOutDot(const std::string& LSQ_node_name, int optimal_load_depth, int optimal_store_depth)
{
    return DFI->changeOutDot(LSQ_node_name, optimal_load_depth, optimal_store_depth);
}

void DFnetlist::eraseNonSCC()
{
    DFI->eraseNonSCC();
}

bool DFnetlist::calculateBasicBlocks()
{
    return DFI->calculateBasicBlocks();
}

void DFnetlist::computeSCC(bool onlyMarked) {
    DFI->computeSCCpublic(onlyMarked);
}

void DFnetlist::printBlockSCCs() {
    DFI->printBlockSCCs();
}

//////////////////////
//  DFlib functions //
//////////////////////

DFlib::DFlib() : DFLI(new DFlib_Impl()) {}

DFlib::DFlib(const std::string& filename) : DFLI(new DFlib_Impl(filename)) {}

bool DFlib::add(const std::string& filename)
{
    return DFLI->add(filename);
}

DFlib::~DFlib()
{
    delete DFLI;
}

bool DFlib::writeDot(const std::string& filename)
{
    return DFLI->writeDot(filename);
}

void DFlib::setError(const string& err)
{
    DFLI->setError(err);
}

const string& DFlib::getError() const
{
    return DFLI->getError();
}

void DFlib::clearError()
{
    DFLI->clearError();
}

bool DFlib::hasError() const
{
    return DFLI->hasError();
}

int DFlib::numFuncs() const
{
    return DFLI->numFuncs();
}

funcID DFlib::getFuncID(const std::string& fname) const
{
    return DFLI->getFuncID(fname);
}

DFnetlist& DFlib::operator[](const std::string& fname)
{
    return (*DFLI)[fname];
}

const DFnetlist& DFlib::operator[](const std::string& fname) const
{
    return (*DFLI)[fname];
}


DFnetlist& DFlib::operator[](funcID id)
{
    return (*DFLI)[id];
}

const DFnetlist& DFlib::operator[](funcID id) const
{
    return (*DFLI)[id];
}

