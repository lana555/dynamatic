/*
*  C++ Implementation: dhls
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
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <algorithm> 
#include <list>
#include <cctype>

#include <unistd.h>

#include "cmd_parser.h"
#include "cmd_list.h"
#include "dhls_strings.h"


using namespace std;

#define OK	0
#define ERR	1
#define EXIT	255

#define DUMMY	"dummy"

string filename = DUMMY;

#define DEFAULT_PERIOD 3
int period = DEFAULT_PERIOD;
int use_default_period = 1;



string current_file;
string project_dir;

#define OUTPUT_DIR "/reports/"

enum
{
    FALSE,
    TRUE
};

bool set_filename = FALSE;

bool is_target_set = FALSE;
string target;

string vendor = "xilinx";

void string_split(const string& s, char c, vector<string>& v)
{
   string::size_type i = 0;
   string::size_type j = s.find(c);

   while (j != string::npos) {
      v.push_back(s.substr(i, j-i));
      i = ++j;
      j = s.find(c, j);

      if (j == string::npos)
         v.push_back(s.substr(i, s.length()));
   }
}

void stripExtension(std::string &path, string extension )
{
    int dot = path.rfind(extension);
    if (dot != std::string::npos)
    {
        path.resize(dot);
    }
}


void eraseAllSubStr(std::string & mainStr, const std::string & toErase)
{
	size_t pos = std::string::npos;
 
	// Search for the substring in string in a loop untill nothing is found
	while ((pos  = mainStr.find(toErase) )!= std::string::npos)
	{
		// If found then erase it from string
		mainStr.erase(pos, toErase.length());
	}
}

string clean_path ( string filename )
{
    vector<string> v;
    string return_string;
    string_split( filename, '/', v);
    
    //cout << "size: "<< v.size();
    
    if ( v.size() > 0 )
    {
    
        for ( int indx = 0; indx < v.size(); indx++ )
        {
            return_string = v[indx];
        }
    }
    else
    {
        return_string = filename;
    }
    return return_string;
}


string GetStdoutFromCommand(std::string cmd)
{
    string data;
    FILE *stream;
    const int max_buffer = 4096;
    char buffer[4096];
    cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");
    if (stream)
    {
//         while (!feof(stream))
//         {
//             if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
//         }
        while ( fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        //while (fgets(buffer, sizeof(buffer)-1, stream) != NULL) 
        //    printf ("%s", buffer);
            //cout << to_string(buffer);
        //pclose(stream);
    }

    return data;
}

int help ( string input_cmp )
{
    int indx;
    std::cout << "List of supported commands:" << endl;

    for ( indx = 0; indx < CMD_MAX; indx++ )
    {
        std::cout << ui_cmds[indx].cmd << ui_cmds[indx].help << endl;
    }

    return OK;
}

int exit_funct ( string input_cmp )
{
    return EXIT;
}

int synth ( string input_cmp )
{
    std::cout << "Synthesize" << endl;
    bool verbose = 0;
    bool help = 0;

    if ( input_cmp.find("-verbose") != std::string::npos )
    {
        eraseAllSubStr(input_cmp, "-verbose");
        verbose = 1;
    }

    if ( input_cmp.find("-help") != std::string::npos )
    {
        eraseAllSubStr(input_cmp, "-help");
        
        cout << "This command perform synthesis. " << endl;
        cout << "Available options are: " << endl;
        cout << "-use-lsq={true|false} : enable/disable LSQ insertion in the design. Default value = true." << endl;
        cout << "-simple-buffers={true|false} : enable/dispable placement of naive buffers. Default value = false." << endl;
        cout << "-target={part-number} : part number of the targeted FPGA. Default value = 7k160tfbg484" << endl;
        return OK;
    }

    string command;
    if ( set_filename )
    {        
        
        //stripExtension(current_file, ".cpp");
        //stripExtension(current_file, ".c");    

        //current_file.erase( remove( current_file.begin(), current_file.end(), ".cpp" ), current_file.end() );
        command = "compile ";
        command += clean_path ( current_file );
        command += " ";
        command += project_dir;

        command += " ";
        command += input_cmp;

        if (is_target_set) {
            command += " ";
            command += "-target=";
            command += target;
        }
        
        cout << command;
        //current_file = current_file;
        string com = GetStdoutFromCommand( command.c_str() );
        if ( verbose )
        {
            cout <<  com << endl;
        }
        cout << endl;
    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;
}

//dcfg $REPORT_DIR/$name"_graph.dot" &

int cdfg ( string input_cmp )
{
    std::cout << "Display Data Control Flow Graph" << endl;
    
    

    string command;
    if ( set_filename )
    {        
        command = "dcfg ";
        current_file = clean_path ( current_file );
        command += project_dir;
        command += OUTPUT_DIR;
        stripExtension(current_file, ".cpp");    
        stripExtension(current_file, ".c");
        stripExtension(current_file, ".dot");
        command += current_file;
        command += ".dot &";


        cout << command;
        
        //string com = GetStdoutFromCommand( command.c_str() );
        
        system ( command.c_str() );
        
        //cout <<  com << endl;
    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;
}



int write_hdl ( string input_cmp )
{
    std::cout << "Write hdl" << endl;

    char compilation[1024];
    
    stripExtension( current_file, ".dot");
    //current_file.erase( remove( current_file.begin(), current_file.end(), ".dot" ), current_file.end() );
    
    string command;
    if ( set_filename )
    {       
        //command = "dot2vhdl ";
        command = "write_hdl ";
        //command += current_file;
        command += " ";
        command += project_dir;
        command += " ";
        current_file = clean_path ( current_file );
        command += project_dir;
        command += OUTPUT_DIR;
        stripExtension(current_file, ".cpp");
        stripExtension(current_file, ".c");    

        command += current_file;
        //command += filename;
        //command += "_optimized";

        command +=input_cmp;
        
        cout << command;
        string com = GetStdoutFromCommand( command.c_str() );
        cout <<  com << endl;
    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;
}

int set_project_dir ( string input_cmp )
{
    std::string project_name;

    string command;

    command = "create_project ";
    command += input_cmp;
    
    string com = GetStdoutFromCommand( command.c_str() );

    project_dir = input_cmp;
    
    return OK;
}

int report_status ( string input_cmp )
{
  
    cout << "Report Design Status" << endl;
    cout << "Current project: " << project_dir << endl;
    cout << "Top level file: " << current_file << endl;
    
    
    return OK;
}

#include <sys/stat.h>

inline bool file_exists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}


int set_file ( string input_cmp )
{
    std::cout << "set input file: " << input_cmp << endl;
    
    filename = project_dir;
    filename += "/src/";
    filename += input_cmp;
    
    if ( file_exists( filename ) )
    {
        //filename = input_cmp;
        cout << "current input filename: "<< filename << endl;
        current_file = filename;
        set_filename = TRUE;
        design_status = DESIGN_STATUS_FILE_SET;
    }
    else
    {
        cout << "file not found: "<< filename << endl;
        set_filename = FALSE;
        design_status = DESIGN_STATUS_BEGIN;
    }
    
    return OK;

}

int shell ( string input_cmp )
{
    //std::cout << "DHLS Command Not-Recognized--passing to system shell " << endl;
    std::vector<string> sub_cmd;

    //cout << input_cmp;
    
    string_split( input_cmp, ' ', sub_cmd );
    
    //cout << "sub_cmd.size()" << sub_cmd.size() << endl;
    
    if ( sub_cmd.size() > 0 )
    {
        //cout << "sub_cmd[0]" << sub_cmd[0] << "sub_cmd[1]" << sub_cmd[1];
        
        if ( sub_cmd[0] == "cd" )
        {
            chdir (sub_cmd[1].c_str());
        }
        else
        {
            system ( input_cmp.c_str() );   
        }        
    }
    else
    {
        system ( input_cmp.c_str() );
    }

    // string com = GetStdoutFromCommand(input_cmp);
    // cout << com << endl;

    //std::cout << "DHLS Command Not-Recognized--output from system shell " << endl;

    return OK;
}

int about ( string input_cmp )
{
    std::cout << INIT_STRING;
    std::cout << VERSION_STRING;

    return OK;
}

#define PRAGMAS_LIST_PATH "$DHLS_INSTALL_DIR/etc/#pragmas/"

int analyze ( string input_cmp )
{
    std::cout << "Analyze" << endl;

    char compilation[1024];

    string command;
    if ( set_filename )
    {        
        command = "analyze ";
        //command += project_dir;
        //command += "/src/";
        command += filename;
        //command += ".cpp ";
        command += " ";
        //command += project_dir;
        //command += "/src/";
        stripExtension(filename, ".cpp");    
        stripExtension(filename, ".c");
        
        command += filename;
        command += "_analyzed.cpp ";
        command += PRAGMAS_LIST_PATH;
        
        cout << command;
        
        string com = GetStdoutFromCommand( command.c_str() );
        cout <<  com << endl;

        //current_file = project_dir;
        //current_file += "/src/";
        current_file = filename;
        current_file += "_analyzed.cpp";

    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;
}

int elaborate ( string input_cmp )
{
    std::cout << "Elaborate" << endl;

    string command;
    if ( DESIGN_STATUS_FILE_SET )
    {
        if ( set_filename )
        {        
            command = "elaborate ";
            
            command += current_file;
            //command += project_dir;
            //command += "/src/";
            //command += filename;
            //command += "_analyzed.cpp ";
            command += " ";

            //command += project_dir;
            //command += "/src/";
            stripExtension(filename, ".cpp");
            stripExtension(filename, ".c");    
            
            command += filename;
            command += "_elaborated.cpp ";
            
            cout << command;
            string com = GetStdoutFromCommand( command.c_str() );
            cout <<  com << endl;
            
            //current_file = project_dir;
            //current_file += "/src/";
            current_file = filename;
            current_file += "_elaborated.cpp";
        
            
            
 /*           string current_file_tmp = clean_path ( current_file );
            
            command = "mv ";
            command += current_file;
            
            command +=" ";
            
            command +="src/.";
            command += current_file_tmp;

            system (command.c_str());

            current_file = "src/.";
            current_file += current_file_tmp;
 */
            
        }
    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;
}


