#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
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

void DFnetlist_Impl::createMilpVarsEB(Milp_Model& milp, milpVarsEB& vars, bool max_throughput, bool first_MG)
{
    vars.buffer_flop = vector<int>(vecChannelsSize(), -1);
    vars.buffer_slots = vector<int>(vecChannelsSize(), -1);
    vars.has_buffer = vector<int>(vecChannelsSize(), -1);
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_elastic = vector<int>(vecPortsSize(), -1);

    ForAllChannels(c) {
        if (channelIsCovered(c, false, true, false)) continue;

        const string& src = getBlockName(getSrcBlock(c));
        const string& dst = getBlockName(getDstBlock(c));
        // Lana 05/07/19 Adding port index to name
        // Otherwise, if two channels between same two nodes, milp crashes
        const string cname = src + "_" + dst + to_string(getDstPort(c));
        vars.buffer_flop[c] = milp.newBooleanVar(cname + "_flop");
        vars.buffer_slots[c] = milp.newIntegerVar(cname + "_slots");
        vars.has_buffer[c] = milp.newBooleanVar(cname + "_hasBuffer");
    }

    ForAllBlocks(b) {
        const string& bname = getBlockName(b);
        ForAllPorts(b,p) {
            const string& pname = getPortName(p, false);
            vars.time_path[p] = milp.newRealVar("timePath_" + bname + "_" + pname);
            vars.time_elastic[p] = milp.newRealVar("timeElastic_" + bname + "_" + pname);
        }
    }

    if (not max_throughput) return;

    // Variables to model throughput
    vars.in_retime_tokens = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.out_retime_tokens = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.th_tokens = vector<vector<int>>(MG.size(), vector<int> (vecChannelsSize(), -1));
    vars.retime_bubbles = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.th_bubbles = vector<vector<int>>(MG.size(), vector<int> (vecChannelsSize(), -1));
    vars.th_MG = vector<int>(MG.size(), -1);
    for (int mg = 0; mg < MG.size(); ++mg) {

        const string& mg_name = "_mg" + to_string(mg);
        vars.th_MG[mg] = milp.newRealVar();

        for (blockID b: MG[mg].getBlocks()) {
            const string bname = getBlockName(b) + mg_name;
            vars.in_retime_tokens[mg][b] = milp.newRealVar("inRetimeTok_" + bname);
            vars.retime_bubbles[mg][b] = milp.newRealVar("retimeBub_" + bname);
            // If the block is combinational, the in/out retiming variables are the same
            vars.out_retime_tokens[mg][b] = getLatency(b) > 0 ? milp.newRealVar("outRetimeTok_" + bname) : vars.in_retime_tokens[mg][b];
        }

        for (channelID c: MG[mg].getChannels()) {
            const string& src = getBlockName(getSrcBlock(c));
            const string& dst = getBlockName(getDstBlock(c));
            // Lana 05/07/19 Adding port index to name
            // Otherwise, if two channels between same two nodes, milp crashes
            const string cname = src + "_" + dst + to_string(getDstPort(c)) + mg_name;
            vars.th_tokens[mg][c] = milp.newRealVar("thTok_" + cname);
            vars.th_bubbles[mg][c] = milp.newRealVar("thBub_" + cname);
        }
        if (first_MG) break;
    }
}

void DFnetlist_Impl::createMilpVarsEB_sc(Milp_Model &milp, milpVarsEB &vars, bool max_throughput, int mg, bool first_MG) {
    vars.buffer_flop = vector<int>(vecChannelsSize(), -1);
    vars.buffer_slots = vector<int>(vecChannelsSize(), -1);
    vars.has_buffer = vector<int>(vecChannelsSize(), -1);
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_elastic = vector<int>(vecPortsSize(), -1);

    const string& mg_name = "_mg" + to_string(mg);

    ///////////////////////
    /// CHANNELS IN MG  ///
    ///////////////////////
    for (channelID c: MG_disjoint[mg].getChannels()) {
        if (channelIsCovered(c, false, true, false)) continue;

        const string& src = getBlockName(getSrcBlock(c));
        const string& dst = getBlockName(getDstBlock(c));

        // Lana 05/07/19 Adding port index to name
        // Otherwise, if two channels between same two nodes, milp crashes
        const string cname = src + "_" + dst + to_string(getDstPort(c)) + mg_name;
        vars.buffer_flop[c] = milp.newBooleanVar(cname + "_flop");
        vars.buffer_slots[c] = milp.newIntegerVar(cname + "_slots");
        vars.has_buffer[c] = milp.newBooleanVar(cname + "_hasBuffer");
    }

    ////////////////////
    /// BLOCKS IN MG ///
    ////////////////////
    for (blockID b: MG_disjoint[mg].getBlocks()) {
        const string& bname = getBlockName(b);
        ForAllPorts(b,p) {
            if (!MG_disjoint[mg].hasChannel(getConnectedChannel(p)))
                continue;
            const string& pname = getPortName(p, false);
            vars.time_path[p] = milp.newRealVar("timePath_" + bname + "_" + pname + mg_name);
            vars.time_elastic[p] = milp.newRealVar("timeElastic_" + bname + "_" + pname + mg_name);
        }
    }

    ///////////////////////////
    /// BLOCKS IN MG BORDER ///
    ///////////////////////////
    for (blockID b: blocks_in_borders) {
        const string& bname = getBlockName(b);
        ForAllPorts(b,p) {
            portID other_p;
            if (isInputPort(p)) {
                if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(p)))) {
                    continue;
                }
                other_p = getSrcPort(getConnectedChannel(p));
            }
            if (isOutputPort(p)) {
                if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(p)))) {
                    continue;
                }
                other_p = getDstPort(getConnectedChannel(p));
            }

            const string& pname = getPortName(p, false);
            vars.time_path[p] = milp.newRealVar("timePath_" + bname + "_" + pname + mg_name);
            vars.time_elastic[p] = milp.newRealVar("timeElastic_" + bname + "_" + pname + mg_name);

            const string& pname_other = getPortName(other_p, false);
            vars.time_path[other_p] = milp.newRealVar("timePath_" + bname + "_" + pname_other + mg_name);
            vars.time_elastic[other_p] = milp.newRealVar("timeElastic_" + bname + "_" + pname_other + mg_name);
        }
    }

    if (not max_throughput) return;

    // Variables to model throughput
    vars.in_retime_tokens = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.out_retime_tokens = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.th_tokens = vector<vector<int>>(MG.size(), vector<int> (vecChannelsSize(), -1));
    vars.retime_bubbles = vector<vector<int>>(MG.size(), vector<int> (vecBlocksSize(), -1));
    vars.th_bubbles = vector<vector<int>>(MG.size(), vector<int> (vecChannelsSize(), -1));
    vars.th_MG = vector<int>(MG.size(), -1);

    for (auto sub_mg: components[mg]) {

        cout << " creating throughput vars for sub_mg" << sub_mg << endl;
        //cout << "\tadding throughput variables for sub_mg " << sub_mg << endl;
        const string& sub_mg_name = mg_name + "_submg" + to_string(sub_mg);
        vars.th_MG[sub_mg] = milp.newRealVar();

        for (blockID b: MG[sub_mg].getBlocks()) {
            const string bname = getBlockName(b) + sub_mg_name;
            vars.in_retime_tokens[sub_mg][b] = milp.newRealVar("inRetimeTok_" + bname);
            vars.retime_bubbles[sub_mg][b] = milp.newRealVar("retimeBub_" + bname);

            // If the block is combinational, the in/out retiming variables are the same
            vars.out_retime_tokens[sub_mg][b] = getLatency(b) > 0 ? milp.newRealVar("outRetimeTok_" + bname) : vars.in_retime_tokens[sub_mg][b];
        }

        for (channelID c: MG[sub_mg].getChannels()) {
            const string& src = getBlockName(getSrcBlock(c));
            const string& dst = getBlockName(getDstBlock(c));

            // Lana 05/07/19 Adding port index to name
            // Otherwise, if two channels between same two nodes, milp crashes
            const string cname = src + "_" + dst + to_string(getDstPort(c)) + sub_mg_name;
            vars.th_tokens[sub_mg][c] = milp.newRealVar("thTok_" + cname);
            vars.th_bubbles[sub_mg][c] = milp.newRealVar("thBub_" + cname);
        }

        if (first_MG) break;
    }
}

