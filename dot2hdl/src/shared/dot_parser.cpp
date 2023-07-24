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
#include "assert.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include <stdlib.h>     /* exit, EXIT_FAILURE */

#include "dot2hdl.h"
#include "../VHDL/string_utils.h"

using namespace std;

NODE_T nodes[MAX_NODES];

int components_in_netlist;
int lsqs_in_netlist;

#define COMPONENT_DESCRIPTION_LINE  0
#define COMPONENT_CONNECTION_LINE  1


void printNode(NODE_T node){
	cout << "Name: " << node.name
			<< "\tType: " << node.type
			<< "\tParameters: " << node.parameters
			<< "\tID: " << node.node_id
			<< "\tComponent Type: " << node.component_type
			<< "\tOperator: " << node.component_operator
			<< "\tValue: " << node.component_value
			<< "\tControl: " << node.component_control
			<< "\tSlots: " << node.slots
			<< "\tTransparent: " << node.trasparent
			<< "\tMemory: " << node.memory
			<< "\tBBCount: " << node.bbcount
			<< "\tBBID: " << node.bbId
			<< "\tPort ID: " << node.portId;

			cout << "\n\nIn Size: " << node.inputs.size;
	for(int i = 0; i < node.inputs.size; i++){
		cout << "\n\n#" << (i + 1) << "\nBit Size: " << node.inputs.input[i].bit_size
				<< "\tPrev Node Id: " << node.inputs.input[i].prev_nodes_id
				<< "\tType: " << node.inputs.input[i].type
				<< "\tPort: " << node.inputs.input[i].port
				<< "\tInfo Type: " << node.inputs.input[i].info_type;
	}

	cout << "\n\nOut Size: " << node.outputs.size;
	for(int i = 0; i < node.outputs.size; i++){
		cout << "\n\n#" << (i + 1) << "\nBit Size: " << node.outputs.output[i].bit_size
				<< "\tNext Node Id: " << node.outputs.output[i].next_nodes_id
				<< "\tType: " << node.outputs.output[i].type
				<< "\tPort: " << node.outputs.output[i].port
				<< "\tInfo Type: " << node.outputs.output[i].info_type;
	}

			cout << endl;
}


bool check_line ( string line )
{

	if ( line.find("type") != std::string::npos )
	{
		return COMPONENT_DESCRIPTION_LINE;
	}
	if ( line.find(">") != std::string::npos )
	{
		return COMPONENT_CONNECTION_LINE;
	}
}

string get_value ( string parameter )
{
	vector<string> v;
	string_split( parameter, '=', v );
	if ( v.size() > 0 )
	{
		return v[1];
	}

}

string get_component_type ( string parameters )
{

	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	nodes[components_in_netlist].component_type = COMPONENT_GENERIC;

	return type;
}

string get_component_operator ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );

	return type;
}

string get_component_value ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return type;
}

bool get_component_control (  string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );

	return ((( type == "true" )) ? TRUE : FALSE);
}


int get_component_slots ( string parameters )
{

	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}


bool get_component_transparent (  string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );

	return ((( type == "true" )) ? TRUE : FALSE);
}


string get_component_memory ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return type;
}


string get_component_numloads ( string parameters )
{
	//parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return type;
}

string get_component_numstores ( string parameters )
{
	//parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return type;
}


int get_component_bbcount ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}

int get_component_bbId ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}


int get_component_portId ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}

int get_component_offset ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}

bool get_component_mem_address (  string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );

	return ((( type == "true" )) ? TRUE : FALSE);

}

int get_component_constants ( string parameters )
{
	parameters = string_clean( parameters );

	string type = get_value ( parameters );
	return stoi_p( type );
}

