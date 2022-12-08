#include <cassert>
#include <sys/time.h>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

// Frequencies must be defined before calling this function.
// If frequency of an arc is not set or set to zero or less, it will not be included in the MILP.
DFnetlist_Impl::subNetlistBB DFnetlist_Impl::extractMarkedGraphBB(const map<bbID, double> &freq) {
    Milp_Model milp;
    milp.init();

    // set the execution frequencies and calculate maximum frequency.
    double N_max = 0;
    map<bbArcID, double> N_e;
    for (bbArcID i = 0; i < BBG.numArcs(); i++){
        double value = 0;
        if (freq.find(i) != freq.end()) value = (freq.find(i))->second;

        N_e[i] = value;
        N_max = max(N_e[i], N_max);
    }

    vector<int> S_BB(BBG.numBasicBlocks() + 1, -1);     // Variables to model BB presence
    vector<int> S_e(BBG.numArcs(), -1);                 // Variables to model edge presence
    vector<int> NxS_e(BBG.numArcs(), -1);               // Variables to model N * S_e
    int N;                                              // Variable to model CFDFC frequency

    // create the variables
    for (bbID i = 1; i <= BBG.numBasicBlocks(); i++){
        S_BB[i] = milp.newBooleanVar("S_BB_" + to_string(i));
    }
    for (bbArcID i = 0; i < BBG.numArcs(); i++){
        if (BBG.getSrcBB(i) == 0 || BBG.getDstBB(i) == 0) continue;
        if (N_e[i] <= 0) continue;

        S_e[i] = milp.newBooleanVar("S_e_" + to_string(i));
        NxS_e[i] = milp.newRealVar("NxS_e" + to_string(i), 0, N_e[i]);
    }
    N = milp.newIntegerVar("N", 0, N_max);

    vector<int> inVars, outVars;
    for (bbID i = 1; i <= BBG.numBasicBlocks(); i++){
        inVars.clear();
        outVars.clear();

        for (bbArcID succ: BBG.successors(i)){
            if (S_e[succ] != -1)
                inVars.push_back(S_e[succ]);
        }

        for (bbArcID pred: BBG.predecessors(i)){
            if (S_e[pred] != -1)
                outVars.push_back(S_e[pred]);
        }

        milp.equalSum(inVars, {S_BB[i]});
        milp.equalSum(outVars, {S_BB[i]});
    }

    Milp_Model::vecTerms one_MG_constraint;
    for (bbArcID i = 0; i < BBG.numArcs(); i++) {
        if (S_e[i] == -1) continue;

        milp.newRow( {{1, NxS_e[i]}, {-1, N}}, '<', 0);
        milp.newRow( {{1, NxS_e[i]}, {-N_max, S_e[i]}}, '<', 0);
        milp.newRow( {{1, NxS_e[i]}, {-1, N}, {-N_max, S_e[i]}}, '>', -N_max);

        Milp_Model::vecTerms N_constraint;
        N_constraint.push_back({N_e[i], S_e[i]});
        N_constraint.push_back({-1 * N_max, S_e[i]});
        N_constraint.push_back({-1, N});
        milp.newRow(N_constraint, '>', -1 * N_max);

        if (BBG.isBackArc(i)) {
            one_MG_constraint.push_back({1, S_e[i]});
        }
    }
    milp.newRow(one_MG_constraint, '=', 1);

    for (bbArcID i = 0; i < BBG.numArcs(); i++) {
        if (S_e[i] == -1) continue;

        milp.newCostTerm(1, NxS_e[i]);
    }
    milp.setMaximize();

    // Run MILP
    milp.solve();

    // add the selected blocks and arcs to the cfdfc.
    subNetlistBB selected;
    for (bbID i = 1; i <= BBG.numBasicBlocks(); i++){
        if (S_BB[i] >= 0 and milp.isTrue(S_BB[i])){
            selected.insertBasicBlock(i);
        }
    }

    cout << "Arcs in the CFDFC:" << endl;
    for (bbArcID i = 0; i < BBG.numArcs(); i++) {
        if (S_e[i] >= 0 and milp.isTrue(S_e[i])){
            selected.insertBasicBlockArc(i);

            cout << "\t" << BBG.getSrcBB(i) << "->" << BBG.getDstBB(i) << ":" << (freq.find(i))->second << endl;
        }
    }

    return selected;
}

