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
#include <math.h>

#include "dot2vhdl.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include "string_utils.h"
#include "lsq_generator.h"


string entity_name[] = {
    ENTITY_MERGE, 
    ENTITY_READ_MEMORY, 
    ENTITY_SINGLE_OP,
    ENTITY_GET_PTR,
    ENTITY_INT_MUL,
    ENTITY_INT_ADD,
    ENTITY_INT_SUB,
    ENTITY_BUF,
    ENTITY_TEHB,
    ENTITY_OEHB,
    ENTITY_FIFO,
    ENTITY_NFIFO,
    ENTITY_TFIFO,
    ENTITY_FORK,
    ENTITY_LFORK,
    ENTITY_ICMP,
    ENTITY_CONSTANT,
    ENTITY_BRANCH,
    ENTITY_END,
    ENTITY_START,
    ENTITY_WRITE_MEMORY,
    ENTITY_SOURCE,
    ENTITY_SINK,
    ENTITY_CTRLMERGE,
    ENTITY_MUX,
    ENTITY_LSQ,
    ENTITY_MC,
    ENTITY_DISTRIBUTOR,
    ENTITY_INJECTOR,
    ENTITY_SELECTOR
};

string component_types[] = {
    COMPONENT_MERGE, 
    COMPONENT_READ_MEMORY, 
    COMPONENT_SINGLE_OP,
    COMPONENT_GET_PTR,
    COMPONENT_INT_MUL,
    COMPONENT_INT_ADD,
    COMPONENT_INT_SUB,
    COMPONENT_BUF,
    COMPONENT_TEHB,
    COMPONENT_OEHB,
    COMPONENT_FIFO,
    COMPONENT_NFIFO,
    COMPONENT_TFIFO,
    COMPONENT_FORK,
    COMPONENT_LFORK,
    COMPONENT_ICMP,
    COMPONENT_CONSTANT_,
    COMPONENT_BRANCH,
    COMPONENT_END,
    COMPONENT_START,
    COMPONENT_WRITE_MEMORY,
    COMPONENT_SOURCE,
    COMPONENT_SINK,
    COMPONENT_CTRLMERGE,
    COMPONENT_MUX,
    COMPONENT_LSQ,
    COMPONENT_MC,
    COMPONENT_DISTRIBUTOR,
    COMPONENT_INJECTOR,
    COMPONENT_SELECTOR
};

string inputs_name[] = {
    DATAIN_ARRAY, 
    PVALID_ARRAY, 
    NREADY_ARRAY
    
};
string inputs_type[] = {
    "std_logic_vector(", 
    "std_logic", 
    "std_logic_vector("
    
};
string outputs_name[] = {
    DATAOUT_ARRAY,
    READY_ARRAY,
    VALID_ARRAY
    
};
string outputs_type[] = {
    "std_logic_vector(",
    "std_logic_vector(",
    "std_logic_vector("
    
};

vector<string> in_ports_name_generic(inputs_name, inputs_name + sizeof(inputs_name)/sizeof(string));
vector<string> in_ports_type_generic(inputs_type, inputs_type + sizeof(inputs_type)/sizeof(string));
vector<string> out_ports_name_generic(outputs_name, outputs_name + sizeof(outputs_name)/sizeof(string));
vector<string> out_ports_type_generic(outputs_type, outputs_type + sizeof(outputs_type)/sizeof(string));

typedef struct component
{
    int in_ports;
    vector<string> in_ports_name_str;
    vector<string> in_ports_type_str;
    int out_ports;
    vector<string> out_ports_name_str;
    vector<string> out_ports_type_str;
    
}COMPONENT_T;




COMPONENT_T components_type[MAX_COMPONENTS];


ofstream netlist;
ofstream tb_wrapper;


void write_signals (  )
{
    int indx;
    string signal;

    for ( int i = 0; i < components_in_netlist; i++ ) 
    {
        if ( ( nodes[i].name.empty() ) ) //Check if the name is not empty
        {
                cout << "**Warning: node " << i << " does not have an instance name -- skipping node **" << endl; 
        }
        else
        {
                netlist << endl;
                netlist << "\t" << SIGNAL_STRING<< nodes[i].name << "_clk : std_logic;"<< endl;
                netlist << "\t" << SIGNAL_STRING<< nodes[i].name << "_rst : std_logic;" << endl;
                
                for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
                {

//                     if ( nodes[i].type == "Branch" && indx == 1 )
//                     {
//                             signal = SIGNAL;
//                             signal += nodes[i].name;
//                             signal += UNDERSCORE;
//                             signal += "Condition_valid";
// 
//                             signal += UNDERSCORE;
//                             signal += to_string( indx );
//                             signal += COLOUMN;
//                             signal += " std_logic;";
//                             signal += '\n';
//                             
//                             netlist << "\t"  << signal ;
//                         
//                     }
//                     else
                    {
                        //for ( int in_port_indx = 0; in_port_indx < components_type[nodes[i].component_type].in_ports; in_port_indx++ ) 
                        for ( int in_port_indx = 0; in_port_indx < 1; in_port_indx++ ) 
                        {
                            signal = SIGNAL_STRING;
                            signal += nodes[i].name;
                            signal += UNDERSCORE;
                            signal += components_type[0].in_ports_name_str[in_port_indx];

                            signal += UNDERSCORE;
                            signal += to_string( indx );
                            signal += COLOUMN;
                            // Lana 20.01.22 Branch condition no longer needs to be treated as a special case 
                            // Dot specifies branch condition bitwidth correctly, so just rea value like for any other port
                            //if ( nodes[i].type == "Branch" && indx == 1 )
                            //{
                                 //signal +="std_logic_vector (0 downto 0);";
                            //}
                            //else 
                            if (nodes[i].type == COMPONENT_DISTRIBUTOR && indx == 1)
                        	{
                            	int cond_size = nodes[i].inputs.input[nodes[i].inputs.size - 1].bit_size;
                           		signal += "std_logic_vector (" + to_string(cond_size - 1) + " downto 0);";
                        	}
                            else
                            {
                                signal += components_type[0].in_ports_type_str[in_port_indx];
                                signal += to_string( ( nodes[i].inputs.input[indx].bit_size - 1 >= 0 ) ? nodes[i].inputs.input[indx].bit_size - 1 : DEFAULT_BITWIDTH-1 );
                                signal += " downto 0);";
                            }  
                            signal += '\n';
                            netlist << "\t"  << signal ;
                        }
                    }
                }   
                for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
                {

                    //Write the Valid Signals
                    signal = SIGNAL_STRING;
                    signal += nodes[i].name;
                    signal += UNDERSCORE;
                    signal += PVALID_ARRAY; //Valid
                    signal += UNDERSCORE;
                    signal += to_string( indx );
                    signal += COLOUMN;
                    signal += STD_LOGIC ;
                    signal += '\n';
                    netlist << "\t"  << signal ;
                }
                for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
                {

                    //Write the Ready Signals
                    signal = SIGNAL_STRING;
                    signal += nodes[i].name;
                    signal += UNDERSCORE;
                    signal += READY_ARRAY; //Valid
                    signal += UNDERSCORE;
                    signal += to_string( indx );
                    signal += COLOUMN;
                    signal += STD_LOGIC ;
                    signal += '\n';
                    netlist << "\t"  << signal ;
                    
              }
                
                for ( indx = 0; indx < nodes[i].outputs.size; indx++ )
                {
                    //Write the Ready Signals
                    signal = SIGNAL_STRING;
                    signal += nodes[i].name;
                    signal += UNDERSCORE;
                    signal += NREADY_ARRAY; //Ready
                    signal += UNDERSCORE;
                    signal += to_string( indx );
                    signal += COLOUMN;
                    signal += STD_LOGIC ;
                    signal += '\n';
                    netlist << "\t"  << signal ;
                    
                    //Write the Valid Signals
                    signal = SIGNAL_STRING;
                    signal += nodes[i].name;
                    signal += UNDERSCORE;
                    signal += VALID_ARRAY; //Ready
                    signal += UNDERSCORE;
                    signal += to_string( indx );
                    signal += COLOUMN;
                    signal += STD_LOGIC ;
                    signal += '\n';
                    netlist << "\t"  << signal ;

                    for ( int out_port_indx = 0; out_port_indx < components_type[nodes[i].component_type].out_ports; out_port_indx++) 
                    {
                        
                        signal = SIGNAL_STRING;
                        signal += nodes[i].name;
                        signal += UNDERSCORE;
                        signal += components_type[0].out_ports_name_str[out_port_indx];
                        signal += UNDERSCORE;
                        signal += to_string( indx );
                        signal += COLOUMN;
                        signal += components_type[0].out_ports_type_str[out_port_indx];
                        signal += to_string( ( nodes[i].outputs.output[indx].bit_size-1 >=0 ) ? nodes[i].outputs.output[indx].bit_size - 1 : DEFAULT_BITWIDTH-1);
                        signal += " downto 0);";
                        signal += '\n';

                        netlist << "\t" << signal ;
                        
                    }
                }
        }
        
            if ( nodes[i].type == "Exit" )
            {
                
                signal = SIGNAL_STRING;
                signal += nodes[i].name;
                signal += UNDERSCORE;
                //signal += "validArray_0";
                signal += "validArray";
                signal += UNDERSCORE;
                signal += to_string( indx );
                signal += COLOUMN;
                signal +=" std_logic;";
                signal += '\n';
                
                netlist << "\t" << signal ;

                signal = SIGNAL_STRING;
                signal += nodes[i].name;
                signal += UNDERSCORE;
                //signal += "dataOutArray_0"; 
                signal += "dataOutArray"; 
                signal += UNDERSCORE;
                signal += to_string( indx );
                signal += COLOUMN;
                signal +=" std_logic_vector (31 downto 0);";
                signal += '\n';
                
                netlist << "\t" << signal ;

                signal = SIGNAL_STRING;
                signal += nodes[i].name;
                signal += UNDERSCORE;
                //signal += "nReadyArray_0"; 
                signal += "nReadyArray"; 
                signal += UNDERSCORE;
                signal += to_string( indx );
                signal += COLOUMN;
                signal +=" std_logic;";
                signal += '\n';
                
                netlist << "\t" << signal ;

            }

        if ( nodes[i].type == "LSQ" )
        {
            signal = SIGNAL_STRING;
            signal += nodes[i].name;
            signal += UNDERSCORE;
            signal += "io_queueEmpty";
            signal += COLOUMN;
            signal += STD_LOGIC ;
            netlist << "\t" << signal << endl;
        }
        
        if ( nodes[i].type == "MC" || nodes[i].type == "LSQ" )
        {
            signal = SIGNAL_STRING;
            signal += nodes[i].name;
            signal += UNDERSCORE;
            signal += "we0_ce0";
            signal += COLOUMN;
            signal += STD_LOGIC ;
            netlist << "\t" << signal << endl;
            
        }
        
        //LSQ-MC Modifications
        if ( nodes[i].type.find("LSQ") != std::string::npos )
        {

            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_address0 : std_logic_vector (" << (nodes[i].address_size -1 ) << " downto 0);" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_ce0 : std_logic;" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_we0 : std_logic;" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_dout0 : std_logic_vector (31 downto 0);" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_din0 : std_logic_vector (31 downto 0);" << endl;
            
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_address1 : std_logic_vector (" << (nodes[i].address_size - 1)  << " downto 0);" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_ce1 : std_logic;" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_we1 : std_logic;" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_dout1 : std_logic_vector (31 downto 0);" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_din1 : std_logic_vector (31 downto 0);" << endl;
            
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_load_ready : std_logic;" << endl;
            netlist << "\t" << SIGNAL_STRING << nodes[i].name << "_store_ready : std_logic;" << endl;

            
        }

    }

}

