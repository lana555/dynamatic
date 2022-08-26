#include <cassert>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

bool DFnetlist_Impl::checkSamePortWidth(const setPorts& ports)
{
    if (ports.size() <= 1) return true;
    int width = getPortWidth(*(ports.begin()));
    for (portID p: ports) {
        if (getPortWidth(p) != width) {
            setError("Inconsistent port width in block " + getBlockName(getBlockFromPort(p)) + ".");
            return false;
        }
    }
    return true;
}

bool DFnetlist_Impl::checkSamePortWidth(portID p1, portID p2)
{
    if (getPortWidth(p1) != getPortWidth(p2)) {
        setError("Inconsistent port width between " + getPortName(p1) + " (" + to_string(getPortWidth(p1)) + ") and " +
                 getPortName(p2) + " (" +  to_string(getPortWidth(p2)) + ").");
        return false;
    }
    return true;
}

bool DFnetlist_Impl::check()
{
    // Detect orphan forks and add fake constants at the inputs
    vecBlocks orphan_forks;
    ForAllBlocks(b) {
        if (getBlockType(b) == FORK and not isPortConnected(getInPort(b))) orphan_forks.push_back(b);
    }

    if (not orphan_forks.empty()) {
        cout << "*** Warning: adding fake input constants for fork blocks" << endl;
        cout << "*** This is a temporary hack that should be fixed in the future" << endl;
        cout << "Orphan forks:";
        for (blockID b: orphan_forks) cout << ' ' << getBlockName(b);
        cout << endl;
    }

    for (blockID b: orphan_forks) {
        blockID k = createBlock(CONSTANT);
        portID p = createPort(k, false);
        setPortWidth(p, 1);
        setValue(k, 1);
        createChannel(p, getInPort(b));
    }

    // Check that blocks are well defined
    if (not checkPortsConnected()) return false;

    inferChannelWidth(default_width > 0 ? default_width : 32);

    bool status;
    ForAllBlocks(b) {
        switch (getBlockType(b)) {
        case ELASTIC_BUFFER:
            status = checkElasticBuffer(b);
            break;
        case FORK:
            status = checkFork(b);
            break;
        case BRANCH:
            status = checkBranch(b);
            break;
        case MERGE:
            status = checkMerge(b);
            break;
        case SELECT:
            status = checkSelect(b);
            break;
        case OPERATOR:
            status = checkFunc(b);
            break;
        case CONSTANT:
            status = checkConstant(b);
            break;
        case FUNC_ENTRY:
            status = checkFuncEntry(b);
            break;
        case FUNC_EXIT:
            status = checkFuncExit(b);
            break;
        case DEMUX:
            status = checkDemux(b);
            break;
        case SINK:
        case SOURCE:
        case MUX:
        case CNTRL_MG:
        	break;
        case UNKNOWN:
            assert(false);
        }

        if (not status) return false;
    }

    if (entryControl == invalidDataflowID) {
        setError("Missing Entry control module.");
        return false;
    }

    // Lana 24.03.19 Removing check (exit doesn't have to be control)
    if (exitControl.empty()) {
        //setError("Missing Exit control module.");
        //return false;
    }

    // Check that the channels are well formed, i.e.
    // that go from an output port to an input port.
    ForAllChannels(c) {
        portID src_id = getSrcPort(c);
        portID dst_id = getDstPort(c);
        if (isInputPort(src_id) or isOutputPort(dst_id)) {
            setError("Channel " + getPortName(src_id) + " -> "
                     + getPortName(dst_id) + " is not well-formed: wrong port direction.");
            return false;
        }

        // Lana 29.03.19 Removing check 
        // (sizes can be different if cntr input)
        /* if (getPortWidth(src_id) != getPortWidth(dst_id)) {
            if (getBlockType(dst_id) != CONSTANT)
                setError("Channel " + getPortName(src_id) + " -> "
                         + getPortName(dst_id) + ": ports have different width.");
            return false;
        }*/
    }

    calculateBackEdges();
    
    return true;
}

bool DFnetlist_Impl::checkPortsConnected(blockID b)
{
    assert (b == invalidDataflowID or validBlock(b));
    const setPorts& P = (b == invalidDataflowID) ? allPorts : getPorts(b, ALL_PORTS);
    vecPorts removed;

    for (auto p: P) {
        if (not isPortConnected(p)) {
            // Special case: output of a branch -> remove port
            blockID b = getBlockFromPort(p);
            if (getBlockType(b) == BRANCH and isOutputPort(p)) removed.push_back(p);
            // Lana 24.03.19 Remove entry port (not connected)
            else if (getBlockType(b) == FUNC_ENTRY and isInputPort(p)) removed.push_back(p);
            else {
                setError("Port " + getPortName(p) + " not connected.");
                return false;
            }
        }
    }

    for (portID p: removed) removePort(p);
    return true;
}

