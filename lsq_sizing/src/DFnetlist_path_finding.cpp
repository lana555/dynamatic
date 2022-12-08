// Copyright 2022 ETH Zurich
//
// Author: Jiantao Liu <jianliu@student.ethz.ch>
// Date: 24/05/2022 (Last update)
// Revision: 18/10/2022
// Description:
// This file is used for finding path info in the important
// cycles of the CDFG

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <list>
#include <stack>
#include <fstream>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

/**
 * @brief main function for the lsq_sizing flow.
 * @param coverage Desired coverage for MG calculation.
 * @param dot_name Name of the dot file after buffers flow.
 * @param out_netlist Object of the output dot file that has to be changed for params of the load and store queue.
 * @param selected_case User specified case for calculation
 * @return True if the flow successfully ended, and false if not.
 * @note Start point of the lsq_sizing flow
 */
bool DFnetlist_Impl::buildAdjMatrix(double coverage, std::string& dot_name, DFnetlist& out_netlist, int selected_case){
    // Variable Definition for the whole flow
    End_points.clear();
    // Adjacent_nodes_store.clear();
    End_points_Load_queue.clear();
    End_points_Store_queue.clear();
    Start_points.clear();
    MP.clear();
    TL_traces.clear();
    T_BB.clear();
    vector<int> latency_vector; // Store the latency of the found path
    loopback_edge.clear();
    set<blockID> path_set;
    int latency = 0;

    // Stage -1: Check whether the LSQ flow is needed
    cout << endl;
    cout << "****************************************************" << endl;
    cout << "Check the need of LSQ depth calculation flow" << endl;
    cout << "****************************************************" << endl;
    if (needLSQFLow()) {
        cout << "The flow of LSQ Depth calculation needed" << endl;
    } else {
        cout << "No LSQ node detected in the specified DOT file" << endl;
        cout << "END FLOW" << endl;
        return false;
    }

    // Stage 0: Mark the start of the LSQ depth flow
    cout << endl;
    cout << "****************************************************" << endl;
    cout << "Start the flow of finding the optimal LSQ size" << endl;
    cout << "****************************************************" << endl;
    cout << endl;

    // Stage 1: Extract MGs after the buffer adding process
    cout << "****************************************************" << endl;
    cout << "STAGE 1: Extract MGs after the buffer adding process" << endl;
    cout << "****************************************************" << endl;
    cout << endl;
    extractMGsBB_post_Buffer(coverage);

    // Testing
    cout << "[MG NUMBER]Number of MGs extracted :" << endl;
    cout << "   "<< MG_post_buffer.size() << endl;

    // Stage 4: Extract nodes that are connected to the LSQ, add them to End_points
    cout << "****************************************************" << endl;
    cout << "STAGE 4: Extract nodes that are connected to the LSQ" << endl;
    cout << "****************************************************" << endl;
    cout << endl;
    for (auto sel_MG: MG_post_buffer) {
        // cout << "[Extracting nodes that are connected to the LSQ]" << endl;
        extractLSQnodes(sel_MG);
    }

    // Stage 5: Calculate II for each MG
    // Extract nodes connected to the store block
    // Two flows needed for calculating the II, best case II and worst case II
    cout << "****************************************************" << endl;
    cout << "STAGE 5: Calculate II for each MG" << endl;
    cout << "****************************************************" << endl;
    cout << endl;

    // Variable definition
    std::vector<double> throughput_value;
    std::vector<std::string> throughput_var_value;
    int MG_counter = 0;
    std::string MG_throughput;
    std::vector<int> II_value;
    std::string throughput_file;
    ifstream in_file;

    // Read the throughput from the log file
    if (selected_case == 1) {
        throughput_file = dot_name + "_worst_II.log";
    } else if (selected_case == 0) {
        throughput_file = dot_name + "_best_II.log";
    } else {
        throughput_file = dot_name + "_variable_II.log";
    }

    
    cout << "Reading the throughput from " << throughput_file << endl;
    cout << endl;

    in_file.open(throughput_file);
    // Check whether the file is opened or not
    if (!in_file.is_open()) {
        cout << "[Error] The specified file: " << throughput_file << " can't be opened!" << endl;
        return false;
    }

    if (selected_case == 2) {
        // Read the varible throughput file
        while (getline(in_file, MG_throughput)) {
            cout << "   The input sequence for MG " << MG_counter << "is: " << MG_throughput << endl;
            throughput_var_value.push_back(MG_throughput);
	    MG_counter++;
        }
    } else {
        while (getline(in_file, MG_throughput)) {
            cout << "   The Throughput for MG " << MG_counter << " is " << MG_throughput << endl;
            throughput_value.push_back(stod(MG_throughput));
	    MG_counter++;
        }
    }

    // Close the file
    in_file.close();

    // Testing
    // cout << "The size of MA is " << MA.size() << endl;

    // Calculate the best II, do nothing to the graph
    // For now throughput is stored in II_traces and only the first part(best_case) of the pair can be used
    // TODO: Add processes for other throughput values, for now we only consider the best case
    for (int i = 0; i < MG_post_buffer.size(); i++) {
        // Clean the storing structure
        II_value.clear();

        // Calculate II for the corresponding MG
        if (selected_case == 2) {
            // Variable throughput
            if (throughput_var_value[i] != "0") {
                // The throughput shall not be 0
                cout << "II_Base is " << get_II_base(throughput_var_value[i]);
                II_value.push_back(get_II_base(throughput_var_value[i]));
            } else {
                // ERROR!
                cout << "[ERROR] The throughput value shall not be 0!!" << endl;
                return false;
            }
        } else {
            // Static throughput
            if (throughput_value[i] != 0) {
                // The throughput shall not be 0
                II_value.push_back(ceil(1 / throughput_value[i]));
            } else {
                // ERROR!
                cout << "[ERROR] The throughput value shall not be 0!!" << endl;
                return false;
            }
        }

        // Add obtained II_value vector to the global storing structure
        II_traces.push_back(II_value);

        // Testing
        cout << endl;
        cout << "II value for MG " << i << " is " << II_traces[i][0] << endl;
        cout << endl;

    }

    // Stage 6: Cut down loopback edges for all MGs
    cout << "****************************************************" << endl;
    cout << "STAGE 6: Change the latency of the branch nodes " << endl;
    cout << "****************************************************" << endl;
    cout << endl;

    for (int i = 0; i < MG_post_buffer.size(); i++) {
        cout << "Change the latency for branch nodes in MG " << i << endl;
        loopbackChange(MG_post_buffer[i], i);
    }

    // Stage 7: Build Adjacency list for all MGs
    cout << "****************************************************" << endl;
    cout << "STAGE 7: Build Adjacency lists for all MGs" << endl;
    cout << "****************************************************" << endl;
    cout << endl;
    getAdjRecordfromMGs(MG_post_buffer);

    // Stage 8: Extract all simple paths in all MGs
    // From the start_point of the MG to all End_points
    cout << "****************************************************" << endl;
    cout << "STAGE 8: Extract all simple paths in all MGs" << endl;
    cout << "****************************************************" << endl;
    cout << endl;

    // Testing
    cout << "Contents in Start_Points:" << endl;
    for (auto com_start: Start_points) {
        cout << "   " << getBlockName(com_start) << endl;
    }

    for (int i = 0; i < MA.size(); i++) {
        // Testing
        cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
        cout << "Searching all simple paths in MG " << i << endl;
        cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
        for (auto end_point: End_points) {
            if (MG_post_buffer[i].hasBlock(end_point)) {
                // Testing
                cout << "[Finding paths] Searching path for node " << getBlockName(end_point) << endl;

                MP.push_back(findPathsinMG(MA[i], Start_points[i], end_point, 0, i));
            } else {
                // Key is not found
                cout << "[WARNING] Block " << getBlockName(end_point) << " is not found in the MG_post_buffer!!!" << endl;
            }
        }
    }

    // Stage 9: Calculate TL for different paths in different MGs
    cout << "****************************************************" << endl;
    cout << "STAGE 9: Calculate TL for all paths in all MGs" << endl;
    cout << "****************************************************" << endl;
    cout << endl;
    // Variable Definition
    int flag_path = 0; // Used to show the validity of the path
    std::vector<int> path_index;
    int path_latency = 0;
    timeInfo temp_time_info; // Used to store temp_info for one MG

    // Testing
    cout << endl;
    cout << "Contents in the End_Points" << endl;
    for (auto con: End_points) {
        cout << "   " << getBlockName(con) << endl;
    }

    for (int i = 0; i < MA.size(); i++) {
        // Clear the temp_time_info
        temp_time_info.clear();
        path_latency = 0;
        for (auto end_point: End_points) {
            path_index.clear();
            flag_path = 0;
            if (MG_post_buffer[i].hasBlock(end_point)) {
                // Set path_index
                path_index.push_back(Start_points[i]);
                path_index.push_back(end_point);

                // Find the corresponding path
                for (auto mp: MP) {
                    // Check whether the path exists or not
                    if (mp.hasKey(path_index)) {
                        path_latency = pathLatency(mp.getaPath(path_index), i);
                        temp_time_info.inserttrace(path_index, path_latency);
                        flag_path = 1;

                        // Testing
                        cout << "[MG " << i << "] " << "Found path from " << getBlockName(Start_points[i]) << " to "
                             << getBlockName(end_point);
                        cout <<  " with latency: " << path_latency << " cycles;" << endl;
//                        cout << "[STORED][MG " << i << "] " << "Found path from " << getBlockName(Start_points[i]) << " to "
//                             << getBlockName(end_point);
//                        cout <<  " with latency: " << path_latency << " cycles;" << endl;
                    }
                }


                // If the path doesn't exist in the extracted path
                // [ERROR] this situation shouldn't happen
                if (flag_path == 0) {
                    cout << "[ERROR] Path between block " << getBlockName(Start_points[i]) << " and block "
                         << getBlockName(end_point) << " doesn't exist!" << endl;
                    return 0;
                }
            }
        }

        // Add temp_time_info to TL
        TL_traces.push_back(temp_time_info);
    }

    // Formatting
    cout << endl;

    // Stage 10: Calculate Titer for different MG
    cout << "****************************************************" << endl;
    cout << "STAGE 10: Calculate Titer for different MG" << endl;
    cout << "****************************************************" << endl;
    cout << endl;

    // Variable definition
    int max_latency = 0; // Used to store max latency in one Graph
    int temp_time = 0;

    for (int i = 0; i < MG_post_buffer.size(); i++) {
        // Find the end of the corresponding MG
        max_latency = 0;
        path_index.clear();
        for (auto end_point: End_points) {
            // Clean those Variables
            temp_time = 0;
            path_index.clear();
            // Check whether the Graph has the key
            if (MG_post_buffer[i].hasBlock(end_point)) {
                // Build index
                path_index.push_back(Start_points[i]);
                path_index.push_back(end_point);
                temp_time = TL_traces[i].getTime(path_index);

                if (temp_time > max_latency) {
                    max_latency = temp_time;
                }
            }
        }

        // Testing
        // cout << "Titer for MG " << i << " is " << max_latency << " cycles;" << endl;
        cout << "[Real value]Titer stored for MG " << max_latency << " cycles;" << endl;

        // Store Titer for different MGs
        if (max_latency < 0) {
            cout << "[ERROR] Titer is less than 0" << endl;
        }
        T_iter.push_back(max_latency);
    }

    // Stage 11: Extract TBB for different MGs
    cout << "*******************************************************" << endl;
    cout << "STAGE 11: Extract TBB for different MGs" << endl;
    cout << "*******************************************************" << endl;
    cout << endl;

    // Variable definition
    mgPathInfo temp_path;
    BB_info temp_bb_time;
    blockID bb_start_point;
    path_latency = 0;

    // Check for different start points
    for (int i = 0; i < MG_post_buffer.size(); i++) {
        // Testing
        cout << "================== Extracting TBB for MG " << i << " ==================" << endl;

        temp_bb_time.clear();
        temp_path.clear();

        // Check for different BBs
        for (auto BB_index: MG_post_buffer[i].getBasicBlocks()) {
            // Variable definition
            path_index.clear();
            // Get the destination of the path
            bb_start_point = BB_StartPoints[i].get_an_element(BB_index);

            // Testing
            // cout << endl;
            // cout << "================== Testing ==================" << endl;
            // cout << "In MG " << i << " ,BB " << BB_index << " with BB_start_point: " << getBlockName(bb_start_point) << endl;
            // cout << "=============================================" << endl;

            // Set the index
            path_index.push_back(Start_points[i]);
            path_index.push_back(bb_start_point);
            temp_path = findPathsinMG(MA[i], Start_points[i], bb_start_point, 1, i);
            path_latency = pathLatency(temp_path.getaPath(path_index), i);

            // Testing
            cout << endl;
            cout << "TBB for BasicBlock " << BB_index << " in MG " << i;
            cout << " is " << path_latency << endl;
            cout << "*******************************************************" << endl;
            cout << endl;

            // Store the latency
            temp_bb_time.insert_an_element(BB_index, path_latency);
        }

        // Store the extracted TBB in one MG into the global variable
        T_BB.push_back(temp_bb_time);

    }

    // Stage 13(Final Stage!) : Calculate Depths for different MGs
    cout << "*******************************************************" << endl;
    cout << "STAGE 13: Calculate LSQ depths for different MGs" << endl;
    cout << "*******************************************************" << endl;
    cout << endl;

    // Variable definition
    std::vector<int> Optimal_Load_Depth;
    std::vector<int> load_depth_list;
    std::vector<int> store_depth_list;
    int opt_load_depth = 0;
    int opt_store_depth = 0;

    // Check the value
    for (int i = 0; i < MG_post_buffer.size(); i++) {
        load_depth_list.push_back(optLoadDepCal(i, selected_case, throughput_var_value));
        store_depth_list.push_back(optStoreDepCal(i, selected_case, throughput_var_value));
    }

    // Choose the maximum value as the optimal load and store queue depth
    // Load list
    for (auto sel_iter: load_depth_list) {
        if (sel_iter > opt_load_depth) {
            opt_load_depth = sel_iter;
        }
    }

    // Store list
    for (auto sel_iter: store_depth_list) {
        if (sel_iter > opt_store_depth) {
            opt_store_depth = sel_iter;
        }
    }

    // Testing
    cout << endl;
    cout << "OPTIMAL LOAD QUEUE DEPTH: " << opt_load_depth << endl;
    cout << "OPTIMAL STORE QUEUE DEPTH: " << opt_store_depth << endl;

    // Mark the end of the LSQ depth calculation flow
    cout << endl;
    cout << "***********************************************" << endl;
    cout << "End process for finding the optimal LSQ size" << endl;
    cout << "***********************************************" << endl;
    cout << endl;

    // Modify the Dot file for optimal Load and Store Queue size
    // For now, just read the dot file again and modify the dot accordingly for the new json file
    cout << endl;
    cout << "**********************" << endl;
    cout << "Modify the DOT file" << endl;
    cout << "**********************" << endl;
    cout << endl;

    string suffix = "_optimized.dot"; //"_graph_buf.dot"
    cout << "Reading " << dot_name + suffix << endl;

    // For now, only one LSQ block is considered, the Dot file can only have 1 LSQ block!
    // TODO: extend the flow to support multiple LSQ blocks inside one Dot file
    // One thing is that the index of LSQ node in the original dot and the newly read dot file is not the same!!!
    ForAllBlocks(b) {
        // If this is a LSQ block
        if (getBlockType(b) == LSQ) {
            // This must be a valid block
            assert(validBlock(b));

            // Change the depth of the LSQ
            cout << "Change the params of block " << getBlockName(b) << endl;
            out_netlist.changeOutDot(getBlockName(b), opt_load_depth, opt_store_depth);
        }
    }

    // Write back the new dot file
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << endl;
    cout << "Generate the new dot file "+ dot_name + "_optimized_lsq.dot" << endl;
    out_netlist.writeDot(dot_name + "_optimized_lsq.dot");
    out_netlist.writeDotBB(dot_name + "_bbgraph_lsq.dot");

    return 1;
}

