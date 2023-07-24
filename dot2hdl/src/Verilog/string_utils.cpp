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
#include <stdexcept>

#include "dot_parser.h"
#include "vhdl_writer.h"
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include "string_utils.h"
#include <bitset>
#include <sstream>      // std::stringstream

using namespace std;

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

string string_remove_blank ( string string_input )
{
    string_input.erase( remove( string_input.begin(), string_input.end(), ' ' ), string_input.end() );
    return string_input;
}

string string_clean ( string string_input )
{
    string_input.erase( remove( string_input.begin(), string_input.end(), '\t' ), string_input.end() );

    string_input.erase( remove( string_input.begin(), string_input.end(), ' ' ), string_input.end() );
    string_input.erase( remove( string_input.begin(), string_input.end(), '"' ), string_input.end() );
    string_input.erase( remove( string_input.begin(), string_input.end(), ']' ), string_input.end() );
    string_input.erase( remove( string_input.begin(), string_input.end(), ';' ), string_input.end() );
        
    return string_input;

}




string stripExtension( string string_input, string extension )
{
    int dot = string_input.rfind(extension);
    if (dot != std::string::npos)
    {
        string_input.resize(dot);
    }
    
    return string_input;
}



int stoi_p ( string str )
{
    int x = 0;
    try 
    {
        x = stoi(str);
    }
    catch(std::invalid_argument& e)
    {
    // if no conversion could be performed
    }
    catch(std::out_of_range& e)
    {
    // if the converted value would fall out of the range of the result type 
    // or if the underlying function (std::strtol or std::strtoull) sets errno 
    // to ERANGE.
    }
    catch(...) {
       cout << "Illegal netlist" << endl; 
    // everything else
    }
    
    return x;
}


string string_constant ( unsigned long int value, int size )
{

    string str_cst;
    
    stringstream ss; //= to_string(  nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value );
//#define CONSTANT_HEX
    #ifdef CONSTANT_HEX
        //X"00000000"
        str_cst = "X\"";
        int fill_value = 8;
        fill_value = nodes[nodes[i].outputs.output[indx].next_nodes_id].outputs.output[0].bit_size / 4 ;
        ss << setfill('0') << setw( fill_value ) << hex << nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value;
        //cout << "fill_value" << fill_value <<  endl;
    #else
        
        //unsigned long int num = nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value;
        
#if 1                               
        //cout << num << endl;
        //cout << std::bitset<64>(num) << endl;
        //str_cst = "\"";
 
        //cout << "size" << size << endl;
        switch ( size )
        {
            case 1:
                ss << std::bitset<1>( value );
                break;
            case 2:
                ss << std::bitset<2>( value );
                break;
            case 3:
                ss << std::bitset<3>( value );
                break;
            case 4:
                ss << std::bitset<4>( value );
                break;
            case 5:
                ss << std::bitset<5>( value );
                break;
            case 6:
                ss << std::bitset<6>( value );
                break;
            case 7:
                ss << std::bitset<7>( value );
                break;
            case 8:
                ss << std::bitset<8>( value );
                break;
            case 9:
                ss << std::bitset<9>( value );
                break;
            case 10:
                ss << std::bitset<10>( value );
                break;
            case 11:
                ss << std::bitset<11>( value );
                break;
            case 12:
                ss << std::bitset<12>( value );
                break;
            case 13:
                ss << std::bitset<13>( value );
                break;
            case 14:
                ss << std::bitset<14>( value );
                break;
            case 15:
                ss << std::bitset<15>( value );
                break;
            case 16:
                ss << std::bitset<16>( value );
                break;                
            default:
                if ( ( value > 0xFFFFFFFF ) && ( size > 32 ) )
                {
                    ss << std::bitset<64>( value );
                }
                else
                {
                    ss << std::bitset<32>( value );
                }
                break;
        }
        
        
#endif

#if 0
        unsigned int rem;
        unsigned int binary = 0;
        unsigned int base = 1;
        while( num > 0 )
        {
            rem=num%2;
            binary=binary+rem*base;
            num=num/2;
            base=base*10;
        }
        
        signal_2 = "\"";
        int fill_value = 8;
        fill_value = nodes[nodes[i].outputs.output[indx].next_nodes_id].outputs.output[0].bit_size ;
        //ss << bitset<fill_value> (nodes[nodes[i].outputs.output[indx].next_nodes_id].component_value);
        ss << setfill('0') << setw( fill_value ) << binary;
#endif                                
        
    #endif

     str_cst += ss.str(); 
     return str_cst;
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

string clean_entity ( string filename )
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
    
    
    eraseAllSubStr ( return_string , "_elaborated" );
    eraseAllSubStr ( return_string , "_optimized" );
    eraseAllSubStr ( return_string , "_area" );

    return return_string;
}