void DFnetlist_Impl::createMilpVars_remaining(Milp_Model &milp, milpVarsEB &vars) {
    vars.buffer_flop = vector<int>(vecChannelsSize(), -1);
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_elastic = vector<int>(vecPortsSize(), -1);

    ForAllChannels(c) {
        if (channelIsCovered(c, true, true, false))
            continue;

        const string& src = getBlockName(getSrcBlock(c));
        const string& dst = getBlockName(getDstBlock(c));
        // Lana 05/07/19 Adding port index to name
        // Otherwise, if two channels between same two nodes, milp crashes
        const string cname = src + "_" + dst + to_string(getDstPort(c));
        vars.buffer_flop[c] = milp.newBooleanVar(cname + "_flop");
    }

    ForAllBlocks(b) {
        const string& bname = getBlockName(b);
        ForAllPorts(b,p) {
            const string& pname = getPortName(p, false);
            vars.time_path[p] = milp.newRealVar("timePath_" + bname + "_" + pname);
            vars.time_elastic[p] = milp.newRealVar("timeElastic_" + bname + "_" + pname);
        }
    }
}


#include <sys/time.h>
long long get_timestamp( void )
{
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    //printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

bool DFnetlist_Impl::addElasticBuffers(double Period, double BufferDelay, bool MaxThroughput, double coverage)
{
    Milp_Model milp;

    //setUnitDelays();

    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }

    hideElasticBuffers();

    if (MaxThroughput) {
        assert (coverage >= 0.0 and coverage <= 1.0);
        coverage = extractMarkedGraphs(coverage);
        cout << "Extracted Marked Graphs with coverage " << coverage << endl;
    }

    // Lana 05/07/19 If nothing to optimize, exit
    if (coverage == 0) {
        return false;
    }

//    addBorderBuffers();
    // Create the variables
    milpVarsEB milpVars;
    createMilpVarsEB(milp, milpVars, MaxThroughput);

    if (not createPathConstraints(milp, milpVars, Period, BufferDelay)) return false;
    if (not createElasticityConstraints(milp, milpVars)) return false;

    // Cost function for buffers: 0.01 * buffers + 0.001 * slots
    double buf_cost = -0.000001;
    double slot_cost = -0.0000001;
    ForAllChannels(c) {
        milp.newCostTerm(buf_cost, milpVars.has_buffer[c]);
        milp.newCostTerm(slot_cost, milpVars.buffer_slots[c]);
    }

    if (MaxThroughput) {
        createThroughputConstraints(milp, milpVars);

        milp.newCostTerm(1, milpVars.th_MG[0]);
       /* for (int i = 0; i < MG.size(); i++) {
            cout << "coef is : " << MGfreq[i] * MG[i].numBlocks() << endl;
            milp.newCostTerm(MGfreq[i] * MG[i].numBlocks(), milpVars.th_MG[i]);
        }*/
        milp.setMaximize();
    }

    milp.setMaximize();
    cout << "Solving MILP for elastic buffers" << endl;

    long long start_time, end_time;
    uint32_t elapsed_time;

    start_time = get_timestamp();
    milp.solve();
    end_time = get_timestamp();

    elapsed_time = ( uint32_t ) ( end_time - start_time ) ;
    printf ("Milp time: [ms] %d \n\r", elapsed_time);

    Milp_Model::Status stat = milp.getStatus();

    if (stat != Milp_Model::OPTIMAL and stat != Milp_Model::NONOPTIMAL) {
        setError("No solution found to add elastic buffers.");
        return false;
    }

    if (MaxThroughput) {
        cout << "************************" << endl;
        cout << "*** Throughput: " << fixed << setprecision(2) << milp[milpVars.th_MG[0]] << " ***" << endl;
        cout << "************************" << endl;
    }

    //dumpMilpSolution(milp, milpVars);

    // Add channels
    vector<channelID> buffers;
    ForAllChannels(c) {
        if (milp[milpVars.buffer_slots[c]] > 0.5) {
            buffers.push_back(c);
        }
    }

    for (channelID c: buffers) {
        int slots = milp[milpVars.buffer_slots[c]] + 0.5; // Automatically truncated
        bool transparent = milp.isFalse(milpVars.buffer_flop[c]);
        setChannelTransparency(c, transparent);
        setChannelBufferSize(c, slots);
        printChannelInfo(c, slots, transparent);
    }

    /*
    for (channelID c: buffers) {
        int v = milpVars.th_tokens[0][c];
        if (v >= 0) cout << "Throughput " << getChannelName(c) << ": " << milp[milpVars.th_tokens[0][c]] << endl;
    }
    */

    //makeNonTransparentBuffers();
    return true;
}

