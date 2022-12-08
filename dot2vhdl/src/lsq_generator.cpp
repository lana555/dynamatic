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




#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm> 
#include <list>
#include <cctype>
#include <sstream> 

#include "dot2vhdl.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include "lsq_generator.h"


ofstream lsq_configuration_file;

#define LSQ_CONFIGURATION_FNAME "lsq.json"

#define LSQ_ADDRESSWIDTH_DEFAULT    32
#define LSQ_DATAWIDTH_DEFAULT       32
#define LSQ_FIFODEPTH_DEFAULT       4

#define MAX_LSQ 256

LSQ_CONFIGURATION_T lsq_conf[MAX_LSQ];


#define MAX_BBs 16

int bbcount;

typedef struct bb_port
{
    int componend_id = -1;
    int load_offset;
    int load_port;
    int store_offset;
    int store_port;
    
}BB_PORT_T;


typedef struct bb_st
{
    int bbcount;
    int bbId;
    BB_PORT_T bbid[MAX_BBs];
    int load_ports;
    int store_ports;
    
}BB_T;

BB_T bb[MAX_BBs];


string get_lsq_name ( int lsq_indx )
{
    string lsq_name = "LSQ";
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                lsq_name = nodes[i].name;
                break;
            }
        }

    }

    return lsq_name;
}

int get_lsq_datawidth ()
{
    int datawidth = LSQ_DATAWIDTH_DEFAULT;
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            datawidth = nodes[i].inputs.input[0].bit_size;
        }

    }
    
    return datawidth;
    
}

int get_lsq_addresswidth ( int lsq_indx )
{
    int addresswidth = LSQ_ADDRESSWIDTH_DEFAULT;
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                addresswidth = nodes[i].address_size;
                break;
            }
        }

#if 0

        if ( nodes[i].name.find("load") != std::string::npos )
        {
            addresswidth = nodes[i].inputs.input[0].bit_size;
        }
#endif
    }
    
    return addresswidth;
}


int get_lsq_fifo_depth ( int lsq_indx )
{
    int fifodepth;
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                fifodepth = nodes[i].fifodepth;
                break;
            }
        }

        
    }
    
    return fifodepth;

    
    //return LSQ_FIFODEPTH_DEFAULT; //default???
}

// Jiantao, 14/06/2022
int get_lsq_fifo_L_depth ( int lsq_indx )
{
    int fifodepth_L;

    for (int i = 0; i < components_in_netlist; i++)
    {

        if ( nodes[i].type.find("LSQ") != std::string::npos)
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                // Jiantao, 05/09/2022
                // Check whether the depths is given or not.
                if (nodes[i].fifodepth_L != 0) {
                    fifodepth_L = nodes[i].fifodepth_L;
                } else {
                    fifodepth_L = nodes[i].fifodepth;
                }

                break;
            }
        }
    }

    return fifodepth_L;
}

int get_lsq_fifo_S_depth ( int lsq_indx )
{
    int fifodepth_S;

    for (int i = 0; i < components_in_netlist; i++)
    {

        if ( nodes[i].type.find("LSQ") != std::string::npos)
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                // Jiantao, 05/09/2022
                // Check whether the depths is given or not.
                if (nodes[i].fifodepth_S != 0) {
                    fifodepth_S = nodes[i].fifodepth_S;
                } else {
                    fifodepth_S = nodes[i].fifodepth;
                }
                break;
            }
        }
    }

    return fifodepth_S;
}

int get_lsq_loadPorts ( int lsq_indx )
{
    int load_ports = 0;
    for (int i = 0; i < components_in_netlist; i++) 
    {
#if 0
        if ( nodes[i].component_operator == "lsq_load_op" )
        //if ( nodes[i].name.find("load") != std::string::npos )
        {
            load_ports++;
        }
#endif
    
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                load_ports = nodes[i].load_count;
                break;
            }
        }
    }
    return load_ports;
}

int get_lsq_storePorts ( int lsq_indx )
{
    int store_ports = 0;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                store_ports = nodes[i].store_count;
                break;
            }
        }

        
    }
    
    return store_ports;
}


