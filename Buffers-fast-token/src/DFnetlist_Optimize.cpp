#include <cassert>
#include "DFnetlist.h"
#include "SetUtil.h"

using namespace Dataflow;

void DFnetlist_Impl::calculateDefinitions()
{
    // First of all, calculate srcCond for all branches
    ForAllBlocks(b) {
        if (getBlockType(b) != BRANCH) continue;
        portID p = getConnectedPort(getConditionalPort(b));
        blockID fork = getBlockFromPort(p);
        while (getBlockType(fork) == FORK) {
            p = getConnectedPort(getInPort(fork));
            fork = getBlockFromPort(p);
        }
        blocks[b].srcCond = p;
    }

    // Initialization of defs: all sets empty, except those
    // that define some data (op, eb, entry, constant)
    ForAllBlocks(b) {
        ForAllPorts(b, p) ports[p].defs.clear();
        BlockType type = getBlockType(b);
        if (type == OPERATOR or type == ELASTIC_BUFFER or type == FUNC_ENTRY or type == CONSTANT) {
            ForAllOutputPorts(b, p) ports[p].defs = {p};
        }
    }

    // Fixpoint algorithm propagating the information forward
    bool changes = true;
    while (changes) {

        // Propagation through channels
        ForAllChannels(c) {
            portID src = getSrcPort(c);
            portID dst = getDstPort(c);
            if (ports[src].defs.size() != ports[dst].defs.size()) {
                ports[dst].defs = ports[src].defs;
            }
        }

        changes = false;

        // Propagation through blocks
        ForAllBlocks(b) {
            setPorts newDefs;
            portID inp, outp, truep, falsep;
            switch (getBlockType(b)) {
            case OPERATOR:
            case ELASTIC_BUFFER:
            case FUNC_ENTRY:
            case FUNC_EXIT:
            case CONSTANT:
                break;
            case FORK:
                inp = getInPort(b);
                outp = getOutPort(b);
                if (ports[inp].defs.size() != ports[outp].defs.size()) {
                    changes = true;
                    ForAllOutputPorts(b, p) ports[p].defs = ports[inp].defs;
                }
                break;
            case MERGE:
                ForAllInputPorts(b, p) newDefs = setOp::setUnion(newDefs, ports[p].defs);
                outp = getOutPort(b);
                if (newDefs.size() != ports[outp].defs.size()) {
                    changes = true;
                    ports[outp].defs = newDefs;
                }
                break;
            case BRANCH:
                inp = getDataPort(b);
                truep = getTruePort(b);
                falsep = getFalsePort(b);
                if (not validPort(truep)) swap(truep, falsep);
                if (ports[truep].defs.size() != ports[inp].defs.size()) {
                    changes = true;
                    ports[truep].defs = ports[inp].defs;
                    if (validPort(falsep)) ports[falsep].defs = ports[inp].defs;
                }
                break;
            case DEMUX:
                inp = getDataPort(b);
                outp = getOutPort(b);
                if (ports[inp].defs.size() != ports[outp].defs.size()) {
                    changes = true;
                    ForAllOutputPorts(b, p) ports[p].defs = ports[inp].defs;
                }
                break;
            case SELECT:
                truep = getTruePort(b);
                falsep = getFalsePort(b);
                newDefs = setOp::setUnion(ports[truep].defs, ports[falsep].defs);
                outp = getOutPort(b);
                if (newDefs.size() != ports[outp].defs.size()) {
                    changes = true;
                    ports[outp].defs = newDefs;
                }
                break;
            default:
                assert(false);
            }
        }
    }

    // A fixpoint has been reached: all defs have been calculated.

    /*
    ForAllBlocks(b) {
        cout << "Defs for block " << getBlockName(b) << ":" << endl;
        ForAllPorts(b, p) {
            cout << "  Port " << getPortName(p, false) << ":";
            for (portID def: getDefinitions(p)) cout << " " << getPortName(def);
            cout << endl;
        }
    }
    */
}