#include <bits/stdc++.h> 

                    
void write_connections (  int indx )
{
    string signal_1, signal_2;

    netlist << endl;

    //netlist << "\t" << "ap_ready <= '1';" << endl;
    
    if ( indx == 0 ) // Top-level module
    {
        for (int i = 0; i < components_in_netlist; i++ ) 
        {
            netlist << endl;
            
            netlist << "\t" << nodes[i].name << UNDERSCORE  << "clk" << " <= " << "clk" <<  SEMICOLOUMN << endl;
            netlist << "\t" << nodes[i].name << UNDERSCORE  << "rst" << " <= " << "rst" <<  SEMICOLOUMN << endl;
            
            
            
            if ( nodes[i].type == "MC" )
            {
                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "ce0";
                
                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 +="we0_ce0";
                    
                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;                

                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "we0";

                
                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 +="we0_ce0";
                    
                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;                
            }

                    //LSQ-MC Modifications
            if ( nodes[i].type == "LSQ" )
            {
                bool mc_lsq = false; 

                for ( int indx = 0; indx < nodes[i].inputs.size; indx++ )
                {
                    //cout << nodes[i].name << " input " << indx << " type " << nodes[i].inputs.input[indx].type << endl;  
                    if ( nodes[i].inputs.input[indx].type == "x" )
                    {
                        // if x port exists, lsq is connected to mc and not to memory directly
                        mc_lsq = true;

                        // - for the port x0d: 
                        // LSQ_x_din1 <= LSQ_x_dataInArray_4;
                        // LSQ_x_readyArray_4 <= '1';

                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += "din1";
                
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += DATAIN_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string( indx );
                    
                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     
                        
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += READY_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );

                
                        signal_2 = "'1'";
                        
                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     
                        
                        
 
                    }
                    //if ( nodes[i].inputs.input[indx].type == "y" )
                    {
                        
                    }
                }   

                if (!mc_lsq) {
                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += "din1";

                    signal_2 = nodes[i].memory;
                    signal_2 += UNDERSCORE;
                    signal_2 += "din1";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  

                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += "store_ready";

                    signal_2 = "'1'";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl; 

                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += "load_ready";

                    signal_2 = "'1'";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl; 
                }  

                for ( int indx = 0; indx < nodes[i].outputs.size; indx++ )
                {
                    //cout << nodes[i].name << " output " << indx << " type " << nodes[i].outputs.output[indx].type << endl;  

                    if ( nodes[i].outputs.output[indx].type == "x" )
                    {
                        //- for the port x0a, check the index (in this case, it's out3) and build a load address interface as follows:                        
                        // LSQ_x_load_ready <= LSQ_x_nReadyArray_2;
                        // LSQ_x_dataOutArray_2 <= LSQ_x_address1;
                        // LSQ_x_validArray_2 <= LSQ_x_ce1;

                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += "load_ready";
                
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += NREADY_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string( indx );
                    
                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     
                        
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += DATAOUT_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );

                
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += "address1";

                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     
                        
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += VALID_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );

                
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += "ce1";

                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     
                        
 
                    }else
                    if ( nodes[i].outputs.output[indx].type == "y" )
                    {
                        
                        if ( nodes[i].outputs.output[indx].info_type == "a")
                        {
                            // 
                            // - for the port y0a:
                            // LSQ_x_validArray_3 <= LSQ_x_we0_ce0;
                            // LSQ_x_store_ready <= LSQ_x_nReadyArray_3;
                            // LSQ_x_dataOutArray_3 <= LSQ_x_address0;
                            signal_1 = nodes[i].name;
                            signal_1 += UNDERSCORE;
                            signal_1 += VALID_ARRAY;
                            signal_1 += UNDERSCORE;
                            signal_1 += to_string( indx );

                    
                            signal_2 = nodes[i].name;
                            signal_2 += UNDERSCORE;
                            signal_2 += "we0_ce0";

                            netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     

                            signal_1 = nodes[i].name;
                            signal_1 += UNDERSCORE;
                            signal_1 += "store_ready";

                    
                            signal_2 = nodes[i].name;
                            signal_2 += UNDERSCORE;
                            signal_2 += NREADY_ARRAY;
                            signal_2 += UNDERSCORE;
                            signal_2 += to_string( indx );

                            netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     

                            
                            signal_1 = nodes[i].name;
                            signal_1 += UNDERSCORE;
                            signal_1 += DATAOUT_ARRAY;
                            signal_1 += UNDERSCORE;
                            signal_1 += to_string( indx );
                            
                    
                            signal_2 = nodes[i].name;
                            signal_2 += UNDERSCORE;
                            signal_2 += "address0";


                            netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     

                            
                        }
                        else
                        if ( nodes[i].outputs.output[indx].info_type == "d")
                        {
                            // 
                            // - for the port y0d: 
                            // LSQ_x_validArray_4 <= LSQ_x_we0_ce0;
                            // LSQ_x_dataOutArray_4 <= LSQ_x_dout0;
                            signal_1 = nodes[i].name;
                            signal_1 += UNDERSCORE;
                            signal_1 += VALID_ARRAY;
                            signal_1 += UNDERSCORE;
                            signal_1 += to_string( indx );

                    
                            signal_2 = nodes[i].name;
                            signal_2 += UNDERSCORE;
                            signal_2 += "we0_ce0";

                            netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     

                            signal_1 = nodes[i].name;
                            signal_1 += UNDERSCORE;
                            signal_1 += DATAOUT_ARRAY;
                            signal_1 += UNDERSCORE;
                            signal_1 += to_string( indx );
                            
                    
                            signal_2 = nodes[i].name;
                            signal_2 += UNDERSCORE;
                            signal_2 += "dout0";
                            
                            netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;     

                            
                        }

                        
                    }
                   



                }

                if (!mc_lsq) {

                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "address1";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "address1";
                    
                    //netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  
                    
                    netlist << "\t" << signal_1 << " <= std_logic_vector (resize(unsigned(" << signal_2 << ")," << signal_1 << "'length))"<<SEMICOLOUMN << endl;


                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "ce1";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "ce1";
                    
                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  

                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "address0";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "address0";

                    //netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  
                    netlist << "\t" << signal_1 << " <= std_logic_vector (resize(unsigned(" << signal_2 << ")," << signal_1 << "'length))"<<SEMICOLOUMN << endl;


                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "ce0";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "we0_ce0";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  

                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "we0";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "we0_ce0";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  

                    signal_1 = nodes[i].memory;
                    signal_1 += UNDERSCORE;
                    signal_1 += "dout0";

                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 += "dout0";

                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;  
                }      
        }

            if ( nodes[i].type == "Entry" )
            {

                if ( ! ( nodes[i].name.find("start") != std::string::npos ) ) // If not start 
                if ( ! ( nodes[i].component_control ) )
                {            
                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += DATAIN_ARRAY;
                    signal_1 += UNDERSCORE;
                    signal_1 += "0";

                
                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 +="din";
                    
                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;                
                }
                
                signal_1 = nodes[i].name;
                signal_1 += UNDERSCORE;
                signal_1 += PVALID_ARRAY;
                signal_1 += UNDERSCORE;
                signal_1 += "0";

            
                //signal_2 = "ap_start";
                
//                 signal_2 = nodes[i].name;
//                 signal_2 += UNDERSCORE;
//                 signal_2 +="valid_in";

                signal_2 = "start_valid";

                netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;

                
                if ( ( nodes[i].name.find("start") != std::string::npos ) )
                {
                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += READY_ARRAY;
                    signal_1 += UNDERSCORE;
                    signal_1 += "0";

                
                    //signal_2 = "ap_start";
                    
    //                 signal_2 = nodes[i].name;
    //                 signal_2 += UNDERSCORE;
    //                 signal_2 +="valid_in";

                    signal_2 = "start_ready";

                    netlist << "\t"  << signal_2  << " <= " << signal_1 << SEMICOLOUMN << endl;
                }
                
                
            }
            
            
            if ( nodes[i].type == "Exit" )
            {
                
                signal_1 = "end_valid"; 

                signal_2 = nodes[i].name;
                signal_2 += UNDERSCORE;
                signal_2 += VALID_ARRAY;
                
            
                netlist << "\t"  << signal_1 << " <= " << signal_2 << UNDERSCORE << indx <<SEMICOLOUMN << endl;

                signal_1 = "end_out"; 

                signal_2 = nodes[i].name;
                signal_2 += UNDERSCORE;
                signal_2 += DATAOUT_ARRAY;
                
            
                netlist << "\t"  << signal_1 << " <= " << signal_2 << UNDERSCORE << indx <<SEMICOLOUMN << endl;
                
  
                signal_1 = nodes[i].name;
                signal_1 += UNDERSCORE;
                signal_1 += NREADY_ARRAY;
                signal_1 += UNDERSCORE;
                signal_1 += to_string(indx);
            
                signal_2 = "end_ready"; 

                netlist << "\t"  << signal_1 << " <= " << signal_2 <<SEMICOLOUMN << endl;
  
                
            }
            
            for ( int indx = 0; indx < nodes[i].outputs.size; indx++ )
            {
                    if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND ) //if Unconnected, skip the signal
                    {
                
                        signal_1 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_1 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += PVALID_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( nodes[i].outputs.output[indx].next_nodes_port );

                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += VALID_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string( indx );;

                    
                        netlist << "\t"  << signal_1 << " <= " << signal_2 << SEMICOLOUMN << endl;

                    }
                    
                    if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND )
                    {
 

                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += NREADY_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );
                    
                        signal_2 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_2 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += READY_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string( nodes[i].outputs.output[indx].next_nodes_port );

                        //outFile << "\t"  << signal_1 << nodes[i].outputs.output[indx].next_nodes_port << " <= " << signal_2 << UNDERSCORE << indx <<SEMICOLOUMN << endl;
                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;

                        
                    }
                    
                    
                //for ( int in_port_indx = 0; in_port_indx < components_type[nodes[i].component_type].in_ports; in_port_indx++) 
                for ( int in_port_indx = 0; in_port_indx < 1; in_port_indx++) 
                {

                    if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND )
                    {

                        signal_1 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_1 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += components_type[0].in_ports_name_str[in_port_indx];
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( nodes[i].outputs.output[indx].next_nodes_port );
                        
                        
                        //inverted
                        string inverted;
                        
                        if ( nodes[nodes[i].outputs.output[indx].next_nodes_id].inputs.input[nodes[i].outputs.output[indx].next_nodes_port].type == "i" )
                        {
                            inverted = "not "; //inverted
                        }
                        else
                        {
                            inverted = "";
                        }

                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += components_type[0].out_ports_name_str[in_port_indx];
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string(indx);
                    
                        if ( nodes[nodes[i].outputs.output[indx].next_nodes_id].type.find("Constant") != std::string::npos ) // Overwrite predecessor with constant value
                        {
                            signal_2 = "\"";
                            signal_2 += string_constant ( nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value , nodes[nodes[i].outputs.output[indx].next_nodes_id].inputs.input[0].bit_size );
                            signal_2 += "\"";                        
                        netlist << "\t" << signal_1  << " <= " << signal_2 <<SEMICOLOUMN << endl;
                    }
                        else
                        {
                            netlist << "\t" << signal_1 << " <= " << inverted << "std_logic_vector (resize(unsigned(" << signal_2 << ")," << signal_1 << "'length))"<<SEMICOLOUMN << endl;
                        }
                    }
                }
            }
        }        
    }
    else
    {
        
        for (int i = 0; i < components_in_netlist; i++ ) 
        {
            netlist << endl;
            
            netlist << "\t" << nodes[i].name << UNDERSCORE  << "clk" << " <= " << "clk" <<  SEMICOLOUMN << endl;
            netlist << "\t" << nodes[i].name << UNDERSCORE  << "rst" << " <= " << "rst" <<  SEMICOLOUMN << endl;
                
            
            if ( nodes[i].type == "Entry" )
            {

                if ( ! ( nodes[i].name.find("start") != std::string::npos ) ) // If not start 
                if ( ! ( nodes[i].component_control ) )
                {            
                    signal_1 = nodes[i].name;
                    signal_1 += UNDERSCORE;
                    signal_1 += DATAIN_ARRAY;
                    signal_1 += UNDERSCORE;
                    signal_1 += "0";

                
                    signal_2 = nodes[i].name;
                    signal_2 += UNDERSCORE;
                    signal_2 +="data";
                    
                    netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;                
                }
                
                signal_1 = nodes[i].name;
                signal_1 += UNDERSCORE;
                signal_1 += PVALID_ARRAY;
                signal_1 += UNDERSCORE;
                signal_1 += "0";

            
                signal_2 = "ap_start";

                netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;
    
            }
            
            
            for ( int indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                    cout << nodes[i].name <<  "prev_node_id" << nodes[i].inputs.input[indx].prev_nodes_id << endl;
                    if ( nodes[i].inputs.input[indx].prev_nodes_id == COMPONENT_NOT_FOUND )
                    {
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += DATAIN_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );
                        
                        signal_2 = DATAIN_ARRAY;
                        signal_2 += "(";
                        signal_2 += to_string(i);
                        signal_2 += ")";
   
                        
                        netlist << "\t"  << signal_1 << " <= " << signal_2 << SEMICOLOUMN << endl;
                        
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += PVALID_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );
                        
                        signal_2 = PVALID_ARRAY;
                        signal_2 += "(";
                        signal_2 += to_string(i);
                        signal_2 += ")";
   
                        
                        netlist << "\t"  << signal_1 << " <= " << signal_2 << SEMICOLOUMN << endl;
                                                
                        signal_2 = READY_ARRAY;
                        signal_2 += "(";
                        signal_2 += to_string(i);
                        signal_2 += ")";
   
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += PVALID_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );

                        
                        netlist << "\t"  << signal_2 << " <= " << signal_1 << SEMICOLOUMN << endl;
                        
                            
                    }
            }
            
            for ( int indx = 0; indx < nodes[i].outputs.size; indx++ )
            {
                    if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND ) //if Unconnected, skip the signal
                    {
                
                        signal_1 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_1 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += PVALID_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( nodes[i].outputs.output[indx].next_nodes_port );

                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += VALID_ARRAY;
                    
                        netlist << "\t"  << signal_1 << " <= " << signal_2 << UNDERSCORE << indx <<SEMICOLOUMN << endl;

//                     }
//                     
//                     if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND )
//                     {
                    
                        signal_1 = nodes[i].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += NREADY_ARRAY;
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( indx );

                    
                        signal_2 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_2 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += READY_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string( nodes[i].outputs.output[indx].next_nodes_port );

                        //outFile << "\t"  << signal_1 << nodes[i].outputs.output[indx].next_nodes_port << " <= " << signal_2 << UNDERSCORE << indx <<SEMICOLOUMN << endl;
                        netlist << "\t"  << signal_1  << " <= " << signal_2 << SEMICOLOUMN << endl;

                        
                    }                    
                    
                //for ( int in_port_indx = 0; in_port_indx < components_type[nodes[i].component_type].in_ports; in_port_indx++) 
                for ( int in_port_indx = 0; in_port_indx < 1; in_port_indx++) 
                {

                    if ( nodes[i].outputs.output[indx].next_nodes_id != COMPONENT_NOT_FOUND )
                    {

                        signal_1 = nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        //signal_1 = (nodes[i].outputs.output[indx].next_nodes_id == COMPONENT_NOT_FOUND ) ? "unconnected" : nodes[nodes[i].outputs.output[indx].next_nodes_id].name;
                        signal_1 += UNDERSCORE;
                        signal_1 += components_type[0].in_ports_name_str[in_port_indx];
                        signal_1 += UNDERSCORE;
                        signal_1 += to_string( nodes[i].outputs.output[indx].next_nodes_port );

                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += components_type[0].out_ports_name_str[in_port_indx];
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string(indx);
                    
                        if ( nodes[nodes[i].outputs.output[indx].next_nodes_id].type.find("Constant") != std::string::npos ) // Overwrite predecessor with constant value
                        {
                            //X"00000000"
                            signal_2 = "X\"";
                            stringstream ss; //= to_string(  nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value );
                            //cout << " ******size" << nodes[nodes[i].outputs.output[indx].next_nodes_id].outputs.output[0].bit_size / 4<< endl;
                            //ss << setfill('0') << setw(8) << hex << nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value;
                            int fill_value = 8;
                            fill_value = nodes[nodes[i].outputs.output[indx].next_nodes_id].outputs.output[0].bit_size / 4 ;
                            ss << setfill('0') << setw( fill_value ) << hex << nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value;
                            string val = ss.str();
                            signal_2 += val;
                            signal_2 += "\"";                        
                        }
                        netlist << "\t" << signal_1  << " <= " << signal_2 <<SEMICOLOUMN << endl;
                    }
                    else
                    {
                        
                        signal_1 = DATAOUT_ARRAY; 
                        signal_1 += "(";
                        signal_1 += "0";
                        signal_1 += ")";
                        
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += components_type[0].out_ports_name_str[in_port_indx];
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string(indx);
                        
                        netlist << "\t" << signal_1 << " <= " << signal_2 <<SEMICOLOUMN << endl;

                        signal_1 = VALID_ARRAY; 
                        signal_1 += "(";
                        signal_1 += "0";
                        signal_1 += ")";
                        
                        signal_2 = nodes[i].name;
                        signal_2 += UNDERSCORE;
                        signal_2 += VALID_ARRAY;
                        signal_2 += UNDERSCORE;
                        signal_2 += to_string(indx);
                        
                        netlist << "\t" << signal_1 << " <= " << signal_2 <<SEMICOLOUMN << endl;
                        
                        
                    }
                }
            }
            
        }
    }
}