/**
 * @brief Check whether the lsq_sizing is needed for the given code.
 * @return True if the given code need the lsq_sizing flow, and false if not.
 */
bool DFnetlist_Impl::needLSQFLow(){
    // Variable Definition
    int flag = 0;
    ForAllBlocks(b) {
        // Check whether this is an LSQ block
        if (getBlockType(b) == LSQ) {
            // This must be a valid block
            assert(validBlock(b));
            flag = 1;
            break;
        }
    }

    // LSQ node detected
    if (flag == 1) {
        return true;
    } else {
        return false;
    }
}


/**
 * @brief Extract a set of marked graphs consisting of Basic Blocks to achieve a certain execution frequency coverage.
 * The execution is stopped when the coverage is achieved or no more marked graphs are found.
 * @param coverage The target coverage.
 * @date 28/04/2022
 * @note This function is a modified version of the `extractMarkedGraphsBB` function which is used in the buffers flow.
 */
void DFnetlist_Impl::extractMGsBB_post_Buffer(double coverage) {
    // Variable Definition
    subNetlist included_BB; // Used to store info for BBs in the MG
    blockID potential_start_point;
    blockID last_branchC;
    int indicator = 0;

    double total_freq = 0;
    for (bbArcID i = 0; i < BBG.numArcs(); i++){
        total_freq += BBG.getFrequencyArc(i);
    }

    map<bbArcID, double>freq;
    if (total_freq > 0){
        for (bbArcID i = 0; i < BBG.numArcs(); i++){
            freq[i] = BBG.getFrequencyArc(i);
        }
    } else {
        for (bbArcID i = 0; i < BBG.numArcs(); i++){
            freq[i] = 1.0;
        }
        total_freq = BBG.numArcs();
    }

    if (coverage <= 0)  coverage = 1.0 / total_freq;

    // Variables used to store the extracted MG
    CFDFC.clear();
    CFDFCfreq.clear();
    MG_post_buffer.clear();
    MGfreq.clear();
    AG.clear();
    BB_StartPoints.clear(); // Usd to store start points of different BBs

    BBG.calculateBackArcs();

    int iter = 1;
    int MG_count = 0;
    BB_info temp_start_points;
    double covered_freq = 0;
    while (covered_freq < coverage * total_freq) {
        cout << "--------------------------------------------------------------" << endl;
        cout << "Post Buffers_Adding Iteration " << iter << endl;

        subNetlistBB extracted_CFDFC = extractMarkedGraphBB(freq);

        if (extracted_CFDFC.empty()){
            cout << "No new MG can be extracted to increase coverage." << endl;
            break;
        }

        // Stage 10: Store info about different BB start points
        temp_start_points.clear();

        // Stage 2: Add connections for different BBs
        cout << "****************************************************" << endl;
        cout << "STAGE 2: Add connections for different BBs" << endl;
        cout << "****************************************************" << endl;
        cout << endl;

        cout << "===================================================" << endl;
        cout << "Manually adding connections for phiC and Phi Nodes" << endl;
        cout << "===================================================" << endl;
        indicator = 0; // Mark the start of each Marked Graph, add it to Start_Points if it's 0
        for (auto sel_BB: extracted_CFDFC.getBasicBlocks()){
            // Testing
            cout << endl;
            cout << "Checking BB " << sel_BB << endl;
            cout << endl;

            if (AG.count(sel_BB) == 0){
                AG.insert(sel_BB);
                cout << "Adding connections for Basic Block " << sel_BB << endl;
                included_BB = blocksExtractionsingleBB(sel_BB);
                potential_start_point = addConnectionToPhiBB(included_BB, sel_BB);

                // Store info about different BB start points
                temp_start_points.insert_an_element(sel_BB, potential_start_point);
            } else {
                // We have processed this block just add it to the BB_start_point
                for (auto sel_BB_MG: BB_StartPoints) {
                    if (sel_BB_MG.hasBB(sel_BB)) {
                        // The selected structure has the corresponding BB
                        potential_start_point = sel_BB_MG.get_an_element(sel_BB);
                        break;
                    }
                }

                // Store info about different BB start points
                temp_start_points.insert_an_element(sel_BB, potential_start_point);

            }

            // If this is the first block in the MG, add it to the start point
            if (indicator == 0) {
                Start_points.push_back(potential_start_point);
            }

            indicator++;
        }

        // Add the start_points info to the global variable
        MG_count++;
        BB_StartPoints.push_back(temp_start_points);

        // Print contained basic blocks
        cout << "Contained BBs:" << endl;
        for (auto bbs: extracted_CFDFC.getBasicBlocks()) {
            cout << bbs << " " ;
        }
        cout << endl;

        cout << "Updating frequencies..." << endl;
        // calculate the minimum freq of this CFDFC
        double min_freq = total_freq;
        for (auto arc: extracted_CFDFC.getBasicBlockArcs()) {
            min_freq = min(min_freq, freq[arc]);
        }
        // update the frequencies of this CFDFC
        for (auto arc: extracted_CFDFC.getBasicBlockArcs())
            freq[arc] -= min_freq;

        for (auto arc: extracted_CFDFC.getBasicBlockArcs())
            BBG.addMGnumber(arc, iter);

        // store CFDFC
        // Stage 3: Store info for different MGs
        cout << "****************************************************" << endl;
        cout << "STAGE 3: Store info about different BBs in different MGs" << endl;
        cout << "****************************************************" << endl;
        cout << endl;

        cout << "Storing Post BUffers CFDFC and corresponding Marked Graph..." << endl;
        CFDFC.push_back(extracted_CFDFC);
        CFDFCfreq.push_back(min_freq);
        MG_post_buffer.push_back(getMGfromCFDFC_post_Buffer(extracted_CFDFC));
        MGfreq.push_back(min_freq);

        covered_freq += extracted_CFDFC.numBasicBlockArcs() * min_freq;
        iter++;
    }

    cout << endl;
    cout << "=======================================" << endl;
    cout << "Post Buffers CFDFG Extraction Finished!" << endl;
    cout << "=======================================" << endl;
    cout << endl;

}

