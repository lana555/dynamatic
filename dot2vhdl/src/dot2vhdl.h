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


#ifndef _DOT2VHDL_
#define _DOT2VHDL_


using namespace std;

#define INIT_STRING	"\n\r\n\r\
******************************************************************\n\r\
******Dynamic High-Level Synthesis Compiler **********************\n\r\
******Andrea Guerrieri - Lana Josipovic - EPFL-LAP 2019 **********\n\r\
******Dot to VHDL Generator***************************************\n\r\
******************************************************************\n\r\
";
//#define VERSION_STRING	"******Version 0.1 - Build 0.1 ************************************\n\r******************************************************************\n\r\n\r"

#define VERSION_STRING "2.0"

#define BUG_STRING	"\n\r\n\r\
***********************************************************************\n\r\
Illegal netlist: Please send an email with your netlist to ************\n\r\
andrea.guerrieri@epfl.ch | lana.josipovic@epfl.ch to report the error**\n\r\
Thank you!*************************************************************\n\r\
***********************************************************************\n\r\
";


#define MAX_INPUT_FILES 16

extern int debug_mode;

extern string input_filename[MAX_INPUT_FILES];
extern string output_filename[MAX_INPUT_FILES];
extern string top_level_filename;
extern int dot_input_files;

#define FALSE   0
#define TRUE    1

#endif
