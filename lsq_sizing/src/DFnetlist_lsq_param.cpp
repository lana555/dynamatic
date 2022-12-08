// Copyright 2022 ETH Zurich
//
// Author: Jiantao Liu <jianliu@student.ethz.ch>
// Date : 22/03/2022
// Description:
// This file is used for setting the size of Load queue and Store queue separately
// Warning: please make sure the default depth of LSQ in ElasticPass/src/ is set to a large value!!
//          at least larger than 32, or all functions below will fail;

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <regex>
#include <algorithm>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

/*
 * We set the fifodepth of the LSQ depth in the original dot file to max(16, 2 * max(#Loads, #Stores). Thus, we will only shrink the size fo LSQ here).
 */

/**
 * @brief Set the depth of the load queue to the given value
 * @param id Identifier of the block
 * @param load_depth desired depth of the load queue
 */
bool DFnetlist_Impl::setSPLoadQDepth(blockID id, int load_depth) {
    cout << "**********************************************************************************************" << endl;
    cout << "************ Setting the depth of Load Queue to " << load_depth << " ************" <<endl;

    // Testing
    cout << "The selected node is " << getBlockName(id) << endl;

    // The depth of LSQ shall not be smaller than 0
    assert(load_depth > 0);

    // Original depth of Load queue
    int or_lsq_depth = getLSQDepth(id);
    // Testing
    cout << "Original depth of the Load queue is " << or_lsq_depth << endl;
    cout << "The specified optimal Load queue depth is " << load_depth << endl;

    // Find the maximum number of loads among all BBs
    int max_loads_BB = 0;
    vector<int> num_loads;
    string numLoads = getNumLoads(id); // Format "{ X; X; X...}"
    string number = "";

    for (int i = 0; i < numLoads.length(); i++) {
        if ((numLoads[i] >= '0') && (numLoads[i] <= '9')) {
            number.push_back(numLoads[i]);
        } else if ((numLoads[i] == ';') || (numLoads[i] == '}')) {
            num_loads.push_back(stoi(number));
            number.clear();
        }
    }

    max_loads_BB = *max_element(num_loads.begin(), num_loads.end());
    if (max_loads_BB < 0) {
        cout << "[ERROR] NUM LOADS LESS THAN 0, IMPOSSIBLE!" << endl;
        return false;
    }

    // Testing
    cout << "Maximum number of Loads in all BBs: " << max_loads_BB << endl;

    // Set the load depth to either the optimal load size or the maximum number of loads in all BB
    int max_load_depth = 0;
    if (load_depth > max_loads_BB) {
        max_load_depth = load_depth;
    } else {
        max_load_depth = max_loads_BB;
    }

    // Find the cloest multiples of 2 to the specified Load Depth
    // Check whether the specified value is a power of 2 
    // if (((max_load_depth & (max_load_depth - 1)) != 0) || (max_load_depth == 1)) {
    //     max_load_depth--;
    //     max_load_depth |= max_load_depth >> 1;
    //     max_load_depth |= max_load_depth >> 2;
    //     max_load_depth |= max_load_depth >> 4;
    //     max_load_depth |= max_load_depth >> 8;
    //     max_load_depth |= max_load_depth >> 16;
    //     max_load_depth++;
    // }

    // Set depth of the Load queue
    cout << "Set the depth of the Load queue to " << max_load_depth << endl;
    setLoadQDepth(id, max_load_depth);

    // Testing
    cout << "Set Load queue depth = " << getLoadQDepth(id) << endl;
    cout << "**********************************************************************************************" << endl;
    cout << endl;

    return true;
}

/**
 * @brief Set the depth of the store queue to the given value
 * @param id Identifier of the block
 * @param load_depth desired depth of the store queue
 */