/**
 * @brief A modified verison of the `getMGfromCFDFC` function used in the buffers flow, with different input types and added part
 * for MG indexing.
 * @date 17/05/2022
 * @param BB_CFDFC A subNetlistBB of BBs and Arcs
 * @return A subNetlistPostBuffer of blocks and channels between them
 */
DFnetlist_Impl::subNetlistPostBuffer DFnetlist_Impl::getMGfromCFDFC_post_Buffer(Dataflow::DFnetlist_Impl::subNetlistBB &BB_CFDFC) {
    subNetlistPostBuffer snl;

    ForAllChannels(i) {
        bbID src = getBasicBlock(getSrcBlock(i));
        bbID dst = getBasicBlock(getDstBlock(i));

        // check if the BBs are present in the extracted CFDFC
        if (!BB_CFDFC.hasBasicBlock(src)) continue;
        if (!BB_CFDFC.hasBasicBlock(dst)) continue;

        // Testing
//        cout << "Checking channel starts from " << getBlockName(getSrcBlock(i)) << " to " << getBlockName(getDstBlock(i)) << endl;

        // two cases: 1. outer channel (between two separate BBs or back arc) 2. inner channel (within BB)
        if (!isInnerChannel_postBuffer(i)) {
            bbArcID connection = BBG.findArc(src, dst);
            // Testing
            //cout << "   This is not an inner channel:" << endl;
            assert(connection != invalidDataflowID);
            // Testing
            //cout << "   Vaild connection" << endl;
            if (BB_CFDFC.hasBasicBlockArc(connection)) {
                snl.insertChannel(*this, i);
                channels_in_MGs.insert(i);
                blocks_in_MGs.insert(getSrcBlock(i));
                blocks_in_MGs.insert(getDstBlock(i));
            }
        } else if (src == dst) {
            // Testing
            //cout << "   Inner Channel" << endl;
            snl.insertChannel(*this, i);
            channels_in_MGs.insert(i);
            blocks_in_MGs.insert(getSrcBlock(i));
            blocks_in_MGs.insert(getDstBlock(i));
        } else if (isBuffertoPhiChannel(i, BB_CFDFC.getBasicBlocks())){
            // Testing
//            cout << "   Buufer to phi channel found" << endl;
            snl.insertChannel(*this, i);
            channels_in_MGs.insert(i);
            blocks_in_MGs.insert(getSrcBlock(i));
            blocks_in_MGs.insert(getDstBlock(i));
        }
    }

    // Stage 12: Extract #Loads and #Stores for different BBs in different MGs
    cout << "*******************************************************" << endl;
    cout << "    Extract #Loads and #Stores for different BBs" << endl;
    cout << "*******************************************************" << endl;
    cout << endl;

    pair<int, int> extracted_mem_ops;
    // Count number of loads and stores in different BBs
    for (auto BB_index: snl.getBasicBlocks()) {
        extracted_mem_ops = countNumMemOps(BB_index, snl);

        // Testing
        cout << "Number of Loads in BB " << BB_index << " is " << extracted_mem_ops.first << endl;
        cout << "Number of Stores in BB " << BB_index << " is " << extracted_mem_ops.second << endl;


        snl.insertNumLoad(BB_index, extracted_mem_ops.first);
        snl.insertNumStore(BB_index, extracted_mem_ops.second);


    }

    return snl;
}

/**
 * @brief Count the number of LSQ_loads and LSQ_stores in the specified BB
 * @param BB_index Identifier of the BB
 * @param sel_MG Object of one MG
 * @return a pair of ints, representing the number of loads and stores in the specified BB
 */
std::pair<int, int> DFnetlist_Impl::countNumMemOps(bbID BB_index, DFnetlist_Impl::subNetlistPostBuffer &sel_MG) {
    // Variable Definition
    pair<int, int> num_mem_ops;
    int num_loads = 0;
    int num_stores = 0;

    // Testing
    cout << "Num of blocks in the selected MG is " << sel_MG.numBlocks() << endl;

    // Count num of mem operations
    for (auto sel_block: sel_MG.getBlocks()) {
        // Check whether the block is inside the specified BB or not

        if (getBasicBlock(sel_block) == BB_index) {
            // Check whether this is a LSQ node
            if (getBlockType(sel_block) == OPERATOR) {
                // Count corresponding load and store
                if (getOperation(sel_block) == "lsq_load_op") {
                    num_loads++;
                } else if (getOperation(sel_block) == "lsq_store_op") {
                    num_stores++;
                }
            }
        }
    }

    // store the results
    num_mem_ops.first = num_loads;
    num_mem_ops.second = num_stores;

    // Testing
    cout << "[NUM_LOADS] BB " << BB_index << " has " << num_loads << " Load operation(s)" << endl;
    cout << "[NUM_STORES] BB " << BB_index << " has " << num_stores << " Store operation(s)" << endl;

    return num_mem_ops;
}


/**
 * @brief This function will extract all blocks in a specified BB
 * @param BB_index Identifier of the desired BB
 * @return A subnetlist of the specified BB
 */
DFnetlist_Impl::subNetlist DFnetlist_Impl::blocksExtractionsingleBB(bbID BB_index) {
    subNetlist snl;

    ForAllChannels(i) {
        bbID src = getBasicBlock(getSrcBlock(i));
        bbID dst = getBasicBlock(getDstBlock(i));

        // Check whether the channel is in the same BB
        if (!(src == BB_index)) continue;
        if (!(dst == BB_index)) continue;

        // Insert channel to the subNetlist
        bbArcID connection = BBG.findArc(src, dst);

        // Testing
        // TODO: Check why all those connections are set to invalid
        // if (connection == invalidDataflowID) {
        //     cout << "[ERROR] the connection bbArcID shall not be invalid" << endl;
        //     cout << "SRC node: " << getBlockName(getSrcBlock(i)) << endl;
        //     cout << "DST node: " << getBlockName(getDstBlock(i)) << endl;
        // }

        // assert(connection != invalidDataflowID);
        snl.insertChannel(*this, i);
    }

    // Testing: print some test info
    // cout << "============================================" << endl;
    // cout << "Finding channels in one BB" << endl;
    // cout << "============================================" << endl;
    // cout << "Channels in BB " << BB_index << " :" << endl;
    // for (channelID c: snl.getChannels()) {
    //     cout << "  ";
    //     writeChannelDot(cout, c);
    // }

    return snl;

}