int optimize ( string input_cmp )
{
    string source_file2;
    std::cout << "Optimize" << endl;
    bool area_opt = 0;
    
    if ( input_cmp.find("-area") != std::string::npos )
    {
        eraseAllSubStr(input_cmp, "-area");
        area_opt = 1;
    }

    
    string command;
    if ( set_filename )
    {        
        string source_file;
        
        current_file = clean_path ( current_file );
        
        stripExtension(current_file, ".cpp");
        stripExtension(current_file, ".c");  
            
        source_file += project_dir;
        source_file += OUTPUT_DIR;
        //source_file += filename;
        //source_file += "_elaborated.dot";
        source_file += current_file;
        source_file += ".dot";
        

        source_file2 = project_dir;
        source_file2 += OUTPUT_DIR;
        //source_file += filename;
        //source_file += "_elaborated.dot";
        source_file2 += current_file;
        source_file2 += "_bbgraph.dot";




        string output_file;
        string output_file2;

        output_file += project_dir;
        output_file += OUTPUT_DIR;
        output_file += current_file;
        output_file += "_optimized.dot";
        
        output_file2 = project_dir;
        output_file2 += OUTPUT_DIR;
        output_file2 += current_file;
        output_file2 += "_bbgraph_optimized.dot";
        
        //Original Version
        //bin/buffers buffers 3 0.0 cbc examples/fir_graph.dot examples/_build/fir_graph.dot

//         command = "buffers buffers ";
//         command += to_string ( period );
//         command += " 0.0 ";
//         command += "cbc ";
//         command += source_file;
//         command += " ";
//         command += output_file;
        
        
        //Shabnam version
        //bin/buffers shab 10 0.0 cbc 1 20 examples/example.dot examples/example_bbgraph.dot example_10.dot example_bbgraph.dot 0

//         command = "buffers shab ";
//         command += to_string ( period );
//         command += " 0.0 ";
//         command += "cbc ";
//         command += "1 ";
//         command += "20 ";
//         command += source_file;
//         command += " ";
//         command += source_file2;        
//         command += " ";
//         command += output_file;
//         command += " ";
//         command += output_file2;
//         command += " 0 ";

        //New version 20200128
        
        command = "buffers buffers";
        command += " -filename=";
        command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
        
        if ( use_default_period == 0 )
        {
        command += " -period=";
        command += to_string ( period );
        }
        command += " ";
        command += input_cmp;
        
        
        cout << command;
        string com = GetStdoutFromCommand( command.c_str() );
        cout <<  com << endl;
        
        if ( area_opt == 1 )
        {
            
            //cd $targetFolder
            system ( "mkdir _input" );
            system ( "mkdir _output" );
            system ( "mkdir _tmp");
            
            
            //cp $1_graph.dot _input
            command = "cp ";
        command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
            //command += "_optimized.dot ";
        command += "_graph_buf.dot ";
        
        command += project_dir;
            //command += OUTPUT_DIR;
            command += "/_input/";
            command += current_file;
            command += "_graph.dot ";

            system (command.c_str());
            cout << command<< endl;

            
            //cp $1_bbgraph.dot _input
            command = "cp ";
            command += project_dir;
            command += OUTPUT_DIR;
            command += current_file;
            command += "_bbgraph_buf.dot ";
            
            command += project_dir;
            //command += OUTPUT_DIR;
            command += "/_input/";
            //command += OUTPUT_DIR;
            command += current_file;
            command += "_bbgraph.dot ";
            
            system (command.c_str());
            cout << command<< endl;



            //cp $1_bbgraph.dot _tmp/out_bbgraph.dot"
            command = "cp ";
            command += project_dir;
            command += OUTPUT_DIR;
            command += current_file;
            command += "_bbgraph_buf.dot ";
            
            command += project_dir;
            //command += OUTPUT_DIR;
            command += "/_tmp/out_bbgraph.dot";

            system (command.c_str());
            cout << command<< endl;

            
            
            //system (  $ressourceMinExec min $1 
            command = "resource_minimization ";
            command += "min ";
            command += current_file;
            //command += "_graph.dot ";
                        
            system (command.c_str());
            cout << command<< endl;

            
            //cp _output/$1_graph.dot .

            command = "cp ";
            command += project_dir;
            //command += OUTPUT_DIR;
            command += "/_output/";
            command += current_file;
            command += "_graph.dot ";
            
            current_file += "_area";

            command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
        command += "_optimized.dot";
        
            system (command.c_str());
            cout << command << endl;

            //Clean-up
            system ( "rm -r _input" );
            system ( "rm -r _output" );
            system ( "rm -r _tmp");

            
        }
        else
        {
            command = "mv ";
            command += project_dir;
            command += OUTPUT_DIR;
            command += current_file;
            command += "_graph_buf.dot ";
            
            command += project_dir;
            command += OUTPUT_DIR;
            command += current_file;
            command += "_optimized.dot";
        
        system (command.c_str());
        
        }
        
        //dot -Tpng reports/histogram_elaborated_optimized.dot > file.png
        command = "dot -Tpng ";
        command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
        command += "_optimized.dot";
        command += " > ";
        command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
        command += "_optimized.png";
        
        
        system (command.c_str());
        
        output_file = project_dir;
        output_file += OUTPUT_DIR;
        output_file += current_file;
        output_file += "_optimized.dot";
        
        current_file = output_file;

    }
    else
    {
        cout << "Source File not set\n\r";
    }

    
    return OK;
}