string get_component_entity ( string component, int component_id )
{
    string component_entity;
        
    
    for (int indx = 0; indx < ENTITY_MAX; indx++)
    {
        //cout  << "component_id" << component_id << "component "<< component << " " << component_types[indx] << endl;
        if ( component.compare(component_types[indx]) == 0 )
        {
            component_entity =  entity_name[indx];
            break;
        }
    }
        
//     if ( component_entity == ENTITY_BUF && nodes[component_id].slots == 1 )
//     {
//         if ( nodes[component_id].trasparent )
//         {
//             component_entity = "TEHB";
//         }
//         else
//         {
//             component_entity = "OEHB";
//         }
//     }

    //cout << "component_id" << component_id << "component_entity" << component_entity;
    return component_entity;
}


int get_memory_inputs ( int node_id )
{
    
    int memory_inputs = nodes[node_id].inputs.size;
    for ( int indx = 0; indx < nodes[node_id].inputs.size; indx++ )
    {
        if ( nodes[node_id].inputs.input[indx].type != "e" )
        {
            memory_inputs--;
        }
    }
    
    return memory_inputs;
}

string get_generic ( int node_id )
{
    string generic;

    if ( nodes[node_id].inputs.input[0].bit_size == 0 )
    {
        nodes[node_id].inputs.input[0].bit_size = 32;
    }
    
    if ( nodes[node_id].outputs.output[0].bit_size == 0)
    {
        nodes[node_id].outputs.output[0].bit_size = 32;
    }
    
    if ( nodes[node_id].type.find("Branch") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }
    if (nodes[node_id].type.find(COMPONENT_DISTRIBUTOR) != std::string::npos)
    {
        //INPUTS
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        //OUTPUTS
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        //COND_SIZE
        generic += to_string((int)ceil(log2(nodes[node_id].outputs.size)));
        generic += COMMA;
        //DATA_SIZE_IN
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        //DATA_SIZE_OUT
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }
    if ( nodes[node_id].type.find("Buf") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    
    
    if ( nodes[node_id].type.find("Merge") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Fork") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("LazyFork") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Constant") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Operator") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        if ( nodes[node_id].component_operator.find("select") != std::string::npos )
        {
            generic += to_string(nodes[node_id].inputs.input[1].bit_size);
        }
	// Lana 9.6.2021 Sizes for memory address ports
        else if ( nodes[node_id].component_operator.find("mc_load_op") != std::string::npos 
            || nodes[node_id].component_operator.find("mc_store_op") != std::string::npos 
            || nodes[node_id].component_operator.find("lsq_load_op") != std::string::npos 
            || nodes[node_id].component_operator.find("lsq_store_op") != std::string::npos )
        {
            generic += to_string(nodes[node_id].inputs.input[1].bit_size);
        }
        else
        {
            generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        }
        
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
//         if ( nodes[node_id].component_operator == COMPONENT_READ_MEMORY || nodes[node_id].component_operator == COMPONENT_WRITE_MEMORY )
//         {
//             generic += COMMA;
//             generic += to_string(nodes[node_id].outputs.output[0].bit_size);
//         }
        
        if ( nodes[node_id].component_operator.find("getelementptr_op") != std::string::npos )
        {
            generic += COMMA;
            generic += to_string(nodes[node_id].constants);
        }
        
    }    

    if ( nodes[node_id].type.find("Entry") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Exit") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size - get_memory_inputs ( node_id ));
        generic += COMMA;
        generic += to_string( get_memory_inputs ( node_id ) );
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;

#if 0
        int size_max = 0;
        for ( int indx = 0; indx < nodes[node_id].inputs.size; indx++ )
        {
            if ( nodes[node_id].inputs.input[indx].bit_size > size_max )
            {
                size_max = nodes[node_id].inputs.input[indx].bit_size;
            }
        }
        generic += to_string(size_max);
#endif

        //generic += to_string(nodes[node_id].inputs.input[nodes[node_id].inputs.size].bit_size);
        generic += to_string(nodes[node_id].inputs.input[nodes[node_id].inputs.size-1].bit_size);
        
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Sink") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    
    
    if ( nodes[node_id].type.find("Source") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Fifo") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].slots);
    } 

    if ( nodes[node_id].type.find("nFifo") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].slots);
    }   

    if ( nodes[node_id].type.find("tFifo") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].slots);
    }      

    if ( nodes[node_id].type.find("TEHB") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    
    if ( nodes[node_id].type.find("OEHB") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
    }    

    if ( nodes[node_id].type.find("Mux") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[1].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size); // condition size inputs.input[input_indx].type
        /*
         *         
         * generic += COMMA;

        if ( nodes[node_id].inputs.input[1].type == "i" ) 
            generic += "1"; // input is inverted
        else
            generic += "0"; // input is not inverted
        generic += COMMA;
        if ( nodes[node_id].inputs.input[2].type == "i" ) 
            generic += "1"; // input is inverted
        else
            generic += "0"; // input is not inverted
        */
        
    }    

    if ( nodes[node_id].type.find("Inj") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[1].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size); // condition size inputs.input[input_indx].type
        /*
         *         
         * generic += COMMA;

        if ( nodes[node_id].inputs.input[1].type == "i" ) 
            generic += "1"; // input is inverted
        else
            generic += "0"; // input is not inverted
        generic += COMMA;
        if ( nodes[node_id].inputs.input[2].type == "i" ) 
            generic += "1"; // input is inverted
        else
            generic += "0"; // input is not inverted
        */
        
    }    

    if ( nodes[node_id].type.find("CntrlMerge") != std::string::npos )
    {
        generic = to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[1].bit_size); // condition size
    }    

    if ( nodes[node_id].type.find("MC") != std::string::npos )
    {
        generic += to_string(nodes[node_id].data_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].address_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].bbcount);
        generic += COMMA;
        generic += to_string(nodes[node_id].load_count);
        generic += COMMA;
        generic += to_string(nodes[node_id].store_count);


    }    

    if (nodes[node_id].type.find("Selector") != std::string::npos)
    {

    //INPUTS : integer 
    //OUTPUTS : integer
    //COND_SIZE : integer
    //DATA_SIZE_IN: integer
    //DATA_SIZE_OUT: integer
    //
    //AMOUNT_OF_BB_IDS: integer
    //AMOUNT_OF_SHARED_COMPONENTS: integer
    //BB_ID_INFO_SIZE : integer
    //BB_COUNT_INFO_SIZE : integer

        
        int amount_of_bbs = nodes[node_id].orderings.size();
        int bb_id_info_size = amount_of_bbs <= 1 ? 1 : (int)ceil(log2(amount_of_bbs));
        int max_shared_components = -1;
        for(auto ordering_per_bb : nodes[node_id].orderings){
            int size = ordering_per_bb.size();
            if(max_shared_components < size){
                max_shared_components = size;
            }
        }
        int bb_count_info_size = max_shared_components <= 1 ? 1 : ceil(log2(max_shared_components));

        //INPUTS
        generic += to_string(nodes[node_id].inputs.size - amount_of_bbs);
        generic += COMMA;
        //OUTPUTS
        generic += to_string(nodes[node_id].outputs.size);
        generic += COMMA;
        //COND_SIZE
        generic += to_string(nodes[node_id].outputs.output[nodes[node_id].outputs.size - 1].bit_size);
        generic += COMMA;
        //DATA_SIZE_IN
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        //DATA_SIZE_OUT
        generic += to_string(nodes[node_id].outputs.output[0].bit_size);
        generic += COMMA;
        //AMOUNT_OF_BB_IDS
        generic += to_string(nodes[node_id].orderings.size());
        generic += COMMA;
        //AMOUNT_OF_SHARED_COMPONENTS
        generic += to_string(max_shared_components);
        generic += COMMA;
        //BB_ID_INFO_SIZE
        generic += to_string(bb_id_info_size);
        generic += COMMA;
        //BB_COUNT_INFO_SIZE
        generic += to_string(bb_count_info_size);
    }
    else if (nodes[node_id].type.find("SEL") != std::string::npos)
    {
        generic += to_string(nodes[node_id].inputs.size);
        generic += COMMA;
        // TODO change hardcoded number of groups
        // TODO change to number of groups
        generic += to_string(2);
        generic += COMMA;
        generic += to_string(nodes[node_id].inputs.input[0].bit_size);
        generic += COMMA;
        generic += to_string(nodes[node_id].outputs.output[nodes[node_id].outputs.size - 1].bit_size);
    }

    return generic;
}