bool DFnetlist_Impl::addElasticBuffersBB(double Period, double BufferDelay, bool MaxThroughput, double coverage, int timeout, bool first_MG) {

    cleanElasticBuffers();

    cout << "======================" << endl;
    cout << "ADDING ELASTIC BUFFERS" << endl;
    cout << "======================" << endl;

    Milp_Model milp;
    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }

    if (MaxThroughput) {
        assert (coverage >= 0.0 and coverage <= 1.0);
        cout << "Extracting marked graphs" << endl;
        coverage = extractMarkedGraphsBB(coverage);
    }

    // Lana 05/07/19 If nothing to optimize, exit
    if (coverage == 0) {
        return false;
    }

    findMCLSQ_load_channels();

    cout << "===========================" << endl;
    cout << "Initiating MILP for buffers" << endl;
    cout << "===========================" << endl;

    milpVarsEB milpVars;
    createMilpVarsEB(milp, milpVars, MaxThroughput, first_MG);
    if (not createPathConstraints(milp, milpVars, Period, BufferDelay)) return false;
    if (not createElasticityConstraints(milp, milpVars)) return false;

    double highest_coef = 1.0, order_buf = 0.0001, order_slot = 0.00001;
    if (MaxThroughput) {
        createThroughputConstraints(milp, milpVars, first_MG);

        computeChannelFrequencies();
        double total_freq = 0;
        ForAllChannels(c) total_freq += getChannelFrequency(c);

        int optimize_num = first_MG ? 1 : MG.size();
        double mg_highest_coef = 0.0;
        for (int i = 0; i < optimize_num; i++) {
            double coef = MG[i].numChannels() * MGfreq[i] / total_freq;
            milp.newCostTerm(coef, milpVars.th_MG[i]);
            mg_highest_coef = mg_highest_coef > coef ? mg_highest_coef : coef;
        }
        highest_coef = mg_highest_coef;
    }

    ForAllChannels(c) {
        milp.newCostTerm(-1 * order_buf * highest_coef, milpVars.has_buffer[c]);
        milp.newCostTerm(-1 * order_slot * highest_coef, milpVars.buffer_slots[c]);
    }

    milp.setMaximize();

    cout << "Solving MILP for elastic buffers" << endl;
    long long start_time, end_time;
    uint32_t elapsed_time;
    start_time = get_timestamp();
    if (timeout > 0) milp.solve(timeout);
    else milp.solve();
    end_time = get_timestamp();
    elapsed_time = ( uint32_t ) ( end_time - start_time ) ;
    printf ("Milp time: [ms] %d \n\r", elapsed_time);

    Milp_Model::Status stat = milp.getStatus();
    if (stat != Milp_Model::OPTIMAL and stat != Milp_Model::NONOPTIMAL) {
        setError("No solution found to add elastic buffers.");
        return false;
    }

    if (MaxThroughput) {
        int optimize_num = first_MG ? 1 : MG.size();
        for (int i = 0; i < optimize_num; i++) {
            cout << "************************" << endl;
            cout << "*** Throughput for MG " << i << ": ";
            cout << fixed << setprecision(2) << milp[milpVars.th_MG[i]] << " ***" << endl;
            cout << "************************" << endl;
        }
    }

    // Add channels
    vector<channelID> buffers;
    ForAllChannels(c) {
        if (channelIsCovered(c, false, false, true)) continue;
        if (milp[milpVars.has_buffer[c]]) {
            buffers.push_back(c);
        }
    }

    int total_slot_count = 0;
    for (channelID c: buffers) {
        int slots = milp[milpVars.buffer_slots[c]] + 0.5; // Automatically truncated
        bool transparent = milp.isFalse(milpVars.buffer_flop[c]);
        setChannelTransparency(c, transparent);
        setChannelBufferSize(c, slots);

        printChannelInfo(c, slots, transparent);
    }

    return true;
}

