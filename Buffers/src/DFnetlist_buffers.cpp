#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <regex>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

#define FPL_22_CARMINE_LIB false
//#define FPL_22_CARMINE_LIB true

#define OPEN_CIRCUIT_DELAY 100.0

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


//Carmine 22.02.22 This vector defines a table of mixed connections between pins of different timing domains inside a block
//
//                  For each block there is a string that specifies this connections. The format of the string is the following:
//                  typeConnection1=pinsConnected1/pinsConnected2/pinsConnected3/...
//                    for example if Mux has a condition to valid linking from data pin 1 and valid pin 0 and from data pin 1 and valid pin 2
//                    the format is the following "cv=d1v0/d1v2"
//
//                  If the linking is among each data with its corresponding valid (for example) the pinsConnected is not specified like "dv"
//                  which indicates linking between data 0 and valid 0, data 1 and valid 1 and so on. At the same way the * means to every
//                  so for example "d1v*" means from data 1 to any valid
//
//                  if there are multiple typeConnections they are separated from underscore so for example "cv=d1v0/d1v2_cr=d1r0" would 
//                  take in consideration apart from the previous mux linking between condition and valid also a linking between condition and 
//                  ready from data input 1 and ready 0
//
//                  There is a copy of this table also in the folder dot2vhdl in the file "reports.cpp" If you intend to modify this table you 
//                  need to modify it also there
//
/*vector<pair <BlockType, string> > MixedConnectionBlock ={
    {OPERATOR, "vr=v0r1/v1r0"},
    {BRANCH, "cv=d1v*_cr=d1v*_vr=v0r1/v1r0"},
    {CNTRL_MG, "vc=v0d1"},
    {FUNC_EXIT, "vr=v0r1/v1r0"},
    {MERGE, "vd"},
    {MUX, "cr=d0r*_cv=d0v*_vd=v*d*_vr=v*r*"}
};*/
vector<pair <BlockType, string> > MixedConnectionBlock ={   //Carmine 22.02.22 indexes in the dot file start from 1 not 0
    {OPERATOR, "vr=v1r2/v2r1"},
    {BRANCH, "cv=d2v*_cr=d2r*_vr=v1r2/v2r1"},
    {CNTRL_MG, "vc=v1d2"},
    {FUNC_EXIT, "vr=v1r1"},
    {MERGE, "vd"},
    {MUX, "cr=d1r*_cv=d1v*_vd=v*d*_vr=v*r*"}
};

void DFnetlist_Impl::execute_reduction(blockID b){  //Carmine 09.03.22 function to eliminate  a block
        portID port_src = getInPort(b);
        channelID channel_src = getChannelID(port_src);
        portID new_port_src = getSrcPort(channel_src);
        bool back_src = isBackEdge(channel_src);

        portID port_dst = getOutPort(b);
        channelID channel_dst = getChannelID(port_dst);
        portID new_port_dst = getDstPort(channel_dst);
        bool back_dst = isBackEdge(channel_dst);

        bool back = back_dst or back_src;

        removeChannel(channel_src);
        removeChannel(channel_dst);
        removeBlock(b);

        channelID c = createChannel(new_port_src ,new_port_dst);
        setBackEdge(c, back);
        setChannelMerge(c); // to preserve the information of the previous presence of the merge for dot writing
}

void DFnetlist_Impl::reduceMerges(){ //Carmine 09.03.22 function to eliminate the merges with 1 input - they reduce to wire
    
    ForAllBlocks(b) {
        if(getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1){
            execute_reduction(b);
        }
    }
    ForAllChannels(c){
        blockID b1, b2=invalidDataflowID;
        b1 = getBlockFromPort(getSrcPort(c));
        if(getBlockType(b1) == MERGE && getPorts(b1, INPUT_PORTS).size() == 1) b2 = b1;
        b1 = getBlockFromPort(getDstPort(c));
        if(getBlockType(b1) == MERGE && getPorts(b1, INPUT_PORTS).size() == 1) b2 = b1;
        if(b2 != invalidDataflowID){
            execute_reduction(b2);
        }
    }

}

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

void DFnetlist_Impl::createMilpVarsEB_sc(Milp_Model &milp, milpVarsEB &vars, bool max_throughput, int mg, bool first_MG, string model_mode) {
    vars.buffer_flop = vector<int>(vecChannelsSize(), -1);
    vars.buffer_flop_valid = vector<int>(vecChannelsSize(), -1); //Carmine 17.02.22
    vars.buffer_flop_ready = vector<int>(vecChannelsSize(), -1); //Carmine 17.02.22
    vars.buffer_slots = vector<int>(vecChannelsSize(), -1);
    vars.has_buffer = vector<int>(vecChannelsSize(), -1);
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_valid_path = vector<int>(vecPortsSize(), -1); //Carmine 16.02.22
    vars.time_ready_path = vector<int>(vecPortsSize(), -1); //Carmine 17.02.22
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

        if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
            vars.buffer_flop_valid[c] = milp.newBooleanVar(cname + "_flop_valid");
        if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
            vars.buffer_flop_ready[c] = milp.newBooleanVar(cname + "_flop_ready");
        
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

            if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0) //Carmine 17.02.22 the variables defined need to be used in the milp
                vars.time_valid_path[p] = milp.newRealVar("timePath_valid_" + bname + "_" + pname + mg_name); //Carmine 16.02.22 delays valid ports
            if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                string tmp;  //Carmine 17.02.22 for ready signals inputs and outputs are inverted wrt data signals
                if(pname.substr(0,3).compare("out") == 0)
                    tmp = "in" + pname.substr(3);
                else
                    tmp = "out" + pname.substr(2);
                vars.time_ready_path[p] = milp.newRealVar("timePath_ready_" + bname + "_" + tmp + mg_name); //Carmine 17.02.22 delays ready ports
            }
            
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

            if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
                vars.time_valid_path[p] = milp.newRealVar("timePath_valid_" + bname + "_" + pname + mg_name); //Carmine 16.02.22 delays valid ports
            if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                string tmp;
                if(pname.substr(0,3).compare("out") == 0)
                    tmp = "in" + pname.substr(3);
                else
                    tmp = "out" + pname.substr(2);
                vars.time_ready_path[p] = milp.newRealVar("timePath_ready_" + bname + "_" + tmp + mg_name); //Carmine 17.02.22 delays ready ports
            }
            vars.time_elastic[p] = milp.newRealVar("timeElastic_" + bname + "_" + pname + mg_name);

            const string& pname_other = getPortName(other_p, false);
            vars.time_path[other_p] = milp.newRealVar("timePath_" + bname + "_" + pname_other + mg_name);

            if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
                vars.time_valid_path[other_p] = milp.newRealVar("timePath_valid_" + bname + "_" + pname_other + mg_name); //Carmine 16.02.22 delays valid ports
            if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                string tmp;
                if(pname_other.substr(0,3).compare("out") == 0)
                    tmp = "in" + pname_other.substr(3);
                else
                    tmp = "out" + pname_other.substr(2);
                vars.time_ready_path[other_p] = milp.newRealVar("timePath_ready_" + bname + "_" + tmp + mg_name); //Carmine 17.02.22 delays ready ports
            }
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