string get_numLoads( int lsq_indx )
{
    string numLoads;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                numLoads = nodes[i].numLoads;
                replace( numLoads.begin(), numLoads.end(), '{', '[' );
                replace( numLoads.begin(), numLoads.end(), '}', ']' );
                replace( numLoads.begin(), numLoads.end(), ';', ',' );
                replace( numLoads.begin(), numLoads.end(), '"', ' ' );
                break;
            }
        }

        
    }
    
    return numLoads;
    
}

string get_numStores( int lsq_indx )
{
    string numStores;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                numStores = nodes[i].numStores;
                replace( numStores.begin(), numStores.end(), '{', '[' );
                replace( numStores.begin(), numStores.end(), '}', ']' );
                replace( numStores.begin(), numStores.end(), ';', ',' );
                replace( numStores.begin(), numStores.end(), '"', ' ' );

                break;
            }
        }

        
    }
    
    return numStores;
    
}


string get_loadOffset( int lsq_indx )
{
    string loadOffset;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                loadOffset = nodes[i].loadOffsets;
                replace( loadOffset.begin(), loadOffset.end(), '{', '[' );
                replace( loadOffset.begin(), loadOffset.end(), '}', ']' );
                replace( loadOffset.begin(), loadOffset.end(), ';', ',' );
                replace( loadOffset.begin(), loadOffset.end(), '"', ' ' );
                break;
            }
        }

        
    }
    
    
    return loadOffset;
}

string get_storeOffset( int lsq_indx )
{
    string storeOffset;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                storeOffset = nodes[i].storeOffsets;
                replace( storeOffset.begin(), storeOffset.end(), '{', '[' );
                replace( storeOffset.begin(), storeOffset.end(), '}', ']' );
                replace( storeOffset.begin(), storeOffset.end(), ';', ',' );
                replace( storeOffset.begin(), storeOffset.end(), '"', ' ' );

                break;
            }
        }

        
    }
    
    return storeOffset;
}


string get_loadPorts( int lsq_indx )
{
    string loadPorts;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                loadPorts = nodes[i].loadPorts;
                replace( loadPorts.begin(), loadPorts.end(), '{', '[' );
                replace( loadPorts.begin(), loadPorts.end(), '}', ']' );
                replace( loadPorts.begin(), loadPorts.end(), ';', ',' );
                replace( loadPorts.begin(), loadPorts.end(), '"', ' ' );

                break;
            }
        }

        
    }
    
    return loadPorts;
    
}

string get_storePorts( int lsq_indx )
{
    string storePorts;
    for (int i = 0; i < components_in_netlist; i++) 
    {

#if 0
        //if ( nodes[i].name.find("store") != std::string::npos )
        if ( nodes[i].component_operator == "lsq_store_op" )
        {
            store_ports++;
        }
#endif        
        
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {
            if ( lsq_indx == nodes[i].lsq_indx )
            {
                storePorts = nodes[i].storePorts;
                replace( storePorts.begin(), storePorts.end(), '{', '[' );
                replace( storePorts.begin(), storePorts.end(), '}', ']' );
                replace( storePorts.begin(), storePorts.end(), ';', ',' );
                replace( storePorts.begin(), storePorts.end(), '"', ' ' );

                break;
            }
        }

        
    }
    
    return storePorts;
    
}

#define INVALID_BB_COUNT -1


int get_lsq_loadSizes( int indx )
{

//     //int bbcount;
//     for (int i = 0; i < components_in_netlist; i++) 
//     {
//     
//         if ( nodes[i].bbId != INVALID_BB_COUNT )
//         {
//             
//             cout << "bbId" << nodes[i].bbId << " " << nodes[i].name << "  nodes[i].portId " << nodes[i].portId << "  nodes[i].offset " << nodes[i].offset << endl;
//         }
//     }
    
    return 1;
}

int get_lsq_loadOffsets( int indx )
{
    return 1;
    
}

int get_lsq_loadPortsList( int indx )
{
    return 1;
    
}

int get_lsq_storeSizes( int indx )
{

    return 1;
}


int get_lsq_storeOffsets( int indx )
{
    return 1;
    
}


int get_lsq_storePortsList( int indx )
{
    return 1;
    
}


int get_bbindx ( int bbId )
{
    
    static int return_indx = 0;
    
    for (int indx = 0; indx < MAX_BBs; indx++) 
    {
        if ( bb[indx].bbId ==  bbId )
        {
            return indx;
        }
        else // means not found yet 
        {
            bb[indx].bbId = bbId;
        }
    }
    
    return return_indx++;
}

