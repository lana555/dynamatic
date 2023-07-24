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

#ifndef _LSQ_GENERATOR_
#define _LSQ_GENERATOR_

void lsq_generate_configuration ( string top_level_filename );
void lsq_generate ( string top_level_filename );

int get_lsq_datawidth ();
int get_lsq_addresswidth ();

#define MAX_SIZES   16
typedef struct bb_params
{

    int loadSizes[MAX_SIZES];//     "loadSizes": [1],
    int storeSizes[MAX_SIZES];//     "storeSizes": [1],
    int loadOffsets[MAX_SIZES];//     "loadOffsets": [[0, 0, 0, 0]],
    int storeOffsets[MAX_SIZES];//     "storeOffsets": [[1, 0, 0, 0]],
    int loadPortsList[MAX_SIZES];//     "loadPortsList": [[0, 0, 0, 1]],
    int storePortsList[MAX_SIZES];//     "storePortsList": [[0, 0, 0, 0]]

    
} BB_PARAMS_T;

typedef struct lsq_configuration
{
    string name;    //     "name": "hist",
    int dataWidth;//     "dataWidth": 32,
    int addressWidth;//     "addressWidth": 10,
    int fifoDepth;    //     "fifoDepth": 4,
    int fifoDepth_L; // "fifoDepth_L": 4,
    int fifoDepth_S; // "fifoDepth_S": 4,
    int loadPorts;//     "loadPorts": 1,
    int storePorts;//     "storePorts": 1,
    BB_PARAMS_T bbParams;//     "bbParams": {

} LSQ_CONFIGURATION_T;


#endif
