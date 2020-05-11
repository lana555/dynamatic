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
#include "string_utils.h"


using namespace std;




void write_vivado_script ( string top_level_filename  )
{
    ofstream outFile;

    //cout << "top_level_filename" << top_level_filename << endl;
    string vivado_script_filename = top_level_filename+"_vivado_synt.tcl";
    
    outFile.open (vivado_script_filename);

    //outFile << "read_vhdl -vhdl2008 " << "Elastic_components.vhd" << endl;
    outFile << "read_vhdl -vhdl2008  /home/dynamatic/Dynamatic/etc/dynamatic/components/elastic_components.vhd" << endl;
    outFile << "read_vhdl -vhdl2008  /home/dynamatic/Dynamatic/etc/dynamatic/components/arithmetic_units.vhd" << endl;
    outFile << "read_vhdl -vhdl2008  /home/dynamatic/Dynamatic/etc/dynamatic/components/MemCont.vhd" << endl;

    
    for ( int indx =dot_input_files-1; indx >= 0; indx-- )
    {
        for ( int i = 0; i < components_in_netlist; i++ ) 
        {
            if (nodes[i].type == "LSQ" )
            {
                outFile << "read_verilog hdl/" << ".v" << nodes[i].name << endl ;
            }
        }

        outFile << "read_vhdl -vhdl2008 hdl/" << clean_entity ( output_filename[indx] ) << ".vhd" << endl;
    }
    
    //outFile << "synth_design -mode out_of_context -flatten_hierarchy none -top "<< top_level_filename <<" -part 7k160tfbg484-1" << endl;

    //outFile << "create_clock -period 1.000 -name clk -waveform {0.000 0.500} -add [get_ports -filter { NAME =~  \"*ap_clk*\" && DIRECTION == \"IN\" }] " << endl;

    outFile.close();

}

void write_modelsim_script ( string top_level_filename )
{
    ofstream outFile;

    string modelsim_script_filename = top_level_filename+"_modelsim.tcl";
    
    outFile.open (modelsim_script_filename);

    outFile << "vcom -2008 /home/dynamatic/Dynamatic/etc/dynamatic/components/elastic_components.vhd" << endl;  
    outFile << "vcom -2008 /home/dynamatic/Dynamatic/etc/dynamatic/components/delay_buffer.vhd" << endl;
    outFile << "vcom -2008 /home/dynamatic/Dynamatic/etc/dynamatic/components/arithmetic_units.vhd" << endl;
    outFile << "vcom -2008 /home/dynamatic/Dynamatic/etc/dynamatic/components/MemCont.vhd" << endl;
    
    for ( int indx =dot_input_files-1; indx >= 0; indx-- )
    {
        for ( int i = 0; i < components_in_netlist; i++ ) 
        {
            if (nodes[i].type == "LSQ" )
            {
                //outFile << "vlog hdl/" << nodes[i].name << ".v" << endl ;
            }
        }
        //outFile << "vcom -2008 hdl/" << clean_entity ( output_filename[indx] )  << ".vhd" << endl;
        
        //outFile << "vcom -2008 hdl/" << top_level_filename << ".vhd" << endl;
    }
    
    outFile.close();
    
}

