#include <cassert>
#include <cmath>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

/*
 * This file contains the methods used for the insertion of elastic buffers (EBs).
 * The EBs can be inserted in two ways: (1) by explicitly adding blocks with type
 * ELASTIC_BUFFER or (2) by annotating the information of the EB in a channel.
 * In both cases, the attributes "slots" (integer) and "transparent" (boolean)
 * indicate the characteristics of the EBs.
 *
 * The annotated form is meant to be used internally before the netlist is exported
 * into a dot file. For example, the annotations in the channels can be used as
 * a temporary solution before some buffers are retimed for performance optimization.
 *
 * The method instantiateElasticBuffers() is the one that creates EB blocks from the
 * information annotated in the channels. After creating the EBs explicitly, the
 * annotations in the channels dissapear (slots=0).
 *
 * Difference between transparent and opaque EBs:
 *      _ _ _
 *     | | | |
 * --> | | | | -->  This is an opaque EB with 3 slots (latency = 1)
 *     |_|_|_|
 *
 *                 _
 * -------------->| |
 *   |            |M|
 *   |   _ _ _    |U|--> This is a transparent EB with 3 slots (latency = 0 or 1).
 *   |  | | | |   |X|
 *   -->| | | |-->|_|
 *      |_|_|_|
 *
 * The location of the EBs is calculated with an MILP model (inspired in retiming).
 * The model guarantees that no combinational path is longer than the period
 * (path constraints). The model also considers the presence of pipelined units.
 * In this case, the combinational paths are cut by the registers of the units.
 * However, the model also guarantees that every cycle is elastic (elasticity
 * constraints).
 *
 * The presence of an EB for the path constraints implies the presence of an EB
 * for the elasticity constraints, but not vice versa. At the end, the EBs for
 * elasticity constraints determine the location of the EBs. Those buffers that
 * are only needed to guarantee elasticity (but not short paths) can be transparent.
 *
*/

// This function generates the variables of the MILP model
void DFnetlist_Impl::createMilpVarsEB(Milp_Model& milp, milpVarsEB& vars, bool max_throughput)
{
    vars.buffer_token = vector<int>(vecChannelsSize(), -1);
    vars.buffer_bubble = vector<int>(vecChannelsSize(), -1);
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_elastic = vector<int>(vecPortsSize(), -1);
    ForAllChannels(c) {
        vars.buffer_token[c] = milp.newBooleanVar();
        vars.buffer_bubble[c] = milp.newIntegerVar();
    }

    ForAllBlocks(b) {
        ForAllPorts(b,p) {
            vars.time_path[p] = milp.newRealVar();
            vars.time_elastic[p] = milp.newRealVar();
        }
    }

    if (not max_throughput) return;

    // Variables to model throughput
    vars.in_retime_tokens = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.out_retime_bubbles = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.th_channel = vector<vector<int>>(MG.size(), vector<int> (vecChannelsSize(), -1));
    vars.th_MG = vector<int>(MG.size(), -1);
    for (int mg = 0; mg < MG.size(); ++mg) {
        vars.th_MG[mg] = milp.newRealVar();
        for (blockID b: MG[mg].getBlocks()) {
            vars.in_retime_tokens[mg][b] = milp.newRealVar();
            // If the block is combinational, the in/out retiming variables are the same
            vars.out_retime_bubbles[mg][b] = getLatency(b) > 0 ? milp.newRealVar() : vars.in_retime_tokens[mg][b];
        }

        for (channelID c: MG[mg].getChannels()) {
            vars.th_channel[mg][c] = milp.newRealVar();
        }
    }
}

// Main function to add elastic buffers satisfying a certain clock period