#include <fstream>

string check_comments ( string strline )
{
    std::vector<string> sub_strline;

    string_split( strline, '#', sub_strline );

    if ( sub_strline.size() > 0 )
    {
        return sub_strline[0];
    }
    else
    {
        return strline;
    }
}

int source_script ( string input_cmp )
{
    
    ifstream script_file(input_cmp);
    string strline;
    int ret_val; 

    if (script_file.is_open()) 
    {
        while ( script_file )
        {
            getline(script_file, strline );
            
            if ( cmd_parser ( check_comments ( strline ) ) == EXIT )
            {
                fe_end();
            }
        }

        script_file.close();
    }
    else
    {
        cout << "File " << input_cmp << " not found "<< endl << endl<< endl;
    }
    
    return OK;
}


int set_period ( string input_cmp )
{
    
    period = stoi ( input_cmp );

    use_default_period = 0;
    
    return OK;

}





int reports ( string input_cmp )
{
    
    cout << current_file << endl;
    
    stripExtension(current_file, ".dot"); 
    //current_file.erase( remove( current_file.begin(), current_file.end(), ".dot" ), current_file.end() );
    
    cout << current_file << endl;
    
    string command;

    if ( set_filename )
    {       
        command = "write_hdl ";
        current_file = clean_path ( current_file );
        command += project_dir;
        command += OUTPUT_DIR;
        command += current_file;
        command +=" -report_area";
        
        
        cout << command;
        string com = GetStdoutFromCommand( command.c_str() );
        cout <<  com << endl;
    }
    else
    {
        cout << "Source File not set\n\r";
    }

    return OK;

}