void write_components ( )
{
    string entity="";
    string generic="";
    string input_port="";
    string input_signal="";
    string output_port="";
    string output_signal="";
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        
        netlist << endl;
        
        entity = nodes[i].name;
        entity += ": entity work.";
        if ( nodes[i].type == "Operator" )
        {
            entity += nodes[i].component_operator;
        }
        else if ( nodes[i].type == "LSQ" )
        {
            entity = "c_"+nodes[i].name + ":" + nodes[i].name;
        }
        else
        {
            entity += get_component_entity ( nodes[i].component_operator, i );
        }
        
        
        if ( nodes[i].type != "LSQ" )
        {
            entity += "(arch)";

            generic = " generic map (";
            
            //generic += get_generic ( nodes[i].node_id );
            generic += get_generic ( i );
            
            generic += ")";
            
            netlist << entity << generic << endl;
        }
        else
        {
            netlist << entity << endl;
        }
        
        netlist << "port map (" << endl;

        if ( nodes[i].type != "LSQ" )
        {        
            netlist << "\t" << "clk => " << nodes[i].name << "_clk";
            netlist << COMMA << endl<< "\t" << "rst => " << nodes[i].name << "_rst";
        }
        else
        {
            netlist << "\t" << "clock => " << nodes[i].name << "_clk";
            netlist << COMMA << endl<< "\t" << "reset => " << nodes[i].name << "_rst";
            
            // Andrea 20200117 Added to be compatible with chisel LSQ
            netlist << "," << endl;
//            netlist << "\t" << "io_memIsReadyForLoads => '1' ," << endl;
//            netlist << "\t" << "io_memIsReadyForStores => '1' ";
            netlist << "\t" << "io_memIsReadyForLoads => " <<  nodes[i].name << "_load_ready" << COMMA << endl;
            netlist << "\t" << "io_memIsReadyForStores => " <<  nodes[i].name << "_store_ready";
            
        }
        int indx = 0;

        
//         if ( nodes[i].type == "Entry" )
//         {
//                 netlist << "\t" << "ap_start" << " => " << "ap_start" << COMMA << endl;
// 
//                 //Write the Ready ports
//                 input_port = "elastic_start";
//                 
//                 input_signal = nodes[i].name;
//                 input_signal += UNDERSCORE;
//                 input_signal += VALID_ARRAY;
//                 input_signal += UNDERSCORE;
//                 input_signal += "0";
// 
//                 
//                 netlist << "\t" << VALID_ARRAY << " => " << input_signal << COMMA << endl;
// 
//                 //Write the Ready ports
//                 input_port = "elastic_start";
//                 
//                 input_signal = nodes[i].name;
//                 input_signal += UNDERSCORE;
//                 input_signal += NREADY_ARRAY;
//                 input_signal += UNDERSCORE;
//                 input_signal += "0";
//                 netlist << "\t" << NREADY_ARRAY << " => " << input_signal << endl;            
//         }
//         else
        if ( nodes[i].type == "LSQ" || nodes[i].type == "MC" )
        {
            
                static int load_indx = 0;
                load_indx = 0;
                
                static int store_add_indx = 0;
                static int store_data_indx = 0;
                store_add_indx = 0;
                store_data_indx = 0;
                
                for ( int lsq_indx = 0; lsq_indx < nodes[i].inputs.size; lsq_indx++ )
                {    
                    //cout << nodes[i].name << "LSQ input "<< lsq_indx << " = " << nodes[i].inputs.input[lsq_indx].type << " port = " << nodes[i].inputs.input[lsq_indx].port << " info_type = " <<nodes[i].inputs.input[lsq_indx].info_type << endl;     
                }

                for ( int lsq_indx = 0; lsq_indx < nodes[i].outputs.size; lsq_indx++ )
                {    
                    //cout << nodes[i].name << "LSQ output "<< lsq_indx << " = " << nodes[i].outputs.output[lsq_indx].type << " port = " << nodes[i].outputs.output[lsq_indx].port << " info_type = " <<nodes[i].outputs.output[lsq_indx].info_type << endl;     
                }

                netlist << "," << endl;
                
                if ( nodes[i].type == "LSQ" )
                {
                    input_signal = nodes[i].name;
                }
                else
                {
                    input_signal = nodes[i].memory;
                }
                input_signal += UNDERSCORE;
                input_signal += "dout0";
                input_signal += COMMA;
                
                netlist << "\t" << "io_storeDataOut" << " => "   << input_signal << endl;

                if ( nodes[i].type == "LSQ" )
                {
                    input_signal = nodes[i].name;
                }
                else
                {
                    input_signal = nodes[i].memory;
                }
                input_signal += UNDERSCORE;
                input_signal += "address0";
                input_signal += COMMA;

                netlist << "\t" << "io_storeAddrOut" << " => "  << input_signal << endl;
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += "we0_ce0";
                input_signal += COMMA;

                netlist << "\t" << "io_storeEnable"<< " => "  << input_signal << endl;
                
                if ( nodes[i].type == "LSQ" )
                {
                    input_signal = nodes[i].name;
                }
                else
                {
                    input_signal = nodes[i].memory;
                }
                input_signal += UNDERSCORE;
                input_signal += "din1";
                input_signal += COMMA;

                netlist << "\t" << "io_loadDataIn" << " => "  << input_signal << endl;
                
                if ( nodes[i].type == "LSQ" )
                {
                    input_signal = nodes[i].name;
                }
                else
                {
                    input_signal = nodes[i].memory;
                }
                input_signal += UNDERSCORE;
                input_signal += "address1";
                input_signal += COMMA;

                netlist << "\t" << "io_loadAddrOut"<< " => "  << input_signal  << endl;
                
                if ( nodes[i].type == "LSQ" )
                {
                    input_signal = nodes[i].name;
                }
                else
                {
                    input_signal = nodes[i].memory;
                }
                input_signal += UNDERSCORE;
                input_signal += "ce1";
                //input_signal += COMMA;
                
                netlist << "\t" << "io_loadEnable" << " => "  << input_signal;

                

            string bbReadyPrev = "";
            string bbValidPrev = "";
            string bbCountPrev = "";
            string rdReadyPrev = "";
            string rdValidPrev = "";
            string rdBitsPrev = "";
            string stAdReadyPrev = "";
            string stAdValidPrev = "";
            string stAdBitsPrev = "";
            string stDataReadyPrev = "";
            string stDataValidPrev = "";
            string stDataBitsPrev = "";

            netlist << COMMA << endl;
            for ( int lsq_indx = 0; lsq_indx < nodes[i].inputs.size; lsq_indx++ )
            {    
                //cout << nodes[i].name; 
                //cout << " LSQ input "<< lsq_indx << " = " << nodes[i].inputs.input[lsq_indx].type << "port = " << nodes[i].inputs.input[lsq_indx].port << "info_type = " <<nodes[i].inputs.input[lsq_indx].info_type << endl;   
                
                
                //if ( nodes[i].inputs.input[lsq_indx].type == "c" || (nodes[i].bbcount-- > 0 ) )
                if ( nodes[i].inputs.input[lsq_indx].type == "c" )
                {
                    //netlist << COMMA << endl;
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "bbpValids";
                    //input_port += UNDERSCORE;
                    if ( nodes[i].type == "MC" )  
                    { 
                        input_port +="("; 
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port); 
                        input_port +=")"; 
                        
                    } 
                    else 
                    { 
                        input_port += UNDERSCORE; 
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port); 
                        
                    }
                    
                    if ( nodes[i].inputs.input[lsq_indx].info_type == "fake" ) //Andrea 20200128 Try to force 0 to inputs.
                    {
                        input_signal = "'0',";
                    }
                    else
                    {
                        input_signal = nodes[i].name;
                        input_signal += UNDERSCORE;
                        input_signal += PVALID_ARRAY;
                        input_signal += UNDERSCORE;
                        input_signal += to_string(lsq_indx);
                        input_signal += COMMA;
                    }
                    //netlist << "\t" << input_port << " => "  << input_signal << endl;
                    bbValidPrev += "\t" + input_port + " => "  + input_signal + "\n";

                    
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "bbReadyToPrevs";
                    //input_port += UNDERSCORE;
                if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += READY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    //netlist << "\t" << input_port << " => "  << input_signal << endl;
                    bbReadyPrev += "\t" + input_port + " => "  + input_signal + "\n";

                    
                    if ( nodes[i].type == "MC" )
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "bb_stCountArray";
                        //input_port += UNDERSCORE;
                        if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                        
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "fake" ) //Andrea 20200128 Try to force 0 to inputs.
                        {
                            input_signal = "x\"00000000\",";
                        }
                        else
                        {

                            input_signal = nodes[i].name;
                            input_signal += UNDERSCORE;
                            input_signal += DATAIN_ARRAY;
                            input_signal += UNDERSCORE;
                            input_signal += to_string(lsq_indx);
                            input_signal += COMMA;
                        }
                        
                        //netlist << "\t" << input_port << " => "  << input_signal;
                        bbCountPrev += "\t" + input_port + " => "  + input_signal + "\n";
                    }
                    
                }
                else
                if ( nodes[i].inputs.input[lsq_indx].type == "l" )
                {
                    //netlist << COMMA << endl;
                    //static int load_indx = 0;
                    //io_rdPortsPrev_0_ready"

                    if ( nodes[i].type == "LSQ" )
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "ready";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        input_port += "ready";
                        input_port += "(";
//                    input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);
                        
                        input_port += ")";
                        
                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += READY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    rdReadyPrev += "\t" + input_port + " => "  + input_signal + "\n";
                    //netlist << "\t" << input_port << " => "  << input_signal << endl;

                    
                    if ( nodes[i].type == "LSQ" )
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "valid";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        input_port += "valid";
                        input_port += "(";
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += ")";
                        
                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += PVALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    rdValidPrev += "\t" + input_port + " => "  + input_signal  + "\n";
                    //netlist << "\t" << input_port << " => "  << input_signal << endl;

                    
                    if ( nodes[i].type == "LSQ" )
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "bits";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsPrev";
                        input_port += UNDERSCORE;
                        input_port += "bits";
                        input_port += "(";
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += ")";
                        
                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += DATAIN_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    //input_signal += COMMA;

                    rdBitsPrev += "\t" + input_port + " => "  + input_signal + COMMA + "\n";
                    //netlist << "\t" << input_port << " => "  << input_signal;

                    load_indx++;


                }
                else
                if ( nodes[i].inputs.input[lsq_indx].type == "s" )
                {
                    
                    //netlist << COMMA << endl;
                    //static int store_add_indx = 0;
                    //static int store_data_indx = 0;

                    if ( nodes[i].type == "LSQ" )
                    {
                        //"io_wrAddrPorts_0_ready"
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //input_port += to_string(store_add_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        }
                        else
                        {
                            input_port += "Data";

                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //input_port += to_string(store_data_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        }
                        

                        input_port += UNDERSCORE;
                        input_port += "valid";
                    }
                    else
                    {
                        //"io_wrAddrPorts_0_ready"
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                        }
                        else
                        {
                            input_port += "Data";
                        }
                        
                        input_port += "Ports";
                        input_port += UNDERSCORE;
                        input_port += "valid";
                        input_port += "(";
                        //input_port += to_string(store_data_indx);
                        input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        input_port += ")";


                    }
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += PVALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        stAdValidPrev += "\t" + input_port + " => "  + input_signal + "\n";
                    else
                        stDataValidPrev += "\t" + input_port + " => "  + input_signal + "\n";

                    //netlist << "\t" << input_port << " => "  << input_signal << endl;

                    
                    
                    if ( nodes[i].type == "LSQ" )
                    {

                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                            //input_port += to_string(store_add_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            
                        }
                        else
                        {
                            input_port += "Data";

                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                            //input_port += to_string(store_data_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                        }
                        

                        input_port += UNDERSCORE;
                        input_port += "ready";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            input_port += "ready";
                            input_port += "(";
                            //input_port += to_string(store_add_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            input_port += ")";
                        }
                        else
                        {
                            input_port += "Data";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            input_port += "ready";
                            input_port += "(";
                            //input_port += to_string(store_data_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            input_port += ")";
                        }
                        
                    }
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += READY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        stAdReadyPrev += "\t" + input_port + " => "  + input_signal + "\n";
                    else
                        stDataReadyPrev += "\t" + input_port + " => "  + input_signal + "\n";

                    //netlist << "\t" << input_port << " => "  << input_signal << endl;


                    
                    if ( nodes[i].type == "LSQ" )
                    {
            
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                            //input_port += to_string(store_add_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            store_add_indx++;


                        }
                        else
                        {
                            input_port += "Data";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                            //input_port += to_string(store_data_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            store_data_indx++;

                            
                        }
                        
                        input_port += UNDERSCORE;
                        input_port += "bits";

                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "wr";
                        if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        {
                            input_port += "Addr";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            input_port += "bits";
                            input_port += "(";
                            //input_port += to_string(store_add_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            input_port += ")";
                            store_add_indx++;
                        }
                        else
                        {
                            input_port += "Data";
                            input_port += "Ports";
                            input_port += UNDERSCORE;
                            input_port += "bits";
                            input_port += "(";
                            //input_port += to_string(store_data_indx);
                            input_port += to_string(nodes[i].inputs.input[lsq_indx].port);

                            input_port += ")";
                            store_data_indx++;

                        }
                    }
                    
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += DATAIN_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    //input_signal += COMMA;

                    if ( nodes[i].inputs.input[lsq_indx].info_type == "a" )
                        stAdBitsPrev += "\t" + input_port + " => "  + input_signal + COMMA + "\n";
                    else
                        stDataBitsPrev += "\t" + input_port + " => "  + input_signal + COMMA + "\n";

                    //netlist << "\t" << input_port << " => "  << input_signal;
                    


                }

                            
            }

            netlist << bbReadyPrev;
            netlist << bbValidPrev;
            netlist << bbCountPrev;
            netlist << rdReadyPrev;
            netlist << rdValidPrev;
            netlist << rdBitsPrev;
            netlist << stAdReadyPrev;
            netlist << stAdValidPrev;
            netlist << stAdBitsPrev;
            netlist << stDataReadyPrev;
            netlist << stDataValidPrev;
            netlist << stDataBitsPrev;

            string rdReadyNext = "";
            string rdValidNext = "";
            string rdBitsNext = "";
            string emptyReady = "";
            string emptyValid = "";


            for ( int lsq_indx = 0; lsq_indx < nodes[i].outputs.size; lsq_indx++ )
            {    
                //cout << "LSQ output "<< lsq_indx << " = " << nodes[i].outputs.output[lsq_indx].type << "port = " << nodes[i].outputs.output[lsq_indx].port << "info_type = " <<nodes[i].outputs.output[lsq_indx].info_type << endl;     

                if ( nodes[i].outputs.output[lsq_indx].type == "c" )
                {
                    //LANA REMOVE???
                    netlist << COMMA << endl;
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "bbValids";
                    //input_port += UNDERSCORE;
                if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += VALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    netlist << "\t" << input_port << " => "  << input_signal << endl;

                    
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "bbReadyToNexts";
                    input_port += UNDERSCORE;
                    if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += NREADY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    //input_signal += COMMA;

                    netlist << "\t" << input_port << " => "  << input_signal;

                    
                }
                else
                if ( nodes[i].outputs.output[lsq_indx].type == "l" )
                {
                    //static int load_indx = 0;

                    //netlist << COMMA << endl;

                    if ( nodes[i].type == "LSQ" )
                    {

                        //io_rdPortsPrev_0_ready"
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                        //input_port += to_string(load_indx);
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "ready";
                    }
                    else
                    {
                        //io_rdPortsPrev_0_ready"
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        input_port += "ready";
                        input_port += "(";
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);
                        input_port += ")";
    
                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += NREADY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    //netlist << "\t" << input_port << " => "  << input_signal << endl;
                    rdReadyNext += "\t" + input_port + " => "  + input_signal + "\n";

                    
                    
                    if ( nodes[i].type == "LSQ" )
                    {

                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "valid";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        input_port += "valid";
                        input_port += "(";
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);
                        input_port += ")";
                        
                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += VALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                // netlist << "\t" << input_port << " => "  << input_signal << endl;
                    rdValidNext += "\t" + input_port + " => "  + input_signal + "\n";

                    
                    if ( nodes[i].type == "LSQ" )
                    {

                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);

                        input_port += UNDERSCORE;
                        input_port += "bits";
                    }
                    else
                    {
                        input_port = "io";
                        input_port += UNDERSCORE;
                        input_port += "rdPortsNext";
                        input_port += UNDERSCORE;
                        input_port += "bits";
                        input_port += "(";
                        input_port += to_string(nodes[i].outputs.output[lsq_indx].port);
                        input_port += ")";

                    }
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += DATAOUT_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                // netlist << "\t" << input_port << " => "  << input_signal; 
                    rdBitsNext += "\t" + input_port + " => "  + input_signal + "\n";
                    load_indx++;
                    

                }
                else
                if ( nodes[i].outputs.output[lsq_indx].type == "s" )
                {
                    //LANA REMOVE???
                    netlist << COMMA << endl;
                    static int store_indx = 0;

                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "wrpValids";
                    input_port += UNDERSCORE;
                    //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                    //input_port += to_string(store_indx);
                    input_port += to_string(nodes[i].outputs.output[lsq_indx].port);

                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += VALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    input_signal += COMMA;

                    netlist << "\t" << input_port << " => "  << input_signal << endl;

                    
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "wrReadyToPrevs";
                    input_port += UNDERSCORE;
                    //if ( nodes[i].type == "MC" )  { input_port +="("; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); input_port +=")"; } else { input_port += UNDERSCORE; input_port += to_string(nodes[i].inputs.input[lsq_indx].port); }
                    //input_port += to_string(store_indx);
                    input_port += to_string(nodes[i].outputs.output[lsq_indx].port);

                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += NREADY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    //input_signal += COMMA;

                    netlist << "\t" << input_port << " => "  << input_signal;
                    
                    store_indx++;

                }
                else
                if ( nodes[i].outputs.output[lsq_indx].type == "e" )
                {
                    
                    //netlist << COMMA << endl;
                    static int store_indx = 0;

                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "Empty_Valid";

                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += VALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    
                    if ( nodes[i].type != "LSQ" ) // Andrea 20200117 Added to be compatible with chisel LSQ
                        input_signal += COMMA;

                    //netlist << "\t" << input_port << " => "  << input_signal << endl;
                    emptyValid += "\t" + input_port + " => "  + input_signal + "\n";

                    
                    input_port = "io";
                    input_port += UNDERSCORE;
                    input_port += "Empty_Ready";

                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += NREADY_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(lsq_indx);
                    //input_signal += COMMA;

                    //netlist << "\t" << input_port << " => "  << input_signal; 
                    emptyReady += "\t" + input_port + " => "  + input_signal + "\n";
                    
                    store_indx++;

                }

                            
            }

            netlist << rdReadyNext;
            netlist << rdValidNext;
            netlist << rdBitsNext;
            netlist << emptyValid;
            
            if ( nodes[i].type != "LSQ" ) // Andrea 20200117 Added to be compatible with chisel LSQ
            {
                netlist << emptyReady;
                
            }