/**
 * @brief Check the existence of the connection between two blocks
 * @param src_block Identifier of the first block
 * @param dst_block Identifier of the second block
 * @return True if the connection exist, and false if not.
 */
bool DFnetlist_Impl::checkConnectionSingleBB(blockID src_block, blockID dst_block) {
    ForAllChannels(i) {
        blockID src_index = getSrcBlock(i);
        blockID dst_index = getDstBlock(i);

        if ((src_block == src_index) && (dst_block == dst_index)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Add links from the phiC node in the specified BB to all phi nodes within the same BB,
 * if the connections doesn't exist out1 port of the phiC node will always be connected to the in1 port of all other phi node.
 * @note This change won't be write back to the new dot file, thus, it won't influence the functionality of the generated dataflow circuit
 * @param sel_MG A subNetlist of the selected MG
 * @param BB_index Identifier of the desired BB
 * @return blockID of the phiC node in the BB
 */
blockID DFnetlist_Impl::addConnectionToPhiBB(DFnetlist_Impl::subNetlist &sel_MG, bbID BB_index) {
    // Variable Definition
    subNetlist specified_BB = blocksExtractionsingleBB(BB_index);
    blockID startblock; // blockID of phiC node
    vector<blockID> phi_nodes; // all phi nodes inside the BB(including phiC node)
    int counter = 0;

    // Enumerate all blocks in the specified BB
    for (blockID b: specified_BB.getBlocks()) {
        // Check the type of the node
        if ((getBlockType(b) == MERGE) || (getBlockType(b) == MUX) || (getBlockType(b) == CNTRL_MG)) {
            // phi node
            phi_nodes.push_back(b);
        }
    }

    cout << endl;
    cout << phi_nodes.size() << " Phi nodes are found in BB " << BB_index << endl;

    // Find PhiC node in the extracted vector of phi nodes
    for (blockID p_node: phi_nodes) {
        string p_node_name = getBlockName(p_node);
        // Check whether the node is a phiC node
        if (p_node_name[3] == 'C') {
            cout << endl;
            cout << "Found one PhiC node with name: " << p_node_name << endl;
            startblock = p_node;
            phi_nodes.erase(phi_nodes.begin() + counter);
            counter = 0;
            break;
        }
        counter++;
    }

    // Connect phiC node to all other phi nodes,
    // if the connection doesn't exist
    counter = 0;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    for (blockID p_node: phi_nodes) {
        // Check the connection
        int flag = checkConnectionSingleBB(startblock, p_node);

        // If the connection already exists, then do nothing
        if (flag) {
            cout << endl;
            cout << "In Basic Block " << BB_index << " : " << endl;
            cout << "Connection from " << getBlockName(startblock) << " to " << getBlockName(p_node) << " already exists!" << endl;
            continue;
        } else {
            // If the connection doesn't exist, manually add the connection
            // Create new output port for the phiC node
            string phiC_out_port_name = "phi_out" + to_string(counter);
            string phi_in_port_name = "phiC_in0";
            counter++;
            // Add those two new ports
            portID phiC_out_port = createPort(startblock, false, phiC_out_port_name, 32);
            portID phi_in_port = createPort(p_node, true, phi_in_port_name, 32);

            // Connect the created port together
            createChannel(phiC_out_port, phi_in_port);

            // Testing
            cout << "Add connections from " << getBlockName(startblock) << " to " << getBlockName(p_node) << endl;

            cout << endl;
        }

        // Clear flag
        flag = 0;

    }

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

    return startblock;
}

/**
 * @brief Extract all blocks that are connected to the LSQ
 * @param sel_MG A subNetlistPostBuffer that describes a MG
 * @note All found nodes will be pushed to the End_points variable
 */
void DFnetlist_Impl::extractLSQnodes(DFnetlist_Impl::subNetlistPostBuffer &sel_MG) {
    // Variable definitions
    vector<blockID> extracted_blocks;
    cout << "==============================================" << endl;
    cout << "Extracting nodes that are connected to the LSQ" <<  endl;
    cout << "==============================================" << endl;

    // Check the corresponding connection
    ForAllChannels(i) {
        blockID src_index = getSrcBlock(i);
        blockID dst_index = getDstBlock(i);

        // Check whether one of them is from LSQ
        if (getBlockType(src_index) == LSQ) {
            if (find(End_points.begin(), End_points.end(), dst_index) == End_points.end()){
                if (sel_MG.hasBlock(dst_index)){
                    if (getBlockType(dst_index) == OPERATOR) {
                        End_points.push_back(dst_index);
                        // Check whether this is a store or load node
                        if (getOperation(dst_index) == "lsq_load_op") {
                            End_points_Load_queue.push_back(dst_index);
                        } else {
                            End_points_Store_queue.push_back(dst_index);
                        }
                        cout << getBlockName(dst_index) << " is connected to the LSQ" <<  endl;
                    }
                }
            } else {
                // cout << getBlockName(dst_index) << " is already in the Stored connected nodes" <<  endl;
            }
        } else if (getBlockType(dst_index) == LSQ) {
            if (find(End_points.begin(), End_points.end(), src_index) == End_points.end()){
                if (sel_MG.hasBlock(src_index)){
                    if (getBlockType(src_index) == OPERATOR) {
                        End_points.push_back(src_index);
                        // Check whether this is a store or load node
                        if (getOperation(src_index) == "lsq_load_op") {
                            End_points_Load_queue.push_back(src_index);
                        } else {
                            End_points_Store_queue.push_back(src_index);
                        }
                        cout << getBlockName(src_index) << " is connected to the LSQ" <<  endl;
                    }
                }
            } else {
                // cout << getBlockName(src_index) << " is already in the Stored connected nodes" <<  endl;
            }
        }
    }

    cout << "------------------------------------------------" << endl;
}

/**
 * @brief Record the channelID of all those loopback edges in the given MG
 * @param sel_MG A subNetlistPostBuffer that describes details of a MG
 * @param MG_index Identifier of the corresponding MG
 * @return True if the function ended successfully, false if not
 * @note The MG_index is used to help storing all those found channel IDs.
 */
bool DFnetlist_Impl::loopbackChange(DFnetlist_Impl::subNetlistPostBuffer &sel_MG, int MG_index) {
    // Variable definition
    set<bbID> BB_set = sel_MG.getBasicBlocks();
    bbID start_BB_index = 0;
    std::vector<blockID> MUX_nodes;
    channelID loop_back_edge = 0;

    // Extract the beginning BB in the given Marked Graph
    set<bbID>::iterator it = BB_set.begin();
    start_BB_index = *it;

    // Testing_1
    cout << "Start Basic Block in the given Marked Graph is " << start_BB_index << endl;


    // Check for the MUX node in the Start Block
    for (auto sel_blo: sel_MG.getBlocks()) {
        if (getBasicBlock(sel_blo) == start_BB_index) {
            BlockType dst_type = getBlockType(sel_blo);
            if ((dst_type == MUX || dst_type == MERGE || dst_type == CNTRL_MG)){
                MUX_nodes.push_back(sel_blo);
            }
        }
    }

    // Testing_2
    cout << "Phi nodes in the extracted Start Basic Block are: " << endl;
    for (auto sel_blo: MUX_nodes) {
        cout << "   " << getBlockName(sel_blo) << endl;
    }
    cout << endl;

    // Mark all loopback edges
    for (auto Phi_node: MUX_nodes) {
        // Testing
        cout << "Search for loopback edges that are connected to " << getBlockName(Phi_node) << endl;
        // Checking all those channels
        for (auto sel_channel: sel_MG.getChannels()) {
            // The Phi_node shall be the dst node for the channel
            if (getDstBlock(sel_channel) == Phi_node) {
                if (getBlockType(getSrcBlock(sel_channel)) == BRANCH) {
                    // Check whether the src node is in the
                    cout << "The selected node is " << getBlockName(getSrcBlock(sel_channel)) << endl;
                    if (BB_set.count(getBasicBlock(getSrcBlock(sel_channel))) > 0) {
                        // The src node is in the selected Marked Graph, branch nodes
                        loop_back_edge = sel_channel;

                        // Check whether this is a loopback edge
                        if (isBackEdge(loop_back_edge)){
                            // Testing
                            cout << endl;
                            cout << "One loopback edge found" << endl;
                            // Change the latency of the selected node
                            //                        setLatency(getSrcBlock(sel_channel), II_traces[MG_index][0]);
                            // cout << endl;
                            cout << "The loopback edge: [Start_Point] " << getBlockName(getSrcBlock(loop_back_edge)) << "; [End_Point] " << getBlockName(getDstBlock(loop_back_edge)) << endl;
                            cout << "Insert the found edge " << loop_back_edge << " to loopback_edge" << endl;
                            loopback_edge.insert(loop_back_edge);
                            // del_edge_MGs(sel_channel);
                            // Remove the loopback edge
                            // removeChannel(loop_back_edge);
                            break;
                        }
                    }
                }
            }
        }
    }


    return true;
}

/**
 * @brief Extracting graph adjacency lists from the given MGs
 * @param Marked_Graphs A vector of subNetlistPostBuffers which describe all extraced MGs in the given code
 * @return True if the adjacency list building process ended successfully, false if not
 */
bool DFnetlist_Impl::getAdjRecordfromMGs(std::vector<DFnetlist_Impl::subNetlistPostBuffer> &Marked_Graphs) {
    // Clean the structure for storing adjacent vertex info.
    MA.clear();

    // Build Adj list for different blocks based on the extracted MGs
    for (auto iter : Marked_Graphs) {
        // Create graphAdjrecord structure and clear it
        graphAdjrecord extracted_Adj_Record;
        extracted_Adj_Record.clear();

        // Build adjacency lists
        // We assume that the MG is strongly connected, thus all block will have
        // at least one node in the adjacency list(the adjacency list won't be empty)
        // Two important things:
        // 1. Store blocks are not connecting to any other blocks inside BB, so the adjacency
        //    list for store blocks should always be empty
        for (channelID c: iter.getChannels()) {
            extracted_Adj_Record.insertBlock(getSrcBlock(c), getDstBlock(c));
        }

        // Add the record to MA
        MA.push_back(extracted_Adj_Record);

        // Testing
//        cout << "-------------------------------" << endl;
//        cout << "Adjacency lists of Marked Graph:" << endl;
//        for (auto record_map: extracted_Adj_Record.adj) {
//            cout << "Adj list for block " << getBlockName(record_map.first) << " : " ;
//            for (auto adj_block: record_map.second) {
//                cout << getBlockName(adj_block) << "[blockID]: " << adj_block << "; ";
//            }
//            cout << endl;
//        }
//        cout << "-------------------------------" << endl;

    }

    return true;
}

/**
 * @brief Find all simple paths within one Marked Graph (Use vectors to simulate the functionality of a stack);
 * @note For the implemented algorithm, two stacks are needed for the DFS-Backtracking algorithm.
 * Stack_1 for storing the possible path, Stack_2 keeps track of available nodes.
 * Recursive methods are not used as they may overflow for large graphs
 * @param sel_MG A graphAdjrecord with all adjacency lists within one MG
 * @param start_block Block identifier of the specified start block of the path
 * @param end_block Block identifier of the specified end block of the path
 * @param type_selection 
 *                  0. Used to alculate the path for LSQ (select the shortest for each path_index)
 *                  1. Other cases (Still select the shortest path for the given path_index)
 * @param MG_index Identifier of the specified MG
 * @return A mgPathInfo structure with the info of all found paths in the selected MG
 * @date 28/05/2022
 * @author Jiantao Liu
 */
DFnetlist_Impl::mgPathInfo DFnetlist_Impl::findPathsinMG(DFnetlist_Impl::graphAdjrecord &sel_MG, blockID start_block, blockID end_block, int type_selection, int MG_index) {
    // Formatting output
    cout << endl;
    // cout << "*********************************************************************************" << endl;
    cout << "Searching for a path from " << getBlockName(start_block) << " to " << getBlockName(end_block) << endl;
    cout << endl;

    // Variable Definitions
    // Two stacks are needed for the back-tracking algorithm
    std::vector<int> path_index;
    std::vector<blockID> Path_Stack_S1; // Main stack for tracking the path
    std::vector< std::set<blockID> > Adj_Block_S2; // Supporting Stack for recording potential paths
    mgPathInfo found_paths; // Used to store the found paths in the selected MG
    std::vector< std::vector<int> > All_found_paths; // Used to store all found paths

    // Supporting Variables
    int counter = 0;
    std::set<blockID> adj_list; // Usd to store the adj_list of the selected node

    // Initialize and build the starting point
    All_found_paths.clear();
    Path_Stack_S1.clear();
    Adj_Block_S2.clear();
    path_index.clear();
    path_index.push_back(start_block);
    path_index.push_back(end_block);

    // Initialize stacks
    Path_Stack_S1.push_back(start_block);
    Adj_Block_S2.push_back(sel_MG.getAdjList(start_block));

    // Search for the path
    while (!Path_Stack_S1.empty()) {
        // Variable Definition
        blockID tem_block = 0;
        // Extract the adjacency list of the top element in the main stack, and pop the corresponding element
        adj_list = Adj_Block_S2.back();
        Adj_Block_S2.pop_back();

        // Testing
        // print_main_stack(Path_Stack_S1);
        // print_adj_stack(Adj_Block_S2);

        // If the adj list is not empty
        if (adj_list.size() > 0) {
            // Add the first element of the Adj_list to the main stack
            // and push back the adj_list without the first element
            tem_block = *adj_list.begin();
            adj_list.erase(tem_block);
            // Push the corresponding node to the Adj stack
            Path_Stack_S1.push_back(tem_block);
            Adj_Block_S2.push_back(adj_list);

            // Testing
            // print_main_stack(Path_Stack_S1);
            // print_adj_stack(Adj_Block_S2);

            // Create new adj_list for the top element in the Main stack
            std::set<blockID> temp_adj_list;
            temp_adj_list.clear();
            adj_list = sel_MG.getAdjList(tem_block);
            for (auto elem: adj_list) {
                // Check whether this element is in the main stack
                if (std::find(Path_Stack_S1.begin(), Path_Stack_S1.end(), elem) == Path_Stack_S1.end()) {
                    // The elem is not in the main stack
                    temp_adj_list.insert(elem);

                } else {
                    // The elem is in the main stack
                    // cout << getBlockName(elem) << " is already in the Main Stack!" << endl;
                }
            }

            // Push new adj_list to the Adj stack
            Adj_Block_S2.push_back(temp_adj_list);
        } else {
            // The adj_list of the corresponding top elem in the Main stack is empty
            // Cutdown the stack
            Path_Stack_S1.pop_back();
        }

        // Testing
        // print_main_stack(Path_Stack_S1);
        // print_adj_stack(Adj_Block_S2);

        // Found one Path!
        if (Path_Stack_S1.size() > 0) {
            if (Path_Stack_S1.back() == end_block) {
                // ONE Path found!
                // Save the path
                All_found_paths.push_back(Path_Stack_S1);
                // Testing
//                cout << endl;
//                cout << "[ONE_PATH_FOUND] Number " << counter << " Path found" << endl;
//                for (auto insert_block: Path_Stack_S1) {
//                    cout << "Push " << getBlockName(insert_block) << " to path holder" << endl;
//                     //found_paths.insertBlock(path_index, insert_block);
//                }

                // Increase the counter
                counter++;

                // Testing: print the found path
//                cout << endl;
//                cout << "One direct path from " << getBlockName(start_block) << " to " << getBlockName(end_block) << " is found as below: " << endl;
//                found_paths.print_path(path_index);

                // Cutdown the stack
                Path_Stack_S1.pop_back();
                if (Adj_Block_S2.size() > 0) {
                    Adj_Block_S2.pop_back();
                }
            }
        }
    }

    // Check the validity of the found paths
    if (All_found_paths.size() == 0) {
        // Error
        cout << endl;
        cout << "[ERROR] AT LEAST ONE PATH SHALL BE FOUND FOR THE SPECIFIED START END POINT" << endl;
        return found_paths;
    }

    // Check for the shortest path
    // For load we can directly calculate the shortest path
    // For store we should consider the data dependency(WAR data hazard) between load and store.
    //      if we found any load operation in the
    int min_position = 0;
    int max_position = 0;
    int war_dep_flag = 0;

    if (type_selection == 0) {
        cout << "==================== Find the Longest Path in all extracted paths ====================" << endl;
        // Check whether this is a load operation
        if (getOperation(end_block) == "lsq_load_op") {
            cout << "[LOAD OP] No need for WAR dependency checking" << endl;

            // Search for the shortest path
            for (int i = 0; i < All_found_paths.size(); i++) {
                // Check whether the path has the icmp node or not
                // if (check_for_icmp_node(All_found_paths[i])) continue;

                if (pathLatency(All_found_paths[i], MG_index) > pathLatency(All_found_paths[max_position], MG_index)) {
                    max_position = i;
                }
            }
        } else {
            cout << "[STORE OP] Need to check WAR dependency" << endl;
            string set_op = "lsq_load_op";
            vector<int> path_with_load;

            // Search for the shortest path
            for (int i = 0; i < All_found_paths.size(); i++) {
                if (pathLatency(All_found_paths[i], MG_index) > pathLatency(All_found_paths[max_position], MG_index)) {
                    max_position = i;
                }
            }
        }

        // Set min_index as the same value
        min_position = max_position;


    } else {
        cout << "==================== Find the shortest Path in all extracted paths ====================" << endl;
        cout << "Selecting the shortest path in all extracted paths" << endl;
        // Search for the shortest path
        for (int i = 0; i < All_found_paths.size(); i++) {
            // Check whether the path has the icmp node or not
            // if (check_for_icmp_node(All_found_paths[i])) continue;

            if (All_found_paths[i].size() < All_found_paths[min_position].size()) {
                min_position = i;
            }
        }
    }

    // Store the shortest path
    for (auto insert_block: All_found_paths[min_position]) {
        cout << "Push " << getBlockName(insert_block) << " to the final path storing structure" << endl;
        found_paths.insertBlock(path_index, insert_block);
    }

    cout << "=======================================================================================" << endl;

    cout << "*********************************************************************************" << endl;
    return found_paths;

}

// THis function will calculate the latency for the given path
// Without output anything
int DFnetlist_Impl::sim_path_latency_cal(std::vector<blockID> sel_path, int MG_index){
    // Variable Definition
    int lat_total = 0;
    int counter = 0;
    int last_load_flag = 0; // Flag used to indicate whether the last operation is
    int Buffer_flag = 0;

    // Testing
    // cout << "   Lastnode: " << getBlockName(sel_path.back()) << endl;
    // Check whether the last element is a load
    if (getOperation(sel_path.back()) == "lsq_load_op" || (getOperation(sel_path.back()) == "lsq_store_op")) {
        // Testing
        // cout << "LSQ LOAD as last node of the path" << endl;
        last_load_flag = 1;
    }

    // Add up all latency for the nodes in the set
    for (auto sel_block: sel_path) {
        // Check whether the block is a buffer or not
        if (getBlockType(sel_block) == ELASTIC_BUFFER) {
            // Check whether this is a transparent buffer or not
            if (isBufferTransparent(sel_block)){
                counter++;
                continue;
            } else {
                Buffer_flag = 1;
                lat_total += 1;
                counter++;
                continue;
            }
        }

        // Check whether this is a branch node
        int flag = 0;
        if (getBlockType(sel_block) == BRANCH) {
            for (auto sel_chan: loopback_edge) {
                if (sel_path[counter + 1] == getDstBlock(sel_chan)) {
                    flag = 1;
                    break;
                }
            }
            // If loopback edge detected;
            if (flag) {
                lat_total += (-II_traces[MG_index][0]);
                counter++;
                continue;
            }
        }

        if (getOperation(sel_block) == "lsq_load_op") {
            if (lat_total >= 2) {
                counter++;
                lat_total += getLatency(sel_block) - 1;
                // cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) - 1 << endl;
                continue;
            } else {
                counter++;
                lat_total = getLatency(sel_block) + 1;
                // cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) + 1 << endl;
                continue;
            }
        } else {
            if (getLatency(sel_block) > 0) {
                // Check whether a Buffer is preceding this operation
                if (Buffer_flag) {
                    Buffer_flag = 0;
                    counter++;
                    // cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) << endl;
                    lat_total += getLatency(sel_block);
                    continue;
                } else {
                    // No Buffer preceding the operation
                    // cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) - 1 << "; [Overlapping], latency - 1." <<endl;
                    counter++;
                    lat_total += getLatency(sel_block) - 1;
                    continue;
                }
            }
        }

        // cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) << endl;
        counter++;
        lat_total += getLatency(sel_block);
    }

    // Check whether load is the only element with latency in the path
    if (last_load_flag) {
        lat_total += 1;
    }

    return lat_total;
}

// This function will calculate the latency for the founded path
// TODO: For now we only considered the position of allocation for lsq_load operation, as load operation just requires the address,
//       for store we should do the same thing, but the adj structure shall be changed completely
int DFnetlist_Impl::pathLatency(std::vector<blockID> node_set, int MG_index){
    // Variable Definition
    int lat_total = 0;
    int counter = 0;
//    int last_load_flag = 0; // Flag used to indicate whether the last operation is lsq_load
    int Buffer_flag = 0;


    cout << "==================================" << endl;
    cout << "Path Latency Calculation " << endl;
    cout << "==================================" << endl;

//    // Check whether the last element is a load
//    if (getOperation(node_set.back()) == "lsq_load_op"|| (getOperation(node_set.back()) == "lsq_store_op")) {
//        last_load_flag = 1;
//    }

    // Add up all latency for the nodes in the set
    for (auto sel_block: node_set) {
        // Check whether the block is a buffer or not
        if (getBlockType(sel_block) == ELASTIC_BUFFER) {
            // Check whether this is a transparent buffer or not
            if (isBufferTransparent(sel_block)){
                // Transparent Buffer
                cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) << " [Transparent Buffer]" << endl;
                counter++;
                continue;
            } else {
                // Opaque Buffer
                cout << "Latency for " << getBlockName(sel_block) <<" is " << 1 << " [Opaque Buffer]" << endl;
                 Buffer_flag = 1;
                lat_total += 1;
                counter++;
                continue;
            }
        }

        // Check whether this is a branch node
        int flag = 0;
        if (getBlockType(sel_block) == BRANCH) {
            // Check whether this is a loopback edge
            // Testing
            cout << "Branch node detected" << endl;

            for (auto sel_chan: loopback_edge) {

                // Testing
                cout << "   Src node: " << getBlockName(getSrcBlock(sel_chan)) << " Dst node: " << getBlockName(getDstBlock(sel_chan)) << endl;
                cout << "   The next node in node_set is " << getBlockName(node_set[counter + 1]) << endl;
                if (sel_block == getSrcBlock(sel_chan)){
                    if (node_set[counter + 1] == getDstBlock(sel_chan)) {
                        flag = 1;
                        break;
                    }
                }
            }

            // If loopback edge detected;
            if (flag) {
                cout << "Loopback edge detected for " << getBlockName(sel_block) << " with latency -" << II_traces[MG_index][0] << endl;
                counter++;
                lat_total += (-II_traces[MG_index][0]);
                continue;
            }
        }

        // Check whether the component has a latency
        if (getOperation(sel_block) == "lsq_load_op") {
            if (lat_total >= 2) {
                // Check for the buffer flag
                if (Buffer_flag) {
                    counter++;
                    Buffer_flag = 0;
                    lat_total += getLatency(sel_block) ;
                    cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) - 1 << endl;
                    continue;
                } else {
                    counter++;
                    lat_total += getLatency(sel_block) - 1;
                    cout << "Latency for " << getBlockName(sel_block) << " is " << getLatency(sel_block) - 1 << endl;
                    continue;
                }
            } else {
                if (Buffer_flag) {
                    Buffer_flag = 0;
                }
                counter++;
                lat_total = getLatency(sel_block) + 1;
                cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) + 1 << endl;
                continue;
            }
        } else {
            if (getLatency(sel_block) > 0) {
                // Check whether a Buffer is preceding this operation
                if (Buffer_flag) {
                    Buffer_flag = 0;
                    counter++;

                    // Temporary stuff, which has to be deleted later
                    if (getOperation(sel_block) == "mul_op") {
                        cout << "Latency for " << getBlockName(sel_block) <<" is " << 5 << endl;
                        lat_total += 5;
                        continue;
                    } else if (getOperation(sel_block) == "mc_load_op") {
                        cout << "Latency for " << getBlockName(sel_block) <<" is " << 2 << endl;
                        lat_total += 2;
                        continue;
                    }

                    cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) << endl;
                    lat_total += getLatency(sel_block);
                    continue;
                } else {

                    // Temporary stuff, which has to be deleted later
                    if (getOperation(sel_block) == "mul_op") {
                        cout << "Latency for " << getBlockName(sel_block) <<" is " << 4 << endl;
                        lat_total += 4;
                        continue;
                    } else if (getOperation(sel_block) == "mc_load_op") {
			if ( lat_total == 0){ // Carmine -- added this condition and first part branch -- initialization time of memory needed also for MC at the beginning of iteration
                        	cout << "Latency for " << getBlockName(sel_block) <<" is " << 3 << endl;
                        	lat_total += 3;
			}else{
	                        cout << "Latency for " << getBlockName(sel_block) <<" is " << 1 << endl;
	                        lat_total += 1;
			}
                        continue;
                    }

                    // No Buffer preceding the operation
                    cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) - 1 << "; [Overlapping], latency - 1." <<endl;
                    counter++;
                    lat_total += getLatency(sel_block) - 1;
                    continue;
                }
//                // New flow, Jiantao 21/07/2022
//                This one is for using valid as the referencing point (in one word, don't use it)
//                counter++;
//                cout << "Latency for " << getBlockName(sel_block) << " is " << getLatency(sel_block) - 1 << endl;
//                lat_total += getLatency(sel_block) - 1;
//                continue;
            }
        }

        cout << "Latency for " << getBlockName(sel_block) <<" is " << getLatency(sel_block) << endl;
        counter++;
        lat_total += getLatency(sel_block);
    }