void DFnetlist_Impl::createMilpVars_remaining(Milp_Model &milp, milpVarsEB &vars, string model_mode) {
    vars.buffer_flop = vector<int>(vecChannelsSize(), -1);
    vars.buffer_flop_valid = vector<int>(vecChannelsSize(), -1); //Carmine 18.02.22
    vars.buffer_flop_ready = vector<int>(vecChannelsSize(), -1); //Carmine 18.02.22
    vars.has_buffer = vector<int>(vecChannelsSize(), -1); //Carmine 18.02.22 since the remaining channels are considered only for CP the presence of a transparent buffer is in general ignored
                                                            //this variable is necessary to include this case scenario
    vars.time_path = vector<int>(vecPortsSize(), -1);
    vars.time_valid_path = vector<int>(vecPortsSize(), -1); //Carmine 18.02.22
    vars.time_ready_path = vector<int>(vecPortsSize(), -1); //Carmine 18.02.22
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

        if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
            vars.buffer_flop_valid[c] = milp.newBooleanVar(cname + "_flop_valid");
        if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
            vars.buffer_flop_ready[c] = milp.newBooleanVar(cname + "_flop_ready");
            //vars.has_buffer[c] = milp.newBooleanVar(cname + "_hasBuffer");      //Carmine 18.02.22 has_buffer is necessary only if ready is considered
        }
    }

    ForAllBlocks(b) {
        const string& bname = getBlockName(b);
        ForAllPorts(b,p) {
            const string& pname = getPortName(p, false);
            vars.time_path[p] = milp.newRealVar("timePath_" + bname + "_" + pname);

            if(model_mode.compare("valid")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0)
                vars.time_valid_path[p] = milp.newRealVar("timePath_valid_" + bname + "_" + pname); //Carmine 16.02.22 delays valid ports
            if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                string tmp;
                if(pname.substr(0,3).compare("out") == 0)
                    tmp = "in" + pname.substr(3);
                else
                    tmp = "out" + pname.substr(2);
                vars.time_ready_path[p] = milp.newRealVar("timePath_ready_" + bname + "_" + tmp); //Carmine 17.02.22 delays ready ports
            }

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

    // dumpMilpSolution(milp, milpVars);

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

bool DFnetlist_Impl::addElasticBuffersBB_sc(double Period, double BufferDelay, bool MaxThroughput, double coverage, int timeout, bool first_MG, const std::string& model_mode,const std::string& lib_path) {

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
    //addBorderBuffers(); Carmine 04.02.22 removal conservative buffering (between indipendent loops)

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

    float min_th_mg = 1.1; //Carmine 28.02.22 lowest throughput among MGs

    if (MaxThroughput) computeChannelFrequencies();

    for (int i = 0; i < MG_disjoint.size(); i++) {
        cout << "-------------------------------" << endl;
        cout << "Initiating MILP for MG number " << i << endl;
        cout << "-------------------------------" << endl;

        createMilpVarsEB_sc(milp, milpVars_sc[i], MaxThroughput, i, first_MG, model_mode);
        if (not createPathConstraints_sc(milp, milpVars_sc[i], Period, BufferDelay, i)) return false;
        if (model_mode.compare("default")){ //Carmine 16.02.22 additional model_mode to have different working principle
            if (not createPathConstraintsOthers_sc(milp, milpVars_sc[i], Period, BufferDelay, i, model_mode, lib_path)){ 
                cout << "*ERROR* creating path constraint for non-data signals" << endl;
                return false;
            }else
                cout << "*INFO* MILP mode executed: " << model_mode << endl;
        }
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

        dumpMilpSolution(milp, milpVars_sc[i]);

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

            bool print_reduced=false; //Carmine 25.03.22 variable to manage printing following the createChannel function //give a look to function for doubts
            if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                if(!transparent && milp.isTrue(milpVars_sc[i].buffer_flop_ready[c]) ){
                    setChannelEB(c);
                    printChannelInfo(c, slots-1, 1);
                    print_reduced = true;//Carmine 25.03.22
                }
            }
            if(print_reduced)
                printChannelInfo(c, 1, transparent);
            else
                printChannelInfo(c, slots, transparent);
        }

        //write retiming diffs
        writeRetimingDiffs(milp, milpVars_sc[i]);


        if (MaxThroughput) {
            for (auto sub_mg: components[i]) {
                cout << "\n*** Throughput achieved in sub MG " << sub_mg << ": " <<
                     fixed << setprecision(2) << milp[milpVars_sc[i].th_MG[sub_mg]] << " ***\n" << endl;
                double th_MG_i = milp[milpVars_sc[i].th_MG[sub_mg]];
                if(th_MG_i < min_th_mg) min_th_mg = th_MG_i; //Carmine 28.02.22 keeping track of the throughput of each MG
                if (first_MG) break;
            }
        }

        milp.writeOutDelays("delays_output.txt"); //Carmine 07.02.22 trying to get from milp the timing output of the blocks

        if (not milp.init(getMilpSolver())) {
            setError(milp.getError());
            return false;
        }
    }

    
    if(min_th_mg <= 1.0)
        cout << "\n*** Lowest throughput achieved " << min_th_mg  << " ***\n" << endl; //Carmine 28.02.22 it shows the lowest throughput among MGs


    cout << "--------------------------------------" << endl;
    cout << "Initiating MILP for remaining channels" << endl;
    cout << "--------------------------------------" << endl;

    if (not milp.init(getMilpSolver())) {
        setError(milp.getError());
        return false;
    }



    milpVarsEB remaining;

    createMilpVars_remaining(milp, remaining, model_mode);
    createPathConstraints_remaining(milp, remaining, Period, BufferDelay);

    if (model_mode.compare("default")){ //Carmine 18.02.22 additional model_mode to have different working principle for remaining channels
            if (not createPathConstraintsOthers_remaining(milp, remaining, Period, BufferDelay, model_mode, lib_path)){ 
                cout << "*ERROR* creating path constraint for non-data signals" << endl;
                return false;
            }else
                cout << "*INFO* MILP mode executed for remaining channels: " << model_mode << endl;
    }

    createElasticityConstraints_remaining(milp, remaining);

    ForAllChannels(c) {
        if (channelIsCovered(c, true, true, false))
            continue;
        if (model_mode.compare("default")!=0 && model_mode.compare("valid")!=0)      //Carmine 18.02.22 if the mode is different from default and valid
            milp.newCostTerm(1, remaining.buffer_flop_ready[c]);
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
    vector<channelID> buffers_trans; //Carmine 18.02.22 include transparent buffers
    ForAllChannels(c) {
        if (channelIsCovered(c, true, true, true)) continue;
      
        if (milp[remaining.buffer_flop[c]] > 0.5) {
            buffers.push_back(c);
            continue;
        }

        if (model_mode.compare("default")!=0 && model_mode.compare("valid")!=0){   //Carmine 18.02.22
            if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 01.03.22 MILP it is already present transparent buffers in MUX and MERGE 
                || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                    continue;  
            }
            if (milp[remaining.buffer_flop_ready[c]] > 0.5 )
                buffers_trans.push_back(c);
        }
    }

    for (channelID c: buffers) {
        setChannelTransparency(c, 0);
        setChannelBufferSize(c, 1);
        printChannelInfo(c, 1, 0);

        if(model_mode.compare("ready")==0 || model_mode.compare("all")==0 || model_mode.compare("mixed")==0){
                if(milp[remaining.buffer_flop_ready[c]] > 0.5){
                    if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 01.03.22 MILP it is already present transparent buffers in MUX and MERGE 
                    || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                        continue;  
                    }
                    setChannelEB(c);
                    printChannelInfo(c, 1, 1);
                }
        }
    }

    for (channelID c: buffers_trans) {    //Carmine 18.02.22
        setChannelTransparency(c, 1);
        setChannelBufferSize(c, 1);
        printChannelInfo(c, 1, 1);
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
        double d = getBlockDelay(b, -1);

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

            if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 21.02.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                int tmp = Vars.has_buffer[c];
                milp.newRow ( {{1, tmp}}, '>', 1);  //mux presents by design a transparent buffer
                tmp = Vars.buffer_slots[c];
                milp.newRow ( {{1, tmp}}, '>', 1);  //mux presents by design a transparent buffer with number of slots >= 1
            }


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

            if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 21.02.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                int tmp = Vars.has_buffer[c];
                milp.newRow ( {{1, tmp}}, '>', 1);  //mux presents by design a transparent buffer
                tmp = Vars.buffer_slots[c];
                milp.newRow ( {{1, tmp}}, '>', 1);  //mux presents by design a transparent buffer with number of slots >= 1
            }
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
        double d = getBlockDelay(b, -1);

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
                    if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire
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
        double d = getBlockDelay(b, -1);

        // First: combinational blocks
        if (getLatency(b) == 0) {
            ForAllOutputPorts(b, out_p) {
                if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

                int v_out = Vars.time_path[out_p];

                ForAllInputPorts(b, in_p) {
                    if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                    int v_in = Vars.time_path[in_p];

                    double D = getCombinationalDelay(in_p, out_p);
                    if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire
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

//Carmine 18.02.22 function to read block delays from library
vector< pair<string, vector<double>>> DFnetlist_Impl::read_lib_file(string lib_file_path){

    vector< pair<string, vector<double>>> output = vector< pair<string, vector<double>>>(30);
    vector<double> delays =  vector< double>(7); //vector containing delays for 7 different bitwidths
    //for now it is assumed a large number of types although the actual number depends on number of operators

    cout << "Reading file lib " << lib_file_path << endl;
    ifstream file (lib_file_path);
    string line, word;
    string block;
    regex rg("\\t+");
    int cnt;    
    while(getline(file,line)){
        sregex_token_iterator rg_iter(line.begin(), line.end(), rg, -1);
        sregex_token_iterator end_iter;
        cnt = 0;
        delays.clear();
        for(; rg_iter != end_iter; rg_iter++){
            word = *rg_iter;
            switch (cnt)
            {
            case 0:
                block = word;
                break;
            //case 1:
            //    output.push_back(make_pair(block, stod(word)));
            //    break;
            default:
                delays.push_back(stod(word));
                break;
            }
            cnt++;
        }
        output.push_back(make_pair(block, delays));
    }

    file.close();
    return output;
}

//Carmine 18.07.22 function to obtain the index inside the table of mixed from  the type of mixed connection
int get_index_mixed_table(string conn_type){

    int idx_table = -1, i=0;

    std::vector<string> connections = {"vr", "cv", "cr", "vc", "vd"}; //{"vr", "dv", "dr", "vd", "vd"}; 

    for(string x: connections){
        if (x == conn_type){
            idx_table = i;
        }
        i++;
    }

    return idx_table;

}



bool include_buff_delays = true; //Carmine 22.03.22 boolean if including the delay of the buffers or not

//Carmine 16.02.22 function to add timing delays of non-data wires
bool DFnetlist_Impl::createPathConstraintsOthers_sc(Milp_Model &milp, milpVarsEB &Vars, double Period,
                                              double BufferDelay, int mg, string model_mode, string lib_path) {   

    // Create the variables and constraints for all channels
    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;

    vector<int> time_paths, ob_presence;
    vector<string> modes;
    vector< pair<string, vector<double>>> block_delays = vector< pair<string, vector<double>>>(30);


    double delay_TEHB, delay_OEHB;//Carmine 22.03.22 include delays of buffers

    if(!model_mode.compare("mixed")){
        modes = {"valid", "ready", "mixed"};
    }else if(!model_mode.compare("all"))
        modes = {"valid", "ready"};
    else
        modes = {model_mode};
    
//Andrea 20220727 delay readed in the dot
    
     for(string mode: modes){
         if(mode.compare("valid") == 0){
             time_paths = Vars.time_valid_path;
             ob_presence = Vars.buffer_flop_valid;
             if(FPL_22_CARMINE_LIB)
                 block_delays = read_lib_file(lib_path+"/delays_lib_valid.txt");
         }else if(mode.compare("ready") == 0){
             time_paths = Vars.time_ready_path;
             ob_presence = Vars.buffer_flop_ready;
             if(FPL_22_CARMINE_LIB)
                 block_delays = read_lib_file(lib_path+"/delays_lib_ready.txt");
         }else if(mode.compare("mixed") == 0){
            if(FPL_22_CARMINE_LIB)
                block_delays = read_lib_file(lib_path+"/delays_lib_mixed.txt");
             goto only_mixed_label_1;
         }else{
             cout << "*ERROR* mode " << mode << " is not available" << endl;
             continue;
         }

        delay_TEHB = -1.0;
        delay_OEHB = -1.0;
        //Carmine 22.03.22 include delays of buffers
        if(FPL_22_CARMINE_LIB){
            for(pair <string, vector<double>> x: block_delays){ if (x.first == "") continue; regex rg (x.first+"(.)*"); if(regex_match("TEHB", rg)) delay_TEHB = x.second[0]; }
            for(pair <string, vector<double>> x: block_delays){ if (x.first == "") continue; regex rg (x.first+"(.)*"); if(regex_match("OEHB", rg)) delay_OEHB = x.second[0]; }
        }else{
            delay_TEHB = 0.1;
            delay_OEHB = 0.1;
        }
        //the buffers delays are included only if the model mode is ALL or MIXED
        include_buff_delays = include_buff_delays & (  (model_mode.compare("mixed")==0) | (model_mode.compare("all")==0)  );

        //////////////////////
        /** CHANNELS IN MG **/
        //////////////////////
        

        for (channelID c: MG_disjoint[mg].getChannels()) {
            // Lana 02.05.20. paths from/to memory do not need buffers
            if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
            || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
                continue;


            int v1 =  time_paths[getSrcPort(c)]; //modified
            int v2 =  time_paths[getDstPort(c)]; //modified

            if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                int tmp = v1;
                v1 = v2;
                v2 = tmp;
            }


            int R = ob_presence[c];


            if (hasPeriod) {
                // v1, v2 <= Period
                milp.newRow( {{1,v1}}, '<', Period);
                milp.newRow( {{1,v2}}, '<', Period);

                // v2 >= v1 - 2*period*R
                if(!include_buff_delays){   
                    milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}}, '>', 0);
                }else{ //Carmine 22.03.22 include the delays of buffer along channel
                    if(mode.compare("ready") == 0 && delay_OEHB>=0.0){ 
                        milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0);
                        milp.newRow ( {{1,v2}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of OEHB is not accounted for
                    }else if(mode.compare("valid") == 0 && delay_TEHB>=0.0) { //for now the delay of TEHB along valid path is the same of the data path one
                        //Valid constraints
                        milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0);
                        milp.newRow ( {{1,v2}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for
                        //Data constraints
                        milp.newRow ( {{-1,Vars.time_path[getSrcPort(c)]}, {1,Vars.time_path[getDstPort(c)]}, {2*Period, Vars.buffer_flop[c]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Datapath
                        milp.newRow ( {{1,Vars.time_path[getDstPort(c)]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for

                    }
                }
            }

            // v2 >= Buffer Delay
            if (BufferDelay > 0) milp.newRow( {{1,v2}}, '>', BufferDelay);

            //Carmine 17.02.22 //the presence of a buffer has to be consistent among different timing domains
            if(mode.compare("valid") == 0){
                milp.newRow( {{1,Vars.buffer_flop[c]}, {-1,ob_presence[c]}}, '=', 0);
            }else if(mode.compare("ready") == 0){
                //milp.newRow( {{1,Vars.has_buffer[c]}, {-1,Vars.buffer_flop[c]}, {-1,ob_presence[c]}}, '>', 0);
                //milp.newRow( {{1,Vars.buffer_slots[c]}, {-1,ob_presence[c]}}, '>', 0); //Carmine 18.02.22 if there is a transparent buffer the size must be at least 1
                milp.newRow( {{1,Vars.buffer_slots[c]}, {-1,ob_presence[c]}, {-1,Vars.buffer_flop[c]}}, '>', 0); //Carmine 25.03.22 since the slots are connected to throughput, thoughput constraints need to know about the possibility of having an additional register (OEHB+TEHB)
                if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 21.02.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                    milp.newRow( {{1,ob_presence[c]}}, '>', 1);  //MILP needs to distinguish between the 3 cases transp, nontransp and EB
                }

            }else{

            }

        }
        
        //////////////////////////////
        /// CHANNELS IN MG BORDERS ///
        //////////////////////////////

        for (channelID c: channels_in_borders) {
            if (!MG_disjoint[mg].hasBlock(getSrcBlock(c)) && !MG_disjoint[mg].hasBlock(getDstBlock(c))) continue;

            int v1 = time_paths[getSrcPort(c)]; //modified
            int v2 = time_paths[getDstPort(c)]; //modified

            if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                int tmp = v1;
                v1 = v2;
                v2 = tmp;
            }

            int R = !isChannelTransparent(c); //here you dont check if it transparent or not but assume that all channels in borders are isolated from the rest

            assert(R == 1);

            if (hasPeriod) {
                // v1, v2 <= Period
                milp.newRow( {{1,v1}}, '<', Period);
                milp.newRow( {{1,v2}}, '<', Period);

                // v2 >= v1 - 2*period*R
                //milp.newRow ( {{-1,v1}, {1,v2}}, '>', -2 * Period * R);
                if(!include_buff_delays)   
                    milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}}, '>', 0);
                else{ //Carmine 22.03.22 include the delays of buffer along channel
                    if(mode.compare("ready") == 0 && delay_OEHB>=0.0) {
                        milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0);
                        milp.newRow ( {{1,v2}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of OEHB is not accounted for

                    }else if(mode.compare("valid") == 0 && delay_TEHB>=0.0) { //for now the delay of TEHB along valid path is the same of the data path one
                        //Valid constraints
                        milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0);
                        milp.newRow ( {{1,v2}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for
                        //Data constraints
                        milp.newRow ( {{-1,Vars.time_path[getSrcPort(c)]}, {1,Vars.time_path[getDstPort(c)]}, {2*Period, Vars.buffer_flop[c]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Datapath
                        milp.newRow ( {{1,Vars.time_path[getDstPort(c)]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for

                    }
                }
            }

            // v2 >= Buffer Delay
            if (BufferDelay > 0) milp.newRow( {{1,v2}}, '>', BufferDelay);

            //Carmine 17.02.22 //the presence of a buffer has to be consistent among different timing domains
            if(mode.compare("valid") == 0){
                milp.newRow( {{1,Vars.buffer_flop[c]}, {-1,ob_presence[c]}}, '=', 0);
            }else if(mode.compare("ready") == 0){
                //milp.newRow( {{1,Vars.has_buffer[c]}, {-1,Vars.buffer_flop[c]}, {-1,ob_presence[c]}}, '>', 0);
                milp.newRow( {{1,Vars.buffer_slots[c]}, {-1,ob_presence[c]}}, '>', 0); //Carmine 18.02.22 if there is a transparent buffer the size must be at least 1
            
                if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 21.02.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                    milp.newRow( {{1,ob_presence[c]}}, '>', 1);  //MILP needs to distinguish between the 3 cases transp, nontransp and EB
                }
            
            }else{

            }
        }

        ////////////////////
        /// BLOCKS IN MG ///
        ////////////////////
        
        // Create the constraints to propagate the delays of the blocks
        for (blockID b: MG_disjoint[mg].getBlocks()) {
            //double d = getBlockDelay(b, -1);  //Carmine 18.02.22 delays of valid and ready are read from library
            
            double D = -1.0;

            
            
                string name_block, type_block;
                name_block = getBlockName(b);
                int bitwidth_max=0, bitwidth; //Carmine 07.03.22 extract the delay of the component for the right bitwidth
                for(portID x: getPorts(b,INPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                for(portID x: getPorts(b,OUTPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                if(bitwidth_max == 0) bitwidth_max = 1;
                int pos_bitwidth = ceil(log2(float(bitwidth_max)));
            //Carmine 02.08.22 Flag to decide if using old library or new function
            if(FPL_22_CARMINE_LIB){
                for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22 extract the delay of the component for the right bitwidth
                    if (x.first == "") continue;
                    regex rg (x.first+"(.)*");
                    if(regex_match(name_block, rg)){
                        D = x.second[pos_bitwidth];
                    }
                }
                if(D == -1.0){
                    type_block = printBlockType(getBlockType(b));
                    for(int i=0;i<type_block.size();i++){ type_block[i]=tolower(type_block[i]);}
                    for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                        if (x.first == "") continue;
                        regex rg (x.first+"(.)*");
                        if(regex_match(type_block, rg)){
                            D = x.second[pos_bitwidth];
                        }
                    }
                }
                if(D == -1.0){D=0.0; cout << "**WARNING** Delay of block " << name_block <<" between "<<mode<<" cannot be found!!"<< endl; }
            }else{
                    //Carmine 02.08.22 obtaining mixed connections delays from the new function getBlockDelay (OPEN_CIRCUIT_DELAY is the special value)
                if(mode.compare("valid") == 0) //offset of ready in the map is + 7 (+7 data)
                    D = getBlockDelay(b, 1);
                    //D = getBlockDelay(b, pos_bitwidth + 7);
                if(mode.compare("ready") == 0) //offset of ready in the map is + 14 (+7 data +7 valid)
                    D = getBlockDelay(b, 2);
                    //D = getBlockDelay(b, pos_bitwidth + 14);
                
                if(D>= OPEN_CIRCUIT_DELAY)
                    continue;                


            }
            // First: combinational blocks
            //if (getLatency(b) == 0) { //Carmine 18.02.22 for valid and ready the latency is 0
                ForAllOutputPorts(b, out_p) {
                    if (!MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                        continue;

                    int v_out = time_paths[out_p];

                    ForAllInputPorts(b, in_p) {
                        if (!MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                            continue;

                        int v_in = time_paths[in_p];
                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*double delayBlock_data; //Carmine 22.03.22 necessary to compute the delay on data path for constraints with buffers
                        if(getLatency(b)==0) 
                            delayBlock_data = getCombinationalDelay(in_p, out_p); 
                        else
                            delayBlock_data = 0.0;*/                        
                        if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire 
                        //Carmine 18.02.22 for now the delays valid and ready are suppossed to be the same amount amoong different inputs and outputs
                        if (D + BufferDelay > Period) {
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                            return false;
                        }

                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*if (include_buff_delays && (D + BufferDelay + delay_TEHB > Period  || delayBlock_data + BufferDelay + delay_TEHB > Period  )){  //Carmine 22.03.22 it is important to include the case in which TEHB is present since the Period constraint canno be respected
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied since it is preceded by a TEHB.");
                            return false;
                        }*/

                        int tmp_out = v_out;
                        int tmp_in = v_in;

                        if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                            tmp_out = v_in;
                            tmp_in = v_out;
                        }

                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*if(include_buff_delays && mode.compare("valid") == 0){  //Carmine 22.03.22 include the delays of TEHB in the valid and data delays 
                            channelID c = getConnectedChannel(in_p);
                            milp.newRow( {{1, tmp_out}, {-1,tmp_in}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', D);
                            if(getLatency(b)==0)
                                milp.newRow( {{1, Vars.time_path[out_p] }, {-1,Vars.time_path[in_p]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', delayBlock_data);
                        }else{*/  
                        milp.newRow( {{1, tmp_out}, {-1,tmp_in}, }, '>', D);
                        //}
                    }
                }
            //}  //Carmine 18.02.22 for valid and ready the latency is 0 // the else part is the same as the blocks in MG borders
        }

        ////////////////////////////
        /// BLOCKS IN MG BORDERS ///
        ////////////////////////////

        for (blockID b: blocks_in_borders) {
            //double d = getBlockDelay(b, -1); //Carmine 18.02.22 delays of valid and ready are read from library
            string name_block, type_block;
            double D = -1.0;
            name_block = getBlockName(b);
            int bitwidth_max=0, bitwidth; //Carmine 07.03.22 extract the delay of the component for the right bitwidth
            for(portID x: getPorts(b,INPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
            for(portID x: getPorts(b,OUTPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
            if(bitwidth_max == 0) bitwidth_max = 1;
            int pos_bitwidth = ceil(log2(float(bitwidth_max)));

            if(FPL_22_CARMINE_LIB){
                for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                    if (x.first == "") continue;
                    regex rg (x.first+"(.)*");
                    if(regex_match(name_block, rg))
                        D = x.second[pos_bitwidth];
                }
                if(D == -1.0){
                    type_block = printBlockType(getBlockType(b));
                    for(int i=0;i<type_block.size();i++){ type_block[i]=tolower(type_block[i]);}
                    for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                        if (x.first == "") continue;
                        regex rg (x.first+"(.)*");
                        if(regex_match(type_block, rg))
                            D = x.second[pos_bitwidth];
                    }
                }
                if(D == -1.0){D=0.0; cout << "**WARNING** Delay of block " << name_block <<" between "<<mode<<" cannot be found!!"<< endl; }
            }else{                     
                    //Carmine 02.08.22 obtaining mixed connections delays from the new function getBlockDelay (OPEN_CIRCUIT_DELAY is the special value)
                if(mode.compare("valid") == 0) //offset of ready in the map is + 7 (+7 data)
                    D = getBlockDelay(b, 1);
                    //D = getBlockDelay(b, pos_bitwidth + 7);
                if(mode.compare("ready") == 0) //offset of ready in the map is + 14 (+7 data +7 valid)
                    D = getBlockDelay(b, 2);
                    //D = getBlockDelay(b, pos_bitwidth + 14);

                if(D>= OPEN_CIRCUIT_DELAY)
                    continue;

            }
            // First: combinational blocks
            //if (getLatency(b) == 0) { //Carmine 18.02.22 for valid and ready the latency is 0
                ForAllOutputPorts(b, out_p) {
                    if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

                    int v_out = time_paths[out_p];

                    ForAllInputPorts(b, in_p) {
                        if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                        int v_in = time_paths[in_p];
                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*double delayBlock_data; //Carmine 22.03.22 necessary to compute the delay on data path for constraints with buffers
                        if(getLatency(b)==0) 
                            delayBlock_data = getCombinationalDelay(in_p, out_p); 
                        else
                            delayBlock_data = 0.0; */                       
                        if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire
                        //Carmine 18.02.22 for now the delays valid and ready are suppossed to be the same amount amoong different inputs and outputs
                        if (D + BufferDelay > Period) {
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                            return false;
                        }

                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*if (include_buff_delays && (D + BufferDelay + delay_TEHB > Period|| delayBlock_data + BufferDelay + delay_TEHB > Period  )){  //Carmine 22.03.22 it is important to include the case in which TEHB is present since the Period constraint canno be respected
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied since it is preceded by a TEHB.");
                            return false;
                        }*/

                        int tmp_out = v_out;
                        int tmp_in = v_in;

                        if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                            tmp_out = v_in;
                            tmp_in = v_out;
                        }

                        //Carmine 24.03.22 this set of equations is inclued in the channel delays
                        /*if(include_buff_delays && mode.compare("valid") == 0){  //Carmine 22.03.22 include the delays of TEHB in the valid and data delays 
                            channelID c = getConnectedChannel(in_p);
                            milp.newRow( {{1, tmp_out}, {-1,tmp_in}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', D);
                            if(getLatency(b)==0)
                                milp.newRow( {{1, Vars.time_path[out_p] }, {-1,Vars.time_path[in_p]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', delayBlock_data);
                        }else{*/  
                        milp.newRow( {{1, tmp_out}, {-1,tmp_in}, }, '>', D);
                        //}
                    }
                }
            /*} else {
                // Pipelined units
                if (d > Period) {
                    setError("Block " + getBlockName(b) + ": period cannot be satisfied.");
                    return false;
                }

                if(mode.compare("ready") != 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                    ForAllOutputPorts(b, out_p) {
                        if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

                        double d_out = getPortDelay(out_p);
                        int v_out = time_paths[out_p];
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
                        int v_in = time_paths[in_p];
                        if (d_in + BufferDelay > Period) {
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                            return false;
                        }
                        if (hasPeriod) milp.newRow( {{1, v_in}}, '<', Period - d_in);
                    }
                }else{
                    ForAllInputPorts(b, out_p) {
                        if (!MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p)))) continue;

                        double d_out = getPortDelay(out_p);
                        int v_out = time_paths[out_p];
                        if (d_out > Period) {
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                            return false;
                        }
                        // Add constraint: v_out = d_out;
                        milp.newRow( {{1, v_out}}, '=', d_out);
                    }

                    ForAllOutputPorts(b, in_p) {
                        if (!MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p)))) continue;

                        double d_in = getPortDelay(in_p);
                        int v_in = time_paths[in_p];
                        if (d_in + BufferDelay > Period) {
                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                            return false;
                        }
                        if (hasPeriod) milp.newRow( {{1, v_in}}, '<', Period - d_in);
                    }

                }
            }*/
        }

        only_mixed_label_1: //Carmine 22.02.22 label to jump directly for mixed since it is only necessary insert blocks mixed connections delays

        if(mode.compare("mixed") == 0){
            setBlocks tot_blocks(MG_disjoint[mg].getBlocks());
            int size_MG_blocks = tot_blocks.size();
            tot_blocks.insert(blocks_in_borders.begin(), blocks_in_borders.end());
            int cnt_blocks = 0;

            for (blockID b: tot_blocks) {
                vector< pair<string, double>> delays_per_conn = vector< pair<string, double>>(30);
                
                //Carmine 02.08.22 New macro to use the old libraries or the new function to get the delays
                if(FPL_22_CARMINE_LIB){
                    bool found = false;
                    string name_block, type_block;
                    
                    name_block = getBlockName(b);
                    int bitwidth_max=0, bitwidth; //Carmine 07.03.22 extract the delay of the component for the right bitwidth
                    for(portID x: getPorts(b,INPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                    for(portID x: getPorts(b,OUTPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                    if(bitwidth_max == 0) bitwidth_max = 1;
                    int pos_bitwidth = ceil(log2(float(bitwidth_max)));
                    for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                        if (x.first == "") continue;
                        string name_block_lib, mixed_conn;
                        regex rg_lib("-"); //extract from the mixed library the name of the block and which connection it refers to
                        sregex_token_iterator rg_iter(x.first.begin(), x.first.end(), rg_lib, -1);
                        name_block_lib = *rg_iter;
                        rg_iter++;
                        mixed_conn = *rg_iter;
                        regex rg (name_block_lib+"(.)*");
                        if(regex_match(name_block, rg)){
                            found = true;
                            delays_per_conn.push_back(make_pair(mixed_conn, x.second[pos_bitwidth]));
                            //delays_per_conn.push_back(make_pair(mixed_conn, getBlockDelay(b, get_index_mixed_table(mixed_conn))));
                        }
                    }
                    if(!found){
                        type_block = printBlockType(getBlockType(b));
                        for(int i=0;i<type_block.size();i++){ type_block[i]=tolower(type_block[i]);}
                        for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                            if (x.first == "") continue;

                            string name_block_lib, mixed_conn;
                            regex rg_lib("-"); //extract from the mixed library the name of the block and which connection it refers to
                            sregex_token_iterator rg_iter(x.first.begin(), x.first.end(), rg_lib, -1);
                            name_block_lib = *rg_iter;
                            rg_iter++;
                            mixed_conn = *rg_iter;
                            regex rg (name_block_lib+"(.)*");

                            if(regex_match(type_block, rg)){
                                found = true;
                                delays_per_conn.push_back(make_pair(mixed_conn, x.second[pos_bitwidth]));
                                //delays_per_conn.push_back(make_pair(mixed_conn, getBlockDelay(b, get_index_mixed_table(mixed_conn))));
                            }
                        }
                    }
                    if(!found){continue;}

                }else{
                    //Carmine 02.08.22 obtaining mixed connections delays from the new function getBlockDelay (OPEN_CIRCUIT_DELAY is the special value)
                    std::vector<string> vec_mixed_connections = {"vr", "cv", "cr", "vc", "vd"};

                    for(string mixed_conn: vec_mixed_connections){
                        int index = get_index_mixed_table(mixed_conn);
                        double delay = getBlockDelay(b, index+3); 
//                        double delay = getBlockDelay(b, index+21); //the initial offset of mixed is + 21 (+7 data +7 valid +7 ready)
                        if (delay< OPEN_CIRCUIT_DELAY)
                            delays_per_conn.push_back(make_pair(mixed_conn, delay));
                    }

                }

                for(pair <string, double> x: delays_per_conn){
                    if (x.first == "") continue;

                    for( pair<BlockType, string> y: MixedConnectionBlock){
                        if(y.first == getBlockType(b)){
                            string mix_conn_entire, mix_conn, spec_conn;
                            regex rg_table("_");
                            sregex_token_iterator rg_iter(y.second.begin(), y.second.end(), rg_table, -1);
                            sregex_token_iterator end_iter;

                            for(; rg_iter != end_iter; rg_iter++){
                                mix_conn_entire = *rg_iter;

                                regex reg_any_eq("(.*)=(.*)");
                                if(regex_match(mix_conn_entire, reg_any_eq)){  //there is a specification about single connections
                                    regex rg_conn("=");
                                    sregex_token_iterator rg_iter_conn(mix_conn_entire.begin(), mix_conn_entire.end(), rg_conn, -1);
                                    mix_conn = *rg_iter_conn;
                                }else{
                                    mix_conn = mix_conn_entire;
                                    spec_conn = "empty";
                                }

                                if(mix_conn.compare(x.first) == 0){ //the connection saved in the table MixecConnectionBlock is equal to the one extracted from the lib
                                    vector<pair<string, string>> conn_list = vector<pair<string, string>>(30); //list of connections to ask about
                                    int num_inp, num_out;
                                    if(spec_conn.compare("empty") == 0){ //case scenario correspendence ordered one per one
                                        num_inp = getPorts(b, INPUT_PORTS).size(); //number of inputs and outputs should be the same
                                        for(int ind=0; ind< num_inp; ind++){
                                            conn_list.push_back(make_pair(mix_conn[0]+to_string(ind+1), mix_conn[1]+to_string(ind+1))); //Carmine 23.02.22 index starts from 1
                                        }
                                    }else{
                                        regex rg_conn("=");
                                        sregex_token_iterator rg_iter_conn(mix_conn_entire.begin(), mix_conn_entire.end(), rg_conn, -1);
                                        rg_iter_conn++;
                                        string multiple_conn = *rg_iter_conn; //second half of the connection table specifications with specific info about specific connections
                                        string single_conn;
                                        regex rg_single_conn("/");
                                        sregex_token_iterator rg_iter_mul_conn(multiple_conn.begin(), multiple_conn.end(), rg_single_conn, -1);
                                        sregex_token_iterator end_iter;

                                        for(; rg_iter_mul_conn != end_iter; rg_iter_mul_conn++){

                                            single_conn = *rg_iter_mul_conn;
                                            bool all_inp = false, all_out = false;
                                            if(single_conn[1] == '*') all_inp = true; //* means all inputs or outputs - depending on the position
                                            if(single_conn[3] == '*') all_out = true;

                                            if(all_inp){  //in case every inputs is included
                                                if(single_conn[0] == 'd' || single_conn[0] == 'v')
                                                    num_inp = getPorts(b, INPUT_PORTS).size();
                                                else
                                                    num_inp = getPorts(b, OUTPUT_PORTS).size();
                                                for(int ind_1 = 0; ind_1<num_inp; ind_1++){
                                                    if(all_out){    //in case every outputs is included
                                                        if(single_conn[2] == 'd' || single_conn[2] == 'v')
                                                            num_out = getPorts(b, OUTPUT_PORTS).size();
                                                        else
                                                            num_out = getPorts(b, INPUT_PORTS).size();
                                                        for(int ind_2 = 0; ind_2<num_out; ind_2++){
                                                            conn_list.push_back(make_pair(single_conn[0]+to_string(ind_1+1), single_conn[2]+to_string(ind_2+1))); //Carmine 23.02.22 index starts from 1
                                                        }
                                                    }else{ //in case not every output is included

                                                        conn_list.push_back(make_pair(single_conn[0]+to_string(ind_1+1), single_conn.substr(2,2))); //Carmine 23.02.22 index starts from 1
                                                    }
                                                }
                                            }else if(all_out){ //in case every outputs is included
                                                if(single_conn[2] == 'd' || single_conn[2] == 'v')
                                                    num_out = getPorts(b, OUTPUT_PORTS).size();
                                                else
                                                    num_out = getPorts(b, INPUT_PORTS).size();
                                                for(int ind_2 = 0; ind_2<num_out; ind_2++){
                                                    conn_list.push_back(make_pair(single_conn.substr(0,2), single_conn[2]+to_string(ind_2+1))); //Carmine 23.02.22 index starts from 1
                                                }
                                            }else{ //in case specific inputs and outputs are specified

                                                conn_list.push_back(make_pair(single_conn.substr(0,2), single_conn.substr(2,2)));
                                            }
                                        }
                                    }
                                    //now the connections pins extracted from the tables are used to define new rules in MILP
                                    //these pins have been saved into conn_list

                                    for(pair<string, string> connection: conn_list){
                                        if (connection.first == "") continue;
                                        string input = connection.first, output=connection.second;
                                        
                                        string inp_regex_exp = "(.*)_in" + input.substr(1,1) + "_mg"+to_string(mg);
                                        string out_regex_exp = "(.*)_out" + output.substr(1,1) + "_mg"+to_string(mg);
                                        //string inp_regex_exp = "", out_regex_exp = "";
                                        vector<int> time_paths_in, time_paths_out;
                                        bool inv_inp=false, inv_out=false;
                                        switch (input[0])
                                        {
                                        case 'd':
                                            time_paths_in = Vars.time_path;
                                            break;
                                        case 'v':
                                            time_paths_in = Vars.time_valid_path;
                                            break;
                                        case 'r':
                                            time_paths_in = Vars.time_ready_path;
                                            inv_inp=true;
                                            break;
                                        default:
                                            inp_regex_exp = "";
                                            cout << "*ERROR* table of modules. Impossible identify the input port " << input << endl;
                                            break;
                                        }

                                        switch (output[0])
                                        {
                                        case 'd':
                                            time_paths_out = Vars.time_path;
                                            break;
                                        case 'v':
                                            time_paths_out = Vars.time_valid_path;
                                            break;
                                        case 'r':
                                            time_paths_out = Vars.time_ready_path;
                                            inv_out=true;
                                            break;
                                        default:
                                            out_regex_exp = "";
                                            cout << "*ERROR* table of modules. Impossible identify the output port " << output << endl;
                                            break;
                                        }
                                        regex reg_inp(inp_regex_exp);
                                        regex reg_out(out_regex_exp);

                                        int v_in=-2, v_out=-2;
                                        if(!inv_out){
                                            ForAllOutputPorts(b, out_p) {
                                                if(cnt_blocks < size_MG_blocks && !MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                                                    continue;
                                                if(cnt_blocks >= size_MG_blocks && !MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(out_p))))
                                                    continue;

                                                int ind = time_paths_out[out_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_out)){
                                                    v_out = ind;
                                                }
                                            }
                                        } else {
                                            ForAllInputPorts(b, out_p) {
                                                if (cnt_blocks < size_MG_blocks && !MG_disjoint[mg].hasChannel(getConnectedChannel(out_p)))
                                                    continue;
                                                if(cnt_blocks >= size_MG_blocks && !MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(out_p))))
                                                    continue;

                                                int ind = time_paths_out[out_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_out)){
                                                    v_out = ind;
                                                }
                                            }
                                        }

                                        if(!inv_inp){
                                            ForAllInputPorts(b, in_p) {
                                                if (cnt_blocks < size_MG_blocks && !MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                                                    continue;
                                                if(cnt_blocks >= size_MG_blocks && !MG_disjoint[mg].hasBlock(getSrcBlock(getConnectedChannel(in_p))))
                                                    continue;

                                                int ind = time_paths_in[in_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_inp)){
                                                    v_in = ind;
                                                }
                                            }
                                        }else{
                                            ForAllOutputPorts(b, in_p) {
                                                if(cnt_blocks < size_MG_blocks && !MG_disjoint[mg].hasChannel(getConnectedChannel(in_p)))
                                                    continue;
                                                if(cnt_blocks >= size_MG_blocks && !MG_disjoint[mg].hasBlock(getDstBlock(getConnectedChannel(in_p))))
                                                    continue;

                                                int ind = time_paths_in[in_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_inp)){
                                                    v_in = ind;
                                                }
                                            }
                                        }
                                        if(v_in == -2 || v_out == -2){
                                            continue; //case in which the channel is not considered because out of MG
                                        }

                                        double D = x.second;
                                        string conn_type = string(1,input[0])+string(1,output[0]);

                                        //Andrea 20220727 Use delay read from the dot
                                        //int i = get_index_mixed_table(conn_type);
                                       // cout << " *****1***Andrea block: " <<  getBlockName(b) << " delay " << D  << endl; 
                                        //D = getBlockDelay(b, i);
                                        //cout << " *****1***Andrea block: " <<  getBlockName(b) << " delay " << D << " from index  "<< i << endl; 
                                        //
                                        
                                        if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire                                            
                                        if (D + BufferDelay > Period) {
                                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                                            return false;
                                        }

                                        milp.newRow( {{1, v_out}, {-1,v_in}}, '>', D);

                                    }

                                }
                            }
                        }
                    }

                }
                cnt_blocks++;
            }
        }

    }
    return true;
}

//Carmine 18.02.22 create paths constraints for valid and ready for remaining channels
bool DFnetlist_Impl::createPathConstraintsOthers_remaining(Milp_Model &milp, milpVarsEB &Vars, double Period,
                                                     double BufferDelay, string model_mode, string lib_path) {

    bool hasPeriod = Period > 0;
    if (not hasPeriod) Period = INFINITY;

    vector<int> time_paths, ob_presence;
    vector<string> modes;
    vector< pair<string, vector<double>>> block_delays = vector< pair<string, vector<double>>>(30); //Carmine 07.03.22 extract the delay of the component for the right bitwidth

	//Andrea 18.07.22 check delays

   ForAllBlocks(b) {
  //  	cout << "Component " << getBlockName(b) << "delay0: " << getBlockDelay(b, 0) << " delay1: " << getBlockDelay(b, 0) << " delay2: " << getBlockDelay(b, 0) << " delay3: " << getBlockDelay(b, 0) <<  endl;
    }
    //Carmine 22.03.22 include delays of buffers
    double delay_TEHB = -1.0, delay_OEHB = -1.0;

    if(!model_mode.compare("mixed"))
        modes = {"valid", "ready", "mixed"};
    else if(!model_mode.compare("all"))
        modes = {"valid", "ready"};
    else
        modes = {model_mode};
    

    for(string mode: modes){
        if(mode.compare("valid") == 0){
            time_paths = Vars.time_valid_path;
            ob_presence = Vars.buffer_flop_valid;
            
            if(FPL_22_CARMINE_LIB)
                block_delays = read_lib_file(lib_path+"/delays_lib_valid.txt");
        }else if(mode.compare("ready") == 0){
            time_paths = Vars.time_ready_path;
            ob_presence = Vars.buffer_flop_ready;
            if(FPL_22_CARMINE_LIB)
                block_delays = read_lib_file(lib_path+"/delays_lib_ready.txt");
        }else if(mode.compare("mixed") == 0){
            if(FPL_22_CARMINE_LIB)
                block_delays = read_lib_file(lib_path+"/delays_lib_mixed.txt");
            goto only_mixed_label_2;
        }else{
            cout << "*ERROR* mode " << mode << " is not available in the remaining channels analysis" << endl;
            continue;
        }

        //Carmine 22.03.22 include delays of buffers
        delay_TEHB = -1.0;
        delay_OEHB = -1.0;
        if(FPL_22_CARMINE_LIB){
            for(pair <string, vector<double>> x: block_delays){ if (x.first == "") continue; regex rg (x.first+"(.)*"); if(regex_match("TEHB", rg)) delay_TEHB = x.second[0]; }
            for(pair <string, vector<double>> x: block_delays){ if (x.first == "") continue; regex rg (x.first+"(.)*"); if(regex_match("OEHB", rg)) delay_OEHB = x.second[0]; }
        }else{
            delay_TEHB = 0.1;
            delay_OEHB = 0.1;
        }
        //the buffers delays are included only if the model mode is ALL or MIXED
        include_buff_delays = include_buff_delays & (  (model_mode.compare("mixed")==0) | (model_mode.compare("all")==0)  );

        ForAllChannels(c) {
            // Lana 02.05.20. paths from/to memory do not need buffers
            if (getBlockType(getDstBlock(c)) == LSQ || getBlockType(getSrcBlock(c)) == LSQ
            || getBlockType(getDstBlock(c)) == MC || getBlockType(getSrcBlock(c)) == MC )
                continue;

            int v1 =  time_paths[getSrcPort(c)]; //modified
            int v2 =  time_paths[getDstPort(c)]; //modified

            if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                int tmp = v1;
                v1 = v2;
                v2 = tmp;
            }

            int R = ob_presence[c];

            if (hasPeriod) {
                // v1, v2 <= Period
                milp.newRow( {{1,v1}}, '<', Period);

                milp.newRow( {{1,v2}}, '<', Period);
                
                // v2 >= v1 - 2*period*R
                if (channelIsCovered(c, true, true, false)){

                    bool buff_pres = getChannelBufferSize(c) > 0.5;
                    bool TEHB_presence, OEHB_presence;
                    if(!isChannelTransparent(c)){ 
                        TEHB_presence = buff_pres & getChannelEB(c); //Carmine 01.03.22 check if there is transparent buffer
                        OEHB_presence = buff_pres;
                    }else{
                        TEHB_presence = buff_pres;
                        OEHB_presence = false;
                    }
                    if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 01.03.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                    || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                        TEHB_presence = true;  
                    }

                    if(mode.compare("ready") != 0){
                        if(!include_buff_delays){
                            milp.newRow({{-1, v1}, {1, v2}}, '>', -2 * Period * !isChannelTransparent(c));
                        }else{ //Carmine 22.03.22 include the delays of buffer along channel
                            if(isChannelTransparent(c)){ //if the channel is transparent the only possible delay to influence the output is the TEHB presence
                                //Valid constraints
                                milp.newRow({{-1, v1}, {1, v2}}, '>', (delay_TEHB * TEHB_presence) );
                                //Data constraints
                                milp.newRow({{-1, Vars.time_path[getSrcPort(c)]}, {1, Vars.time_path[getDstPort(c)]}}, '>', (delay_TEHB * TEHB_presence) );
                            }else{  //if the channel is not transparent the input value is  "cut" and only the TEHB value can cause delay
                                //Valid constraints
                                milp.newRow({{1, v2}}, '>', (delay_TEHB * TEHB_presence) );
                                //Data constraints
                                milp.newRow({{1, Vars.time_path[getDstPort(c)]}}, '>', (delay_TEHB * TEHB_presence) );
                            }
                        }
                    }else{      
                        if(!include_buff_delays){                  
                            milp.newRow({{-1, v1}, {1, v2}}, '>', -2 * Period * TEHB_presence);
                        }else{ //Carmine 22.03.22 include the delays of buffer along channel
                            if(TEHB_presence){ //if there is a TEHB the input value is ignored since the channel is "cut"
                                milp.newRow({{1, v2}}, '>',(delay_OEHB * OEHB_presence) );
                            }else{
                                milp.newRow({{-1, v1}, {1, v2}}, '>', (delay_OEHB * OEHB_presence) );
                            }
                        }
                    }
                }else{
                    //milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}}, '>', 0);
                    if(!include_buff_delays)   
                        milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}}, '>', 0);
                    else{ //Carmine 22.03.22 include the delays of buffer along channel
                        if(mode.compare("ready") == 0){ 
                            assert(delay_OEHB>=0.0); //check if the delay of OEHB is bigger than 0 becuase it should be present in the library
                            milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0);
                            milp.newRow ( {{1,v2}, {-delay_OEHB, Vars.buffer_flop[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of OEHB is not accounted for

                        }else if(mode.compare("valid") == 0) { //for now the delay of TEHB along valid path is the same of the data path one
                            assert(delay_TEHB>=0.0); //check if the delay of TEHB is bigger than 0 becuase it should be present in the library
                            //Valid constraints
                            milp.newRow ( {{-1,v1}, {1,v2}, {2*Period, R}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0);
                            milp.newRow ( {{1,v2}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for

                            //Data constraints
                            milp.newRow ( {{-1,Vars.time_path[getSrcPort(c)]}, {1,Vars.time_path[getDstPort(c)]}, {2*Period, Vars.buffer_flop[c]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0);
                            milp.newRow ( {{1,Vars.time_path[getDstPort(c)]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', 0); //Carmine 24.03.22 it has to be included the case in which both buffers are present and and the delay of TEHB is not accounted for

                        }
                    }
                }
            }
            
            // v2 >= Buffer Delay
            if (BufferDelay > 0) milp.newRow( {{1,v2}}, '>', BufferDelay);

            if(!channelIsCovered(c, true, true, false)){ //Carmine 18.02.22 if channel is covered there is no need to question the presence of buffers
                //Carmine 17.02.22 //the presence of a buffer has to be consistent among different timing domains
                if(mode.compare("valid") == 0){
                    milp.newRow( {{1,Vars.buffer_flop[c]}, {-1,ob_presence[c]}}, '=', 0);
                }else if(mode.compare("ready") == 0){
                    if ((getBlockType(getSrcBlock(c)) == MUX)     //Carmine 21.02.22 MILP should be aware of presence of transparent buffers in MUX and MERGE
                    || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
                        milp.newRow( {{1,ob_presence[c]}}, '>', 1); 
                    }
                }
            }
            
        }

        // Create the constraints to propagate the delays of the blocks
        ForAllBlocks(b) {
            string name_block, type_block;
            double D = -1.0;
            name_block = getBlockName(b);
            int bitwidth_max=0, bitwidth; //Carmine 07.03.22 extract the delay of the component for the right bitwidth
            for(portID x: getPorts(b,INPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
            for(portID x: getPorts(b,OUTPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
            if(bitwidth_max == 0) bitwidth_max = 1;
            int pos_bitwidth = ceil(log2(float(bitwidth_max)));

            if(FPL_22_CARMINE_LIB){
                for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                    if (x.first == "") continue;
                    regex rg (x.first+"(.)*");
                    if(regex_match(name_block, rg)){
                        D = x.second[pos_bitwidth];
                    }
                }
                if(D == -1.0){
                    type_block = printBlockType(getBlockType(b));
                    for(int i=0;i<type_block.size();i++){ type_block[i]=tolower(type_block[i]);}
                    for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                        if (x.first == "") continue;
                        regex rg (x.first+"(.)*");
                        if(regex_match(type_block, rg)){
                            D = x.second[pos_bitwidth];
                        }
                    }
                }
                if(D == -1.0){D=0.0; cout << "**WARNING** Delay of block " << name_block <<" between "<<mode<<" cannot be found!!"<< endl; }
            }else{
                    //Carmine 02.08.22 obtaining mixed connections delays from the new function getBlockDelay (OPEN_CIRCUIT_DELAY is the special value)
                if(mode.compare("valid") == 0) //offset of ready in the map is + 7 (+7 data)
                    D = getBlockDelay(b, 1);
                    //D = getBlockDelay(b, pos_bitwidth + 7);
                if(mode.compare("ready") == 0) //offset of ready in the map is + 14 (+7 data +7 valid)
                    D = getBlockDelay(b, 2);
                    //D = getBlockDelay(b, pos_bitwidth + 14);

                if(D>= OPEN_CIRCUIT_DELAY)
                    continue;
            }
            // First: combinational blocks
            //if (getLatency(b) == 0) { //Carmine 18.02.22 for valid and ready the latency is 0
            ForAllOutputPorts(b, out_p) {
                int v_out = time_paths[out_p];

                ForAllInputPorts(b, in_p) {
                    int v_in = time_paths[in_p];
                    //Carmine 24.03.22 this set of equations is inclued in the channel delays
                    /*double delayBlock_data; //Carmine 22.03.22 necessary to compute the delay on data path for constraints with buffers
                    if(getLatency(b)==0) 
                        delayBlock_data = getCombinationalDelay(in_p, out_p); 
                    else
                        delayBlock_data = 0.0;*/
                    if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire
                    //Carmine 18.02.22 for now the delays valid and ready are suppossed to be the same amount amoong different inputs and outputs
                    if (D + BufferDelay > Period) {
                        setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                        return false;
                    }

                    //Carmine 24.03.22 this set of equations is inclued in the channel delays
                    /*if (include_buff_delays && (D + BufferDelay + delay_TEHB > Period || delayBlock_data + BufferDelay + delay_TEHB > Period  )){  //Carmine 22.03.22 it is important to include the case in which TEHB is present since the Period constraint canno be respected
                        setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied since it is preceded by a TEHB.");
                        return false;
                    }*/

                    int tmp_out = v_out;
                    int tmp_in = v_in;

                    if(mode.compare("ready") == 0){ //Carmine 17.02.22 // the ready signal direction is opposite to the one of valid
                        tmp_out = v_in;
                        tmp_in = v_out;
                    }

                    //Carmine 24.03.22 this set of equations is inclued in the channel delays
                    /*if(include_buff_delays && mode.compare("valid") == 0){  //Carmine 22.03.22 include the delays of TEHB in the valid and data delays 
                        channelID c = getConnectedChannel(in_p);
                        if(Vars.buffer_flop_ready[c] != invalidDataflowID){
                            milp.newRow( {{1, tmp_out}, {-1,tmp_in}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', D);
                            if(getLatency(b)==0)
                                milp.newRow( {{1, Vars.time_path[out_p] }, {-1,Vars.time_path[in_p]}, {-delay_TEHB, Vars.buffer_flop_ready[c]}}, '>', delayBlock_data);
                        }else //in case it is a source with no channel at input
                            milp.newRow( {{1, tmp_out}, {-1,tmp_in}, }, '>', D);
                    }else{*/  
                    milp.newRow( {{1, tmp_out}, {-1,tmp_in}, }, '>', D);
                    //}
                }
            }
        }

        only_mixed_label_2: //Carmine 23.02.22 label to jump directly for mixed since it is only necessary insert blocks mixed connections delays

        if(mode.compare("mixed") == 0){
            
            ForAllBlocks(b) {

                vector< pair<string, double>> delays_per_conn = vector< pair<string, double>>(30);

                //Carmine 02.08.22 New macro to use the old libraries or the new function to get the delays
                if(FPL_22_CARMINE_LIB){
                    bool found = false;
                    string name_block, type_block;
                    
                    name_block = getBlockName(b);
                    int bitwidth_max=0, bitwidth; //Carmine 07.03.22 extract the delay of the component for the right bitwidth
                    for(portID x: getPorts(b,INPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                    for(portID x: getPorts(b,OUTPUT_PORTS)){ bitwidth = getPortWidth(x); if(bitwidth>bitwidth_max) bitwidth_max=bitwidth;}
                    if(bitwidth_max == 0) bitwidth_max = 1;
                    int pos_bitwidth = ceil(log2(float(bitwidth_max)));
                    for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                        if (x.first == "") continue;

                        string name_block_lib, mixed_conn;
                        regex rg_lib("-"); //extract from the mixed library the name of the block and which connection it refers to
                        sregex_token_iterator rg_iter(x.first.begin(), x.first.end(), rg_lib, -1);
                        name_block_lib = *rg_iter;
                        rg_iter++;
                        mixed_conn = *rg_iter;
                        regex rg (name_block_lib+"(.)*");

                        if(regex_match(name_block, rg)){
                            found = true;
                            delays_per_conn.push_back(make_pair(mixed_conn, x.second[pos_bitwidth]));
                            //delays_per_conn.push_back(make_pair(mixed_conn, getBlockDelay(b, get_index_mixed_table(mixed_conn))));

                        }
                    }

                    if(!found){
                        type_block = printBlockType(getBlockType(b));
                        for(int i=0;i<type_block.size();i++){ type_block[i]=tolower(type_block[i]);}
                        for(pair <string, vector<double>> x: block_delays){ //Carmine 07.03.22
                            if (x.first == "") continue;

                            string name_block_lib, mixed_conn;
                            regex rg_lib("-"); //extract from the mixed library the name of the block and which connection it refers to
                            sregex_token_iterator rg_iter(x.first.begin(), x.first.end(), rg_lib, -1);
                            name_block_lib = *rg_iter;
                            rg_iter++;
                            mixed_conn = *rg_iter;
                            regex rg (name_block_lib+"(.)*");

                            if(regex_match(type_block, rg)){
                                found = true;
                                delays_per_conn.push_back(make_pair(mixed_conn, x.second[pos_bitwidth]));
                                //delays_per_conn.push_back(make_pair(mixed_conn, getBlockDelay(b, get_index_mixed_table(mixed_conn))));
                            }
                        }
                    }
                    if(!found){continue;}
                }else{

                    //Carmine 02.08.22 obtaining mixed connections delays from the new function getBlockDelay (OPEN_CIRCUIT_DELAY is the special value)
                    std::vector<string> vec_mixed_connections = {"vr", "cv", "cr", "vc", "vd"};

                    for(string mixed_conn: vec_mixed_connections){
                        int index = get_index_mixed_table(mixed_conn);
                        double delay = getBlockDelay(b, index+3);
                        //double delay = getBlockDelay(b, index+21); //the initial offset of mixed is + 21 (+7 data +7 valid +7 ready)
                        if (delay< OPEN_CIRCUIT_DELAY)
                            delays_per_conn.push_back(make_pair(mixed_conn, delay));
                    }
                }

                for(pair <string, double> x: delays_per_conn){
                    if (x.first == "") continue;

                    for( pair<BlockType, string> y: MixedConnectionBlock){
                        if(y.first == getBlockType(b)){
                            string mix_conn_entire, mix_conn, spec_conn;
                            regex rg_table("_");
                            sregex_token_iterator rg_iter(y.second.begin(), y.second.end(), rg_table, -1);
                            sregex_token_iterator end_iter;

                            for(; rg_iter != end_iter; rg_iter++){
                                mix_conn_entire = *rg_iter;

                                regex reg_any_eq("(.*)=(.*)");
                                if(regex_match(mix_conn_entire, reg_any_eq)){  //there is a specification about single connections
                                    regex rg_conn("=");
                                    sregex_token_iterator rg_iter_conn(mix_conn_entire.begin(), mix_conn_entire.end(), rg_conn, -1);
                                    mix_conn = *rg_iter_conn;
                                }else{
                                    mix_conn = mix_conn_entire;
                                    spec_conn = "empty";
                                }

                                if(mix_conn.compare(x.first) == 0){ //the connection saved in the table MixecConnectionBlock is equal to the one extracted from the lib
                                    vector<pair<string, string>> conn_list = vector<pair<string, string>>(30); //list of connections to ask about
                                    int num_inp, num_out;
                                    if(spec_conn.compare("empty") == 0){ //case scenario correspendence ordered one per one
                                        num_inp = getPorts(b, INPUT_PORTS).size(); //number of inputs and outputs should be the same
                                        for(int ind=0; ind< num_inp; ind++){
                                            conn_list.push_back(make_pair(mix_conn[0]+to_string(ind+1), mix_conn[1]+to_string(ind+1)));
                                        }
                                    }else{
                                        regex rg_conn("=");
                                        sregex_token_iterator rg_iter_conn(mix_conn_entire.begin(), mix_conn_entire.end(), rg_conn, -1);
                                        rg_iter_conn++;
                                        string multiple_conn = *rg_iter_conn; //second half of the connection table specifications with specific info about specific connections
                                        string single_conn;
                                        regex rg_single_conn("/");
                                        sregex_token_iterator rg_iter_mul_conn(multiple_conn.begin(), multiple_conn.end(), rg_single_conn, -1);
                                        sregex_token_iterator end_iter;

                                        for(; rg_iter_mul_conn != end_iter; rg_iter_mul_conn++){

                                            single_conn = *rg_iter_mul_conn;
                                            bool all_inp = false, all_out = false;
                                            if(single_conn[1] == '*') all_inp = true; //* means all inputs or outputs - depending on the position
                                            if(single_conn[3] == '*') all_out = true;

                                            if(all_inp){  //in case every inputs is included
                                                if(single_conn[0] == 'd' || single_conn[0] == 'v')
                                                    num_inp = getPorts(b, INPUT_PORTS).size();
                                                else
                                                    num_inp = getPorts(b, OUTPUT_PORTS).size();
                                                for(int ind_1 = 0; ind_1<num_inp; ind_1++){
                                                    if(all_out){    //in case every outputs is included
                                                        if(single_conn[2] == 'd' || single_conn[2] == 'v')
                                                            num_out = getPorts(b, OUTPUT_PORTS).size();
                                                        else
                                                            num_out = getPorts(b, INPUT_PORTS).size();
                                                        for(int ind_2 = 0; ind_2<num_out; ind_2++){
                                                            conn_list.push_back(make_pair(single_conn[0]+to_string(ind_1+1), single_conn[2]+to_string(ind_2+1)));
                                                        }
                                                    }else{ //in case not every output is included

                                                        conn_list.push_back(make_pair(single_conn[0]+to_string(ind_1+1), single_conn.substr(2,2)));
                                                    }
                                                }
                                            }else if(all_out){ //in case every outputs is included
                                                if(single_conn[2] == 'd' || single_conn[2] == 'v')
                                                    num_out = getPorts(b, OUTPUT_PORTS).size();
                                                else
                                                    num_out = getPorts(b, INPUT_PORTS).size();
                                                for(int ind_2 = 0; ind_2<num_out; ind_2++){
                                                    conn_list.push_back(make_pair(single_conn.substr(0,2), single_conn[2]+to_string(ind_2+1)));
                                                }
                                            }else{ //in case specific inputs and outputs are specified

                                                conn_list.push_back(make_pair(single_conn.substr(0,2), single_conn.substr(2,2)));
                                            }
                                        }
                                    }
                                    //now the connections pins extracted from the tables are used to define new rules in MILP
                                    //these pins have been saved into conn_list

                                    for(pair<string, string> connection: conn_list){
					int i =0;
                                        if (connection.first == "") continue;
                                        string input = connection.first, output=connection.second;
                                        
                                        string inp_regex_exp = "(.*)_in" + input.substr(1,1);
                                        string out_regex_exp = "(.*)_out" + output.substr(1,1);
                                        //string inp_regex_exp = "", out_regex_exp = "";
                                        vector<int> time_paths_in, time_paths_out;
                                        bool inv_inp=false, inv_out=false;
                                        switch (input[0])
                                        {
                                        case 'd':
                                            time_paths_in = Vars.time_path;
                                            break;
                                        case 'v':
                                            time_paths_in = Vars.time_valid_path;
                                            break;
                                        case 'r':
                                            time_paths_in = Vars.time_ready_path;
                                            inv_inp=true;
                                            break;
                                        default:
                                            inp_regex_exp = "";
                                            cout << "*ERROR* table of modules. Impossible identify the input port " << input << endl;
                                            break;
                                        }

                                        switch (output[0])
                                        {
                                        case 'd':
                                            time_paths_out = Vars.time_path;
                                            break;
                                        case 'v':
                                            time_paths_out = Vars.time_valid_path;
                                            break;
                                        case 'r':
                                            time_paths_out = Vars.time_ready_path;
                                            inv_out=true;
                                            break;
                                        default:
                                            out_regex_exp = "";
                                            cout << "*ERROR* table of modules. Impossible identify the output port " << output << endl;
                                            break;
                                        }
                                        regex reg_inp(inp_regex_exp);
                                        regex reg_out(out_regex_exp);

                                        int v_in=-2, v_out=-2;
                                        if(!inv_out){
                                            ForAllOutputPorts(b, out_p) {
                                                
                                                int ind = time_paths_out[out_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_out)){
                                                    v_out = ind;
                                                }
                                            }
                                        } else {
                                            ForAllInputPorts(b, out_p) {
                                                

                                                int ind = time_paths_out[out_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_out)){
                                                    v_out = ind;
                                                }
                                            }
                                        }

                                        if(!inv_inp){
                                            ForAllInputPorts(b, in_p) {
                                                

                                                int ind = time_paths_in[in_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_inp)){
                                                    v_in = ind;
                                                }
                                            }
                                        }else{
                                            ForAllOutputPorts(b, in_p) {
                                                

                                                int ind = time_paths_in[in_p];
                                                string var_name = milp.getVarName(ind);
                                                
                                                if(regex_match(var_name, reg_inp)){
                                                    v_in = ind;
                                                }
                                            }
                                        }
                                        if(v_in == -2 || v_out == -2){
                                            continue; //case in which the channel is not considered because out of MG
                                        }

                                        double D = x.second;

                                        //string conn_type = string(1,input[0])+string(1,output[0]);

                                        //Andrea 20220727 Use delay read from the dot
                                        //i = get_index_mixed_table(conn_type);
                                       // cout << " *****2***Andrea block: " <<  getBlockName(b) << "b" << b <<  " delay " << D << endl; 
                                        //D = getBlockDelay(b, i);
                                        //cout << " *****2***Andrea block: " <<  getBlockName(b) << "b" << b <<  " delay " << D << " from index  "<< i << endl; 
                                        if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire                                         
                                        if (D + BufferDelay > Period) {
                                            setError("Block " + getBlockName(b) + ": path constraints cannot be satisfied.");
                                            return false;
                                        }

                                        milp.newRow( {{1, v_out}, {-1,v_in}}, '>', D);

                                    }

                                }
                            }
                        }
                    }
                }
                
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
        double d = getBlockDelay(b, -1);

        // First: combinational blocks
        if (getLatency(b) == 0) {
            ForAllOutputPorts(b, out_p) {
                int v_out = Vars.time_path[out_p];
                ForAllInputPorts(b, in_p) {
                    int v_in = Vars.time_path[in_p];
                    // Add constraint v_out >= d_in + d + d_out + v_in;
                    double D = getCombinationalDelay(in_p, out_p);
                    if (getBlockType(b) == MERGE && getPorts(b, INPUT_PORTS).size() == 1) D = 0.0; //Carmine 09.03.22 1-input merge corresponds to wire
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
        /*if ((getBlockType(getSrcBlock(c)) == MUX)
            || (getBlockType(getSrcBlock(c)) == MERGE && getPorts(getSrcBlock(c), INPUT_PORTS).size() > 1)) {
            //slots = 1; 
            //hasbuf = 1; 
            //hasflop = 0;
        } In reality this case is already taken into consideration*/

        if (getBlockType(getSrcBlock(c)) == FORK && getBlockType(getDstBlock(c)) == BRANCH) {
            if (getPortWidth(getDstPort(c)) == 0) {
                bool lsq_fork = false;

                ForAllOutputPorts(getSrcBlock(c), out_p) {
                    if (getBlockType(getDstBlock(getConnectedChannel(out_p))) == LSQ)
                        lsq_fork = true;
                    
                }
                if (lsq_fork) {
                    milp.newRow({{1, hasflop}}, '>', 1); //Carmine 28.02.22 it is important to ensure the presence of the nonTrans buffer for any channel connecting to LSQ
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
            // milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}}, '<', maxTokens);
            // milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '>', 0);

            // Lana 24.7.21. Fix pipeline unit occupancy to exactly lat*th_mg
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '=', 0);
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
            // milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}}, '<', maxTokens);
            // Th*Lat <= rout-rin:   rout-rin-Th*lat >= 0
            // milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '>', 0);

            // Lana 24.7.21. Fix pipeline unit occupancy to exactly lat*th_mg
            milp.newRow( {{1, out_ret_tok}, {-1, in_ret_tok}, {-lat, th_mg}}, '=', 0);
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

blockID DFnetlist_Impl::insertBuffer(channelID c, int slots, bool transparent, bool EB) //Carmine 21.02.22 // additional version for EB case
{
    portID src = getSrcPort(c);
    portID dst = getDstPort(c);
    bool back = isBackEdge(c);
    blockID b_src = getSrcBlock(c);
    bbID bb_src = getBasicBlock(b_src);

    bool multiType_multiSlots = EB & (slots>1); //Carmine 25.03.22 to reproduce the elastic buffer if there are 2 slots needed and both 
                                                        // types necessary only 1 TEHB and 1 OEHB are necessary since both are needed
                                                        // to cut the critical path and both can hold a token

    removeChannel(c);

    int width = getPortWidth(src);
    blockID eb = createBlock(ELASTIC_BUFFER);
    if(!multiType_multiSlots)      //Carmine 25.03.22
        setBufferSize(eb, slots);
    else                           //Carmine 25.03.22 if this is the case only 1 OEHB is implemented and the rest is TEHB
        setBufferSize(eb, 1);
    setBufferTransparency(eb, transparent);
    setBasicBlock(eb, bb_src);
    portID in_buf_1 = createPort(eb, true, "in1", width);
    portID out_buf_1 = createPort(eb, false, "out1", width);
    createChannel(src, in_buf_1);
    portID in_buf_2, out_buf_2;

    if(EB){
        blockID eb_2 = createBlock(ELASTIC_BUFFER);
        if(!multiType_multiSlots)      //Carmine 25.03.22
            setBufferSize(eb_2, slots);
        else                           //Carmine 25.03.22 if this is the case only 1 OEHB is implemented and the rest is TEHB
            setBufferSize(eb_2, slots-1); //the tehb covers the remaining slots
        setBufferTransparency(eb_2, true);
        setBasicBlock(eb_2, bb_src);
        in_buf_2 = createPort(eb_2, true, "in1", width);
        out_buf_2 = createPort(eb_2, false, "out1", width);
        channelID buf_to_buf = createChannel(out_buf_1, in_buf_2);
        setBackEdge(buf_to_buf, back);
    }else{
        out_buf_2 = out_buf_1;
    }
    
    channelID new_c = createChannel(out_buf_2, dst);
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
            if (getChannelBufferSize(c) == 2 || (getChannelBufferSize(c) == 1 & !isChannelTransparent(c))) {
            	insertBuffer(c, 1, isChannelTransparent(c));
                continue;
            }
        }

        insertBuffer(c, getChannelBufferSize(c), isChannelTransparent(c), getChannelEB(c));  //Carmine 21.02.22 the EB option has been added
    }

   // Lana 02/07/19 We don't need this
    //invalidateBasicBlocks();
}

void DFnetlist_Impl::instantiateAdditionalElasticBuffers(const std::string& filename) //Carmine 09.02.22 additional buffers option
{
    std::cout << "\nINSTANTIATE ADDITIONAL BUFFER\n";

    vecChannels ebs;

    ifstream file(filename);
    string line, word, src_i, dst_i, extra_s_i;
    bool extra_t_i;
    char *tmp;
    int cnt;
    list<string> src, dst, extra_size;
    list<bool> extra_trans;
    regex rg("\\s+");
    while(getline(file,line)){
        sregex_token_iterator rg_iter(line.begin(), line.end(), rg, -1);
        sregex_token_iterator end_iter;
        cnt = 0;
        for(; rg_iter != end_iter; rg_iter++){
            word = *rg_iter;
            switch (cnt){ // the format of the file is: src -> dst tran/nonTran size
            case 0:
                src.push_back(word);
                break;
            case 2:
                dst.push_back(word);
                break;
            case 3:
                if(word.compare("tran") == 0)
                    extra_trans.push_back(true);
                else if(word.compare("nonTran") == 0)
                    extra_trans.push_back(false);
                else
                    cout<< "*ERROR format line " << line << " \nThe third arg can be tran or nonTran" << endl;
                break;
            case 4:
                extra_size.push_back(word);
                break;
            default:
                break;
            }
            cnt++;
        }
    }
    file.close();

    if(src.size() == 0 or src.size() != dst.size() or dst.size() != extra_size.size() or dst.size() != extra_trans.size()){

        cout<< "*ERROR into reading file for additional buffers" << endl;
        return;
    }

    bool found;

    while(src.size()>0){
        src_i = src.front();
        src.pop_front();

        dst_i = dst.front();
        dst.pop_front();

        found = false;
        ForAllChannels(c) {
            if(getBlockName(getSrcBlock(c)).compare(src_i) == 0 && getBlockName(getDstBlock(c)).compare(dst_i) == 0 && !hasBuffer(c)){
                ebs.push_back(c);
                found = true;
            }
        }

        if(!found)
            cout << "*ERROR edges extraction " << src_i << " -> " << dst_i << endl;
    }
    
    if(ebs.size() != extra_trans.size()){
        return;
    }

    for (channelID c: ebs) {
        extra_t_i = extra_trans.front();
        extra_trans.pop_front();

        extra_s_i = extra_size.front();
        extra_size.pop_front();

        insertBuffer(c, stoi(extra_s_i), extra_t_i);

    }
    
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
    ForAllBlocks(b) setBlockDelay(b, 1, 0);
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
            if(milp[in_ret] < 0.001) continue; //Carmine 01.03.22 if the retiming timing is very small approximate it to 0
            setBlockRetimingDiff(b, milp[in_ret]);
        } else {
			setBlockRetimingDiff(b, milp[out_ret] - milp[in_ret]);
        }
        //cout << ", Retiming bub = " << milp[ret_bub] << endl;
    }
}