//             input_signal = nodes[i].name;
//             input_signal += UNDERSCORE;
//             input_signal += "io_queueEmpty";
//             
// 
//             netlist << "\t" << "io_queueEmpty" << " => " << input_signal << endl;
            
        }
        else
        if ( nodes[i].type == "Exit" )
        {
            
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    input_port = components_type[0].in_ports_name_str[0];
                    input_port += "(";
                    input_port += to_string(indx - get_memory_inputs(i));
                    input_port += ")";
                    
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += components_type[nodes[i].component_type].in_ports_name_str[0];
                    input_signal += UNDERSCORE;
                    input_signal += to_string(indx);
                    netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
                }
            }
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    //Write the Ready ports
                    input_port = PVALID_ARRAY;
                    input_port += "(";
                    input_port += to_string(indx- get_memory_inputs(i));
                    input_port += ")";
                }
                else
                {
                    //Write the Ready ports
                    input_port = "eValidArray";
                    input_port += "(";
                    input_port += to_string(indx);
                    input_port += ")";
                    
                }
                
                //if ( indx == ( nodes[i].inputs.size - 1 ) )
                {
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += PVALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(indx);
                }
                //else
                {
                //    input_signal = "\'0\', --Andrea forced to 0 to run the simulation";
                }
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    //Write the Ready ports
                    input_port = READY_ARRAY;
                    input_port += "(";
                    input_port += to_string(indx- get_memory_inputs(i));
                    input_port += ")";
                }
                else
                {
                    //Write the Ready ports
                    input_port = "eReadyArray";   
                    input_port += "(";
                    input_port += to_string(indx);
                    input_port += ")";

                }
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += READY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string(indx);
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            
            //netlist << COMMA << endl << "\t" << "ap_done" << " => " << "ap_done";
            
                input_port = components_type[0].out_ports_name_str[0];
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += components_type[nodes[i].component_type].out_ports_name_str[0];
                input_signal += UNDERSCORE;
                input_signal += "0";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            

                
                input_port = VALID_ARRAY;
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += VALID_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += "0";

                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = NREADY_ARRAY;
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += NREADY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += "0";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;



            
            
            
            

