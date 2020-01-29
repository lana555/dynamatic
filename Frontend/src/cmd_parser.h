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


using namespace std;

//int parser ( char *input_cmp );
void cmd_parser_init ( void );
int cmd_parser ( string input_cmp );

//extern char filename[256];
extern string filename;

#define MAX_HISTORY 4096
extern string command_history[MAX_HISTORY];
extern int history_indx;

enum 
{
    DESIGN_STATUS_BEGIN,
    DESIGN_STATUS_FILE_SET,
    DESIGN_STATUS_ANALYZED,    
    DESIGN_STATUS_ELABORATED,
    DESIGN_STATUS_SYNTHESIZED,
    DESIGN_STATUS_OPTIMIZED
};

extern int design_status;
