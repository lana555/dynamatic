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
#include "reports.h"
#include "checks.h"
#include "sys_utils.h"


using namespace std;


int debug_mode = FALSE;
int report_area_mode = FALSE;


string input_filename[MAX_INPUT_FILES];
string output_filename[MAX_INPUT_FILES];
string top_level_filename;
int dot_input_files = 0;
    
void arguments_parser ( int argc, char *argv[] )
{		
    switch ( argc )
    {
        case 3:
            if ( ! ( strcmp(argv[2] , "-debug") ) )
            {
                printf ( "Debug Mode Activated\n\r" );
                debug_mode = TRUE;
            }
            if ( ! ( strcmp(argv[2] , "-report_area") ) )
            {
                printf ( "Report Area Activated\n\r" );
                report_area_mode = TRUE;
            }
            break;
        case 2:
            if ( ! ( strcmp(argv[1] , "--version") ) )
            {
                printf ("Dot2Vhdl version %s \n\r", VERSION_STRING );\
                exit(1);

            }
            else
            if ( ! ( strcmp(argv[1] , "--help") ) )
            {
                printf ("Dot2Vhdl version %s \n\r", VERSION_STRING );
                printf ( "Usage: %s filename -debug [opt]\n\r\n\r\n\r", argv[0]);        
                exit(1);

            }
            else
            if ( ! ( strcmp(argv[1] , "about") ) )
            {
                printf("***************************************************** \n\r");
                printf("Author: Andrea Guerrieri Email: andrea.guerrieri@epfl.ch\n\r");
                printf("***************************************************** \n\r");
                exit(1);

            }
            break;
        default:
            printf( "Invalid arguments \n\rTry %s --help for more informations\n\r\n\r\n\r", argv[0] );
            exit ( 0 );
            break;
        }

    return;
}




int main( int argc, char* argv[] )
{
   
    signal_handler();

    vhdl_writer vhdl_writer;
    
    cout << INIT_STRING;
    
    arguments_parser ( argc, argv );
        
    dot_input_files = (argc-1);
            
    top_level_filename = argv[1];

    for ( int indx = 0; indx < dot_input_files; indx++ )
    {
        
        input_filename[indx] = argv[indx+1];
        output_filename[indx] = argv[indx+1];

        cout << "Parsing "<< input_filename[indx] << ".dot" << endl;
        
        parse_dot ( input_filename[indx] );
        
        check_netlist ( );
        
        
        if ( report_area_mode )
        {
            report_instances ();
            return 0;
            //report_area ();
        }
        else
        {
            report_instances ();
            cout << "Generating " << output_filename[indx] << ".vhd" << endl;
            vhdl_writer.write_vhdl ( output_filename[indx] , indx );
        }
    }
    
    
    //vhdl_writer.write_tb_wrapper ( top_level_filename );
    

    lsq_generate_configuration ( top_level_filename );
    lsq_generate ( top_level_filename );

    write_vivado_script ( top_level_filename );
    write_modelsim_script ( top_level_filename );
    
    cout << endl;
    cout << "Done" ;
    
    cout << endl<< endl<< endl;
    return 0;
    
} 