bool DFnetlist_Impl::addElasticBuffers(double Period, double BufferDelay, bool MaxThroughput)
{
    Milp_Model milp;

    setUnitDelays();

    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }

    hideElasticBuffers();

    if (MaxThroughput) {
        double coverage = extractMarkedGraphs(0.8);
        cout << "Extracted Marked Graphs with coverage " << coverage << endl;
    }

    // Create the variables
    milpVarsEB milpVars;
    createMilpVarsEB(milp, milpVars, MaxThroughput);

    if (not createPathConstraints(milp, milpVars, Period, BufferDelay)) return false;
    if (not createElasticityConstraints(milp, milpVars)) return false;

    // Cost function: Rp + Re (buffers for path and elasticity constraints)
    double buf_cost = MaxThroughput ? -0.0001 : 1;
    ForAllChannels(c) {
        milp.newCostTerm(buf_cost, milpVars.buffer_token[c]);
        milp.newCostTerm(buf_cost, milpVars.buffer_bubble[c]);
    }

    if (MaxThroughput) {
        createThroughputConstraints(milp, milpVars);
        milp.newCostTerm(1, milpVars.th_MG[0]);
        milp.setMaximize();
    } else {
        milp.setMinimize();
    }

    cout << "Solving MILP for elastic buffers" << endl;
    milp.solve();

    Milp_Model::Status stat = milp.getStatus();

    if (stat != Milp_Model::OPTIMAL and stat != Milp_Model::NONOPTIMAL) {
        setError("No solution found to add elastic buffers.");
        return false;
    }

    if (MaxThroughput) cout << "Throughput: " << milp[milpVars.th_MG[0]] << endl;

    dumpMilpSolution(milp, milpVars);

    // Add channels
    vector<channelID> buffers;
    ForAllChannels(c) {
        if (milp[milpVars.buffer_bubble[c]] > 0) {
            cout << "Adding buffer in " << getChannelName(c) << " elastic=" << milp[milpVars.buffer_bubble[c]] << " comb=" << milp[milpVars.buffer_token[c]] << endl;
            buffers.push_back(c);
        }
    }

    for (channelID c: buffers) {
        int slots = milp[milpVars.buffer_bubble[c]] + 0.5; // Automatically truncated
        bool transparent = milp.isFalse(milpVars.buffer_token[c]);
        if (not transparent) slots = max(2, slots);
        setChannelTransparency(c, transparent);
        setChannelBufferSize(c, slots);
    }

    for (channelID c: buffers) {
        cout << "Throughput " << getChannelName(c) << ": " << milp[milpVars.th_tokens[0][c]] << endl;
    }

    makeNonTransparentBuffers();
    return true;
}

bool DFnetlist_Impl::createPathConstraints(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay)
{
    // Create the variables and constraints for all channels
    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;

    ForAllChannels(c) {
        int v1 =  Vars.time_path[getSrcPort(c)];
        int v2 =  Vars.time_path[getDstPort(c)];
        int R = Vars.buffer_token[c];

        if (hasPeriod) {
            // v1, v2 <= Period
            milp.newRow( {{1,v1}}, '<', Period);
            milp.newRow( {{1,v2}}, '<', Period);

            // v2 >= v1 - 2*period*R
            milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}}, '>', 0);
        }

        // v2 >= Buffer Delay
        if (BufferDelay > 0) milp.newRow( {{1,v2}}, '>', BufferDelay);
    }

    // Create the constraints to propagate the delays of the blocks
    ForAllBlocks(b) {
        double d = getBlockDelay(b);

        // First: combinational blocks
        if (getLatency(b) == 0) {
            ForAllOutputPorts(b, out_p) {
                int v_out = Vars.time_path[out_p];
                ForAllInputPorts(b, in_p) {
                    int v_in = Vars.time_path[in_p];
                    // Add constraint v_out >= d_in + d + d_out + v_in;
                    double D = getCombinationalDelay(in_p, out_p);
                    if (D + BufferDelay > Period) {
                        setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                        return false;
                    }
                    milp.newRow( {{1, v_out}, {-1,v_in}}, '>', D);
                }
            }
        } else {
            // Pipelined units
            if (d > Period) {
                setError("Block " + getBlockName(b) + ": period cannot be satisfied.");
                return false;
            }

            ForAllOutputPorts(b, out_p) {
                double d_out = getPortDelay(out_p);
                int v_out = Vars.time_path[out_p];
                if (d_out > Period) {
                    setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                    return false;
                }
                // Add constraint: v_out = d_out;
                milp.newRow( {{1, v_out}}, '=', d_out);
            }

            ForAllInputPorts(b, in_p) {
                double d_in = getPortDelay(in_p);
                int v_in = Vars.time_path[in_p];
                if (d_in + BufferDelay > Period) {
                    setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                    return false;
                }
                // Add constraint: v_in + d_in <= Period
                if (hasPeriod) milp.newRow( {{1, v_in}}, '<', Period - d_in);
            }
        }
    }

    return true;
}