void map_bb ( int lsq_indx )
{
    int bbIdindx_load = 0;
    int bbIdindx_store = 0;
    int bbindx = 0;
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("LSQ") != std::string::npos )
        {
            if ( nodes[i].lsq_indx == lsq_indx )
            {
                bbcount = nodes[i].bbcount;    
            }
        }
        if ( nodes[i].bbId != INVALID_BB_COUNT )
        {
//             bb[nodes[i].bbId].bbid[bbIdindx].componend_id = i;
//             
//             if ( nodes[i].name.find("store") != std::string::npos )
//             {
//                 bb[nodes[i].bbId].store_ports++;
//             }
//             if ( nodes[i].name.find("load") != std::string::npos )
//             {
//                 bb[nodes[i].bbId].load_ports++;
//             }
//         
//             bbIdindx++;

            //indexes from 0
            
            bbindx = get_bbindx ( nodes[i].bbId );
            
            
            //if ( nodes[i].name.find("store") != std::string::npos )
            if ( nodes[i].component_operator == "lsq_store_op" )
            {
                bb[bbindx].store_ports++;
              //  cout << " component id " << i << " bbindx" << bbindx << " bbIdindx_store " << bbIdindx_store << " node offset " << nodes[i].offset << endl;
              //  cout << " component id " << i << " bbindx" << bbindx << " bbIdindx_store " << bbIdindx_store << " node portId " << nodes[i].portId << endl;
                bb[bbindx].bbid[bbIdindx_store].store_offset = nodes[i].offset;
                bb[bbindx].bbid[bbIdindx_store].store_port = nodes[i].portId;
                bb[bbindx].bbid[bbIdindx_store].componend_id = i;

                bbIdindx_store++;
            }
            //if ( nodes[i].name.find("load") != std::string::npos )
            if ( nodes[i].component_operator == "lsq_load_op" )

            {
                bb[bbindx].load_ports++;
                //cout << " component id " << i << " bbindx " << bbindx << " bbIdindx_load " << bbIdindx_load << " node offset " << nodes[i].offset << endl;
                //cout << " component id " << i << " bbindx " << bbindx << " bbIdindx_load " << bbIdindx_load << " node portId " << nodes[i].portId << endl;

                bb[bbindx].bbid[bbIdindx_load].load_offset = nodes[i].offset;
                bb[bbindx].bbid[bbIdindx_load].load_port = nodes[i].portId;
                bb[bbindx].bbid[bbIdindx_load].componend_id = i;

                bbIdindx_load++;
            }

            //bbIdindx++;
            //bbindx++;
            //bb[bbindx].bbcount = bbIdindx;
            
        }
    }
    
//     for (int indx = 0; indx < MAX_BBs; indx++) 
//     {
//         for (int indx2 = 0; indx2 < MAX_BBs; indx2++) 
//         {
//             if ( bb[indx].bbid[indx2].componend_id != -1 )
//             {
//                 cout <<  "bb[" << indx << "] " << " component_id " << bb[indx].bbid[indx2].componend_id << endl;
//             }
//         }
//         //if ( bb[indx].store_ports != 0 || bb[indx].load_ports != 0 )
//         {
//             cout <<  "bb[" << indx << "] store_ports " <<  bb[indx].store_ports << endl;
//             cout <<  "bb[" << indx << "] load_ports " <<  bb[indx].load_ports << endl;
// 
//             for (int indx3 = 0; indx3 < 4; indx3++) 
//             {
//                 cout <<  "bb[" << indx << "] load_offset [" << indx3 << " ] "<< bb[indx].bbid[indx3].load_offset << endl;
//                 cout <<  "bb[" << indx << "] store_offset[" << indx3 << " ] "<< bb[indx].bbid[indx3].store_offset << endl;
//                 cout <<  "bb[" << indx << "] load_port [" << indx3 << " ] "<< bb[indx].bbid[indx3].load_port << endl;
//                 cout <<  "bb[" << indx << "] store_port [" << indx3 << " ] "<< bb[indx].bbid[indx3].store_port << endl;
//             }
// 
//         }
//     }
}