int simulate ( string input_cmp )
{
        return OK;
}

int update ( string input_cmp )
{
    string command;
    command = "update-dynamatic";        
    cout << command;
    string com = GetStdoutFromCommand( command.c_str() );
    cout <<  com << endl;
        
    return OK;
}

int history ( string input_cmp )
{
    
    for (int indx = 0; indx < history_indx; indx++ )
    {
        cout << indx << ": "<< command_history[indx] << endl;
    }
    
    return OK;
}

int set_target ( string input_cmp )
{
    is_target_set = TRUE;
    target = input_cmp;

    return OK;
}



void cmd_parser_init ( void )
{
    
    ui_cmds[CMD_HELP1].function = &help;
    ui_cmds[CMD_HELP2].function = &help;
    ui_cmds[CMD_EXIT].function = &exit_funct;
    ui_cmds[CMD_PROJ].function = &set_project_dir;
    ui_cmds[CMD_ADD_FILE].function = &set_file;
    ui_cmds[CMD_SYNTHESIZE].function = &synth;
    ui_cmds[CMD_ABOUT].function = &about;
    ui_cmds[CMD_ELABORATE].function = &elaborate;
    ui_cmds[CMD_ANALYZE].function = &analyze;
    ui_cmds[CMD_OPTIMIZE].function = &optimize;
    ui_cmds[CMD_WRITE_HDL].function = &write_hdl;
    ui_cmds[CMD_SOURCE].function = &source_script;
    ui_cmds[CMD_SET_PERIOD].function = &set_period;
    ui_cmds[CMD_SET_TARGET].function = &set_target;
    //ui_cmds[CMD_REPORTS].function = &reports;
    //ui_cmds[CMD_SIMULATE].function = &simulate;
    ui_cmds[CMD_CDFG].function = &cdfg;
    //ui_cmds[CMD_STATUS].function = &report_status;
    ui_cmds[CMD_UPDATE].function = &update;
    ui_cmds[CMD_HISTORY].function = &history;

}