//             input_signal = nodes[i].name;
//             input_signal += UNDERSCORE;
//             input_signal += "io_queueEmpty";
//             
// 
//             netlist << "\t" << "io_queueEmpty" << " => " << input_signal << endl;
        
        }
        else
        if ( nodes[i].type == "Exit" )
        {
            
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    input_port = components_type[0].in_ports_name_str[0];
                    input_port += "(";
                    input_port += to_string(indx - get_memory_inputs(i));
                    input_port += ")";
                    
                    
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += components_type[nodes[i].component_type].in_ports_name_str[0];
                    input_signal += UNDERSCORE;
                    input_signal += to_string(indx);
                    netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
                }
            }
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    //Write the Ready ports
                    input_port = PVALID_ARRAY;
                    input_port += "(";
                    input_port += to_string(indx- get_memory_inputs(i));
                    input_port += ")";
                }
                else
                {
                    //Write the Ready ports
                    input_port = "eValidArray";
                    input_port += "(";
                    input_port += to_string(indx);
                    input_port += ")";
                    
                }
                
                //if ( indx == ( nodes[i].inputs.size - 1 ) )
                {
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += PVALID_ARRAY;
                    input_signal += UNDERSCORE;
                    input_signal += to_string(indx);
                }
                //else
                {
                //    input_signal = "\'0\', --Andrea forced to 0 to run the simulation";
                }
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                if ( nodes[i].inputs.input[indx].type != "e" )
                {
                    //Write the Ready ports
                    input_port = READY_ARRAY;
                    input_port += "(";
                    input_port += to_string(indx- get_memory_inputs(i));
                    input_port += ")";
                }
                else
                {
                    //Write the Ready ports
                    input_port = "eReadyArray";   
                    input_port += "(";
                    input_port += to_string(indx);
                    input_port += ")";

                }
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += READY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string(indx);
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            
            //netlist << COMMA << endl << "\t" << "ap_done" << " => " << "ap_done";
            
                input_port = components_type[0].out_ports_name_str[0];
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += components_type[nodes[i].component_type].out_ports_name_str[0];
                input_signal += UNDERSCORE;
                input_signal += "0";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            

                
                input_port = VALID_ARRAY;
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += VALID_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += "0";

                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = NREADY_ARRAY;
                input_port += "(";
                input_port += "0";
                input_port += ")";
                
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += NREADY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += "0";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
                

        }
        else
        {
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {
                
    //          for ( int in_port_indx = 0; in_port_indx < components_type[nodes[i].component_type].in_ports; in_port_indx++) 
                for ( int in_port_indx = 0; in_port_indx < 1; in_port_indx++) 
                {
                    if ( ( nodes[i].type.find("Branch") != std::string::npos && indx == 1 ) || ( (nodes[i].component_operator.find("select") != std::string::npos ) && indx == 0 ) || ( (nodes[i].component_operator.find("Mux") != std::string::npos ) && indx == 0 ))
                    {
                            input_port = "Condition(0)"; 
                    }
                    else if (nodes[i].type.find(COMPONENT_DISTRIBUTOR) != std::string::npos && indx == 1)
                    {
                        input_port = "Condition(0)";
                    }
                    else if (nodes[i].type.find("Selector") != std::string::npos && indx >= nodes[i].inputs.size - nodes[i].orderings.size())
                    {
                        input_port = "bbInfoData(";
                        input_port += to_string(indx - (nodes[i].inputs.size - nodes[i].orderings.size()));
                        input_port += ")";
                    }
 		    // Lana 9.6.2021. Changed lsq memory port interface
                    else if ( ( ( nodes[i].component_operator.find("mc_store_op") != std::string::npos ) || ( nodes[i].component_operator.find("mc_load_op") != std::string::npos ) || ( nodes[i].component_operator.find("lsq_store_op") != std::string::npos ) || ( nodes[i].component_operator.find("lsq_load_op") != std::string::npos ) ) && indx == 1 )
                    {
                          input_port = "input_addr";
                    }
                    else
                    {
                        input_port = components_type[0].in_ports_name_str[in_port_indx];
                        input_port += "(";
                        if ( ( nodes[i].component_operator.find("select") != std::string::npos ) || ( (nodes[i].component_operator.find("Mux") != std::string::npos ) ) )
                        {
                            input_port += to_string( indx - 1 );
                        }
                        else
                        {
                            input_port += to_string( indx );
                        }
                        input_port += ")";
                    }
                    
                    /*
                    if ( nodes[i].inputs.input[indx].type == "i" )
                    {
                        input_signal = "not "; //inverted
                        input_signal += nodes[i].name;
                    }
                    else
                        input_signal = nodes[i].name;
                    */
                    input_signal = nodes[i].name;
                    input_signal += UNDERSCORE;
                    input_signal += components_type[nodes[i].component_type].in_ports_name_str[in_port_indx];
                    input_signal += UNDERSCORE;
                    input_signal += to_string( indx );
                    
                    netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
                }
            }
            
            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {

//                 if ( ( nodes[i].component_operator.find("Mux") != std::string::npos ) && indx == 0 )
//                 {
//                     //Write the Ready ports
//                     input_port = "condValid";
//                     input_port += "(";
//                     input_port += to_string( indx );
//                     input_port += ")";
//                     
//                 }
            	if (nodes[i].type.find("Selector") != std::string::npos && indx >= nodes[i].inputs.size - nodes[i].orderings.size())
                {
                    //ctrlForks ports have another name
                    input_port = "bbInfoPValid";
                    input_port += "(";
                    input_port += to_string(indx - (nodes[i].inputs.size - nodes[i].orderings.size()));
                    input_port += ")";
                }

                else
                {
                    //Write the Ready ports
                    input_port = PVALID_ARRAY;
                    input_port += "(";
//                     //if ( ( nodes[i].component_operator.find("select") != std::string::npos ) || ( (nodes[i].component_operator.find("Mux") != std::string::npos ) ) )
//                     {
//                     //    input_port += to_string( indx -1 );
//                     }
//                     //else
                    {
                        input_port += to_string( indx );

                    }
                    input_port += ")";
                }   
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += PVALID_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string( indx );
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }

            for ( indx = 0; indx < nodes[i].inputs.size; indx++ )
            {

//                 if ( ( nodes[i].component_operator.find("Mux") != std::string::npos ) && indx == 0 )
//                 {
//                     //Write the Ready ports
//                     input_port = "condReady";
//                     input_port += "(";
//                     input_port += to_string( indx );
//                     input_port += ")";
//                 }
            	if (nodes[i].type.find("Selector") != std::string::npos && indx >= nodes[i].inputs.size - nodes[i].orderings.size())
                {
                    //ctrlForks ports have another name
                    input_port = "bbInfoReady";
                    input_port += "(";
                    input_port += to_string(indx - (nodes[i].inputs.size - nodes[i].orderings.size()));
                    input_port += ")";
                }
                else
                {
                    //Write the Ready ports
                    input_port = READY_ARRAY;
                    input_port += "(";
//                     if ( ( nodes[i].component_operator.find("select") != std::string::npos ) || ( (nodes[i].component_operator.find("Mux") != std::string::npos ) ) )
//                     {
//                         input_port += to_string( indx -1 );
//                     }
//                     else
                    {
                        input_port += to_string( indx );

                    }
                    input_port += ")";
                }
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += READY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string( indx );
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            
            
            
            //if ( nodes[i].name.find("load") != std::string::npos )
            if ( nodes[i].component_operator == "load_op" )
            {
                input_port = "read_enable";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "read_enable";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = "read_address";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "read_address";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = "data_from_memory";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "data_from_memory";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            
        
            if ( nodes[i].component_operator == "store_op" )
            {
                input_port = "write_enable";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "write_enable";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = "write_address";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "write_address";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;

                input_port = "data_to_memory";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal +=  "data_to_memory";
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }

            
            
            for ( indx = 0; indx < nodes[i].outputs.size; indx++ )
            {            
                
                //Write the Ready ports
                input_port = NREADY_ARRAY;
                input_port += "(";
                input_port += to_string( indx );
                input_port += ")";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += NREADY_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string( indx );
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            for ( indx = 0; indx < nodes[i].outputs.size; indx++ )
            {            
                
                //Write the Ready ports
                input_port = VALID_ARRAY;
                input_port += "(";
                input_port += to_string( indx );
                input_port += ")";
                
                input_signal = nodes[i].name;
                input_signal += UNDERSCORE;
                input_signal += VALID_ARRAY;
                input_signal += UNDERSCORE;
                input_signal += to_string( indx );
                
                netlist << COMMA << endl << "\t" << input_port << " => " << input_signal;
            }
            for ( indx = 0; indx < nodes[i].outputs.size; indx++ )
            {            

                for ( int out_port_indx = 0; out_port_indx < components_type[nodes[i].component_type].out_ports; out_port_indx++) 
                {


                    if ((nodes[i].type.find(COMPONENT_CTRLMERGE) != std::string::npos && indx == 1) ||
                        (nodes[i].type.find(COMPONENT_SEL) != std::string::npos && indx == nodes[i].outputs.size - 1) ||
                        (nodes[i].type.find(COMPONENT_SELECTOR) != std::string::npos && indx == nodes[i].outputs.size - 1))
                    {
                            output_port = "Condition(0)"; 
                    }
                    else if ( ( ( nodes[i].component_operator.find("mc_store_op") != std::string::npos ) || ( nodes[i].component_operator.find("mc_load_op") != std::string::npos ) ) && indx == 1 )
                    {
                            output_port = "output_addr";
                    }
                    // Lana 9.6.2021. Changed lsq memory port interface
                    else if ( ( ( nodes[i].component_operator.find("lsq_store_op") != std::string::npos ) || ( nodes[i].component_operator.find("lsq_load_op") != std::string::npos ) ) && indx == 1 )
                    {
                            output_port = "output_addr";
                    }
                    else
                    {
                        output_port = components_type[nodes[i].component_type].out_ports_name_str[out_port_indx];
                        output_port += "(";
//                         if ( ( nodes[i].component_operator.find("select") != std::string::npos ) || ( (nodes[i].component_operator.find("Mux") != std::string::npos ) ) )
//                         {
//                             output_port += to_string( indx - 1 );
//                         }
//                         else
//                         {
                            output_port += to_string( indx );
//                        }
                        output_port += ")";
                    }


                    output_signal = nodes[i].name;
                    output_signal += UNDERSCORE;
                    output_signal += components_type[nodes[i].component_type].out_ports_name_str[out_port_indx];
                    output_signal += UNDERSCORE;
                    output_signal += to_string( indx );
                    //output_signal += COMMA;
                    
                    //if ( out_port_indx == ( components_type[nodes[i].component_type].out_ports - 1 )  && ( indx == nodes[i].outputs.size -1 ) )
                    //{
                    //    output_signal.erase( remove( output_signal.begin(), output_signal.end(), ',' ), output_signal.end() );
                    //}
                    
                    netlist << COMMA << endl << "\t" << output_port << " => " << output_signal;
                }
            }
            
        } 

        if (nodes[i].type.find("Selector") != std::string::npos)
        {

            int amount_of_bbs = nodes[i].orderings.size();
            int max_shared_components = -1;
            int amount_shared_components = 0;
            for(auto ordering_per_bb : nodes[i].orderings){
                int size = ordering_per_bb.size();
                amount_shared_components += size;
                if(max_shared_components < size){
                    max_shared_components = size;
                }
            }
            int index_size = ceil(log2(amount_shared_components));

            for (int bb_index = 0; bb_index < nodes[i].orderings.size(); ++bb_index){
                for(int comp_index = 0; comp_index < max_shared_components; ++comp_index){
                    input_port = "bbOrderingData";
                    input_port += "(";
                    input_port += to_string(bb_index);
                    input_port += ")(";
                    input_port += to_string(comp_index);
                    input_port += ")";

                    int value;
                    if(comp_index < nodes[i].orderings[bb_index].size()){
                        value = nodes[i].orderings[bb_index][comp_index];
                    }else{
                        value = 0;
                    }
                    input_signal = "\"";
                    input_signal += string_constant(value, index_size);
                    input_signal += "\"";

                    netlist << COMMA << endl
                            << "\t" << input_port << " => " << input_signal;
                }
            }
        }
            netlist << endl << ");" << endl;

    }
}



int get_end_bitsize( void )
{
    int bitsize = 0;
    for ( int i = 0; i < components_in_netlist; i++ ) 
    {
        if ( nodes[i].type == "Exit" ) 
        {
            bitsize = (nodes[i].outputs.output[0].bit_size > 1 ) ? ( nodes[i].outputs.output[0].bit_size - 1 ) : 0;
        }
        
    }
    return bitsize;
}

void write_entity ( string  filename, int indx )
{
    
    string entity = clean_entity ( filename );
    
    string input_port, output_port, input_signal, output_signal, signal;
    if ( indx == 0 ) // Top-level module
    {
        netlist << "entity "<< entity << " is " << endl;
        netlist << "port (" << endl;
        netlist << "\t" << "clk: " << " in std_logic;" << endl;
        netlist << "\t" << "rst: " << " in std_logic;" << endl;
        
        netlist << "\t" << "start_in: " << " in std_logic_vector (0 downto 0);" << endl;
        netlist << "\t" << "start_valid: " << " in std_logic;" << endl;
        netlist << "\t" << "start_ready: " << " out std_logic;" << endl;
        
        netlist << "\t" << "end_out: " << " out std_logic_vector ("<< get_end_bitsize() <<" downto 0);" << endl;
        netlist << "\t" << "end_valid: " << " out std_logic;" << endl;
        netlist << "\t" << "end_ready: " << " in std_logic";

        //netlist << "\t" << "ap_ready: " << " out std_logic";
        
        for (int i = 0; i < components_in_netlist; i++) 
        {
            if ( (nodes[i].name.find("Arg") != std::string::npos) || ( (nodes[i].type.find("Entry") != std::string::npos) && (!(nodes[i].name.find("start") != std::string::npos))) )
            {
                netlist << ";" << endl; 
                netlist << "\t" << nodes[i].name << "_din : in std_logic_vector (31 downto 0);" << endl;
                netlist << "\t" << nodes[i].name << "_valid_in : in std_logic;" << endl;
                netlist << "\t" << nodes[i].name << "_ready_out : out std_logic";
            }

            //if ( nodes[i].name.find("load") != std::string::npos )
            if ( nodes[i].component_operator == "load_op" )
            {
                netlist << ";" << endl; 
                netlist << "\t" << nodes[i].name << "_data_from_memory : in std_logic_vector (31 downto 0);" << endl;
                netlist << "\t" << nodes[i].name << "_read_enable : out std_logic;" << endl;
                netlist << "\t" << nodes[i].name << "_read_address : out std_logic_vector (31 downto 0)";
            }

            //if ( nodes[i].name.find("store") != std::string::npos )
            if ( nodes[i].component_operator == "store_op" )

            {
                netlist << ";" << endl; 
                netlist << "\t" << nodes[i].name << "_data_to_memory : out std_logic_vector (31 downto 0);" << endl;
                netlist << "\t" << nodes[i].name << "_write_enable : out std_logic;" << endl;
                netlist << "\t" << nodes[i].name << "_write_address : out std_logic_vector (31 downto 0)";
            }

                bool mc_lsq = false; 
                if ( nodes[i].type.find("LSQ") != std::string::npos)
                     for ( int indx = 0; indx < nodes[i].inputs.size; indx++ )
                        if ( nodes[i].inputs.input[indx].type == "x" ) {
                            mc_lsq = true;
                            break;
                        }
            
                if ( (nodes[i].type.find("LSQ") != std::string::npos && !mc_lsq) || nodes[i].type.find("MC") != std::string::npos)
                //if ( nodes[i].type.find("MC") != std::string::npos )
                {
                    netlist << ";" << endl; 
                    
//                     netlist << "\t" << nodes[i].memory << "_data_from_memory : in std_logic_vector (31 downto 0);" << endl;
//                     netlist << "\t" << nodes[i].memory << "_read_enable : out std_logic;" << endl;
//                     netlist << "\t" << nodes[i].memory << "_read_address : out std_logic_vector (31 downto 0)";
//                     netlist << ";" << endl; 
//                     netlist << "\t" << nodes[i].memory << "_data_to_memory : out std_logic_vector (31 downto 0);" << endl;
//                     netlist << "\t" << nodes[i].memory << "_write_enable : out std_logic;" << endl;
//                     netlist << "\t" << nodes[i].memory << "_write_address : out std_logic_vector (31 downto 0)";

                    netlist << "\t" << nodes[i].memory << "_address0 : out std_logic_vector (31 downto 0);" << endl;
                    netlist << "\t" << nodes[i].memory << "_ce0 : out std_logic;" << endl;
                    netlist << "\t" << nodes[i].memory << "_we0 : out std_logic;" << endl;
                    netlist << "\t" << nodes[i].memory << "_dout0 : out std_logic_vector (31 downto 0);" << endl;
                    netlist << "\t" << nodes[i].memory << "_din0 : in std_logic_vector (31 downto 0);" << endl;
                    
                    netlist << "\t" << nodes[i].memory << "_address1 : out std_logic_vector (31 downto 0);" << endl;
                    netlist << "\t" << nodes[i].memory << "_ce1 : out std_logic;" << endl;
                    netlist << "\t" << nodes[i].memory << "_we1 : out std_logic;" << endl;
                    netlist << "\t" << nodes[i].memory << "_dout1 : out std_logic_vector (31 downto 0);" << endl;
                    netlist << "\t" << nodes[i].memory << "_din1 : in std_logic_vector (31 downto 0)";
                    
                }

        }
        
        netlist << ");" << endl;
        netlist << "end;" << endl << endl;
    }
    else // Sub-Module module
    {
        netlist << "entity "<< entity << " is " << endl;
        
        netlist << "port (" << endl;

        netlist << "\t" << "clk : in std_logic; " << endl;
        netlist << "\t" << "rst : in std_logic; " << endl;
        netlist << "\t" << "dataInArray : in data_array ( "<< components_in_netlist -1   << " downto 0)(DATA_SIZE_IN-1 downto 0); " << endl;
        netlist << "\t" << "dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0); " << endl;
        netlist << "\t" << "pValidArray : in std_logic_vector ( "<< components_in_netlist -1  << " downto 0); " << endl;
        netlist << "\t" << "nReadyArray : in std_logic_vector ( "<< components_in_netlist -1  << " downto 0); " << endl;
        netlist << "\t" << "validArray : out std_logic_vector ( 0 downto 0); " << endl;
        netlist << "\t" << "readyArray : out std_logic_vector ( "<< components_in_netlist -1 << " downto 0)); " << endl;

//         clk, rst : in std_logic; 
//         dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
//         dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
//         pValidArray : in std_logic_vector(1 downto 0);
//         nReadyArray : in std_logic_vector(0 downto 0);
//         validArray : out std_logic_vector(0 downto 0);
//         readyArray : out std_logic_vector(1 downto 0));



        
        netlist << ");" << endl;
        netlist << "end;" << endl << endl;

    }
}


void write_intro (  )
{
    
    time_t now = time(0);
    char* dt = ctime(&now);

    netlist << "-- ==============================================================" << endl;
    netlist << "-- Generated by Dot2Vhdl ver. " << VERSION_STRING << endl;
    netlist << "-- File created: " << dt << endl;
    netlist << "-- ==============================================================" << endl;

    netlist << "library IEEE; " << endl;
    netlist << "use IEEE.std_logic_1164.all; " << endl;
    netlist << "use IEEE.numeric_std.all; " << endl;
    netlist << "use work.customTypes.all; " << endl;

    netlist << "-- ==============================================================" << endl;

}

void write_lsq_signal(std::string name, bool isInput, std::string type, bool isFinalSignal) {
    netlist << "\t" << name << " : ";
    if (isInput) {netlist << "in ";} else {netlist << "out ";}
    netlist << type;
    if (!isFinalSignal) {netlist << ";";}
    netlist << endl;
}

void write_lsq_declaration ( )
{
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if (nodes[i].type == "LSQ") {
            netlist << endl;
            netlist << "component " << nodes[i].name << endl;
            netlist << "port(" << endl;
            write_lsq_signal("clock", true, "std_logic", false);
            write_lsq_signal("reset", true, "std_logic", false);
            write_lsq_signal("io_memIsReadyForLoads", true, "std_logic", false);
            write_lsq_signal("io_memIsReadyForStores", true, "std_logic", false);
            write_lsq_signal("io_storeDataOut", false, "std_logic_vector("+ to_string(get_lsq_datawidth()-1) +" downto 0)", false);
            //write_lsq_signal("io_storeAddrOut", false, "std_logic_vector("+ to_string(get_lsq_addresswidth()-1) +" downto 0)", false);
            write_lsq_signal("io_storeAddrOut", false, "std_logic_vector("+ to_string(nodes[i].address_size-1) +" downto 0)", false);
            write_lsq_signal("io_storeEnable", false, "std_logic", false);
            

            std::string name = "";
            
            for ( int lsq_indx = 0; lsq_indx < nodes[i].inputs.size; lsq_indx++ ) {
                if ( nodes[i].inputs.input[lsq_indx].type == "c" ) {
                    name = "io_bbpValids_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    write_lsq_signal(name, true, "std_logic", false);
                    name = "io_bbReadyToPrevs_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    write_lsq_signal(name, false, "std_logic", false);

                } else if ( nodes[i].inputs.input[lsq_indx].type == "l" ) {
                    name = "io_rdPortsPrev_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_ready";
                    write_lsq_signal(name, false, "std_logic", false);
                    name = "io_rdPortsPrev_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_valid";
                    write_lsq_signal(name, true, "std_logic", false);
                    name = "io_rdPortsPrev_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_bits";
                    // Lana 9.6.2021. rdPortsPrev is address port, set to address size
                    write_lsq_signal(name, true, "std_logic_vector("+ to_string(nodes[i].address_size-1) +" downto 0)", false);
                } else if (nodes[i].inputs.input[lsq_indx].type == "s") {
                    name = "io_wr";
                    if (nodes[i].inputs.input[lsq_indx].info_type == "a") {
                        name += "AddrPorts_";
                    } else {
                        name += "DataPorts_";
                    }
                    name += to_string(nodes[i].inputs.input[lsq_indx].port) + "_valid";
                    write_lsq_signal(name, true, "std_logic", false);

                    name = "io_wr";
                    if (nodes[i].inputs.input[lsq_indx].info_type == "a") {
                        name += "AddrPorts_";
                    } else {
                        name += "DataPorts_";
                    }
                    name += to_string(nodes[i].inputs.input[lsq_indx].port) + "_ready";
                    write_lsq_signal(name, false, "std_logic", false);

                    name = "io_wr";
                    if (nodes[i].inputs.input[lsq_indx].info_type == "a") {
                        name += "AddrPorts_";
                        name += to_string(nodes[i].inputs.input[lsq_indx].port) + "_bits";
                        //write_lsq_signal(name, true, "std_logic_vector("+ to_string(get_lsq_addresswidth()-1) +" downto 0)", false);
                        write_lsq_signal(name, true, "std_logic_vector("+ to_string(nodes[i].address_size-1) +" downto 0)", false);
                    } else {
                        name += "DataPorts_";
                        name += to_string(nodes[i].inputs.input[lsq_indx].port) + "_bits";
                        write_lsq_signal(name, true, "std_logic_vector("+ to_string(get_lsq_datawidth()-1) + " downto 0)", false);
                    }
                }
            }

            for ( int lsq_indx = 0; lsq_indx < nodes[i].outputs.size; lsq_indx++ ) {
                if ( nodes[i].outputs.output[lsq_indx].type == "c" ) {
                    //name = "io_bbValids_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    name = "io_bbValids_"+to_string(nodes[i].outputs.output[lsq_indx].port);
                    write_lsq_signal(name, false, "std_logic", false);
                    //name = "io_bbReadyToNexts_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    name = "io_bbReadyToNexts_"+to_string(nodes[i].outputs.output[lsq_indx].port);
                    write_lsq_signal(name, false, "std_logic", false);
                } else if ( nodes[i].outputs.output[lsq_indx].type == "l" ) {
                    //name = "io_rdPortsNext_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_ready";
                    name = "io_rdPortsNext_"+to_string(nodes[i].outputs.output[lsq_indx].port) + "_ready";
                    write_lsq_signal(name, true, "std_logic", false);
                    //name = "io_rdPortsNext_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_valid";
                    name = "io_rdPortsNext_"+to_string(nodes[i].outputs.output[lsq_indx].port) + "_valid";
                    write_lsq_signal(name, false, "std_logic", false);
                    //name = "io_rdPortsNext_"+to_string(nodes[i].inputs.input[lsq_indx].port) + "_bits";
                    name = "io_rdPortsNext_"+to_string(nodes[i].outputs.output[lsq_indx].port) + "_bits";
                    write_lsq_signal(name, false, "std_logic_vector("+ to_string(get_lsq_datawidth()-1) + " downto 0)", false);
                } else if ( nodes[i].outputs.output[lsq_indx].type == "s" ) {
                    //name = "io_wrpValids_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    name = "io_wrpValids_"+to_string(nodes[i].outputs.output[lsq_indx].port);
                    write_lsq_signal(name, true, "std_logic", false);
                    //name = "io_wrReadyToPrevs_"+to_string(nodes[i].inputs.input[lsq_indx].port);
                    name = "io_wrReadyToPrevs_"+to_string(nodes[i].outputs.output[lsq_indx].port);
                    write_lsq_signal(name, false, "std_logic", false);
                }
                else if ( nodes[i].outputs.output[lsq_indx].type == "e" ) {
                    write_lsq_signal("io_Empty_Valid", false, "std_logic", false);
                }
            }
            
            

            write_lsq_signal("io_loadDataIn", true, "std_logic_vector("+ to_string(get_lsq_datawidth()-1) + "  downto 0)", false);
            //write_lsq_signal("io_loadAddrOut", false, "std_logic_vector("+ to_string(get_lsq_addresswidth()-1) +"  downto 0)", false);
            write_lsq_signal("io_loadAddrOut", false, "std_logic_vector("+ to_string(nodes[i].address_size-1) +"  downto 0)", false);
            write_lsq_signal("io_loadEnable", false, "std_logic", true);
            netlist << ");" << endl;
            netlist << "end component;" << endl;
        }
      
    }
}

