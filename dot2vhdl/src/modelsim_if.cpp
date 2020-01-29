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
#include "vivado_if.h"
#include "lsq_generator.h"


using namespace std;




void write_vivado_script ( void )
{
    ofstream outFile;

    //cout << "top_level_filename" << top_level_filename << endl;
    string vivado_script_filename = top_level_filename+"_vivado_synt.tcl";
    
    outFile.open (vivado_script_filename);

    outFile << "read_vhdl -vhdl2008 " << "Elastic_components.vhd" << endl;

    for ( int indx =dot_input_files-1; indx >= 0; indx-- )
    {
        outFile << "read_vhdl -vhdl2008 " << output_filename[indx] << ".vhd" << endl;
    }
    
    outFile << "synth_design -mode out_of_context -flatten_hierarchy none -top "<< top_level_filename <<" -part 7k160tfbg484-1" << endl;

    outFile << "create_clock -period 1.000 -name clk -waveform {0.000 0.500} -add [get_ports -filter { NAME =~  \"*ap_clk*\" && DIRECTION == \"IN\" }] " << endl;

    outFile.close();

    // read_vhdl -vhdl2008 ./constants.vhd
// read_vhdl -vhdl2008 ../src/array_RAM.vhd
// synth_design -mode out_of_context -flatten_hierarchy none -top array_RAM -part 7k160tfbg484-1
// create_clock -period 1.000 -name clk -waveform {0.000 0.500} -add [get_ports -filter { NAME =~  "*ap_clk*" && DIRECTION == "IN" }] 
// report_timing_summary -delay_type min_max -report_unconstrained -max_paths 1 -input_pins -name timing_1 -file timing_report
// write_schematic -format pdf -orientation landscape /home/aguerrie/Elastic_component_latency/elastic_merge/schematic/test_method.pdf

}


