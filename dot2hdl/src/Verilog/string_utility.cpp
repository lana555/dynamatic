/*
 * string_utility.cpp
 *
 *  Created on: 01-Jun-2021
 *      Author: madhur
 */

#include "string_utility.h"

//String Utility Functions
//Similar to java substring function
std::string substring(std::string __str, int start, int stop){
	std::string ret(&__str[0] + start, stop - start);
	return ret;
}

//Removes one space from the end of string if present
std::string trim(std::string __str){
	if(*__str.end() == ' '){
		return substring(__str, 0, __str.size() - 1);
	}
	else
		return __str;
}