void lsq_set_configuration ( int lsq_indx )
{
    
    map_bb ( lsq_indx );
    
    lsq_conf[lsq_indx].name = get_lsq_name ( lsq_indx );
    lsq_conf[lsq_indx].dataWidth = get_lsq_datawidth ();//     "dataWidth": 32,
    lsq_conf[lsq_indx].addressWidth = get_lsq_addresswidth ( lsq_indx );//     "addressWidth": 10,
    lsq_conf[lsq_indx].fifoDepth = get_lsq_fifo_depth ( lsq_indx );    //     "fifoDepth": 4,
    lsq_conf[lsq_indx].fifoDepth_L = get_lsq_fifo_L_depth( lsq_indx ); //     "fifoDepth_L": 4,
    lsq_conf[lsq_indx].fifoDepth_S = get_lsq_fifo_S_depth( lsq_indx ); //     "fifoDepth_S": 4,
    lsq_conf[lsq_indx].loadPorts = get_lsq_loadPorts( lsq_indx ); //     "loadPorts": 1,
    lsq_conf[lsq_indx].storePorts = get_lsq_storePorts( lsq_indx ); //     "storePorts": 1,

    for (int indx = 0; indx < MAX_SIZES; indx++ )
    {
        for (int indx2 = 0; indx2 < bb[indx].bbcount; indx2++ )
        {
            lsq_conf[lsq_indx].bbParams.loadSizes[indx2] = bb[indx].load_ports;
            lsq_conf[lsq_indx].bbParams.storeSizes[indx2] = bb[indx].store_ports;            
        }
    }
    
//     for (int indx = 0; indx < MAX_SIZES; indx++ )
//     {
//         lsq_conf.bbParams.loadSizes[indx] = get_lsq_loadSizes(indx);
//         lsq_conf.bbParams.storeSizes[indx] = get_lsq_storeSizes(indx);
//         lsq_conf.bbParams.loadOffsets[indx] = get_lsq_loadOffsets(indx);
//         lsq_conf.bbParams.storeOffsets[indx] = get_lsq_storeOffsets(indx);
//         lsq_conf.bbParams.loadPortsList[indx] = get_lsq_loadPortsList(indx);
//         lsq_conf.bbParams.storePortsList[indx] = get_lsq_storePortsList(indx);
//         
//     }
    
}

void lsq_write_configuration_file ( string top_level_filename, int lsq_indx )
{
    string lsq_filename;
    
    lsq_filename = top_level_filename;
    lsq_filename += "_lsq";
    lsq_filename += to_string(lsq_indx);
    lsq_filename +="_configuration.json";
    
    lsq_configuration_file.open ( lsq_filename );
    
    lsq_configuration_file << "{" << endl;
    lsq_configuration_file << "\"specifications\" :[" << endl;
    lsq_configuration_file << "{" << endl;
    
    lsq_configuration_file << "\"name\": \""            << lsq_conf[lsq_indx].name              << "\", "<< endl;
    lsq_configuration_file << "\"dataWidth\":"       << lsq_conf[lsq_indx].dataWidth         << ", "<< endl;
    
    lsq_configuration_file << "\"accessType\" : \"BRAM\" "<< ", "<< endl;
    lsq_configuration_file << "\"speculation\": \"false\" "<< ", "<< endl;
    lsq_configuration_file << "\"addrWidth\":"          << lsq_conf[lsq_indx].addressWidth      << ", "<< endl;
    lsq_configuration_file << "\"fifoDepth\":"          << lsq_conf[lsq_indx].fifoDepth         << ", "<< endl;
    lsq_configuration_file << "\"fifoDepth_L\":"        << lsq_conf[lsq_indx].fifoDepth_L       << ", "<< endl;
    lsq_configuration_file << "\"fifoDepth_S\":"        << lsq_conf[lsq_indx].fifoDepth_S       << ", "<< endl;

    // Testing
    // lsq_configuration_file << "\"fifoDepth_L\":"        << 4       << ", "<< endl;
    // lsq_configuration_file << "\"fifoDepth_S\":"        << 8       << ", "<< endl;

    lsq_configuration_file << "\"numLoadPorts\":"       << lsq_conf[lsq_indx].loadPorts         << ", "<< endl;
    lsq_configuration_file << "\"numStorePorts\":"      << lsq_conf[lsq_indx].storePorts        << ", "<< endl;
    //lsq_configuration_file << "\"bbParams\":"        <<  "{"                       << endl;

    lsq_configuration_file << "\"numBBs\": "<< bbcount << "," << endl;
    

#if 0    
    lsq_configuration_file << "\"numLoads\": [" ;
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {

            lsq_configuration_file << ", ";

        }
        
        lsq_configuration_file << bb[indx2].load_ports;

    }

    lsq_configuration_file << "]," << endl;


    lsq_configuration_file << "\"numStores\": [" ;
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {
            lsq_configuration_file << ", ";
        }
        lsq_configuration_file << bb[indx2].store_ports;
    }
    lsq_configuration_file << "]," << endl;
    

