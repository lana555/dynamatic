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


#include <string>
#include <vector>

using namespace std;


typedef struct ui_cmd_t
{
	char *cmd;
	char *help;
	int (*function) ( string input_cmp );
} UI_CMD_T;

#define UI_CMD_HELP	"help"

enum
{
  CMD_HELP1,
  CMD_HELP2,
  CMD_SOURCE,
  CMD_PROJ,
  CMD_ADD_FILE,
  CMD_SET_PERIOD,
  CMD_SET_TARGET,
  CMD_ANALYZE,
  CMD_ELABORATE,
  CMD_SYNTHESIZE,
  CMD_OPTIMIZE,
  CMD_WRITE_HDL,
  CMD_REPORTS,
  CMD_CDFG,
  CMD_STATUS,
  CMD_SIMULATE,
  CMD_UPDATE,
  CMD_HISTORY,
  CMD_ABOUT,
  CMD_EXIT,
  CMD_MAX
};

static UI_CMD_T ui_cmds[] =
{
	{"help","         : Shows Available commands"		},
	{"?","            : Shows Available commands"		},
	{"source","       : Source a script file"},
	{"set_project","  : Set the project directory"},
	{"set_top_file"," : Set the top level file"},
	{"set_period","   : Set the hardware period"},
	{"set_target","   : Set target FPGA"},
    {"analyze","      : Analyze source"				},
	{"elaborate","    : Elaborate source"				},
	{"synthesize","   : C synthesis"				},
	{"optimize","     : Timing optmizations"				},
	{"write_hdl","    : Generate VHDL"				},
	{"reports","      : Report resources and timing "},
	{"cdfg","         : Show control data flow graph "},
	{"status","       : Report design status "},
	{"simulate","     : Simulation "},
	{"update","       : Check for updates "},
	{"history","      : History command list "},
 	{"about","        : Disclaimers & Copyrights"		},
    {"exit","         : Exit"				},
 	{NULL,		NULL,	0}
};



