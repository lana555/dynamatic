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


#ifndef _STRING_UTILS_
#define _STRING_UTILS_


#include <string>

using namespace std;

void string_split(const string& s, char c, vector<string>& v);
string string_remove_blank ( string string_input );
string string_clean ( string string_input );
int stoi_p ( string str );
string string_constant ( unsigned long int value , int size );
string clean_entity ( string filename );
string stripExtension( string string_input, string extension );

#endif