//    lsq_configuration_file << "\"loadOffsets\":"     << "[[0, 0, 0, 0]],"          << endl;

    lsq_configuration_file << "\"loadOffsets\": [";
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {
            lsq_configuration_file << ", ";
        }

        lsq_configuration_file << "[";
        for (int indx3 = 0; indx3 < lsq_conf[lsq_indx].fifoDepth_L; indx3++ )
        {
            if ( indx3 != 0 )
            {
                lsq_configuration_file << ", ";
            }
            lsq_configuration_file << bb[indx2].bbid[indx3].load_offset;
        }
        lsq_configuration_file << "]";

    }
    lsq_configuration_file << "]," << endl;
    


    lsq_configuration_file << "\"storeOffsets\": [";
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {
            lsq_configuration_file << ", ";
        }

        lsq_configuration_file << "[";
        for (int indx3 = 0; indx3 < lsq_conf[lsq_indx].fifoDepth_S; indx3++ )
        {
            if ( indx3 != 0 )
            {
                lsq_configuration_file << ", ";
            }
            lsq_configuration_file << bb[indx2].bbid[indx3].store_offset;
        }
        lsq_configuration_file << "]";
    }
    lsq_configuration_file << "]," << endl;
    
    

    lsq_configuration_file << "\"loadPorts\": [";
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {
            lsq_configuration_file << ", ";
        }

        lsq_configuration_file << "[";
        for (int indx3 = 0; indx3 < lsq_conf[lsq_indx].fifoDepth_L; indx3++ )
        {
            if ( indx3 != 0 )
            {
                lsq_configuration_file << ", ";
            }
            lsq_configuration_file << bb[indx2].bbid[indx3].load_port;
        }
        lsq_configuration_file << "]";
    }

    lsq_configuration_file << "]," << endl;




    lsq_configuration_file << "\"storePorts\": [";
    
    for (int indx2 = 0; indx2 < bbcount; indx2++ )
    {
        if ( indx2 != 0 )
        {
            lsq_configuration_file << ", ";
        }

        lsq_configuration_file << "[";
        for (int indx3 = 0; indx3 < lsq_conf[lsq_indx].fifoDepth_S; indx3++ )
        {
            if ( indx3 != 0 )
            {
                lsq_configuration_file << ", ";
            }
            lsq_configuration_file << bb[indx2].bbid[indx3].store_port;
        }
        lsq_configuration_file << "]";
    }

    lsq_configuration_file << "]" << endl;
#endif
    
    lsq_configuration_file << "\"numLoads\": " << get_numLoads( lsq_indx ) << "," << endl;
    lsq_configuration_file << "\"numStores\": " << get_numStores( lsq_indx ) << "," << endl;
    lsq_configuration_file << "\"loadOffsets\": "<< get_loadOffset( lsq_indx ) << "," << endl;
    lsq_configuration_file << "\"storeOffsets\": "<< get_storeOffset( lsq_indx ) << "," << endl;
    lsq_configuration_file << "\"loadPorts\": " << get_loadPorts( lsq_indx ) << "," << endl;
    lsq_configuration_file << "\"storePorts\": " << get_storePorts( lsq_indx ) << "," << endl;
    
    lsq_configuration_file << "\"bufferDepth\": 0 "<< endl;

    
    
    //lsq_configuration_file << "}" << endl;
    lsq_configuration_file << "}" << endl;
    lsq_configuration_file << "]" << endl;
    lsq_configuration_file << "}" << endl;
    
    lsq_configuration_file.close();
    