bool DFnetlist_Impl::addElasticBuffersBB_sc(double Period, double BufferDelay, bool MaxThroughput, double coverage, int timeout, bool first_MG) {

    cleanElasticBuffers();

    cout << "======================" << endl;
    cout << "ADDING ELASTIC BUFFERS" << endl;
    cout << "======================" << endl;

    Milp_Model milp;

    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }

    if (MaxThroughput) {
        assert (coverage >= 0.0 and coverage <= 1.0);
        cout << "Extracting marked graphs" << endl;
        coverage = extractMarkedGraphsBB(coverage);
    }

    // Lana 05/07/19 If nothing to optimize, exit
    if (coverage == 0) {
        return false;
    }

    findMCLSQ_load_channels();
    addBorderBuffers();

    // Lana 03/05/20 Muxes and merges have internal buffers, fork going to lsq must have one after 
    ForAllChannels(c) {
        if ((getBlockType(getSrcBlock(c)) == MUX)
            || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
            setChannelTransparency(c, 1);
            setChannelBufferSize(c, 1);
        }
        if (getBlockType(getSrcBlock(c)) == FORK && getBlockType(getDstBlock(c)) == BRANCH) {
            if (getPortWidth(getDstPort(c)) == 0) {
                bool lsq_fork = false;

                ForAllOutputPorts(getSrcBlock(c), out_p) {
                    if (getBlockType(getDstBlock(getConnectedChannel(out_p))) == LSQ)
                        lsq_fork = true;
                    
                }
                if (lsq_fork) {
                    setChannelTransparency(c, 0);
                    setChannelBufferSize(c, 1);
                }
            }
        }

    }                    

    calculateDisjointCFDFCs();
    makeMGsfromCFDFCs();

    auto milpVars_sc = vector<milpVarsEB>(MG_disjoint.size(), milpVarsEB());
    long long total_time = 0;
    double order_buf = 0.0001, order_slot = 0.00001;

    if (MaxThroughput) computeChannelFrequencies();

    for (int i = 0; i < MG_disjoint.size(); i++) {
        cout << "-------------------------------" << endl;
        cout << "Initiating MILP for MG number " << i << endl;
        cout << "-------------------------------" << endl;

        createMilpVarsEB_sc(milp, milpVars_sc[i], MaxThroughput, i, first_MG);
        if (not createPathConstraints_sc(milp, milpVars_sc[i], Period, BufferDelay, i)) return false;
        if (not createElasticityConstraints_sc(milp, milpVars_sc[i], i)) return false;


        double highest_coef = 1.0;
        if (MaxThroughput) {
            createThroughputConstraints_sc(milp, milpVars_sc[i], i, first_MG);

            double total_freq = 0;
            for (channelID c: MG_disjoint[i].getChannels()) {
                total_freq += getChannelFrequency(c);
            }

            double mg_highest_coef = 0.0;
            for (auto sub_mg: components[i]) {
                double coef = MG[sub_mg].numChannels() * MGfreq[sub_mg] / total_freq;
                milp.newCostTerm(coef, milpVars_sc[i].th_MG[sub_mg]);
                mg_highest_coef = mg_highest_coef > coef ? mg_highest_coef : coef;
                if (first_MG) break;
            }
            highest_coef = mg_highest_coef;
        }

        for (channelID c: MG_disjoint[i].getChannels()) {
            if (channelIsCovered(c, false, true, false)) continue;

            milp.newCostTerm(-1 * order_buf * highest_coef, milpVars_sc[i].has_buffer[c]);
            milp.newCostTerm(-1 * order_slot * highest_coef, milpVars_sc[i].buffer_slots[c]);
        }

        milp.setMaximize();

        cout << "Solving MILP for elastic buffers: MG " << i << endl;

        long long start_time, end_time;
        uint32_t elapsed_time;
        start_time = get_timestamp();
        if (timeout > 0) milp.solve(timeout);
        else milp.solve();
        end_time = get_timestamp();
        elapsed_time = ( uint32_t ) ( end_time - start_time ) ;
        printf ("Milp time for MG %d: [ms] %d \n\n\r", i, elapsed_time);
        total_time += elapsed_time;

        Milp_Model::Status stat = milp.getStatus();
        if (stat != Milp_Model::OPTIMAL and stat != Milp_Model::NONOPTIMAL) {
            setError("No solution found to add elastic buffers.");
            return false;
        }

        if (MaxThroughput) {
            for (auto sub_mg: components[i]) {
                cout << "************************" << endl;
                cout << "*** Throughput for MG " << sub_mg << " in disjoint MG " << i << ": ";
                cout << fixed << setprecision(2) << milp[milpVars_sc[i].th_MG[sub_mg]] << " ***" << endl;
                cout << "************************" << endl;
                if (first_MG) break;
            }
        }

         //dumpMilpSolution(milp, milpVars_sc[i]);

        // Add channels
        vector<channelID> buffers;
        for (channelID c: MG_disjoint[i].getChannels()) {
            if (channelIsCovered(c, false, true, true)) continue;
            if (milp[milpVars_sc[i].buffer_slots[c]] > 0.5) {
                buffers.push_back(c);
            }
        }

        for (channelID c: buffers) {
            int slots = milp[milpVars_sc[i].buffer_slots[c]] + 0.5; // Automatically truncated
            bool transparent = milp.isFalse(milpVars_sc[i].buffer_flop[c]);
            setChannelTransparency(c, transparent);
            setChannelBufferSize(c, slots);

            printChannelInfo(c, slots, transparent);
        }

        //write retiming diffs
        writeRetimingDiffs(milp, milpVars_sc[i]);


        if (MaxThroughput) {
            for (auto sub_mg: components[i]) {
                cout << "\n*** Throughput achieved in sub MG " << sub_mg << ": " <<
                     fixed << setprecision(2) << milp[milpVars_sc[i].th_MG[sub_mg]] << " ***\n" << endl;
                if (first_MG) break;
            }
        }
        if (not milp.init(getMilpSolver())) {
            setError(milp.getError());
            return false;
        }
    }


    cout << "--------------------------------------" << endl;
    cout << "Initiating MILP for remaining channels" << endl;
    cout << "--------------------------------------" << endl;

    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }



    milpVarsEB remaining;

    createMilpVars_remaining(milp, remaining);
    createPathConstraints_remaining(milp, remaining, Period, BufferDelay);
    createElasticityConstraints_remaining(milp, remaining);

    ForAllChannels(c) {
        if (channelIsCovered(c, true, true, false))
            continue;
        milp.newCostTerm(1, remaining.buffer_flop[c]);
    }
    milp.setMinimize();

    cout << "Solving MILP for channels not covered by MGs" << endl;

    long long start_time, end_time;
    uint32_t elapsed_time;
    start_time = get_timestamp();
    if (timeout > 0) milp.solve(timeout);
    else milp.solve();
    end_time = get_timestamp();
    elapsed_time = ( uint32_t ) ( end_time - start_time ) ;
    printf ("Milp time for remaining channels: [ms] %d \n\n\r", elapsed_time);
    total_time += elapsed_time;

    Milp_Model::Status stat = milp.getStatus();
    if (stat != Milp_Model::OPTIMAL and stat != Milp_Model::NONOPTIMAL) {
        setError("No solution found to add elastic buffers.");
        return true;
    }
    // Add channels
    vector<channelID> buffers;
    ForAllChannels(c) {
        if (channelIsCovered(c, true, true, true)) continue;

        if (milp[remaining.buffer_flop[c]]) {
            buffers.push_back(c);
        }
    }

    for (channelID c: buffers) {
        setChannelTransparency(c, 0);
        setChannelBufferSize(c, 1);

        printChannelInfo(c, 1, 0);
    }

    cout << "***************************" << endl;
    printf ("Total MILP time: [ms] %d\n\r", total_time);
    cout << "***************************" << endl;
    return true;
}