bool DFnetlist_Impl::checkElasticBuffer(blockID b)
{
    if (not checkGenericPorts(b)) return false;
    if (not checkNumPorts(b, 1, 1)) return false;
    if (not checkSamePortWidth(getInPort(b), getOutPort(b))) return false;
    if (getBufferSize(b) <= 0) {
        setError("Wrong number of slots for elastic buffer " + getBlockName(b) + ".");
        return false;
    }
    return true;
}

bool DFnetlist_Impl::checkFork(blockID b)
{
    if (not checkGenericPorts(b)) return false;
    if (not checkNumPorts(b, 1, -1)) return false;
    if (not checkSamePortWidth(getPorts(b, ALL_PORTS))) return false;
    return true;
}

bool DFnetlist_Impl::checkMerge(blockID b)
{
    if (not checkGenericPorts(b)) return false;
    if (not checkNumPorts(b, -1, 1)) return false;
    if (not checkSamePortWidth(getPorts(b, ALL_PORTS))) return false;
    return true;
}

bool DFnetlist_Impl::checkFunc(blockID b)
{
    if (numInPorts(b) == 0) {
        setError("Block " + getBlockName(b) + " has no input ports.");
        return false;
    }
    /*
        if (numOutPorts(b) == 0) {
            setError("Block " + getBlockName(b) + " has no output ports.");
            return false;
        }
    */
    if (getBlockType(b)==OPERATOR && getOperation(b) == "select_op")
        return true;
    return checkGenericPorts(b);
}

bool DFnetlist_Impl::checkFuncEntry(blockID b)
{
    // Lana 04.02.22 Commented check (to support all 32 bits) // to go from normal to 32 bits comment from line 214 to 219 and uncomment line 220
    if (not checkNumPorts(b, 0, 1)) return false;
    if(isControlPort(getOutPort(b))) {
        if (entryControl != invalidDataflowID) {
            setError("There is more than one entry control block.");
            return false;
        } else entryControl = b;
    } else 
    //entryControl = b; // Carmine 04.02.22 32 bits control signal
    parameters.insert(b);

    return checkGenericPorts(b);
}

bool DFnetlist_Impl::checkFuncExit(blockID b)
{
    // Lana 29.04.19 removed check (end can have arbitrary nb of inputs)
    //if (not checkNumPorts(b, 1, -1)) return false;
    if(isControlPort(getInPort(b))) exitControl.insert(b);
    else results.insert(b);

    return checkGenericPorts(b);
}

bool DFnetlist_Impl::checkConstant(blockID b)
{
    // For convenience, we will tolerate constants with and without input port
    int n_in = numInPorts(b);
    if (n_in > 1) {
        setError("Too many input ports for constant block " + getBlockName(b) + ".");
        return false;
    }

    if (not checkNumPorts(b, -1, 1)) return false;

    // Lana 24.03.19 Constant input does not have to be control (connection with source)
    //if (n_in == 1 and not isControlPort(getInPort(b))) {
    //    setError("Non-control port for Constant block " + getBlockName(b) + ".");
    //    return false;
    //}
    // Lana 9.6.2021. Axel resource sharing update
    // Normalize the value with sign extension
    // int width = getPortWidth(getOutPort(b));
    // int sign_location = 1 << (width - 1);
    // longValueType v = getValue(b);
    // if ((v & sign_location) != 0) v += ((longValueType) -1) << width;
    // setValue(b, v);
    return true;
}

bool DFnetlist_Impl::checkBranch(blockID b)
{
    // We need 2 input and 2 output ports
    if (not checkNumPorts(b, 2,-1)) return false;

    int n_out = numOutPorts(b);
    if (n_out == 0 or n_out > 2) {
        setError("Block " + getBlockName(b) + ": incorrect number of output ports.");
        return false;
    }

    Block& B = blocks[b];
    B.portCond = B.portTrue = B.portFalse = B.data = invalidDataflowID;
    int nsel = 0;

    // Find the data and condition ports
    for (auto p: getPorts(b, INPUT_PORTS)) {
        if (getPortType(p) == SELECTION_PORT) {
            nsel ++;
            B.portCond = p;
        } else {
            B.data = p;
        }
    }

    if (nsel > 1) {
        setError("Block " + getBlockName(b) + " has two condition ports.");
        return false;
    }

    if (nsel ==0) {
        setError("Block " + getBlockName(b) + " has no condition port.");
        return false;
    }

    // Find the true and false ports
    int ntrue = 0, nfalse = 0;
    for (auto p: getPorts(b, OUTPUT_PORTS)) {
        if (getPortType(p) == TRUE_PORT) {
            ntrue++;
            B.portTrue = p;
        } else if (getPortType(p) == FALSE_PORT) {
            nfalse++;
            B.portFalse = p;
        }
    }

    if (ntrue + nfalse != n_out or ntrue > 1 or nfalse > 1) {
        setError("Block " + getBlockName(b) + ": wrong configuration of output ports for the true/false conditions.");
        return false;
    }

    // Lana 04.02.22 Commented check (to support all 32 bits) // to go from normal to 32 bits comment from line 316 to 319
    // Check that the condition port is Boolean
    if (not isBooleanPort(B.portCond)) {
        setError("Block " + getBlockName(b) + ": the condition port is not Boolean.");
        return false;
    }

    // Check that the other ports have the same width
    if ((validPort(B.portTrue) and not checkSamePortWidth(B.portTrue, B.data)) or
        (validPort(B.portFalse) and not checkSamePortWidth(B.portFalse, B.data))) {
        setError("Block " + getBlockName(b) + ": inconsistent port widths.");
        return false;
    }
    return true;
}