// {
//   "specifications"  :[
//   {
//     "name": "hist",
//     "dataWidth": 32,
//     "addressWidth": 10,
//     "fifoDepth": 4,
//     "loadPorts": 1,
//     "storePorts": 1,
//     "bbParams": {
//     "loadSizes": [1],
//     "storeSizes": [1],
//     "loadOffsets": [[0, 0, 0, 0]],
//     "storeOffsets": [[1, 0, 0, 0]],
//     "loadPortsList": [[0, 0, 0, 1]],
//     "storePortsList": [[0, 0, 0, 0]]
//     }
//   }
//   ]
// }
    
}

void lsq_generate_configuration ( string top_level_filename )
{
    
    for ( int lsq_indx; lsq_indx < lsqs_in_netlist; lsq_indx++ )
    {    
        lsq_set_configuration ( lsq_indx );
        lsq_write_configuration_file( top_level_filename, lsq_indx );
    }
}


void lsq_generate ( string top_level_filename )
{
    
    FILE *fp;
    char path[1035];
    stringstream ss_path, string_path;
    
    char cmd[512];
       
    //int lsq_indx;

    for ( int lsq_indx; lsq_indx < lsqs_in_netlist; lsq_indx++ )
    {    

        cout << "Generating LSQ " << lsq_indx << " component..." << endl;

        //sprintf ( cmd, "java -jar -Xmx7G lsq.jar --target-dir %s --spec-file %s.json", top_level_filename.c_str(), top_level_filename.c_str() );
        //sprintf ( cmd, "java -jar -Xmx7G lsq.jar --target-dir . --spec-file %s_lsq%d_configuration.json",  top_level_filename.c_str(), lsq_indx );
        //sprintf ( cmd, "java -jar -Xmx7G /home/dynamatic/Dynamatic/bin/lsq.jar --target-dir . --spec-file %s_lsq%d_configuration.json",  top_level_filename.c_str(), lsq_indx );
        
        sprintf ( cmd, "lsq_generate %s_lsq%d_configuration.json",  top_level_filename.c_str(), lsq_indx );
        
    
        cout << cmd << endl;

        /* Open the command for reading. */
        fp = popen( cmd, "r" );
        if (fp == NULL) 
        {
            return;
        }

        /* Read the output a line at a time - output it. */
        while (fgets(path, sizeof(path)-1, fp) != NULL) 
        {
            printf("%s", path);
        }

        /* close */
        pclose(fp);    
        
        //cout << "Generating LSQ component..." << endl;

    }
}




// Json Example with three components
// {
//   "specifications"  :[
//   {
//     "name": "hist",
//     "dataWidth": 32,
//     "addressWidth": 10,
//     "fifoDepth": 4,
//     "loadPorts": 1,
//     "storePorts": 1,
//     "bbParams": {
//     "loadSizes": [1],
//     "storeSizes": [1],
//     "loadOffsets": [[0, 0, 0, 0]],
//     "storeOffsets": [[1, 0, 0, 0]],
//     "loadPortsList": [[0, 0, 0, 1]],
//     "storePortsList": [[0, 0, 0, 0]]
//     }
//   },
//   {
//     "name": "hist",
//     "dataWidth": 32,
//     "addressWidth": 10,
//     "fifoDepth": 8,
//     "loadPorts": 1,
//     "storePorts": 1,
//     "bbParams": {
//     "loadSizes": [1],
//     "storeSizes": [1],
//     "loadOffsets": [[0, 0, 0, 0, 0, 0, 0, 0]],
//     "storeOffsets": [[1, 0, 0, 0, 0, 0, 0, 0]],
//     "loadPortsList": [[0, 0, 0, 0, 0, 0, 0, 1]],
//     "storePortsList": [[0, 0, 0, 0, 0, 0, 0, 0]]
//     }
//   },
//   {
//     "name": "hist",
//     "dataWidth": 32,
//     "addressWidth": 10,
//     "fifoDepth": 16,
//     "loadPorts": 1,
//     "storePorts": 1,
//     "bbParams": {
//     "loadSizes": [1],
//     "storeSizes": [1],
//     "loadOffsets": [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]],
//     "storeOffsets": [[1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]],
//     "loadPortsList": [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]],
//     "storePortsList": [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]]
//     }
//   }
//   ]
// }