bool DFnetlist_Impl::setSPStoreQDepth(blockID id, int store_depth) {
    cout << "**********************************************************************************************" << endl;
    cout << "************ Setting the depth of Store Queue to " << store_depth << " ************" <<endl;

    // Testing
    cout << "The selected node is" << getBlockType(id) << endl;

    // Original depth of the Store queue
    int or_lsq_depth = getLSQDepth(id);
    cout << "Original depth of the Load queue is " << or_lsq_depth << endl;
    cout << "The specified optimal Store queue depth is " << store_depth << endl;

    // Find the maximum number of loads among all BBs
    int max_stores_BB = 0;
    vector<int> num_stores;
    string numStores = getNumStores(id); // Format "{ X; X; X...}"
    string number = "";

    for (int i = 0; i < numStores.length(); i++) {
        if ((numStores[i] >= '0') && (numStores[i] <= '9')) {
            number.push_back(numStores[i]);
        } else if ((numStores[i] == ';') || (numStores[i] == '}')) {
            num_stores.push_back(stoi(number));
            number.clear();
        }
    }

    max_stores_BB = *max_element(num_stores.begin(), num_stores.end());
    if (max_stores_BB < 0) {
        cout << "[ERROR] NUM STORES LESS THAN 0, IMPOSSIBLE!!" << endl;
        return false;
    }

    // Testing
    cout << "Maximum number of Stores in all BBs: " << max_stores_BB << endl;

    // Set the load depth to either the optimal load size or the maximum number of loads in all BB
    int max_store_depth = 0;
    if (store_depth > max_stores_BB) {
        max_store_depth = store_depth;
    } else {
        max_store_depth = max_stores_BB;
    }

    // Find the cloest multiples of 2 to the specified Store Depth
    // Check whether the specified value is a power of 2 
    // if (((max_store_depth & (max_store_depth - 1)) != 0) || (max_store_depth == 1)) {
    //     max_store_depth--;
    //     max_store_depth |= max_store_depth >> 1;
    //     max_store_depth |= max_store_depth >> 2;
    //     max_store_depth |= max_store_depth >> 4;
    //     max_store_depth |= max_store_depth >> 8;
    //     max_store_depth |= max_store_depth >> 16;
    //     max_store_depth++;
    // }

    // Set the depth of the store queue
    cout << "Set the depth of the Store queue to " << max_store_depth << endl;
    setStoreQDepth(id, max_store_depth);

    // Get the value specified for Store queue
    // Testing
    cout << "  Store queue depth = " << getStoreQDepth(id) << endl;
    cout << "**********************************************************************************************" << endl;
    cout << endl;

    return true;

}

/**
 * @brief Set the value of Loadoffsets for the load queue
 * @param id Identifier of the block
 */
void DFnetlist_Impl::setLoadOffsets_Separated(blockID id) {
    // Variable definitions
    string new_load_offsets = "";
    new_load_offsets += "{";
    vector<string> matched_parts;
    string str_load = getLoadOffsets(id);
    int num_loads = getLoadQDepth(id);  // Extracted the depth for load queue

    // Testing
    cout << "+++++++++++++++++++++++++++ Set Load offset value +++++++++++++++++++++++++++" << endl;
    cout << "   Optimal Load Depth: " << num_loads << endl;
    cout << "   Original Load Offset string: " << str_load << endl;

    // Define regex pattern matching
    smatch results;
    string pat_loadoff = "\\{(\\d+;?)+\\}";
    regex pattern(pat_loadoff);
    string::const_iterator iterStart = str_load.begin(); // Define iterator
    string::const_iterator iterEnd = str_load.end();

    // Pattern matching
    while (regex_search(iterStart, iterEnd, results, pattern)) {
        matched_parts.push_back(results[0]);
        iterStart = results[0].second;
    }

    // Testing
    cout << "   Matched results for the original Load offset: " << endl;
    for (auto sel_var: matched_parts) {
        cout << "   " << sel_var << endl;
    }

    // Modify the loadoffsets and store back
    // Define pattern 2
    smatch new_results;
    string pat_new_loadof = "\\{(\\d+;){" + to_string(num_loads - 1) + "}\\d";
    regex pattern2(pat_new_loadof);

    // Match and modify the contents
    for (int i = 0; i < matched_parts.size(); i++) {
        regex_search(matched_parts[i], new_results, pattern2);
        if (i != (matched_parts.size() - 1)) {
            new_load_offsets = new_load_offsets + new_results.str(0) + "};";
        }
        else {
            new_load_offsets = new_load_offsets + new_results.str(0) + "}";
        }
    }

    // Complete the string
    new_load_offsets += "}";

    // Testing
    cout << "   **Final Loadoffset string**: " << new_load_offsets << endl;

    // Set load offsets
    setLoadOffsets(id, new_load_offsets);

}