vector<vector<int>> get_component_orderings(string parameter){
	vector<vector<int>> orderings;
	vector<string> par = vector<string>();
	string_split( parameter, '=', par );
	assert(par.size() == 2);
	par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );
	string value = par[1];
	//trim the value
	int start_index = value.find_first_not_of(" ");
	int end_index = value.find_last_not_of(" ") + 1;
	value = value.substr(start_index, end_index);

	vector<string> ordering_per_bb = vector<string>();
	if(value.find(" ") != string::npos){
		string_split(value, ' ', ordering_per_bb);
	}else{
		ordering_per_bb.push_back(value);
	}

	for(auto ordering_inside_bb : ordering_per_bb){
		vector<string> string_indices{};
		if(ordering_inside_bb.find("|") != string::npos){
			string_split(ordering_inside_bb, '|', string_indices);
		}else{
			string_indices.push_back(ordering_inside_bb);
		}
		vector<int> int_indices{};
		for(auto string_index : string_indices){
			int_indices.push_back(stoi_p(string_index));
		}
		orderings.push_back(int_indices);
	}
	return orderings;
}

string get_input_type ( string in )
{
	vector<string> par;
	string ret_val = "u";

	string_split( in, '*', par );

	if ( par.size() )
	{
		par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );
		ret_val = par[1].at(0);
	}


	return ret_val;
}

int get_input_port ( string in )
{
	vector<string> par;
	int ret_val = 0;
	string val;

	string_split( in, '*', par );


	if ( par.size() )
	{
		par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );
		if ( par[1].size() > 1 )
		{
			val = par[1].at(1);
			ret_val =  stoi_p ( val );
		}
	}

	//cout << in << ":" << ret_val << endl;

	return ret_val;
}


string get_info_type ( string in )
{
	vector<string> par;
	string ret_val = "u";

	string_split( in, '*', par );

	if ( par.size() )
	{
		par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );
		if ( par[1].size() > 2 )
		{
			ret_val = par[1].at(2);
		}
	}


	return ret_val;
}


int get_input_size ( string in )
{

	vector<string> bit_sizes;
	int ret_val = 32;

	string_split( in, ':', bit_sizes );

	if ( bit_sizes.size() )
	{
		bit_sizes[1].erase( remove( bit_sizes[1].begin(), bit_sizes[1].end(), '"' ), bit_sizes[1].end() );
		bit_sizes[1].erase( remove( bit_sizes[1].begin(), bit_sizes[1].end(), ']' ), bit_sizes[1].end() );
		bit_sizes[1].erase( remove( bit_sizes[1].begin(), bit_sizes[1].end(), ';' ), bit_sizes[1].end() );

		if ( stoi_p( bit_sizes[1] ) == 0 ) // if 0 force to 1!!
		{
			ret_val = 1;
		}
		else
		{
			ret_val = stoi_p( bit_sizes[1] );
		}

	}

	return ret_val;

}

IN_T get_component_inputs ( string in , int components_in_netlist )
{
	vector<string> v;
	vector<string> par;
	vector<string> bit_sizes;
	IN_T inputs;

	in.erase( remove( in.begin(), in.end(), '\t' ), in.end() );

	inputs.size = 0;

	string_split( in, '=', par );

	if ( par.size() )
	{
		par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );

	}

	string test_string;

	for ( int indx = MAX_INPUTS ; indx > 0; indx-- )
	{
		test_string = "in";
		test_string += to_string(indx);
		if ( par[1].find(test_string) != std::string::npos )
		{
			inputs.size = indx;
			break;
		}

	}

	int input_indx = 0;

	if ( inputs.size == 1 )
	{
		inputs.input[input_indx].bit_size = get_input_size ( par[1] );


		inputs.input[input_indx].type = get_input_type ( par[1] );
		inputs.input[input_indx].port = get_input_port ( par[1] );
		inputs.input[input_indx].info_type = get_info_type ( par[1] );
		if ( inputs.input[input_indx].info_type == "a" )
		{
			nodes[components_in_netlist].address_size =  inputs.input[input_indx].bit_size;
		}
		if ( inputs.input[input_indx].info_type == "d" )
		{
			nodes[components_in_netlist].data_size =  inputs.input[input_indx].bit_size;
		}


	}
	else
	{
		string_split( par[1], ' ', v );

		if ( v.size() )
		{
			for ( int indx = 0; indx <  v.size(); indx++ )
			{
				if ( !(v[indx].empty()) )
				{
					inputs.input[input_indx].bit_size = get_input_size ( v[indx] );
					inputs.input[input_indx].type = get_input_type ( v[indx] );
					inputs.input[input_indx].port = get_input_port ( v[indx] );
					inputs.input[input_indx].info_type = get_info_type ( v[indx] );
					if ( inputs.input[input_indx].info_type == "a" )
					{
						nodes[components_in_netlist].address_size =  inputs.input[input_indx].bit_size;
					}
					if ( inputs.input[input_indx].info_type == "d" )
					{
						nodes[components_in_netlist].data_size =  inputs.input[input_indx].bit_size;
					}

					//cout << nodes[components_in_netlist].name << " input "<< input_indx << ":" << inputs.input[input_indx].type << endl;
					input_indx++;

				}
			}
		}
	}

	return inputs;

}