void vhdl_writer::write_vhdl ( string filename, int indx )
{
    
    string entity = clean_entity ( filename );
    
    string output_filename = filename + ".vhd";

    
    components_type[COMPONENT_GENERIC].in_ports = 2;
    components_type[COMPONENT_GENERIC].out_ports = 1;
    components_type[COMPONENT_GENERIC].in_ports_name_str = in_ports_name_generic;
    components_type[COMPONENT_GENERIC].in_ports_type_str = in_ports_type_generic;
    components_type[COMPONENT_GENERIC].out_ports_name_str = out_ports_name_generic;
    components_type[COMPONENT_GENERIC].out_ports_type_str = out_ports_type_generic;
    
    components_type[COMPONENT_CONSTANT].in_ports = 1;
    components_type[COMPONENT_CONSTANT].out_ports = 1;
    components_type[COMPONENT_CONSTANT].in_ports_name_str = in_ports_name_generic;
    components_type[COMPONENT_CONSTANT].in_ports_type_str = in_ports_type_generic;
    components_type[COMPONENT_CONSTANT].out_ports_name_str = out_ports_name_generic;
    components_type[COMPONENT_CONSTANT].out_ports_type_str = out_ports_type_generic;

    
    netlist.open (output_filename);
    
    write_intro ( );
    
    write_entity ( filename, indx );
    
    netlist << "architecture behavioral of " << entity << " is " << endl;

    write_signals (  );
    
    write_lsq_declaration ( );

    netlist  << endl << "begin" << endl;

    write_connections ( indx );
    
    write_components ( );

    netlist  << endl << "end behavioral; "<< endl;

    netlist.close();
}