bool DFnetlist_Impl::createPathConstraints(Milp_Model& milp, milpVarsEB& Vars, double Period, double BufferDelay)
{

    // Create the variables and constraints for all channels
    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;


    ForAllChannels(c) {
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;

        int v1 =  Vars.time_path[getSrcPort(c)];
        int v2 =  Vars.time_path[getDstPort(c)];
        int R = Vars.buffer_flop[c];

        if (hasPeriod) {
            // v1, v2 <= Period
            milp.newRow( {{1,v1}}, '<', Period);
            milp.newRow( {{1,v2}}, '<', Period);

            // v2 >= v1 - 2*period*R
            if (channelIsCovered(c, false, true, false))
                milp.newRow({{-1, v1}, {1, v2}}, '>', -2 * Period * !isChannelTransparent(c));
            else
                milp.newRow ({{-1,v1}, {1,v2}, {2 * Period, R}}, '>', 0);
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

bool DFnetlist_Impl::createPathConstraints_sc(Milp_Model &milp, milpVarsEB &Vars, double Period,
                                              double BufferDelay, int mg) {

    // Create the variables and constraints for all channels
    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;

    //////////////////////
    /** CHANNELS IN MG **/
    //////////////////////

    //cout << "   path constraints for channels in MG" << endl;
    for (channelID c: MG_disjoint[mg].getChannels()) {
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;
        int v1 =  Vars.time_path[getSrcPort(c)];
        int v2 =  Vars.time_path[getDstPort(c)];
        int R = Vars.buffer_flop[c];

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

    //////////////////////////////
    /// CHANNELS IN MG BORDERS ///
    //////////////////////////////

    //cout << "   path constraints for channels in MG borders" << endl;
    for (channelID c: channels_in_borders) {
        if (!MG_disjoint[mg].hasBlock(getSrcBlock(c)) && !MG_disjoint[mg].hasBlock(getDstBlock(c))) continue;

        int v1 = Vars.time_path[getSrcPort(c)];
        int v2 = Vars.time_path[getDstPort(c)];
        int R = !isChannelTransparent(c);

        assert(R == 1);

        if (hasPeriod) {
            // v1, v2 <= Period
            milp.newRow( {{1,v1}}, '<', Period);
            milp.newRow( {{1,v2}}, '<', Period);

            // v2 >= v1 - 2*period*R
            milp.newRow ( {{-1,v1}, {1,v2}}, '>', -2 * Period * R);
        }

        // v2 >= Buffer Delay
        if (BufferDelay > 0) milp.newRow( {{1,v2}}, '>', BufferDelay);
    }

    ////////////////////
    /// BLOCKS IN MG ///
    ////////////////////

    //cout << "   path constraints for blocks in MG" << endl;
    // Create the constraints to propagate the delays of the blocks
    for (blockID b: MG_disjoint[mg].getBlocks()) {
        double d = getBlockDelay(b);

        // First: combinational blocks
        if (getLatency(b) == 0) {
            ForAllOutputPorts(b, out_p) {
                if (!MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                    continue;

                int v_out = Vars.time_path[out_p];

                ForAllInputPorts(b, in_p) {
                    if (!MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                        continue;

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
                if (!MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                    continue;

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
                if (!MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                    continue;

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

    ////////////////////////////
    /// BLOCKS IN MG BORDERS ///
    ////////////////////////////
    //cout << "   path constraints for blocks in MG border" << endl;
    for (blockID b: blocks_in_borders) {
        double d = getBlockDelay(b);

        // First: combinational blocks
        if (getLatency(b) == 0) {
            ForAllOutputPorts(b, out_p) {
                if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

                int v_out = Vars.time_path[out_p];

                ForAllInputPorts(b, in_p) {
                    if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                    int v_in = Vars.time_path[in_p];

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
                if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

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
                if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                double d_in = getPortDelay(in_p);
                int v_in = Vars.time_path[in_p];
                if (d_in + BufferDelay > Period) {
                    setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                    return false;
                }
                if (hasPeriod) milp.newRow( {{1, v_in}}, '<', Period - d_in);
            }
        }
    }
    return true;
}

bool DFnetlist_Impl::createPathConstraints_remaining(Milp_Model &milp, milpVarsEB &Vars, double Period,
                                                     double BufferDelay) {

    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;

    ForAllChannels(c) {
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;

        int v1 =  Vars.time_path[getSrcPort(c)];
        int v2 =  Vars.time_path[getDstPort(c)];
        int R = Vars.buffer_flop[c];

        if (hasPeriod) {
            // v1, v2 <= Period
            milp.newRow( {{1,v1}}, '<', Period);
            milp.newRow( {{1,v2}}, '<', Period);

            // v2 >= v1 - 2*period*R
            if (channelIsCovered(c, true, true, false))
                milp.newRow({{-1, v1}, {1, v2}}, '>', -2 * Period * !isChannelTransparent(c));
            else
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
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;

        int v1 = Vars.time_elastic[getSrcPort(c)];
        int v2 = Vars.time_elastic[getDstPort(c)];
        int slots = Vars.buffer_slots[c];
        int hasbuf = Vars.has_buffer[c];
        int hasflop = Vars.buffer_flop[c];

        if (channelIsCovered(c, false, true, false)) {
            milp.newRow ( {{-1,v1}, {1,v2}}, '>', -1 * big_constant * !isChannelTransparent(c));
        }
        else {
            // v2 >= v1 - big_constant*R (path constraint: at least one slot)
            milp.newRow ( {{-1,v1}, {1,v2}, {big_constant, hasflop}}, '>', 0);

            // There must be at least one slot per flop (slots >= hasflop)
            milp.newRow ( {{1, slots}, {-1, hasflop}}, '>', 0);

            // HasBuffer >= 0.01 * slots (1 if there is a buffer, and 0 otherwise)
            milp.newRow ( {{1,hasbuf}, {-0.01, slots}}, '>', 0);
        }
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

bool DFnetlist_Impl::createElasticityConstraints_sc(Milp_Model &milp, milpVarsEB &Vars, int mg) {
    // Upper bound for the longest rigid path. Blocks are assumed to have unit delay.
    double big_constant = numBlocks() + 1;

    //////////////////////
    /// CHANNELS IN MG ///
    //////////////////////
    // Create the variables and constraints for all channels
    //cout << "   elasticity constraints for channels in MG" << endl;
    for (channelID c: MG_disjoint[mg].getChannels()) {
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;

        int v1 = Vars.time_elastic[getSrcPort(c)];
        int v2 = Vars.time_elastic[getDstPort(c)];
        int slots = Vars.buffer_slots[c];
        int hasbuf = Vars.has_buffer[c];
        int hasflop = Vars.buffer_flop[c];

        // Lana 03/05/20 Muxes and merges have internal buffers, fork going to lsq must have one after
        if ((getBlockType(getSrcBlock(c)) == MUX)
            || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
            slots = 1; 
            hasbuf = 1; 
            hasflop = 0;
        }

        if (getBlockType(getSrcBlock(c)) == FORK && getBlockType(getDstBlock(c)) == BRANCH) {
            if (getPortWidth(getDstPort(c)) == 0) {
                bool lsq_fork = false;

                ForAllOutputPorts(getSrcBlock(c), out_p) {
                    if (getBlockType(getDstBlock(getConnectedChannel(out_p))) == LSQ)
                        lsq_fork = true;
                    
                }
                if (lsq_fork) {
                    slots = 1; 
                    hasbuf = 1; 
                    hasflop = 1;
                }
            }
        }
        

        // v2 >= v1 - big_constant*R (path constraint: at least one slot)
        milp.newRow ( {{-1,v1}, {1,v2}, {big_constant, hasflop}}, '>', 0);

        // There must be at least one slot per flop (slots >= hasflop)
        milp.newRow ( {{1, slots}, {-1, hasflop}}, '>', 0);

        // HasBuffer >= 0.01 * slots (1 if there is a buffer, and 0 otherwise)
        milp.newRow ( {{1,hasbuf}, {-0.01, slots}}, '>', 0);
    }

    /////////////////////////////
    /// CHANNELS IN MG BORDER ///
    /////////////////////////////
    //cout << "   elasticity constraints for channels in MG border" << endl;
    for (channelID c: channels_in_borders) {
        if (!MG_disjoint[mg].hasBlock(getSrcBlock(c)) && !MG_disjoint[mg].hasBlock(getDstBlock(c))) continue;

        int v1 = Vars.time_elastic[getSrcPort(c)];
        int v2 = Vars.time_elastic[getDstPort(c)];

        milp.newRow ( {{-1,v1}, {1,v2}}, '>', -1 * big_constant * !isChannelTransparent(c));
    }

    ////////////////////
    /// BLOCKS IN MG ///
    ////////////////////
    // Create the constraints to propagate the delays of the blocks
    //cout << "   elasticity constraints for blocks in MG" << endl;
    for (blockID b: MG_disjoint[mg].getBlocks()) {
        ForAllOutputPorts(b, out_p) {
            if (!MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                continue;

            int v_out = Vars.time_elastic[out_p];
            ForAllInputPorts(b, in_p) {
                if (!MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                    continue;

                int v_in = Vars.time_elastic[in_p];
                // Add constraint v_out >= 1 + v_in; (unit delay)
                milp.newRow( {{1, v_out}, {-1,v_in}}, '>', 1);
            }
        }
    }

    ///////////////////////////
    /// BLOCKS IN MG BORDER ///
    ///////////////////////////
    //cout << "   elasticity constraints for blocks in MG border" << endl;
    for (blockID b: blocks_in_borders) {
        ForAllOutputPorts(b, out_p) {
            if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

            int v_out = Vars.time_elastic[out_p];

            ForAllInputPorts(b, in_p) {
                if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                int v_in = Vars.time_elastic[in_p];

                // Add constraint v_out >= 1 + v_in; (unit delay)
                milp.newRow( {{1, v_out}, {-1,v_in}}, '>', 1);
            }
        }
    }

    return true;
}

bool DFnetlist_Impl::createElasticityConstraints_remaining(Milp_Model &milp, milpVarsEB &Vars) {
    // Upper bound for the longest rigid path. Blocks are assumed to have unit delay.
    double big_constant = numBlocks() + 1;

    // Create the variables and constraints for all channels
    ForAllChannels(c) {
        // Lana 02.05.20. paths from/to memory do not need buffers
        if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
         || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
            continue;
        int v1 = Vars.time_elastic[getSrcPort(c)];
        int v2 = Vars.time_elastic[getDstPort(c)];

        if (channelIsCovered(c, true, true, false))
            milp.newRow ( {{-1,v1}, {1,v2}}, '>', -1 * big_constant * !isChannelTransparent(c));
        else
            milp.newRow ( {{-1,v1}, {1,v2}, {big_constant, Vars.buffer_flop[c]}}, '>', 0);
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


bool DFnetlist_Impl::createThroughputConstraints(Milp_Model& milp, milpVarsEB& Vars, bool first_MG)
{
    double frac = 0.5;
    // For every MG, for every channel c
    for (int mg = 0; mg < MG.size(); ++mg) {
        int th_mg = Vars.th_MG[mg]; // Throughput of the marked graph.
        for (channelID c: MG[mg].getChannels()) {

            // Ignore select input channel which is less frequently executed
            if (getBlockType(getDstBlock(c)) == OPERATOR &&  getOperation(getDstBlock(c)) == "select_op") {
                if ((getPortType(getDstPort(c)) == TRUE_PORT) && (getTrueFrac(getDstBlock(c)) < frac)){
                    continue;
                }
                if ((getPortType(getDstPort(c)) == FALSE_PORT) && (getTrueFrac(getDstBlock(c)) > frac)){
                    continue;
                }
            }

             // Lana 02.05.20. LSQ serves as FIFO, so we do not need FIFOs on paths to lsq store
            if (getBlockType(getDstBlock(c)) == OPERATOR &&  getOperation(getDstBlock(c)) == "lsq_store_op") 
                    continue;

            //cout << "  Channel ";
            //if (isBackEdge(c)) cout << "(backedge) ";
            //cout << getChannelName(c) << endl;

            int th_tok = Vars.th_tokens[mg][c];  // throughput of the channel (tokens)
            int Slots = Vars.buffer_slots[c];    // Slots in the channel
            int hasFlop = Vars.buffer_flop[c];   // Is there a flop?

            if (channelIsCovered(c, false, true, false)) cout << "something is wrong" << endl;

            // Variables for retiming tokens and bubbles through blocks
            int ret_src_tok = Vars.out_retime_tokens[mg][getSrcBlock(c)];
            int ret_dst_tok = Vars.in_retime_tokens[mg][getDstBlock(c)];

            // Initial token
            int token = isBackEdge(c) ? 1 : 0;

            milp.newRow( {{-1, ret_dst_tok}, {1, ret_src_tok}, {1, th_tok}}, '=', token);
            milp.newRow( {{1, th_mg}, {1, hasFlop}, {-1, th_tok}}, '<', 1);

            // There must be registers to handle the tokens and bubbles Slots >= th_tok + th_bub
            milp.newRow( {{1, th_tok}, {1, th_mg}, {1, hasFlop}, {-1, Slots}}, '<', 1);
            milp.newRow( {{1, th_tok}, {-1, Slots}}, '<', 0);
        }

        // Special treatment for pipelined units
        for (blockID b: MG[mg].getBlocks()) {

            double lat = getLatency(b);
            if (lat == 0) continue;
            int in_ret_tok = Vars.in_retime_tokens[mg][b];
            int out_ret_tok = Vars.out_retime_tokens[mg][b];

            double maxTokens = lat/getInitiationInterval(b);
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}}, '<', maxTokens);
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '>', 0);
        }

        if (first_MG) break;
    }
    return true;
}

bool DFnetlist_Impl::createThroughputConstraints_sc(Milp_Model &milp, milpVarsEB &Vars, int mg, bool first_MG) {
    // For every MG, for every channel c

    // if you want to only consider one MG in the throughput, comment the loop and uncomment this line:
    //int sub_mg = components[mg][0];
    double frac = 0.5;
    for (auto sub_mg: components[mg]) {
        int th_mg = Vars.th_MG[sub_mg]; // Throughput of the marked graph.
        for (channelID c: MG[sub_mg].getChannels()) {

            // Ignore select input channel which is less frequently executed
            if (getBlockType(getDstBlock(c)) == OPERATOR &&  getOperation(getDstBlock(c)) == "select_op") {
                if ((getPortType(getDstPort(c)) == TRUE_PORT) && (getTrueFrac(getDstBlock(c)) < frac)){
                    continue;
                }
                if ((getPortType(getDstPort(c)) == FALSE_PORT) && (getTrueFrac(getDstBlock(c)) > frac)){
                    continue;
                }
            }

             // Lana 02.05.20. LSQ serves as FIFO, so we do not need FIFOs on paths to lsq store
            if (getBlockType(getDstBlock(c)) == OPERATOR &&  getOperation(getDstBlock(c)) == "lsq_store_op") 
                    continue;
    //        cout << "  Channel ";
    //        if (isBackEdge(c)) cout << "(backedge) ";
     //           cout << getChannelName(c) << " MG " << sub_mg << endl;


            int th_tok = Vars.th_tokens[sub_mg][c];  // throughput of the channel (tokens)
            //int th_bub = Vars.th_bubbles[mg][c]; // throughput of the channel (bubbles)
            int Slots = Vars.buffer_slots[c];    // Slots in the channel
            int hasFlop = Vars.buffer_flop[c];   // Is there a flop?

            // Variables for retiming tokens and bubbles through blocks
            int ret_src_tok = Vars.out_retime_tokens[sub_mg][getSrcBlock(c)];
            int ret_dst_tok = Vars.in_retime_tokens[sub_mg][getDstBlock(c)];
            //int ret_src_bub = Vars.retime_bubbles[mg][getSrcBlock(c)];
            //int ret_dst_bub = Vars.retime_bubbles[mg][getDstBlock(c)];
            // Initial token
            int token = isBackEdge(c) ? 1 : 0;
            // Token + ret_dst - ret_src = Th_channel
            milp.newRow( {{-1, ret_dst_tok}, {1, ret_src_tok}, {1, th_tok}}, '=', token);
            //milp.newRow( {{-1, ret_dst_bub}, {1, ret_src_bub}, {1, th_bub}}, '=', token);
            // Th_channel >= Th - 1 + flop
            milp.newRow( {{1, th_mg}, {1, hasFlop}, {-1, th_tok}}, '<', 1);
            //milp.newRow( {{1, th_mg}, {1, hasFlop}, {-1, th_bub}}, '<', 1);
            // There must be registers to handle the tokens and bubbles Slots >= th_tok + th_bub
            //milp.newRow( {{1,Slots}, {-1,th_tok}, {-1,th_bub}}, '>', 0);
            //milp.newRow( {{1,Slots}, {-2,th_tok}}, '>', 0);
            //milp.newRow( {{1,Slots}, {-1,th_tok}}, '>', 0);
            milp.newRow( {{1, th_tok}, {1, th_mg}, {1, hasFlop}, {-1, Slots}}, '<', 1);
            milp.newRow( {{1, th_tok}, {-1, Slots}}, '<', 0);
        }

        //continue;
        // Special treatment for pipelined units
        for (blockID b: MG[sub_mg].getBlocks()) {

            double lat = getLatency(b);
            if (lat == 0) continue;
            int in_ret_tok = Vars.in_retime_tokens[sub_mg][b];
            int out_ret_tok = Vars.out_retime_tokens[sub_mg][b];

            double maxTokens = lat/getInitiationInterval(b);
            // rout_tok-rin_tok <= Lat/II
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}}, '<', maxTokens);
            // Th*Lat <= rout-rin:   rout-rin-Th*lat >= 0
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '>', 0);
        }

        if (first_MG) break;
    }
    return true;
}



blockID DFnetlist_Impl::insertBuffer(channelID c, int slots, bool transparent)
{
    portID src = getSrcPort(c);
    portID dst = getDstPort(c);
    bool back = isBackEdge(c);
    blockID b_src = getSrcBlock(c);
    bbID bb_src = getBasicBlock(b_src);

    removeChannel(c);

//    cout << "> Inserting buffer on channel " << getPortName(src)
//         << " -> " << getPortName(dst) << endl;
    

    int width = getPortWidth(src);
    blockID eb = createBlock(ELASTIC_BUFFER);
    setBufferSize(eb, slots);
    setBufferTransparency(eb, transparent);
    setBasicBlock(eb, bb_src);
    portID in_buf = createPort(eb, true, "in1", width);
    portID out_buf = createPort(eb, false, "out1", width);
    //cout << "> Buffer created with ports "
    //     << getPortName(in_buf) << " and " << getPortName(out_buf) << endl;
    createChannel(src, in_buf);
    channelID new_c = createChannel(out_buf, dst);
    setBackEdge(new_c, back);
    return eb;
}

channelID DFnetlist_Impl::removeBuffer(blockID buf)
{
    assert(getBlockType(buf) == ELASTIC_BUFFER);

    cout << "SHAB: removing buffer " << getBlockName(buf) << endl;
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
    std::cout << "INSTANTIATE";

    vecChannels ebs;
    ForAllChannels(c) {

        if (hasBuffer(c)) ebs.push_back(c);
    }

    for (channelID c: ebs) {
        // Lana 03/05/20 Muxes and merges have internal buffers, instantiate only if size/transparency changed by milp
        if ((getBlockType(getSrcBlock(c)) == MUX)
            || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
            if (getChannelBufferSize(c) == 1 & isChannelTransparent(c))
                continue;
            if (getChannelBufferSize(c) == 2 ) {
            	insertBuffer(c, 1, isChannelTransparent(c));
                continue;
            }
        }

        insertBuffer(c, getChannelBufferSize(c), isChannelTransparent(c));
    }

   // Lana 02/07/19 We don't need this
    //invalidateBasicBlocks();
}

void DFnetlist_Impl::addBorderBuffers(){

    cout << "----------------------" << endl;
    cout << "buffers on MG borders" << endl;
    cout << "----------------------" << endl;

    ForAllChannels(c) {
        bbID src = getSrcBlock(c);
        bbID dst = getDstBlock(c);

        // ignore the blocks that don't belong to any bb
        if (getBasicBlock(src) == 0 || getBasicBlock(dst) == 0) continue;

        // the channel should have exactly one end in an MG
        if (blockIsInMGs(src) ^ blockIsInMGs(dst)) {

            setChannelTransparency(c, 0);
            setChannelBufferSize(c, 1);

            // so we won't consider it for the next milp
            channels_in_borders.insert(c);
            if (blockIsInMGs(src)) blocks_in_borders.insert(dst);
            else if (blockIsInMGs(dst)) blocks_in_borders.insert(src);

            printChannelInfo(c, 1, 0);
        }
    }
    cout << endl;
}


void DFnetlist_Impl::findMCLSQ_load_channels() {
    cout << "determining buffer from/to MC_LSQ units to/from loads." << endl;
    ForAllChannels(c) {
        BlockType src_type = getBlockType(getSrcBlock(c));
        BlockType dst_type = getBlockType(getDstBlock(c));
        string src_op = getOperation(getSrcBlock(c));
        string dst_op = getOperation(getDstBlock(c));
        bool src_lsq = false, dst_lsq = false, src_load = false, dst_load = false;
        if (src_type == MC || src_type == LSQ) src_lsq = true;
        if (dst_type == MC || dst_type == LSQ) dst_lsq = true;
        if (src_op == "lsq_load_op" || src_op == "mc_load_op") src_load = true;
        if (dst_op == "lsq_load_op" || dst_op == "mc_load_op") dst_load = true;

/*
        cout << getChannelName(c) << endl;
        cout << "\tsource type is " << src_type;
        cout << "destination type is " << dst_type;
        cout << "src_lsq: " << src_lsq << ", src_load: " << src_load;
        cout << "dst_lsq: " << dst_lsq << ", dst_load: " << dst_load << endl;
*/

        assert(!(src_lsq && src_load) && !(dst_lsq && dst_load));
        if ((src_lsq && dst_load) || (src_load && dst_lsq)) {
           // cout << "\tis from/to load to/from MC LSQ" << endl;
            channels_in_MC_LSQ.insert(c);
        }
    }
}

void DFnetlist_Impl::hideElasticBuffers()
{
    vecBlocks bls;
    //cout << "making bls " << endl;
    ForAllBlocks(b) {
        if (getBlockType(b) == ELASTIC_BUFFER) bls.push_back(b);
    }

    for (blockID b: bls) {
        //cout << "removing block " << getBlockName(b) << endl;
        int slots = getBufferSize(b);
        bool transp = isBufferTransparent(b);
        channelID c = removeBuffer(b);
        setChannelBufferSize(c, slots);
        setChannelTransparency(c, transp);
    }
}

void DFnetlist_Impl::cleanElasticBuffers()
{
    //cout << "cleaning buffers" << endl;
    hideElasticBuffers();
    ForAllChannels(c) {
        //cout << "prev=> buf-size: " << getChannelBufferSize(c) << ", trans: " << isChannelTransparent(c) << endl;
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
/*    ForAllBlocks(b) {
        int in_ret = vars.in_retime_tokens[0][b];
        if (in_ret < 0) continue;
        int out_ret = vars.out_retime_tokens[0][b];
        int ret_bub = vars.retime_bubbles[0][b];
        cout << "Block " << getBlockName(b) << ": ";
        if (in_ret == out_ret) cout << "Retiming tok = " << milp[in_ret];
        else cout << "In Retiming tok = " << milp[in_ret] << ", Out Retiming tok = " << milp[out_ret];
        cout << ", Retiming bub = " << milp[ret_bub] << endl;
    }

    ForAllChannels(c) {
        cout << "Channel " << getChannelName(c) << ": Slots = " << milp[vars.buffer_slots[c]] << ", Flop = " << milp[vars.buffer_flop[c]];
        int th = vars.th_tokens[0][c];
        if (th >= 0) cout << ", th_tok = " << milp[th];
        th = vars.th_bubbles[0][c];
        if (th >= 0) cout << ", th_bub = " << milp[th];
        cout << endl;
    }
*/

    cout << "PATH CONSTRAINTS" << endl;
    ForAllChannels(c) {
        int v_in = vars.time_path[getSrcPort(c)], v_out = vars.time_path[getDstPort(c)];
        if (v_in == -1 || v_out == -1) continue;
        cout << getChannelName(c) << endl;
        cout << "time_path src: " << milp[v_in] << ", ";
        cout << "time_path dst: " << milp[v_out] << endl;
        cout << endl;
    }

    cout << "ELASTICITY CONSTRAINTS" << endl;
    ForAllChannels(c) {
        int v_in = vars.time_elastic[getSrcPort(c)], v_out = vars.time_elastic[getDstPort(c)];
        if (v_in == -1 || v_out == -1) continue;
        cout << getChannelName(c) << endl;
        cout << "time_elastic src: " << milp[v_in] << ", ";
        cout << "time_elastic dst: " << milp[v_out] << endl;
        cout << endl;
    }

    cout << "THROUGHPUT CONSTRAINTS" << endl;
    for (int mg = 0; mg < MG.size(); mg++) {
        int th_mg = vars.th_MG[mg];
        if (th_mg == -1) continue;

        cout << "th_mg " << mg <<": "<< milp[th_mg] << endl;
        for (channelID c: MG[mg].getChannels()) {
            int th_tok = vars.th_tokens[mg][c];
            int Slots = vars.buffer_slots[c];
            int hasFlop = vars.buffer_flop[c];
            if (th_tok == -1 || Slots == -1 || hasFlop == -1) continue;

            int ret_src_tok = vars.out_retime_tokens[mg][getSrcBlock(c)];
            int ret_dst_tok = vars.in_retime_tokens[mg][getDstBlock(c)];
            if (ret_src_tok == -1 || ret_dst_tok == -1) continue;

            cout << getChannelName(c) << endl;
            cout << "th_tok: " << milp[th_tok] << ", ";
            cout << "slots: " << milp[Slots] << ", ";
            cout << "hasFlop: " << milp[hasFlop] << ", ";
            cout << "ret_src_tok: " << milp[ret_src_tok] << ", ";
            cout << "ret_dst_tok: " << milp[ret_dst_tok] << endl;
            cout << endl;
        }
    }

}

void DFnetlist_Impl::writeRetimingDiffs(const Milp_Model& milp, const milpVarsEB& vars)
{
    ForAllBlocks(b) {
        int in_ret = vars.in_retime_tokens[0][b];
        if (in_ret < 0) continue;
        int out_ret = vars.out_retime_tokens[0][b];
        int ret_bub = vars.retime_bubbles[0][b];
        //cout << "Block " << getBlockName(b) << ": ";
        if (in_ret == out_ret) {
            setBlockRetimingDiff(b, milp[in_ret]);
        } else {
			setBlockRetimingDiff(b, milp[out_ret] - milp[in_ret]);
        }
        //cout << ", Retiming bub = " << milp[ret_bub] << endl;
    }
}