OUT_T get_component_outputs ( string parameters )
{
	vector<string> v;
	vector<string> bit_sizes;
	vector<string> par;

	OUT_T outputs;

    outputs.size = 0;

	string_split( parameters, '=', par );

	if ( par.size() )
	{
		par[1].erase( remove( par[1].begin(), par[1].end(), '"' ), par[1].end() );

	}

	string test_string;


	for ( int indx = MAX_OUTPUTS ; indx > 0; indx-- )
	{
		test_string = "out";
		test_string += to_string(indx);
		if ( parameters.find(test_string) != std::string::npos )
		{
			outputs.size = indx;
			break;
		}

	}

	int output_indx = 0;

	if ( outputs.size == 1 )
	{
		outputs.output[output_indx].bit_size = get_input_size ( par[1] );
		outputs.output[output_indx].type = get_input_type ( par[1] );
		outputs.output[output_indx].port = get_input_port ( par[1] );
		outputs.output[output_indx].info_type = get_info_type ( par[1] );
	}
	else
	{
		string_split( par[1], ' ', v );
		if ( v.size() )
		{
			for ( int indx = 0; indx <  v.size(); indx++ )
			{
				if ( !(v[indx].empty()) )
				{
					outputs.output[output_indx].bit_size = get_input_size( v[indx] );
					outputs.output[output_indx].type = get_input_type ( v[indx] );
					outputs.output[output_indx].port = get_input_port ( v[indx] );
					outputs.output[output_indx].info_type = get_info_type ( v[indx] );
					output_indx++;
				}
			}
		}
	}

	return outputs;
}

string get_component_name ( string name )
{
	string name_ret;
	name.erase( remove( name.begin(), name.end(), '\t' ), name.end() );
	name.erase( remove( name.begin(), name.end(), '"' ), name.end() );
	name.erase( remove( name.begin(), name.end(), ' ' ), name.end() );

	if ( name[0] == '_' )
	{
		//cout << "***WARNING***: Vivado doesn't support names with '_' as first character. Component "<< name <<" renamed as ";
		name.replace(0,1,"");
		//cout << name << endl;
	}

	name_ret = name;
	return name_ret;

}