DFnetlist_Impl::subNetlist DFnetlist_Impl::extractMarkedGraph(const map<blockID, double>& freq)
{

    // Initialize the MILP model
    Milp_Model milp;
    milp.init();

    // Boolean variables for MILP model
    vector<int> blockMG(vecBlocksSize(), -1); // Block belongs to MG
    vector<int> channelMG(vecChannelsSize(), -1); // Channel belongs to MG

    // Create boolean variables for the channels
    ForAllChannels(c) {

        //shab_note
   //     blockID src = getSrcBlock(c), dst = getDstBlock(c);
  //      if (getBasicBlock(src) == 0) continue;
   //     if (getBasicBlock(dst) == 0) continue;

        channelMG[c] = milp.newBooleanVar();
    }

    // Constraints for the blocks
    vector<int> inVars, outVars, allVars;
    int var;
    ForAllBlocks(b) {
        //shab_note
 //       if (getBasicBlock(b) == 0) continue;

        switch (getBlockType(b)) {
        case ELASTIC_BUFFER:
        case OPERATOR:
        case FORK:
        case SELECT:
        case CONSTANT:
            allVars.clear();
             ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);

                // Lana 30.06.19. Ignore channels from MC/LSQ
                blockID bb = getSrcBlock(c);
                //shab_note
      //          if (getBasicBlock(bb) == 0) continue;

                if ((channelMG[c] >= 0) && getBlockType(bb) != MC && getBlockType(bb) != LSQ) 
                    allVars.push_back(channelMG[c]);
            }
             ForAllOutputPorts(b, p) {
                 channelID c = getConnectedChannel(p);
                 // Lana 30.06.19. Ignore channels to MC/LSQ
                 blockID bb = getDstBlock(c);
                 //shab_note
        //         if (getBasicBlock(bb) == 0) continue;

                 if ((channelMG[c] >= 0) && getBlockType(bb) != MC && getBlockType(bb) != LSQ)
                    allVars.push_back(channelMG[c]);
                 
             }
            if (allVars.size() >= 2) milp.allEqual(allVars);
            break;
        case MERGE:
            // The number of selected input and output channels must be the same
            // Merge only has one output channel.
            inVars.clear();
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                if (channelMG[c] >= 0) inVars.push_back(channelMG[c]);
            }

            var = channelMG[getConnectedChannel(getOutPort(b))];
            assert(var >= 0);
            milp.equalSum(inVars, {var});
            break;

        case MUX:
            // The number of selected non-condition inputs and output channels must be the same
            // If selecting output, select condition input as well
            // Mux only has one output channel.
            int var2;
            inVars.clear();
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                if (channelMG[c] >= 0 && getPortType(p) != SELECTION_PORT) {
                    inVars.push_back(channelMG[c]);
                    //cout << "mux port nonsel\n";
                }
                else if (channelMG[c] >= 0 && getPortType(p) == SELECTION_PORT) {
                    //cout << "mux port sel\n";
                    var2 = channelMG[c];
                }
            }

            var = channelMG[getConnectedChannel(getOutPort(b))];
            assert(var >= 0);
            milp.equalSum(inVars, {var});
            milp.equalSum({var}, {var2});
            break;

        case CNTRL_MG:
            // The number of selected non-condition inputs and output channels must be the same
            // If selecting output, select condition input as well
            // Mux only has one output channel.
            inVars.clear();
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                if (channelMG[c] >= 0) inVars.push_back(channelMG[c]);
            }


            ForAllOutputPorts(b, p) {
                channelID c = getConnectedChannel(p);

                 if (channelMG[c] >= 0)
                    milp.equalSum(inVars, {channelMG[c]});
            
            }
            break;

        case BRANCH:
            // At most one of the output ports must be selected.
            // The input ports must be selected iff one of the output ports is selected.
            inVars.clear();
            outVars.clear();
            //cout <<"-=====\n";
            ForAllOutputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                
                if (channelMG[c] >= 0) outVars.push_back(channelMG[c]);
                // Lana 30.04.19. Removing sink node check (modified under SINK below)
                // blockID bb = getDstBlock(c);
                //if (channelMG[c] >= 0 && getBlockType(bb) == SINK) milp.equalSum({0}, {channelMG[c]});
            }
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                if (channelMG[c] >= 0) milp.equalSum(outVars, {channelMG[c]});
            }
            ForAllBlocks(x) {

                //shab_note
            //    if (getBasicBlock(x) == 0) continue;

                if (getBlockType(x) == BRANCH && getBasicBlock(x) == getBasicBlock(b)) {
                    //cout <<"--------\n";
                    ForAllOutputPorts(b, p) {
                        ForAllOutputPorts(x, q) {
                            if (getPortType(p) == getPortType(q)) {
                                channelID c = getConnectedChannel(p);
                                channelID d = getConnectedChannel(q);
                                //if (channelMG[c] < 0 || channelMG[d] < 0) continue;
                                //cout << "*******\n";
                                // shab_note
                               // if (channelMG[c] < 0) continue;
                               // if (channelMG[d] < 0) continue;

                                milp.equalSum( {channelMG[d]}, {channelMG[c]});
                            }
                        }
                    }
                }
                
            }
            break;

        case DEMUX:
            // The pairs of control/data ports must have the same value.
            // Data is selected iff one of the output is selected
            outVars.clear();
            ForAllOutputPorts(b, outp) {
                portID inp = getDemuxComplementaryPort(outp);
                assert(validPort(inp));
                int invar = channelMG[getConnectedChannel(inp)];
                int outvar = channelMG[getConnectedChannel(outp)];
                assert (invar == outvar or (invar >= 0 and outvar >= 0));
                if (invar >= 0 and outvar >= 0) {
                    milp.equalSum( {invar}, {outvar});
                    outVars.push_back(outvar);
                }
            }
            var = channelMG[getConnectedChannel(getDataPort(b))];
            if (var >= 0) milp.equalSum(outVars, {var});
            break;
        case FUNC_ENTRY:
            // Lana 30.04.19 Explicitly mark entry output as non-MG
        	ForAllOutputPorts(b, p) {
                channelID c = getConnectedChannel(p);

                //shab_note
                //if (channelMG[c] < 0) continue;

                milp.equalSum({}, {channelMG[c]});
            }
            break;
        case FUNC_EXIT:
            // Lana 30.04.19 Explicitly mark exit input as non-MG
        	ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);

                //shab_note
             //   if (channelMG[c] < 0) continue;

                milp.equalSum({}, {channelMG[c]});
            }
            break;
        case SOURCE:
        	// Do nothing. These blocks should not be in SCCs
        	break;
        case SINK:
            // Lana 30.04.19 Explicitly mark sink input as non-MG when predecessor MERGE
        	// Sink CAN be in MG if connected to Br or Store output
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                blockID bb = getSrcBlock(c);

                // shab_note
             //   if (channelMG[c] < 0) continue;
             //   if (getBasicBlock(bb) == 0) continue;

            	if (getBlockType(bb) == MERGE) milp.equalSum({}, {channelMG[c]});
            }
            break;
          // Lana 30.06.19 Explicitly mark all LSQ/MC inputs/outputs as non-MG
        case MC:
        case LSQ:
            ForAllInputPorts(b, p) {
                channelID c = getConnectedChannel(p);

                //shab_note
               // if (channelMG[c] < 0) continue;

                milp.equalSum({}, {channelMG[c]});
            }
            break;

            ForAllOutputPorts(b, p) {
                channelID c = getConnectedChannel(p);
                milp.equalSum({}, {channelMG[c]});
            }
            break;

        default:
            assert(false);
        }
    }

    // Create variables for blocks and relationships with channels
    ForAllBlocks(b) {
        if (getBasicBlock(b) == 0) continue;

        vector<int> ch; // Set of channels eligible for MG
        ForAllPorts(b, p) {
            int v = channelMG[getConnectedChannel(p)];
            if (v >= 0) ch.push_back(v);
        }

        if (not ch.empty()) {
            blockMG[b] = milp.newBooleanVar(getBlockName(b));
            //cout << "Block " << getBlockName(b) << ": var " << blocks[b].inMG << endl;
            milp.implies(blockMG[b], ch);
            milp.implies(ch, blockMG[b]);
        }
    }

    // Now the cost function: maximize the execution frequency
    milp.setMaximize();

    // If no frequencies specified, assume cost=1 for all blocks
    if (freq.empty()) {
        ForAllBlocks(b) {
            int v = blockMG[b];
            //if (v >= 0) cout << "Adding block: " << getBlockName(b) << endl;
            if (v >= 0) milp.newCostTerm(1.0, v);
        }
    } else {
        for (auto it: freq) {
            int v = blockMG[it.first];
            if (v >= 0) milp.newCostTerm(it.second, v);
        }
    }

    milp.solve();

    // cout << "MILP: " << milp.getStatus() << endl;
    // cout << "obj = " << milp.getObj() << endl;

    /*
        ForAllBlocks(b) {
            int v = blocks[b].inMG;
            if (v >= 0) {
                cout << "  " << getBlockName(b) << ": " << milp[v] << endl;
            }
        }
    */

    subNetlist MG;
    ForAllChannels(c) {
        int v = channelMG[c];
        if (v >= 0 and milp.isTrue(v)) MG.insertChannel(*this, c);
    }

    set<bbID> tmp;
    set<pair<bbID, bbID> > connections;
    cout << "Channels in the MG:" << endl;
    for (channelID c: MG.getChannels()) {
//        cout << "  ";
//        writeChannelDot(cout, c);

        bbID src = getBasicBlock(getSrcBlock(c));
        bbID dst = getBasicBlock(getDstBlock(c));
        tmp.insert(src);
        tmp.insert(dst);
        connections.insert(make_pair(src, dst));
        //cout << src << "->" << dst << ":" << isBackEdge(c) << endl;
    }
/*

    cout << "SHAB TEST" << endl;
    for (auto it = tmp.begin(); it != tmp.end();  ++it)
        cout << "adding basic block " << *it << endl;
    for (auto const &con: connections) {
        cout << "adding basic block arc " << con.first << "->" << con.second << endl;
    }
    cout << "----------------" << endl;

*/

    return MG;
}