bool DFnetlist_Impl::createElasticityConstraints(Milp_Model& milp, milpVarsEB& Vars)
{
    // Upper bound for the longest rigid path. Blocks are assumed to have unit delay.
    double big_constant = numBlocks() + 1;

    // Create the variables and constraints for all channels
    ForAllChannels(c) {
        int v1 = Vars.time_elastic[getSrcPort(c)];
        int v2 = Vars.time_elastic[getDstPort(c)];
        int re = Vars.buffer_bubble[c];
        int rp = Vars.buffer_token[c];

        // v2 >= v1 - big_constant*R (path constraint)
        milp.newRow ( {{-1,v1}, {1,v2}, {big_constant,re}}, '>', 0);

        // re >= rp (A buffer for the path constraints
        //           implies a buffer for the elasticity constraints).
        milp.newRow ( {{1,re}, {-1,rp}}, '>', 0);
    }

    // Create the constraints to propagate the delays of the blocks
    ForAllBlocks(b) {
        ForAllOutputPorts(b, out_p) {
            int v_out = Vars.time_elastic[out_p];
            ForAllInputPorts(b, in_p) {
                int v_in = Vars.time_elastic[in_p];
                // Add constraint v_out >= 1 + v_in; (unit delay)
                milp.newRow( {{1, v_out}, {-1,v_in}}, '>', 1);
            }
        }
    }

    return true;
}

bool DFnetlist_Impl::createThroughputConstraints(Milp_Model& milp, milpVarsEB& Vars)
{
    // For every MG, for every channel c
    for (int mg = 0; mg < MG.size(); ++mg) {
        int th_mg = Vars.th_MG[mg]; // Throughput of the marked graph.
        cout << "Marked graph " << mg << " with var " << th_mg << endl;
        for (channelID c: MG[mg].getChannels()) {
            cout << "  Channel " << getChannelName(c) << endl;
            int th_c = Vars.th_channel[mg][c]; // throughput of the channel
            int Re = Vars.buffer_bubble[c];   // Registers in the channel
            // Variables for retiming tokens through blocks
            int ret_src = Vars.out_retime_bubbles[mg][getSrcBlock(c)];
            int ret_dst = Vars.in_retime_tokens[mg][getDstBlock(c)];
            // Initial token
            int token = isBackEdge(c) ? 1 : 0;
            // Token + ret_dst - ret_src = Th_channel
            milp.newRow( {{-1, ret_dst}, {1, ret_src}, {1, th_c}}, '=', token);
            // Th_channel >= Th - 1 + Re
            milp.newRow( {{1, th_mg}, {1, Re}, {-1, th_c}}, '<', 1);
            // There must be registers to handle the tokens
            milp.newRow( {{1,Re},{-1,th_c}}, '>', 0);
        }

        //continue;
        // Special treatment for pipelined units
        for (blockID b: MG[mg].getBlocks()) {
            double lat = getLatency(b);
            if (lat == 0) continue;
            int in_ret = Vars.in_retime_tokens[mg][b];
            int out_ret = Vars.out_retime_bubbles[mg][b];
            double maxTokens = lat/getInitiationInterval(b);
            cout << "max Tokens " << getBlockName(b) << ": " << maxTokens << endl;
            // rout-rin <= Lat/II
            milp.newRow( {{1, out_ret}, {-1, in_ret}}, '<', maxTokens);
            // Th*Lat <= rout-rin:   rout-rin-Th*lat >= 0
            milp.newRow( {{1, out_ret}, {-1, in_ret}, {-lat, th_mg}}, '>', 0);
        }
    }
    return true;
}

blockID DFnetlist_Impl::insertBuffer(channelID c, int slots, bool transparent)
{
    portID src = getSrcPort(c);
    portID dst = getDstPort(c);
    removeChannel(c);

    cout << "> Inserting buffer on channel " << getPortName(src)
         << " -> " << getPortName(dst) << endl;
    int width = getPortWidth(src);
    blockID eb = createBlock(ELASTIC_BUFFER);
    setBufferSize(eb, slots);
    setBufferTransparency(eb, transparent);
    portID in_buf = createPort(eb, true, "in1", width);
    portID out_buf = createPort(eb, false, "out1", width);
    cout << "> Buffer created with ports "
         << getPortName(in_buf) << " and " << getPortName(out_buf) << endl;
    createChannel(src, in_buf);
    createChannel(out_buf, dst);
    return eb;
}