//    // Check whether load is the only element with latency in the path
//    if (last_load_flag) {
//        cout << "Last op is a load or store" << endl;
//        lat_total += 1;
//    }

    // Testing 
    cout << "   Total latency is " << lat_total<< endl;
    return lat_total;
}

// This is the main function of the depth calculation process(The process is the same for both load and store queue)
// @param MG_index: index of the selected MG
// Needed values for depth calculation:
//     1. TBB:      Stored in Global variable T_BB (ex. for BB 3, just invoke T_BB[i].get_an_element(bb_index));
//     2. T_iter:   Stored in T_iter, for MG i , just invoke T_iter[i];
//     3. II:       Stored in II_traces(e.x. for MG i best case II, just invoke II_traces[i][0])
//     4. TL:       Stored in TL_traces(e.x. for MG i, from start point to the corresponding load operation:
//                         vector a; a.insert(Start_points[i]); a.insert(End_points_Load_queue[index]);
//                         TL_traces[i].getTime(a));
//     5. Overall:  Stores in MG_post_buffer[i]
//     6. marker:    Equals to corresponding TBB + 1;
// @return the optimal Load Queue size
int DFnetlist_Impl::optLoadDepCal(int MG_index, int case_selection, std::vector<string> throughput_var) {
    // Variable definition
    int optimal_depth = 0;
    std::vector<int> path_index;
    std::vector<int> marker_cal_result;
    double T_iteration = T_iter[MG_index];
    int num_operations = MG_post_buffer[MG_index].numofLoads_MG();
    double II = II_traces[MG_index][0]; // For now we just consider the best case
    int N_MAX = ceil(T_iteration / II);
    int marker = 0;
    int part_I = 0;
    int part_II = 0;
    int part_III = 0;

    // Formatting printing
    cout << endl;
    cout << "++++++++++++++ Calculate the optimal depth for LOAD QUEUE ++++++++++++++" << endl;


    // Testing
    cout << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << "For MG " << MG_index << " :" << endl;
    cout << "   II: " << II << endl;
    cout << "   Titer: " << T_iteration << endl;
    cout << "   Number of Loads in the MG: " << num_operations << endl;
    cout << "   N_MAX: " << N_MAX << endl;



    // Find the optimal sizes
    // We just need to check cycles with BB allocations
    // Iterate through all Markers
    for (auto sel_BB: MG_post_buffer[MG_index].getBasicBlocks()) {
        // For all basic blocks in the selected MG we check for the one within II
        if (MG_post_buffer[MG_index].numofLoads_BB(sel_BB) > 0){
            // The selected BB do have loads operations
            if (T_BB[MG_index].get_an_element(sel_BB) <= II) { //Carmine
                // The selected BB is within II
                // This marker shall be considered
                // Calculate the value of marker
                marker = T_BB[MG_index].get_an_element(sel_BB) + 1;

                // Testing
                cout << "   For Marker: " << marker << endl;
                cout << "       TBB[" << sel_BB << "]: " << T_BB[MG_index].get_an_element(sel_BB) << endl;


                // PART I: Calculate active loads in the current iteration
                // Check all the loads in the current BB
                cout << "       Calculating PART I: " << endl;
                for (auto load_index: End_points_Load_queue) {
                    // The selected load operation is in the selected MG
                    if (MG_post_buffer[MG_index].hasBlock(load_index)) {
                        // Now calculate
                        if (marker > (T_BB[MG_index].get_an_element(sel_BB) + 1)) {
                            // Setup the index for TL
                            path_index.clear();
                            path_index.push_back(Start_points[MG_index]);
                            path_index.push_back(load_index);


                            if ((TL_traces[MG_index].getTime(path_index) - marker) >= 0) {
                                part_I++;
                            }
                        }
                    }

                    // Testing
                    path_index.clear();
                    path_index.push_back(Start_points[MG_index]);
                    path_index.push_back(load_index);

                    cout << "           MG " << MG_index << " has " << getBlockName(load_index) << " : " << MG_post_buffer[MG_index].hasBlock(load_index) << endl;
                    cout << "           Start Point: " << getBlockName(path_index[0]) << "; End Point: " << getBlockName(path_index[1]) << endl;
                    cout << "           TL for " << getBlockName(load_index) << " is " << TL_traces[MG_index].getTime(path_index) << endl;
                    cout << "       -----------------------------------------------------------" << endl;
                    // TL_traces[MG_index].print_trace("MG 0");
                }

                // Testing
                cout << "       PART I: " << part_I << endl;
                cout << "       ==================================================" << endl;

                // PART II: Calculate Previous but still active Loads
                cout << "   Calculating PART II: " << endl;

                // Iterate through all load operations
                if (case_selection == 2) {
                    // Variable throughput
                    part_II += parse_alph_string_load(throughput_var[MG_index], marker, MG_index);
                } else {
                    for (auto load_index: End_points_Load_queue) {
                        // Check whether the load operation is in the specified marked graph
                        // Testing
                        cout << "       Checking " << getBlockName(load_index) << endl;
                        if (MG_post_buffer[MG_index].hasBlock(load_index)) {
                            // Setup the index for TL
                            path_index.clear();
                            path_index.push_back(Start_points[MG_index]);
                            path_index.push_back(load_index);

                            int part_II_temp = 0;
                            // Testing
                            cout << "   [Testing] the value of marker is " << marker << endl;
                            // Iterate through all possible iterations
                            for (int i = 0; i < N_MAX; i++) {
                                part_II_temp = TL_traces[MG_index].getTime(path_index) - ((i + 1) * II);
                                if (((part_II_temp / marker) - 1) >= 0) {
                                    part_II++;
                                }
                            }
                        }
                    }
                }
                

                cout << "       PART II: " << part_II << endl;

                // PART III: Get the number of allocated loads in the current marker
                cout << "   Calculating PART III: " << endl;

                part_III = MG_post_buffer[MG_index].numofLoads_BB(sel_BB);
                cout << "       PART III: " << part_III << endl;

            }
        }

        // Insert temp result for the optimal_depth
        optimal_depth = part_I + part_II + part_III;
        marker_cal_result.push_back(optimal_depth);
    }

    // Calculate the final opt depth
    for (auto sel_iter: marker_cal_result) {
        if (sel_iter > optimal_depth) {
            optimal_depth = sel_iter;
        }
    }

    cout << "   **OPTIMAL LOAD DEPTH**: " << optimal_depth << endl;
    cout << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;


    return optimal_depth;

}

