/*
 * string_utility.h
 *
 *  Created on: 01-Jun-2021
 *      Author: madhur
 */

#ifndef STRING_UTILITY_H_
#define STRING_UTILITY_H_

#include <string>
#include <cstring>

std::string substring(std::string __str, int start, int stop);
//std::string substring(char* __str, int start, int stop);
std::string trim(std::string __str);



#endif /* STRING_UTILITY_H_ */