channelID DFnetlist_Impl::removeBuffer(blockID buf)
{
    assert(getBlockType(buf) == ELASTIC_BUFFER);

    // Get the ports at the other side of the channels
    portID in_port = getSrcPort(getConnectedChannel(getInPort(buf)));
    portID out_port = getDstPort(getConnectedChannel(getOutPort(buf)));

    // remove the buffer
    removeBlock(buf);

    // reconnect the ports
    return createChannel(in_port, out_port);
}

void DFnetlist_Impl::makeNonTransparentBuffers()
{
    // Calculate SCC without opaque buffers and sequential operators
    markAllBlocks(true);
    markAllChannels(true);

    // Mark the channels with elastic buffers
    ForAllChannels(c) {
        if (not isChannelTransparent(c)) markChannel(c, false);
    }

    ForAllBlocks(b) {
        BlockType type = getBlockType(b);
        if (type == ELASTIC_BUFFER) {
            // Opaque buffer
            if (not isBufferTransparent(b)) markBlock(b, false);
        } else if (type == OPERATOR) {
            // Sequential operator
            if (getLatency(b) > 0) markBlock(b, false);
        }
    }

    // Iterate until all combinational paths have been cut
    while (true) {
        // Calculate SCC without non-transparent buffers
        computeSCC(true);

        if (SCC.empty()) return; // No more combinational paths

        // For each SCC, pick one transparent buffer and make
        // it non-transparent.
        for (const subNetlist& scc: SCC) {
            bool buf_found = false;
            for (channelID c: scc.getChannels()) {
                if (not hasBuffer(c)) continue;
                setChannelTransparency(c, false);
                if (getChannelBufferSize(c) < 2) setChannelBufferSize(c, 2);
                markChannel(c, false);
                buf_found = true;
                break;
            }
            assert(buf_found);
        }
    }
}

void DFnetlist_Impl::instantiateElasticBuffers()
{
    vecChannels ebs;
    ForAllChannels(c) {
        if (hasBuffer(c)) ebs.push_back(c);
    }

    for (channelID c: ebs) {
        insertBuffer(c, getChannelBufferSize(c), isChannelTransparent(c));
    }

    invalidateBasicBlocks();
}


void DFnetlist_Impl::hideElasticBuffers()
{
    vecBlocks bls;
    ForAllBlocks(b) {
        if (getBlockType(b) == ELASTIC_BUFFER) bls.push_back(b);
    }

    for (blockID b: bls) {
        int slots = getBufferSize(b);
        bool transp = isBufferTransparent(b);
        channelID c = removeBuffer(b);
        setChannelBufferSize(c, slots);
        setChannelTransparency(c, transp);
    }
}

void DFnetlist_Impl::cleanElasticBuffers()
{
    hideElasticBuffers();
    ForAllChannels(c) {
        setChannelBufferSize(c, 0);
        setChannelTransparency(c, true);
    }
}

void DFnetlist_Impl::setUnitDelays()
{
    ForAllBlocks(b) setBlockDelay(b, 1);
}

void DFnetlist_Impl::dumpMilpSolution(const Milp_Model& milp, const milpVarsEB& vars) const
{

    ForAllBlocks(b) {
        int in_ret = vars.in_retime_tokens[0][b];
        if (in_ret < 0) continue;
        int out_ret = vars.out_retime_bubbles[0][b];
        cout << "Block " << getBlockName(b) << ": ";
        if (in_ret == out_ret) cout << "Retiming = " << milp[in_ret] << endl;
        else cout << "In Retiming = " << milp[in_ret] << ", Out Retiming = " << milp[out_ret] << endl;
    }

    ForAllChannels(c) {
        cout << "Channel " << getChannelName(c) << ": R = " << milp[vars.buffer_token[c]] << ", Re = " << milp[vars.buffer_bubble[c]];
        int th = vars.th_channel[0][c];
        if (th >= 0) cout << ", th = " << milp[th];
        cout << endl;
    }
}