bool DFnetlist_Impl::checkSelect(blockID b)
{
    if (not checkNumPorts(b, 3,1)) return false;

    Block& B = blocks[b];
    B.portCond = B.portTrue = B.portFalse = invalidDataflowID;

    // Identify the input ports (condition, true and false)
    int nsel = 0, ntrue = 0, nfalse = 0;
    for (auto p: getPorts(b, INPUT_PORTS)) {
        PortType t = getPortType(p);
        if (t == SELECTION_PORT) {
            nsel++;
            B.portCond = p;
        } else if (t == TRUE_PORT) {
            ntrue++;
            B.portTrue = p;
        } else if (t == FALSE_PORT) {
            nfalse++;
            B.portFalse = p;
        }
    }

    if (nsel != 1 or ntrue != 1 or nfalse != 1) {
        setError("Block " + getBlockName(b) + ": wrong configuration of input ports for a select.");
        return false;
    }

    // Identify the output port
    B.data = getOutPort(b);
    if (getPortType(B.data) != GENERIC_PORT) {
        setError("Block " + getBlockName(b) + ": output port should be generic.");
        return false;
    }

    // Check that the condition port is Boolean
    if (not isBooleanPort(B.portCond)) {
        setError("Block " + getBlockName(b) + ": the condition port is not Boolean.");
        return false;
    }

    // Check that the other ports have the same width
    if (not checkSamePortWidth(B.portTrue, B.portFalse) or not checkSamePortWidth(B.portTrue, B.data)) {
        setError("Block " + getBlockName(b) + ": inconsistent port widths.");
        return false;
    }
    return true;
}

bool DFnetlist_Impl::checkDemux(blockID b)
{
    int nin = numInPorts(b);
    int nout = numOutPorts(b);

    if (nin != nout-1) {
        setError("Block " + getBlockName(b) + ": Incorrect number of ports.");
        return false;
    }

    portID data = invalidDataflowID;
    int ncontrol = 0;
    for (auto p: getPorts(b, INPUT_PORTS)) {
        if (not isControlPort(p)) data = p;
        else ncontrol++;
    }

    if (ncontrol != nin - 1) {
        setError("Block " + getBlockName(b) + ": Incorrect number of data ports.");
        return false;
    }

    int width = getPortWidth(data);
    for (auto p: getPorts(b, OUTPUT_PORTS)) {
        if (getPortWidth(p) != width) {
            setError("Block " + getBlockName(b) + ": Width mismatch of data ports.");
            return false;
        }
    }

    for (auto& pair: blocks[b].demuxPairs) {
        portID p1 = pair.first;
        portID p2 = pair.second;
        if (not isControlPort(p1)) swap(p1, p2);
        if (not isControlPort(p1)) {
            setError("Block " + getBlockName(b) + ": Incorrect pairing of control and data ports.");
            return false;
        }
    }
    return true;
}

bool DFnetlist_Impl::checkGenericPorts(blockID b)
{
    for (auto p: getPorts(b, ALL_PORTS)) {
        if (getPortType(p) != GENERIC_PORT) {
            setError("Incorrect type for port " + getPortName(p) + ".");
            return false;
        }
    }
    return true;
}

bool DFnetlist_Impl::checkNumPorts(blockID b, int nin, int nout)
{
    if (nin >= 0 and numInPorts(b) != nin) {
        setError("Block " + getBlockName(b) + ": incorrect number of ports.");
        return false;
    }

    if (nout >= 0 and numOutPorts(b) != nout) {
        setError("Block " + getBlockName(b) + ": incorrect number of ports.");
        return false;
    }
    return true;
}