// This is the main function of the depth calculation process(The process is the same for both load and store queue)
// @param MG_index: index of the selected MG
// Needed values for depth calculation:
//     1. TBB:      Stored in Global variable T_BB (ex. for BB 3, just invoke T_BB[i].get_an_element(bb_index));
//     2. T_iter:   Stored in T_iter, for MG i , just invoke T_iter[i];
//     3. II:       Stored in II_traces(e.x. for MG i best case II, just invoke II_traces[i][0])
//     4. TL:       Stored in TL_traces(e.x. for MG i, from start point to the corresponding load operation:
//                         vector a; a.insert(Start_points[i]); a.insert(End_points_Load_queue[index]);
//                         TL_traces[i].getTime(a));
//     5. Overall:  Stores in MG_post_buffer[i]
//     6. marker:    Equals to corresponding TBB + 1;
// @return the optimal Store Queue size
int DFnetlist_Impl::optStoreDepCal(int MG_index, int case_selection, std::vector<string> throughput_var) {
    // Variable definition
    int optimal_depth = 0;
    std::vector<int> path_index;
    std::vector<int> marker_cal_result;
    double T_iteration = T_iter[MG_index];
    int num_operations = MG_post_buffer[MG_index].numofStores_MG();
    double II = II_traces[MG_index][0]; // For now we just consider the best case
    int N_MAX = ceil(T_iteration / II);
    int marker = 0;
    int part_I = 0;
    int part_II = 0;
    int part_III = 0;

    // Formatting printing
    cout << endl;
    cout << "++++++++++++++ Calculate the optimal depth for STORE QUEUE ++++++++++++++" << endl;


    // Testing
    cout << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << "For MG " << MG_index << " :" << endl;
    cout << "   II: " << II << endl;
    cout << "   Titer: " << T_iteration << endl;
    cout << "   Number of Stores in the MG: " << num_operations << endl;
    cout << "   N_MAX: " << N_MAX << endl;



    // Find the optimal sizes
    // We just need to check cycles with BB allocations
    // Iterate through all Markers
    for (auto sel_BB: MG_post_buffer[MG_index].getBasicBlocks()) {
        // For all basic blocks in the selected MG we check for the one within II
        if (MG_post_buffer[MG_index].numofStores_BB(sel_BB) > 0){
            // The selected BB do have loads operations
            if (T_BB[MG_index].get_an_element(sel_BB) <= II) { //Carmine
                // The selected BB is within II
                // This marker shall be considered
                // Calculate the value of marker
                marker = T_BB[MG_index].get_an_element(sel_BB) + 1;

                // Testing
                cout << "   For Marker: " << marker << endl;
                cout << "       TBB[" << sel_BB << "]: " << T_BB[MG_index].get_an_element(sel_BB) << endl;


                // PART I: Calculate active loads in the current iteration
                // Check all the loads in the current BB
                cout << "       Calculating PART I: " << endl;
                for (auto store_index: End_points_Store_queue) {
                    // The selected load operation is in the selected MG
                    if (MG_post_buffer[MG_index].hasBlock(store_index)) {
                        // Now calculate
                        if (marker > (T_BB[MG_index].get_an_element(sel_BB) + 1)) {
                            // Setup the index for TL
                            path_index.clear();
                            path_index.push_back(Start_points[MG_index]);
                            path_index.push_back(store_index);

                            if ((TL_traces[MG_index].getTime(path_index) - marker) >= 0) {
                                part_I++;
                            }
                        }
                    }

                    // Testing
                    path_index.clear();
                    path_index.push_back(Start_points[MG_index]);
                    path_index.push_back(store_index);

                    cout << "           MG " << MG_index << " has " << getBlockName(store_index) << " : " << MG_post_buffer[MG_index].hasBlock(store_index) << endl;
                    cout << "           Start Point: " << getBlockName(path_index[0]) << "; End Point: " << getBlockName(path_index[1]) << endl;
                    cout << "           TL for " << getBlockName(store_index) << " is " << TL_traces[MG_index].getTime(path_index) << endl;
                    cout << "       -----------------------------------------------------------" << endl;
                    // TL_traces[MG_index].print_trace("MG 0");
                }

                // Testing
                cout << "       PART I: " << part_I << endl;
                cout << "       ==================================================" << endl;

                // PART II: Calculate Previous but still active Loads
                cout << "   Calculating PART II: " << endl;

                // Iterate through all load operations
                if (case_selection == 2) {
                    // Variable throughput
                    part_II += parse_alph_string_store(throughput_var[MG_index], marker, MG_index);
                } else {
                    for (auto store_index: End_points_Store_queue) {
                        // Check whether the load operation is in the specified marked graph
                        // Testing
                        cout << "       Checking " << getBlockName(store_index) << endl;
                        if (MG_post_buffer[MG_index].hasBlock(store_index)) {
                            // Setup the index for TL
                            path_index.clear();
                            path_index.push_back(Start_points[MG_index]);
                            path_index.push_back(store_index);

                            int part_II_temp = 0;
                            // Iterate through all possible iterations
                            for (int i = 0; i < N_MAX; i++) {
                                part_II_temp = TL_traces[MG_index].getTime(path_index) - ((i + 1) * II);
                                if (((part_II_temp / marker) - 1) >= 0) {
                                    part_II++;
                                }
                            }
                        }
                    }
                }

                cout << "       PART II: " << part_II << endl;

                // PART III: Get the number of allocated loads in the current marker
                cout << "   Calculating PART III: " << endl;

                part_III = MG_post_buffer[MG_index].numofStores_BB(sel_BB);
                cout << "       PART III: " << part_III << endl;

            }
        }

        // Insert temp result for the optimal_depth
        optimal_depth = part_I + part_II + part_III;
        marker_cal_result.push_back(optimal_depth);
    }

    // Calculate the final opt depth
    for (auto sel_iter: marker_cal_result) {
        if (sel_iter > optimal_depth) {
            optimal_depth = sel_iter;
        }
    }

    cout << "   **OPTIMAL STORE DEPTH**: " << optimal_depth << endl;
    cout << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;


    return optimal_depth;

}

