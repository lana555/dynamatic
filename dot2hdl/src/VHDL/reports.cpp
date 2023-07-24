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
#include "dot2hdl.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include "eda_if.h"
#include "lsq_generator.h"


using namespace std;



#include "table_printer.h"

using bprinter::TablePrinter;

// void report_instances ( void )
// {
// 
//     cout << endl << "Report Modules "<< endl;
// 
//     TablePrinter tp(&std::cout);
//     tp.AddColumn("Node_ID", 10);
//     tp.AddColumn("Name", 20);
//     tp.AddColumn("Module_type", 20);
//     tp.AddColumn("Inputs", 10);
//     tp.AddColumn("Outputs", 10);
// 
//     tp.PrintHeader();
// 
//     for (int i = 0; i < components_in_netlist; i++) 
//     {
//         tp << i << nodes[i].name << nodes[i].type << nodes[i].inputs.size << nodes[i].outputs.size;
//     }
//     
//     tp.PrintFooter();
// 
// }

void report_instances ( void )
{

    cout << endl << "Report Modules "<< endl;

    TablePrinter tp(&std::cout);
    tp.AddColumn("Node_ID", 10);
    tp.AddColumn("Name", 20);
    tp.AddColumn("Module_type", 20);
    tp.AddColumn("Inputs", 10);
    tp.AddColumn("Outputs", 10);

    tp.PrintHeader();

    for (int i = 0; i < components_in_netlist; i++) 
    {
        tp << i << nodes[i].name << nodes[i].type << nodes[i].inputs.size << nodes[i].outputs.size;
    }
    
    tp.PrintFooter();

}


void report_area ( void )
{

    cout << "Report Estimated Area \n\r";

    int total_area = 0;
    
    TablePrinter tp(&std::cout);
    tp.AddColumn("Node_ID", 8);
    tp.AddColumn("Name", 18);
    tp.AddColumn("Module_type", 20);
    tp.AddColumn("Area", 10);

    tp.PrintHeader();

    for (int i = 0; i < components_in_netlist; i++) 
    {
        tp << i << nodes[i].name << nodes[i].type << 123;
        total_area+=123;
    }
    tp.PrintFooter();
    tp << "" << "" << "Total"  << total_area;
    tp.PrintFooter();
}


void print_netlist ( void )
{
    static int indx;
    cout << "Printing out Netlist" << endl;
    
    for (int i = 0; i < components_in_netlist; i++) 
    {

        cout << "[Node ID: " << i << "]";
        cout << "[Name: " << nodes[i].name<< "]"; 
        cout << "[Type: " << nodes[i].type<< "]"; 
        cout << "[Operator: " << nodes[i].component_operator<< "]"; 
        cout << "[Value: " << nodes[i].component_value<< "]"; 
        cout << "[Inputs: " << nodes[i].inputs.size; 
        for (  indx = 0; indx < nodes[i].inputs.size; ++indx)
        {
            cout << " Bitsize: " << nodes[i].inputs.input[indx].bit_size; 
            cout << " Input: " << indx << " Connected to node :"  << nodes[nodes[i].inputs.input[indx].prev_nodes_id].name; 
        }
        cout << "]";
        cout << "[Outputs: " << nodes[i].outputs.size ; 
        for ( indx = 0; indx < nodes[i].outputs.size; ++indx)
        {
            cout << " Bitsize: " << nodes[i].outputs.output[indx].bit_size;
            cout << " Output: " << indx << " Connected to :"  << nodes[nodes[i].outputs.output[indx].next_nodes_id].name; 

        }
        cout << "]"; 
        cout << endl << endl;
    }
    
    
    report_instances ( );
//    report_area ( );
}   

