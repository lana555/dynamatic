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
#include "stdlib.h"
#include <string.h>
#include "dot2vhdl.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include "eda_if.h"
#include "lsq_generator.h"


using namespace std;



void check_names ( int indx )
{
    if ( ( nodes[indx].name.empty() ) )
    {
        //cout << "**Warning**: Node["<< indx << "] " << "without name" << endl;
    }
    
}

void check_type ( int indx )
{
    if ( ( nodes[indx].type.empty() ) )
    {
        //cout << "**Warning**: Node["<< indx << "] " << "without type" << endl;
    }    
}

void check_inputs ( int indx )
{
    for (int indx2 = 0; indx2 < nodes[indx].inputs.size; indx2++ )
    {
        if ( nodes[indx].inputs.input[indx2].bit_size > 32 )
        {
            //cout << "**Warning**: Node["<< indx << "] Node_Name:"<< nodes[indx].name << " input " << indx2+1 << " bitsize exceed " << endl;
        }
        if ( nodes[indx].inputs.input[indx2].prev_nodes_id > components_in_netlist || nodes[indx].inputs.input[indx2].prev_nodes_id == COMPONENT_NOT_FOUND )
        {
            //cout << "**Warning**: Node["<< indx << "] Node_Name:" << nodes[indx].name << " input " << indx2+1 << " has no predecessor -- signals connections will be omitted in the netlist" << endl;
        }
    //cout << "Node["<< indx << "] Node_Name:" << nodes[indx].name << " input " << indx2+1 << "bit_size " <<  nodes[indx].inputs.input[indx2].bit_size << endl;
    }
}

void check_outputs ( int indx )
{
    for (int indx2 = 0; indx2 < nodes[indx].outputs.size; indx2++ )
    {
        if ( nodes[indx].outputs.output[indx2].bit_size > 32 )
        {
            //cout << "**Warning**: Node["<< indx << "] Node_Name:"<< nodes[indx].name << " output " << indx2+1 << " bitsize exceed " << endl;
        }
        if ( nodes[indx].outputs.output[indx2].next_nodes_id > components_in_netlist || nodes[indx].outputs.output[indx2].next_nodes_id == COMPONENT_NOT_FOUND )
        {
            //cout << "**Warning**: Node["<< indx << "] Node_Name:" << nodes[indx].name << " output " << indx2+1 << " has no successor -- signals connections will be omitted in the netlist" << endl;
        }

    }

}

void check_netlist ( )
{
    for (int indx = 0; indx < components_in_netlist; indx++)
    {
        check_names ( indx );
        check_type ( indx );
        check_inputs ( indx );
        check_outputs ( indx );
    }
}