bool DFnetlist_Impl::optimize()
{
    cout << "**********************" << endl;
    cout << "*** Start optimize ***" << endl;
    cout << "**********************" << endl;
    bool changes = removeConstantBranchSelect();;

    for (;;) {
        bool changes2 = false;
        changes2 = changes2 or collapseMergeFork();
        changes2 = changes2 or removeOneInOneOutMergeForkDemux();
        changes2 = changes2 or removeFakeBranches();
        changes2 = changes2 or removeUnreachableBlocksAndPorts();
        if (not changes2) {
            if (changes) invalidateBasicBlocks();
            calculateDefinitions();
            return changes;
        }
        changes = true;
    }
}

bool DFnetlist_Impl::removeOneInOneOutMergeForkDemux()
{
    vecBlocks to_remove;
    ForAllBlocks(b) {
        BlockType t = getBlockType(b);
        if ((t != MERGE and t != FORK and t != DEMUX) or numOutPorts(b) != 1) continue;
        if (numInPorts(b) == 1 or t == DEMUX) to_remove.push_back(b);
    }

    if (to_remove.empty()) return false;

    // Now remove the blocks and reconnect channels
    for (blockID b: to_remove) {
        BlockType type = getBlockType(b);

        // Get the in/out ports
        portID psrc = type == DEMUX ? getDataPort(b) : getInPort(b);
        psrc = getConnectedPort(psrc);
        portID pdst = getConnectedPort(getOutPort(b));

        // Remove the block and reconnect the channel
        removeBlock(b);
        createChannel(psrc, pdst);
    }

    return true;
}

bool DFnetlist_Impl::collapseMergeFork()
{
    vecBlocks M; // Set of merge blocks
    vecBlocks F; // set of fork blocks

    // Check for merge->merge or fork->fork channels
    ForAllChannels(c) {
        blockID src = getSrcBlock(c);
        blockID dst = getDstBlock(c);
        BlockType tsrc = getBlockType(src);
        BlockType tdst = getBlockType(dst);

        if (tsrc != tdst) continue;

        if (tsrc == MERGE) M.push_back(src);
        else if (tsrc == FORK) F.push_back(dst);
    }

    if (M.empty() and F.empty()) return false;

    // Collapse merge blocks
    for (blockID bsrc: M) {
        portID p_out = getOutPort(bsrc);
        portID q_in = getConnectedPort(p_out);
        blockID bdst = getBlockFromPort(q_in);
        int width = getPortWidth(p_out);

        // Input ports of the first merge
        vecPorts inp;
        ForAllInputPorts(bsrc, p_in) inp.push_back(getConnectedPort(p_in));

        // remove the first merge
        removeBlock(bsrc);

        // Connect the first port
        createChannel(inp[0], q_in);

        // Create extra ports for bdst
        for (int i = 1; i < inp.size(); ++i) {
            portID newp = createPort(bdst, true);
            setPortWidth(newp, width);
            createChannel(inp[i], newp);
        }
    }

    // Collapse Fork blocks
    for (blockID bdst: F) {
        portID q_in = getInPort(bdst);
        portID p_out = getConnectedPort(q_in);
        blockID bsrc = getBlockFromPort(p_out);
        int width = getPortWidth(q_in);

        // Output ports of the second fork
        vecPorts outp;
        ForAllOutputPorts(bdst, q_out) outp.push_back(getConnectedPort(q_out));

        // Remove the second fork
        removeBlock(bdst);

        // Connect the first port
        createChannel(p_out, outp[0]);

        // Create extra ports for bsrc
        for (int i = 1; i < outp.size(); ++i) {
            portID newp = createPort(bsrc, false);
            setPortWidth(newp, width);
            createChannel(newp, outp[i]);
        }
    }

    return true;
}


bool DFnetlist_Impl::removeUnreachableBlocksAndPorts()
{
    bool changes = false;
    // Iteration to remove unreachable blocks
    for (;;) {
        vecBlocks to_remove;
        ForAllBlocks(b) {
            BlockType type = getBlockType(b);
            if (numOutPorts(b) == 0 and type != OPERATOR and type != FUNC_EXIT) to_remove.push_back(b);
            else if (numInPorts(b) == 0 and type != CONSTANT and type != FUNC_ENTRY) to_remove.push_back(b);
        }

        if (to_remove.empty()) break;
        changes = true;
        for (blockID br: to_remove) removeBlock(br);
    }

    // Now remove unused ports
    vecPorts to_remove;
    ForAllBlocks(b) {
        ForAllPorts(b,p) {
            if (not isPortConnected(p)) to_remove.push_back(p);
        }
    }

    if (not to_remove.empty()) {
        changes = true;
        for (portID p: to_remove) removePort(p);
    }

    return changes;
}