void parse_connections ( string line )
{

	vector<string> v;
	vector<string> from_to;
	vector<string> parameters;


	int current_node_id;
	int next_node_id;

	string_split(line, '>', v);

	int i;
	if ( v.size() > 0 )
	{
		v[0].erase( remove( v[0].begin(), v[0].end(), ' ' ), v[0].end() );
		v[0].erase( remove( v[0].begin(), v[0].end(), '-' ), v[0].end() );
		v[0].erase( remove( v[0].begin(), v[0].end(), '\t' ), v[0].end() );
		v[0].erase( remove( v[0].begin(), v[0].end(), '"' ), v[0].end() );

		string_split( v[1], '[', from_to );
		from_to[0].erase( remove( from_to[0].begin(), from_to[0].end(), ' ' ), from_to[0].end() );
		from_to[0].erase( remove( from_to[0].begin(), from_to[0].end(), '\t' ), from_to[0].end() );
		from_to[0].erase( remove( from_to[0].begin(), from_to[0].end(), '"' ), from_to[0].end() );

		if ( v[0][0] == '_' )
		{
			v[0].replace(0,1,"");
		}

		if ( from_to[0][0] == '_' )
		{
			from_to[0].replace(0,1,"");
		}


		current_node_id = COMPONENT_NOT_FOUND;
		for (i = 0; i < components_in_netlist; i++)
		{
			if ( nodes[i].name.compare(v[0]) == 0 )
			{
				current_node_id = i;
				break;
			}
		}
		next_node_id = COMPONENT_NOT_FOUND;

		for (i = 0; i < components_in_netlist; i++)
		{
			if ( nodes[i].name.compare(from_to[0]) == 0 )
			{
				next_node_id = i;
				break;
			}
		}

		string_split( from_to[1], ',', parameters );

		int input_indx;
		int output_indx;
		int indx;
		for ( indx = 0; indx < parameters.size(); indx++ )
		{
			if ( parameters[indx].find("from") != std::string::npos )
			{
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), ' ' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), '\t' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), '"' ), parameters[indx].end() );
				parameters[indx].erase(0, 8);
				output_indx = stoi_p( parameters[indx] );
				output_indx--;
			}
			if ( parameters[indx].find("to") != std::string::npos )
			{
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), ' ' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), '\t' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), '"' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), ';' ), parameters[indx].end() );
				parameters[indx].erase( remove( parameters[indx].begin(), parameters[indx].end(), ']' ), parameters[indx].end() );


				parameters[indx].erase(0, 5);

				input_indx = stoi_p( parameters[indx] );
				input_indx--;
			}

		}

		if ( current_node_id != COMPONENT_NOT_FOUND && next_node_id != COMPONENT_NOT_FOUND )
		{

			nodes[current_node_id].outputs.output[output_indx].next_nodes_id = next_node_id;
			nodes[current_node_id].outputs.output[output_indx].next_nodes_port = input_indx;
			nodes[next_node_id].inputs.input[input_indx].prev_nodes_id = current_node_id;

		}
		else
		{
			cout << "Netlist Error" << endl;

			if ( current_node_id == COMPONENT_NOT_FOUND )
			{
				cout << "Node Description "<< v[0] << " not found. Not ID assigned" << endl;
			}
			else

				if ( next_node_id == COMPONENT_NOT_FOUND )
				{
					cout << "Node ID" << current_node_id << "Node Name: " << nodes[current_node_id].name << " has not next node for output " << output_indx << endl;
				}

			cout << "Exiting without producing netlist" << endl;
			exit ( 0);

		}

	}
}


string check_comments ( string line  )
{
	vector<string> v;
	string_split(line, COMMENT_CHARACTER, v);

	if ( v.size() > 0 )
		return  v[0];
	else
		return line;

}