// This function is used to change the output dot file (specifically the LSQ depth, offsets and ports)
// As the original flow of the design has two layers, the changing part has to be like this
bool DFnetlist_Impl::changeOutDot(const string& LSQ_node_name, int optimal_load_depth, int optimal_store_depth) {
    ForAllBlocks(b) {
        // Check whether the name matches the found LSQ node
        if (getBlockName(b) == LSQ_node_name) {
            // This is the wanted block, and it has to be a valid block
            assert(b);
            // Change the params of the LSQ accordingly
            setSPLSQparams(b, optimal_load_depth, optimal_store_depth);
        }
    }
}

// *************************************************************************************
//
//                       Functions for calculating variable throughput 
//
// *************************************************************************************
// This function is used to parse the input sequence from the user
double DFnetlist_Impl::get_II_base(string input_sequence) {
    // Variable Defnition
    double min_length = input_sequence.length();
    int last_position = 0;
    bool ini_flag = false;

    // Calculate the base_II
    for (int i = 0; i < input_sequence.length(); i++){
        if (input_sequence[i] == '1'){
            // Update the position
            if (((i - last_position) < min_length) && ini_flag){
                min_length = i - last_position;
            }

            last_position = i;
            // Set the flag
            ini_flag = true;
        }
    }

    return min_length;
}