/**
 * @brief Set the value of Storeoffsets for the store queue
 * @param id Identifier of the block
 */
void DFnetlist_Impl::setStoreOffsets_Separated(blockID id) {
    // Define variables
    string new_store_offsets = "";
    new_store_offsets += "{";
    std::vector<string> matched_parts;
    string str_store = getStoreOffsets(id);
    int num_stores = getStoreQDepth(id);

    // Testing
    cout << "+++++++++++++++++++++++++++ Set Store offset value +++++++++++++++++++++++++++" << endl;
    cout << "   Optimal Store Depth: " << num_stores << endl;
    cout << "   Original Store Offset string: " << str_store << endl;

    // Define regex pattern matching
    smatch results;
    string pat_storeoff = "\\{(\\d+;?)+\\}";
    regex pattern(pat_storeoff);
    string::const_iterator iterStart = str_store.begin();
    string::const_iterator iterEnd = str_store.end();

    // Pattern matching
    while (regex_search(iterStart, iterEnd, results, pattern)) {
        matched_parts.push_back(results[0]);
        iterStart = results[0].second;
    }

    // Testing
    cout << "   Matched results for the original Store offset: " << endl;
    for (auto sel_var: matched_parts) {
        cout << "   " << sel_var << endl;
    }

    // Modify the storeoffsets and store back
    // Define pattern 2
    smatch new_results;
    string pat_new_storeof = "\\{(\\d+;){" + to_string(num_stores - 1) + "}\\d";
    regex pattern2(pat_new_storeof);

    // Match and modify the contents
    for (int i = 0; i < matched_parts.size(); i++) {
        regex_search(matched_parts[i], new_results, pattern2);
        if (i != (matched_parts.size() - 1)) {
            new_store_offsets = new_store_offsets + new_results.str(0) + "};";
        }
        else {
            new_store_offsets = new_store_offsets + new_results.str(0) + "}";
        }
    }

    // Complete the string
    new_store_offsets += "}";

    // Testing
    cout << "   **Final Store Offset string**: " << new_store_offsets << endl;

    // Set store offset
    setStoreOffsets(id, new_store_offsets);

}

/**
 * @brief Set the value of Loadports for the load queue
 * @param id Identifier of the block
 */
void DFnetlist_Impl::setLoadPorts_Separated(blockID id) {
    // Define variables
    string new_load_ports = "";
    new_load_ports += "{";
    vector<string> matched_parts;
    string str_load = getLoadPorts(id);
    int num_loads = getLoadQDepth(id);

    // Testing
    cout << "+++++++++++++++++++++++++++ Set Load Ports value +++++++++++++++++++++++++++" << endl;
    cout << "   Original Load Port string: " << str_load << endl;

    // Define regex pattern matching
    smatch results;
    string pat_loadoff = "\\{(\\d+;?)+\\}";
    regex pattern(pat_loadoff);
    string::const_iterator iterStart = str_load.begin(); // Define the start point of the iterator
    string::const_iterator iterEnd = str_load.end();

    // Pattern matching
    while(regex_search(iterStart, iterEnd, results, pattern)) {
        matched_parts.push_back(results[0]);
        iterStart = results[0].second;
    }

    // Testing
    cout << "   Matched results for the original Load ports: " << endl;
    for (auto sel_var: matched_parts) {
        cout << "   " << sel_var << endl;
    }

    // Modify the load ports and store back
    // Define pattern2
    smatch new_results;
    string pat_new_loadport = "\\{(\\d+;){" + to_string(num_loads - 1) + "}\\d";
    regex pattern2(pat_new_loadport);

    // Match and modify the contents
    for (int i = 0; i < matched_parts.size(); i++) {
        regex_search(matched_parts[i], new_results, pattern2);
        if (i != (matched_parts.size() - 1)) {
            new_load_ports = new_load_ports + new_results.str(0) + "};";
        } else {
            new_load_ports = new_load_ports + new_results.str(0) + "}";
        }
    }

    // Complete the string
    new_load_ports += "}";

    // Testing
    cout << "   **Final Load Port string**: " << new_load_ports << endl;

    // Set load ports
    setLoadPorts(id, new_load_ports);
}

