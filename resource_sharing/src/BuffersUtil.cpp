/*
 * BuffersUtil.cpp
 *
 *  Created on: May 17, 2020
 *      Author: dynamatic
 */
#include "BuffersUtil.h"


#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <boost/regex.h>

/**
 * taken from
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
 */
string exec(string command) {
	const char * cmd = command.c_str();
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


vector<string> getThroughputFromFile(std::string filename, bool verbose, int timeout){
	cout << "running MILP with timeout : " << to_string(timeout) << endl;
	string res = verbose ? exec("buffers buffers -filename=" + filename + " -period=5 -timeout=" + to_string(timeout)) :
			exec("buffers buffers -filename=" + filename + " -period=5 -timeout="  + to_string(timeout) + " | grep \"Throughput ach\"");
	stringstream ss(res);
	string token;
	vector<string> throughputs{};
    while (getline(ss, token, '\n')) {
	    throughputs.push_back(token);
	}
	return throughputs;
}