void parse_components ( string v_0, string v_1 )
{
	vector<string> parameters;
	string parameter;

	nodes[components_in_netlist].name = get_component_name ( v_0 );
	if ( !( nodes[components_in_netlist].name.empty() ) ) //Check if the name is not empty
	{

		string_split( v_1, ',', parameters );

		int indx;
		for ( indx = 0; indx < parameters.size(); indx++ )
		{
			parameter = string_remove_blank ( parameters[indx] );

			if ( parameter.find("type") != std::string::npos )
			{
				nodes[components_in_netlist].type = get_component_type ( parameters[indx] );
				nodes[components_in_netlist].component_operator = nodes[components_in_netlist].type; // For the component without an operator, sets the entity type
				if ( nodes[components_in_netlist].type == "LSQ" )
				{
					nodes[components_in_netlist].lsq_indx = lsqs_in_netlist;
					lsqs_in_netlist++;
				}
			}
			if ( parameter.find("in=") != std::string::npos )
			{
				//cout << " nodes " << nodes[components_in_netlist].name << endl;
				nodes[components_in_netlist].inputs = get_component_inputs (parameters[indx], components_in_netlist );
			}
			if ( parameter.find("out=") != std::string::npos )
			{
				//cout << " nodes " << nodes[components_in_netlist].name << endl;
				nodes[components_in_netlist].outputs = get_component_outputs(parameters[indx]);
			}
			if ( parameter.find("op") != std::string::npos )
			{
				nodes[components_in_netlist].component_operator = get_component_operator( parameters[indx] );
			}
			if ( parameter.find("value") != std::string::npos )
			{
				//nodes[components_in_netlist].component_value = protected_stoi( get_component_value ( parameters[indx] ) );
				unsigned long int hex_value;
				hex_value = strtoul( get_component_value( parameters[indx] ).c_str(), 0, 16);
				nodes[components_in_netlist].component_value = hex_value;
			}
			if ( parameter.find("control") != std::string::npos )
			{
				nodes[components_in_netlist].component_control = get_component_control( parameters[indx] );
			}
			if ( parameter.find("slots") != std::string::npos )
			{
				nodes[components_in_netlist].slots = get_component_slots( parameters[indx] );

				//cout << "nodes[components_in_netlist].slots" << nodes[components_in_netlist].slots;

				switch ( nodes[components_in_netlist].slots )
				{
				case 1: //if slots = 1
					//                                 if ( nodes[components_in_netlist].trasparent )
					//                                 {
					//                                      // if transparent = true -> put TEHB
					//                                     nodes[components_in_netlist].type = "TEHB";
					//                                 }
					//                                 else
					//                                 {
					//                                     // if transparent = false -> put OEHB
					//                                     nodes[components_in_netlist].type = "OEHB";
					//
					//                                 }
					//                                 break;
				case 2: //put elasticBuffer (ignore transparent parameter)
				case 0: //put elasticBuffer (ignore transparent parameter)
					;
					break;
				default: // > 2
					nodes[components_in_netlist].type = "Fifo";
					nodes[components_in_netlist].component_operator = nodes[components_in_netlist].type; // For the component without an operator, sets the entity type
					break;
				}

			}
			if ( parameter.find("transparent") != std::string::npos )
			{
				nodes[components_in_netlist].trasparent = get_component_transparent( parameters[indx] );
			}
			if ( parameter.find("memory") != std::string::npos )
			{
				nodes[components_in_netlist].memory = get_component_memory (parameters[indx] );
			}
			if ( parameter.find("bbcount") != std::string::npos )
			{
				nodes[components_in_netlist].bbcount = get_component_bbcount (parameters[indx] );

				//cout << nodes[components_in_netlist].name << " bbcount " << nodes[components_in_netlist].bbcount << endl;
				//cout << nodes[components_in_netlist].name << " inputs.size " << nodes[components_in_netlist].inputs.size << endl;

				if ( nodes[components_in_netlist].bbcount == 0 )
				{
					nodes[components_in_netlist].bbcount = 1;
					nodes[components_in_netlist].inputs.size += 1;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].type = "c";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].bit_size = 32;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].info_type = "fake"; //Andrea 20200128 Try to force 0 to inputs.
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].port = 0; //Andrea 20200211

				}

				//cout << nodes[components_in_netlist].name << " inputs.size " << nodes[components_in_netlist].inputs.size << endl;

			}
			if ( parameter.find("ldcount") != std::string::npos )
			{
				nodes[components_in_netlist].load_count = get_component_bbcount (parameters[indx] );
				if ( nodes[components_in_netlist].load_count == 0 )
				{
					nodes[components_in_netlist].load_count = 1;
					nodes[components_in_netlist].inputs.size += 1;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].type = "l";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].info_type = "a";
                    //Lana 9.6.2021 change to address bitwidth
                    nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].bit_size = nodes[components_in_netlist].address_size;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].port = 0; //Andrea 20200424
					//                     nodes[components_in_netlist].inputs.size += 1;
					//                     nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].type = "l";
					//                     nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].info_type = "d";
					//

					nodes[components_in_netlist].outputs.size += 1;
					nodes[components_in_netlist].outputs.output[nodes[components_in_netlist].outputs.size-1].type = "l";
                    //Lana 9.6.2021 change to data and data bitwidth
                    nodes[components_in_netlist].outputs.output[nodes[components_in_netlist].outputs.size-1].info_type = "d";
                    nodes[components_in_netlist].outputs.output[nodes[components_in_netlist].outputs.size-1].bit_size = nodes[components_in_netlist].data_size;
					nodes[components_in_netlist].outputs.output[nodes[components_in_netlist].outputs.size-1].port = 0; //Andrea 20200424



				}
			}
			if ( parameter.find("stcount") != std::string::npos )
			{
				nodes[components_in_netlist].store_count = get_component_bbcount (parameters[indx] );
				if ( nodes[components_in_netlist].store_count == 0 )
				{
					nodes[components_in_netlist].store_count = 1;
					nodes[components_in_netlist].inputs.size += 1;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].type = "s";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].info_type = "a";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].port = 0; //Andrea 20200424
                    //Lana 9.6.2021 change to address bitwidth
                    nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].bit_size = nodes[components_in_netlist].address_size;//32; //Andrea 20200424


					nodes[components_in_netlist].inputs.size += 1;
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].type = "s";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].info_type = "d";
					nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].port = 0; //Andrea 20200424
                    //Lana 9.6.2021 change to data bitwidth
                    nodes[components_in_netlist].inputs.input[nodes[components_in_netlist].inputs.size-1].bit_size = nodes[components_in_netlist].data_size; //32; //Andrea 20200424


				}
			}
			if ( parameter.find("mem_address") != std::string::npos )
			{
				nodes[components_in_netlist].mem_address = get_component_mem_address ( parameters[indx] );
			}
			if ( parameter.find("bbId") != std::string::npos )
			{
				nodes[components_in_netlist].bbId = get_component_bbId( parameters[indx] );
			}
			if ( parameter.find("portId") != std::string::npos )
			{
				nodes[components_in_netlist].portId = get_component_portId ( parameters[indx] );
				//cout << "component id "<< components_in_netlist << " portId " << nodes[components_in_netlist].portId << endl;
			}
			if ( parameter.find("offset") != std::string::npos )
			{
				nodes[components_in_netlist].offset = get_component_offset ( parameters[indx] );
			}

			if ( parameter.find("fifoDepth") != std::string::npos )
			{
				nodes[components_in_netlist].fifodepth = get_component_bbcount ( parameters[indx] );
            }

            // Jiantao, 14/06/2022, Separate the depth for Load and Store Queue
            if ( parameter.find("fifoDepth_L") != std::string::npos)
            {
                nodes[components_in_netlist].fifodepth_L = get_component_bbcount( parameters[indx] );
            }

            if ( parameter.find("fifoDepth_S") != std::string::npos)
            {
                nodes[components_in_netlist].fifodepth_S = get_component_bbcount( parameters[indx] );
			}

			if ( parameter.find("numLoads") != std::string::npos )
			{
				nodes[components_in_netlist].numLoads = get_component_numloads ( parameters[indx] );
				//cout << "numLoads" << nodes[components_in_netlist].numLoads << endl;
			}
			if ( parameter.find("numStores") != std::string::npos )
			{
				nodes[components_in_netlist].numStores = get_component_numstores( parameters[indx] );
			}
			if ( parameter.find("loadOffsets") != std::string::npos )
			{
				nodes[components_in_netlist].loadOffsets = get_component_numstores( parameters[indx] );
			}
			if ( parameter.find("storeOffsets") != std::string::npos )
			{
				nodes[components_in_netlist].storeOffsets = get_component_numstores( parameters[indx] );
			}
			if ( parameter.find("loadPorts") != std::string::npos )
			{
				nodes[components_in_netlist].loadPorts = get_component_numstores( parameters[indx] );
			}
			if ( parameter.find("storePorts") != std::string::npos )
			{
				//nodes[components_in_netlist].storePorts = get_component_numstores( parameters[indx] );
				nodes[components_in_netlist].storePorts = stripExtension( get_component_numstores( parameters[indx] ), "];" );

			}
			if ( parameter.find("orderings") != std::string::npos){
				nodes[components_in_netlist].orderings = get_component_orderings(parameters[indx]);
			}
			if ( parameter.find("constants") != std::string::npos )
			{
				nodes[components_in_netlist].constants = get_component_constants ( parameters[indx] );
			}


		}
		//                 if ( nodes[components_in_netlist].type == "Entry" )
		//                 {
		//                     nodes[components_in_netlist].inputs.size = 1;
		//                     if ( nodes[components_in_netlist].component_control )
		//                     {
		//                         nodes[components_in_netlist].inputs.input->bit_size = 1;
		//                     }
		//                     nodes[components_in_netlist].inputs.input->bit_size = 32;
		//                     //nodes[components_in_netlist].outputs.output->bit_size = 32;
		//                 }



		if ( nodes[components_in_netlist].type == "Buffer" && nodes[components_in_netlist].slots == 1 )
		{
			if ( nodes[components_in_netlist].trasparent )
			{
				nodes[components_in_netlist].type = "TEHB";
			}
			else
			{
				//nodes[components_in_netlist].type = "OEHB";
			}
			nodes[components_in_netlist].component_operator = nodes[components_in_netlist].type;
		}
		if ( nodes[components_in_netlist].type == "Buffer" && nodes[components_in_netlist].slots == 2 )
		{
			if ( nodes[components_in_netlist].trasparent )
			{
				nodes[components_in_netlist].type = "tFifo";
			}
			nodes[components_in_netlist].component_operator = nodes[components_in_netlist].type;
		}

		if ( nodes[components_in_netlist].type == "Fifo")
		{
			if ( nodes[components_in_netlist].trasparent )
			{
				nodes[components_in_netlist].type = "tFifo";
			}
			else
			{
				nodes[components_in_netlist].type = "nFifo";
			}
			nodes[components_in_netlist].component_operator = nodes[components_in_netlist].type;
		}



		components_in_netlist++;
		if ( components_in_netlist >= MAX_NODES )
		{
			cout << "The number of components in the netlist exceed the maximum allowed "<< MAX_NODES << endl;
		}

	}
}

void parse_line ( string line )
{
	vector<string> v;

	line = check_comments ( line );
	string_split( line, '[', v);

	if ( v.size() > 0 )
	{
		int line_type = check_line ( line );
		if ( line_type == COMPONENT_DESCRIPTION_LINE )
		{
			parse_components ( v[0], v[1] );
		}
		else if ( line_type == COMPONENT_CONNECTION_LINE ) //is a connection line
		{
			parse_connections ( line );
		}
	}
}




void parse_dot ( string filename )
{

	string input_filename = filename + ".dot";
	ifstream inFile(input_filename);
	string strline;

	components_in_netlist = 0;

	if (inFile.is_open())
	{
		while ( inFile )
		{
			getline(inFile, strline );
			parse_line ( strline );
		}

		inFile.close();
	}
	else
	{
		cout << "File " << input_filename << " not found "<< endl << endl<< endl;
		exit ( EXIT_FAILURE );
	}
}