int DFnetlist_Impl::parse_alph_string_load(string input_sequence, int marker, int MG_index) {
    // Variable Defnition
    int min_length = input_sequence.length();
    int last_position = 0;
    bool ini_flag = false;
    int part_II = 0;
    std::vector<int> valid_index;
    std::vector<int> path_index;

    // Calculate the base_II
    for (int i = 0; i < input_sequence.length(); i++){
        if (input_sequence[i] == '1'){
            // Update the index
            valid_index.push_back(i);
            // Update the position
            if (((i - last_position) < min_length) && ini_flag){
                min_length = i - last_position;
            }
            last_position = i;

            // Set the flag
            ini_flag = true;
        }
    }

    // Get the index of the last existing iteration
    int last_index = valid_index.back();

    // Remove the last value of the position index
    valid_index.pop_back();

    // Part II : Calculate memory accesses in previous iterations that are still active
    // Iterate through all load operations
    for (auto load_index: End_points_Load_queue) {
        // Check whether the load operation is in the specified marked graph
        // Testing
        cout << "       Checking " << getBlockName(load_index) << endl;
        if (MG_post_buffer[MG_index].hasBlock(load_index)) {
            // Setup the index for TL
            path_index.clear();
            path_index.push_back(Start_points[MG_index]);
            path_index.push_back(load_index);

            int part_II_temp = 0;

            // Iterate through all possible iteartions
            for (auto iter_index: valid_index) {
                // Testing
                cout << "       ###" << endl;
                cout << "       iter_index: " << iter_index << endl;
                cout << "       TL is: " << TL_traces[MG_index].getTime(path_index) << endl;
                cout << "       last_index is " << last_index << endl;
                cout << "       ###" << endl;
                // Check the validity of the previous load
                part_II_temp = TL_traces[MG_index].getTime(path_index) - ((last_index - iter_index));
                if (((part_II_temp / marker) - 1) >= 0) {
                    part_II++;
                }
            }
        }
    }

    return part_II;
}

// The same function as above, but for store
int DFnetlist_Impl::parse_alph_string_store(string input_sequence, int marker, int MG_index) {
    // Variable Defnition
    int min_length = input_sequence.length();
    int last_position = 0;
    bool ini_flag = false;
    int part_II = 0;
    std::vector<int> valid_index;
    std::vector<int> path_index;

    // Calculate the base_II
    for (int i = 0; i < input_sequence.length(); i++){
        if (input_sequence[i] == '1'){
            // Update the index
            valid_index.push_back(i);
            // Update the position
            if (((i - last_position) < min_length) && ini_flag){
                min_length = i - last_position;
                last_position = i;
            }

            // Set the flag
            ini_flag = true;
        }
    }

    // Get the index of the last existing iteration
    int last_index = valid_index.back();

    // Remove the last value of the position index
    valid_index.pop_back();

    // Part II : Calculate memory accesses in previous iterations that are still active
    // Iterate through all load operations
    for (auto load_index: End_points_Store_queue) {
        // Check whether the load operation is in the specified marked graph
        // Testing
        cout << "       Checking " << getBlockName(load_index) << endl;
        if (MG_post_buffer[MG_index].hasBlock(load_index)) {
            // Setup the index for TL
            path_index.clear();
            path_index.push_back(Start_points[MG_index]);
            path_index.push_back(load_index);

            int part_II_temp = 0;

            // Iterate through all possible iteartions
            for (auto iter_index: valid_index) {
                // Testing
                cout << "       ###" << endl;
                cout << "       iter_index: " << iter_index << endl;
                cout << "       TL is: " << TL_traces[MG_index].getTime(path_index) << endl;
                cout << "       last_index is " << last_index << endl;
                cout << "       ###" << endl;
                // Check the validity of the previous load
                part_II_temp = TL_traces[MG_index].getTime(path_index) - ((last_index - iter_index));
                if (((part_II_temp / marker) - 1) >= 0) {
                    part_II++;
                }
            }
        }
    }

    return part_II;
}

// *************************************************************************************
//
//                          Potentially useful functions. 
//          IMPORTANT: Some of them are not tested, be careful if you want to use 
//
// *************************************************************************************


// This function is used to remove the given edge in all extracted Marked Graphs
void DFnetlist_Impl::del_edge_MGs(channelID sel_channel) {
    cout << endl;
    for (int i = 0; i < MG_post_buffer.size(); i++) {
        // Delete the selected edge in all MGs
        if (MG_post_buffer[i].hasChannel(sel_channel)) {
            // If the channel is in the MG, delete it
            cout << "Delete channel " << sel_channel << " in MG " << i << endl;
            MG_post_buffer[i].del_channel(sel_channel);
        }
    }
}

// This function will check whether the path has a icmp node or not
bool DFnetlist_Impl::check_for_icmp_node(std::vector<blockID> sel_path){
    for (auto node: sel_path){
        // Check whether this node is an Operator
        if (getBlockType(node) == OPERATOR) {
            // Check whether this is an icmp node or not
            char str_match[] = "icmp";

            // String to char conversion
            std:string input_string = getOperation(node);
            char arr_ori[input_string.length() + 1];
            strcpy(arr_ori, input_string.c_str());

            char *p = strstr(arr_ori, str_match);
            if (p == NULL) {
                // Not an ICMP node
                continue;
            } else {
                return true;
            }
        }
    }

    return false;
}

// This function will check whether the specified block is in the block list
bool DFnetlist_Impl::check_spi_node(std::vector<blockID> &block_list, BlockType sel_type, std::string &sel_op, int op_flag) {
    for (auto sel_block: block_list) {
        if (getBlockType(sel_block) == sel_type) {
            if (op_flag) {
                if (getOperation(sel_block) == sel_op) {
                    return true;
                }
            } else {
                return true;
            }
        }
    }

    return false;
}

// Supporting functions for printing all components in the vector
// Jiantao, 29/05/2022
void DFnetlist_Impl::print_main_stack(std::vector<blockID> S1) {
    if (S1.size() > 0) {
        cout << "[MAIN STACK] Contents in the main stack: ";

        for (auto iter: S1) {
            cout << "   " << getBlockName(iter);
        }

        cout << endl;
    } else {
        cout << "[MAIN STACK EMPTY]" << endl;
    }
}

void DFnetlist_Impl::print_adj_stack(std::vector< std::set<blockID> > S2) {
    if (S2.size() > 0) {
        cout << "[ADJ STACK]Contents in the Adj Stack:" << endl;
        int counter = 0;

        for (auto adj_list: S2) {
            cout << "   Adj list For element " << counter << " in the Main stack:" << endl;
            for (auto elem: adj_list) {
                cout << "       " << getBlockName(elem) << endl;
            }
            counter++;
        }
        cout << endl;
    } else {
        cout << "[ADJ LIST STACK EMPTY]" << endl;
    }
}