/**
 * @brief Set the value of Storeports for the store queue
 * @param id Identifier of the block
 */
void DFnetlist_Impl::setStorePorts_Separated(blockID id) {
    // Variable definition
    string new_store_ports = "";
    new_store_ports += "{";
    vector<string> matched_parts;
    string str_store = getStorePorts(id);
    int num_stores = getStoreQDepth(id);

    // Testing
    cout << "+++++++++++++++++++++++++++ Set Store Ports value +++++++++++++++++++++++++++" << endl;
    cout << "   Original Store Port string: " << str_store << endl;

    // Define regex pattern matching
    smatch results;
    string pat_storeoff = "\\{(\\d+;?)+\\}";
    regex pattern(pat_storeoff);
    string::const_iterator iterStart = str_store.begin(); // Define the start of the iterator
    string::const_iterator iterEnd = str_store.end();

    // Pattern matching
    while (regex_search(iterStart, iterEnd, results, pattern)) {
        matched_parts.push_back(results[0]);
        iterStart = results[0].second;
    }

    // Testing
    cout << "   Matched results for the original Store ports: " << endl;
    for (auto sel_var: matched_parts) {
        cout << "   " << sel_var << endl;
    }

    // Modify the storeports and store back
    // Define pattern2
    smatch new_results;
    string pat_new_storeport = "\\{(\\d+;){" + to_string(num_stores - 1) + "}\\d";
    regex pattern2(pat_new_storeport);

    // Match and modify the contents
    for (int i = 0; i < matched_parts.size(); i++) {
        regex_search(matched_parts[i], new_results, pattern2);
        if (i != (matched_parts.size() - 1)) {
            new_store_ports = new_store_ports + new_results.str(0) + "};";
        } else {
            new_store_ports = new_store_ports + new_results.str(0) + "}";
        }
    }

    // Complete the string
    new_store_ports += "}";

    // Testing
    cout << "   **Final Store Port string**: " << new_store_ports << endl;

    // Set Store ports
    setStorePorts(id, new_store_ports);
}

/**
 * @brief This function is used to set the params of both Load Queue and Store Queue
 * @param lsq_block_id Identifier of the lsq block
 * @param load_depth Desired depth of the load queue
 * @param store_depth Desired depth of the store queue
 */
bool DFnetlist_Impl::setSPLSQparams(blockID lsq_block_id, int load_depth, int store_depth) {
    // Variable definition
    int load_depth_set = 0;
    int store_depth_set = 0;

    // Check the value of the given depth
    // It can't be 1 for the implementation of the lsq
    if (load_depth == 1){
        cout << "The given opt_load_depth is 1, change to 2" << endl;
        load_depth_set = 2;
    } else {
        load_depth_set = load_depth;
    }

    if (store_depth == 1) {
        cout << "The given opt_store_depth is 1, change to 2" << endl;
        store_depth_set = 2;
    } else {
        store_depth_set = store_depth;
    }

    // Set the depth of Load and Store queue
    if (not setSPLoadQDepth(lsq_block_id, load_depth_set)) return false;
    if (not setSPStoreQDepth(lsq_block_id, store_depth_set)) return false;

    // Set the new offsets of Load and Store queue
    setLoadOffsets_Separated(lsq_block_id);
    setStoreOffsets_Separated(lsq_block_id);

    // Set the ports of Load and Store queue
    setLoadPorts_Separated(lsq_block_id);
    setStorePorts_Separated(lsq_block_id);

    return true;
}

