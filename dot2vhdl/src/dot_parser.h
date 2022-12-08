/*
*  C++ Implementation: dot2Vhdl
*
* Description:
*
*
* Author: Andrea Guerrieri <andrea.guerrieri@epfl.ch (C) 2019
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef _DOT_PARSER_
#define _DOT_PARSER_


using namespace std;

#define COMMENT_CHARACTER '/'


#define MAX_INPUTS  512//64
#define MAX_OUTPUTS 512// 64

#define COMPONENT_NOT_FOUND -1 

typedef struct input
{
    int bit_size = 32;
    int prev_nodes_id = COMPONENT_NOT_FOUND;
    string type;
    int port;
    string info_type;
} INPUT_T;

typedef struct in
{
    int size = 0;
    INPUT_T input[MAX_OUTPUTS];
} IN_T;

typedef struct output
{
    int bit_size = 32;
    int next_nodes_id = COMPONENT_NOT_FOUND;
    int next_nodes_port;
    string type;
    int port;
    string info_type;

}OUTPUT_T;

typedef struct out
{
    int size = 0;
    OUTPUT_T output[MAX_OUTPUTS];
} OUT_T;


typedef struct node
{
    string  name;
    string  type;
    IN_T    inputs;
    OUT_T   outputs;
    string  parameters;
    int     node_id;
    int     component_type;
    string  component_operator;
    unsigned long int     component_value;
    bool    component_control;
    int     slots;
    bool    trasparent;
    string  memory;
    int     bbcount = -1;
    int     load_count = -1;
    int     store_count = -1;
    int     data_size = 32;
    int     address_size = 32;
    bool    mem_address;
    int     bbId = -1; 
    int     portId = -1; 
    int     offset = -1;
    int     lsq_indx = -1;
    //for sel
    vector<vector<int>> orderings;

    string  numLoads;
    string  numStores;
    string  loadOffsets;
    string  storeOffsets;
    string  loadPorts;
    string  storePorts;
    // Jiantao, 14/06/2022
    int fifodepth_L;
    int fifodepth_S;
    int fifodepth;
    int constants;
} NODE_T;


#define MAX_NODES 16384//4096

void parse_dot ( string filename );

extern NODE_T nodes[MAX_NODES];

extern int components_in_netlist;
extern int lsqs_in_netlist;


#endif