#include <string>
#include <vector>

int cmd_parser ( string input_cmd )
{
    std::vector<string> sub_cmd;
    string sub_command;
    
    string full_string = input_cmd;

    int indx;

    string_split( input_cmd, ' ', sub_cmd );
    
    //cout << sub_cmd.size() << endl;
    
    if ( sub_cmd.size() > 0 )
    {
        input_cmd = sub_cmd[0];
        sub_command = sub_cmd[1];
        if ( sub_cmd.size() > 2 )
        {
            for ( int indx2 = 2; indx2 < sub_cmd.size(); indx2++ )
            {
                sub_command += " ";   
                sub_command += sub_cmd[indx2];
            }
        }
    }     
    
    for ( indx = 0; indx < CMD_MAX; indx++ )
    {
        //if ( input_cmd.find(ui_cmds[indx].cmd) != std::string::npos )
        //cout << input_cmd << " " << ui_cmds[indx].cmd << endl;
        if ( input_cmd.compare (ui_cmds[indx].cmd) == 0 )
        {
            switch ( ui_cmds[indx].function( sub_command ) )
            {
                case EXIT:
                    return EXIT;
                    break;
                case OK:
                    std::cout << "Done " << endl;
                    return OK;
                    break;
                default:
                    return OK;
                    ;
            }
            break;
        }
    }
//    cout << "Command not found!" << endl;

    shell ( full_string );

    return OK;
}