void write_tb_intro (  )
{
    
    time_t now = time(0);
    char* dt = ctime(&now);

    tb_wrapper << "-- ==============================================================" << endl;
    tb_wrapper << "-- Generated by Dot2Vhdl ver. " << VERSION_STRING << endl;
    tb_wrapper << "-- File created: " << dt << endl;
    tb_wrapper << "-- ==============================================================" << endl;

    tb_wrapper << "library IEEE; " << endl;
    tb_wrapper << "use IEEE.std_logic_1164.all; " << endl;
    tb_wrapper << "use IEEE.numeric_std.all; " << endl;
    tb_wrapper << "use work.customTypes.all; " << endl;

    tb_wrapper << "-- ==============================================================" << endl;

}

void write_tb_entity ( string filename )
{
    
    
    
    tb_wrapper << "entity "<< filename << " is " << endl;
    tb_wrapper << "port (" << endl;
    tb_wrapper << "\t" << "clk: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "rst: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "start: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "done: " << " out std_logic;" << endl;
    tb_wrapper << "\t" << "idle: " << " out std_logic";
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            tb_wrapper << ";" << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_from_memory : in std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_enable : out std_logic;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_address : out std_logic_vector (31 downto 0)";
        }

        if ( nodes[i].name.find("store") != std::string::npos )
        {
            tb_wrapper << ";" << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_to_memory : out std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_enable : out std_logic;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_address : out std_logic_vector (31 downto 0)";
        }
    }
    
    tb_wrapper << ");" << endl;
    tb_wrapper << "end;" << endl << endl;

}

void write_tb_signals ( void )
{
    
    tb_wrapper << endl;

    tb_wrapper << "\t" << SIGNAL_STRING << "ap_clk : std_logic;" << endl;
    tb_wrapper << "\t" << SIGNAL_STRING << "ap_rst : std_logic;" << endl;
    tb_wrapper << "\t" << SIGNAL_STRING << "ap_start : std_logic;" << endl;
    tb_wrapper << "\t" << SIGNAL_STRING << "ap_done : std_logic;" << endl;
    tb_wrapper << "\t" << SIGNAL_STRING << "ap_ready : std_logic;" << endl;

    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_data_from_memory : std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_read_enable : std_logic;" << endl;
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_read_address : std_logic_vector (31 downto 0);"<< endl;
            tb_wrapper << endl;

        }

        
        if ( nodes[i].name.find("store") != std::string::npos )
        {
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_data_to_memory : std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_write_enable :  std_logic;" << endl;
            tb_wrapper << "\t" << SIGNAL_STRING << nodes[i].name << "_write_address : std_logic_vector (31 downto 0);"<< endl;
            tb_wrapper << endl;

        }
    }
    
}


void declatre_tb_component ( string filename )
{
    tb_wrapper << "component "<< filename << " is " << endl;
    tb_wrapper << "port (" << endl;
    tb_wrapper << "\t" << "ap_clk: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "ap_rst: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "ap_start: " << " in std_logic;" << endl;
    tb_wrapper << "\t" << "ap_done: " << " out std_logic;" << endl;
    tb_wrapper << "\t" << "ap_ready: " << " out std_logic";
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            tb_wrapper << ";" << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_from_memory : in std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_enable : out std_logic;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_address : out std_logic_vector (31 downto 0)";
        }

        if ( nodes[i].name.find("store") != std::string::npos )
        {
            tb_wrapper << ";" << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_to_memory : out std_logic_vector (31 downto 0);" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_enable : out std_logic;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_address : out std_logic_vector (31 downto 0)";
        }
    }
    
    tb_wrapper << ");" << endl;
    tb_wrapper << "end component;" << endl << endl;


}
void write_tb_components ( string filename )
{
    tb_wrapper <<  filename << "_1" << " : "<< filename << endl;
    tb_wrapper << "port map (" << endl;
    tb_wrapper << "\t" << "ap_clk => clk," << endl;
    tb_wrapper << "\t" << "ap_rst => rst," << endl;
    tb_wrapper << "\t" << "ap_start => start," << endl;
    tb_wrapper << "\t" << "ap_done => done," << endl;
    tb_wrapper << "\t" << "ap_ready => idle";
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            tb_wrapper << "," << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_from_memory => "<< nodes[i].name << "_data_from_memory," << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_enable => "<< nodes[i].name <<  "_read_enable," << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_address =>"<< nodes[i].name << "_read_address";
        }

        if ( nodes[i].name.find("store") != std::string::npos )
        {
            tb_wrapper << "," << endl; 
            tb_wrapper << "\t" << nodes[i].name << "_data_to_memory => " << nodes[i].name <<"_data_to_memory," << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_enable => "<< nodes[i].name << "_write_enable," << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_address => " << nodes[i].name << "_write_address";
        }
    }
    
    tb_wrapper << ");" << endl;

}


void write_tb_connections ( void )
{
    
    for (int i = 0; i < components_in_netlist; i++) 
    {
        if ( nodes[i].name.find("load") != std::string::npos )
        {
            tb_wrapper << "\t" << nodes[i].name << "_data_from_memory <= "<< nodes[i].name << "_data_from_memory;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_enable <="<< nodes[i].name <<  "_read_enable;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_read_address <="<< nodes[i].name << "_read_address;"<< endl;
            tb_wrapper << endl;
        }

        if ( nodes[i].name.find("store") != std::string::npos )
        {
            tb_wrapper << "\t" << nodes[i].name << "_data_to_memory <= " << nodes[i].name <<"_data_to_memory;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_enable <= "<< nodes[i].name << "_write_enable;" << endl;
            tb_wrapper << "\t" << nodes[i].name << "_write_address <= " << nodes[i].name << "_write_address;" << endl;
            tb_wrapper << endl;

        }
    }

    
}

void vhdl_writer::write_tb_wrapper ( string filename  )
{

    string output_filename = filename + "_tb_wrapper.vhd";
    string tb_wrapper_string = filename + "_tb_wrapper";
    
    tb_wrapper.open (output_filename);

    write_tb_intro ( );
    write_tb_entity ( tb_wrapper_string );
    
    tb_wrapper << "architecture behavioral of " << tb_wrapper_string << " is " << endl;

    write_tb_signals ( );
    
    declatre_tb_component ( filename );
    tb_wrapper  << endl << "begin" << endl;

    write_tb_connections ( );
    
    write_tb_components (filename );
    
    
    tb_wrapper  << endl << "end behavioral; "<< endl;


    tb_wrapper.close();


}
