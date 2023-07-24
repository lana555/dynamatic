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
#include "dot2hdl.h"
#include "dot_parser.h"
#include "vhdl_writer.h"
#include "eda_if.h"
#include "lsq_generator.h"

#include <csignal>

using namespace std;

void signalhand ( void )
{
    cout << BUG_STRING;
    exit ( 0 );
}


void signal_handler ( void )
{
    signal(SIGABRT, signalhand);  
    signal(SIGFPE, signalhand);  
    signal(SIGILL, signalhand);  
    signal(SIGINT, signalhand);  
    signal(SIGSEGV, signalhand);  
    signal(SIGTERM, signalhand);  
}



