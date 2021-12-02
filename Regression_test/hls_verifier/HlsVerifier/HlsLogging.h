#ifndef HLSLOGGING_H
#define HLSLOGGING_H

#include <iostream>
#include <string>

using namespace std;

#define LOG_DEBUG

#ifdef LOG_DEBUG
    #define log_dbg(x)  std::cout << "[DEBUG " <<  __FILE__ << ": " << __LINE__ << "] " << (x) << endl
#else
    #define log_debug(x) ;
#endif

void log_inf(string tag, string msg);
void log_err(string tag, string msg);
void log_wrn(string tag, string msg);

#endif