bool DFnetlist_Impl::isBooleanConstant(portID port, bool& value) const
{
    if (not isBooleanPort(port)) return false;
    if (isInputPort(port)) return isBooleanConstant(getConnectedPort(port), value);
    blockID b = getBlockFromPort(port);
    BlockType type = getBlockType(b);
    if (type == CONSTANT) {
        value = getBoolValue(b);
        return true;
    }

    if (type == FORK) return isBooleanConstant(getConnectedPort(getInPort(b)), value);
    return false;
}

bool DFnetlist_Impl::removeConstantBranchSelect()
{
    map<blockID, bool> to_remove;
    ForAllBlocks(b) {
        BlockType type = getBlockType(b);
        if (type != BRANCH and type != SELECT) continue;
        portID cond = getConditionalPort(b);
        bool value;
        if (isBooleanConstant(cond, value)) to_remove[b] = value;
    }


    if (to_remove.empty()) return false;

    for (auto it: to_remove) {
        blockID b = it.first;
        bool value = it.second;
        portID psrc, pdst;
        if (getBlockType(b) == BRANCH) {
            psrc = getDataPort(b);
            pdst = value ? getTruePort(b) : getFalsePort(b);
        } else {
            psrc = value ? getTruePort(b) : getFalsePort(b);
            pdst = getOutPort(b);
        }

        psrc = getConnectedPort(psrc);
        pdst = getConnectedPort(pdst);

        removeBlock(b);
        createChannel(psrc, pdst);
    }

    return true;
}

bool DFnetlist_Impl::removeFakeBranches()
{
    set<blockID> to_remove;

    // Remove fake branches: both outputs go to the same merge block
    ForAllBlocks(b) {
        if (getBlockType(b) != BRANCH) continue;
        portID ptrue = getTruePort(b);
        if (not validPort(ptrue)) continue;
        portID pfalse = getFalsePort(b);
        if (not validPort(pfalse)) continue;
        ptrue = getConnectedPort(ptrue);
        pfalse = getConnectedPort(pfalse);
        blockID btrue = getBlockFromPort(ptrue);
        blockID bfalse = getBlockFromPort(pfalse);
        if (btrue == bfalse and getBlockType(btrue) == MERGE) to_remove.insert(b);
    }

    if (to_remove.empty()) return false;

    for (auto b: to_remove) {
        portID psrc = getDataPort(b);
        portID ptrue = getTruePort(b);
        portID pfalse = getFalsePort(b);

        psrc = getConnectedPort(psrc);
        ptrue = getConnectedPort(ptrue);
        pfalse = getConnectedPort(pfalse);


        removeBlock(b);
        removePort(pfalse);
        createChannel(psrc, ptrue);
    }

    return true;
}

bool DFnetlist_Impl::removeControl()
{
    vecBlocks to_remove;
    ForAllBlocks(b) {
        switch (getBlockType(b)) {
        case OPERATOR:
        case SELECT:
        case CONSTANT:
            break;
        case ELASTIC_BUFFER:
        case FORK:
        case FUNC_EXIT:
            if (isControlPort(getInPort(b)))
                to_remove.push_back(b);
            break;
        case FUNC_ENTRY:
        case MERGE:
        case DEMUX:
            if (isControlPort(getOutPort(b)))
                to_remove.push_back(b);
            break;
        case BRANCH:
            if (isControlPort(getDataPort(b)))
                to_remove.push_back(b);
            break;
        default:
            assert(false);
        }
    }

    if (to_remove.empty()) return false;

    invalidateBasicBlocks();
    for (blockID b: to_remove) removeBlock(b);
    return true;
}
