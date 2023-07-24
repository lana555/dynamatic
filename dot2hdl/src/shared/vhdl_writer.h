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

#ifndef _VHDL_WRITER_
#define _VHDL_WRITER_
#include <string>
using namespace std;

#define COMPONENT_GENERIC   0
#define COMPONENT_CONSTANT  1

#define CLK "clk"
#define RST "rst"
#define DATAIN_ARRAY "dataInArray"
#define DATAOUT_ARRAY "dataOutArray"
#define PVALID_ARRAY "pValidArray"
#define NREADY_ARRAY "nReadyArray"
#define READY_ARRAY "readyArray"
#define VALID_ARRAY "validArray"



#define MAX_COMPONENTS  6




#define ENTITY_MERGE            "merge"
#define ENTITY_READ_MEMORY      "read_memory_single"
#define ENTITY_WRITE_MEMORY     "write_memory_single"
#define ENTITY_SINGLE_OP        "SingleOp"
#define ENTITY_GET_PTR          "getptr"
#define ENTITY_INT_MUL          "intMul"
#define ENTITY_INT_ADD          "intAdd"
#define ENTITY_INT_SUB          "intSub"
#define ENTITY_BUF              "elasticBuffer"
#define ENTITY_TEHB             "TEHB"
#define ENTITY_OEHB             "OEHB"
#define ENTITY_FIFO             "elasticFifo"
#define ENTITY_NFIFO            "nontranspFifo"
#define ENTITY_TFIFO            "transpFifo"
#define ENTITY_FORK             "fork"
#define ENTITY_LFORK            "lazyfork"
#define ENTITY_ICMP             "intCmp"
#define ENTITY_CONSTANT         "Const"
#define ENTITY_BRANCH           "Branch"
#define ENTITY_END              "end_node"
#define ENTITY_START            "start_node"
#define ENTITY_SOURCE           "source"
#define ENTITY_SINK             "sink"
#define ENTITY_MUX              "Mux"
#define ENTITY_CTRLMERGE        "CntrlMerge"
#define ENTITY_LSQ              "LSQ"
#define ENTITY_MC               "MemCont"
#define ENTITY_SEL				"SEL"
#define ENTITY_DISTRIBUTOR      "Distributor"
#define ENTITY_SELECTOR			"Selector"
#define ENTITY_INJECTOR			"Inj"

#define COMPONENT_MERGE         "Merge"
#define COMPONENT_READ_MEMORY   "load"
#define COMPONENT_WRITE_MEMORY  "store"
#define COMPONENT_SINGLE_OP     "Merge"
#define COMPONENT_GET_PTR       "Merge"
#define COMPONENT_INT_MUL       "mul"
#define COMPONENT_INT_ADD       "add"
#define COMPONENT_INT_SUB       "sub"
#define COMPONENT_BUF           "Buffer"
#define COMPONENT_TEHB          "TEHB"
#define COMPONENT_OEHB          "OEHB"
#define COMPONENT_FIFO          "Fifo"
#define COMPONENT_NFIFO         "nFifo"
#define COMPONENT_TFIFO         "tFifo"
#define COMPONENT_FORK          "Fork"
#define COMPONENT_LFORK          "LazyFork"
#define COMPONENT_ICMP          "icmp"
#define COMPONENT_CONSTANT_     "Constant"
#define COMPONENT_BRANCH        "Branch"
#define COMPONENT_END           "Exit"
#define COMPONENT_START         "Entry"
#define COMPONENT_SOURCE        "Source"
#define COMPONENT_SINK          "Sink"
#define COMPONENT_MUX           "Mux"
#define COMPONENT_CTRLMERGE     "CntrlMerge"
#define COMPONENT_LSQ           "LSQ"
#define COMPONENT_MC            "MC"
#define COMPONENT_SEL			"SEL"
#define COMPONENT_DISTRIBUTOR   "Distributor"
#define COMPONENT_SELECTOR		"Selector"
#define COMPONENT_OPERATOR		"Operator"
#define COMPONENT_INJECTOR		"Inj"



#define OPERATOR_ADD			"add_op"
#define OPERATOR_SUB			"sub_op"
#define OPERATOR_MUL			"mul_op"
#define OPERATOR_UREM			"urem_op"

#define OPERATOR_AND			"and_op"
#define OPERATOR_OR				"or_op"
#define OPERATOR_XOR			"xor_op"

#define OPERATOR_SHL			"shl_op"
#define OPERATOR_LSHR			"lshr_op"
#define OPERATOR_ASHR			"ashr_op"

#define OPERATOR_EQ				"icmp_eq_op"
#define OPERATOR_NE				"icmp_ne_op"

#define OPERATOR_UGT			"icmp_ugt_op"
#define OPERATOR_UGE			"icmp_uge_op"
#define OPERATOR_SGT			"icmp_sgt_op"
#define OPERATOR_SGE			"icmp_sge_op"

#define OPERATOR_ULT			"icmp_ult_op"
#define OPERATOR_ULE			"icmp_ule_op"
#define OPERATOR_SLT			"icmp_slt_op"
#define OPERATOR_SLE			"icmp_sle_op"

#define OPERATOR_SELECT			"select_op"

#define OPERATOR_RET			"ret_op"

#define OPERATOR_SEXT			"sext_op"
#define OPERATOR_ZEXT			"zext_op"
#define OPERATOR_GETPTR			"getelementptr_op"

#define OPERATOR_READ_MEMORY	"mc_load_op"
#define OPERATOR_WRITE_MEMORY	"mc_store_op"

#define OPERATOR_READ_LSQ		"lsq_load_op"
#define OPERATOR_WRITE_LSQ		"lsq_store_op"


#define UNDERSCORE  "_"
#define COLOUMN " : "
#define SEMICOLOUMN ";"
#define COMMA   ","


#define DEFAULT_MODULE_NAME "nomodulename"
#define DEFAULT_INSTANCE_NAME "noinstancename"
#define DEFAULT_NAME "noname"
#define DEFAULT_TYPE "notype"
#define DEFAULT_BBID -1
#define DEFAULT_IN NULL
#define DEFAULT_OUT NULL
#define DEFAULT_DELAY 0.0f
#define DEFAULT_LATENCY 0.0f
#define DEFAULT_II 1
#define DEFAULT_SLOTS -1
#define DEFAULT_TRANSPARENT false
#define DEFAULT_OP "nop"
#define DEFAULT_VALUE 0

#define SIGNAL_STRING  "signal "
#define STD_LOGIC      "std_logic;"

#define DEFAULT_BITWIDTH    32

class vhdl_writer {
    
public:
    void write_vhdl ( string filename, int indx );
    void write_tb_wrapper ( string filename  );
    
private:



};


enum
{
    ENTITY_MERGE_INDX,            
    ENTITY_READ_MEMORY_INDX,      
    ENTITY_SINGLE_OP_INDX,        
    ENTITY_GET_PTR_INDX,          
    ENTITY_INT_MUL_INDX,          
    ENTITY_INT_ADD_INDX, 
    ENTITY_INT_SUB_INDX, 
    ENTITY_BUF_INDX,
    ENTITY_TEHB_INDX,
    ENTITY_OEHB_INDX,
    ENTITY_FIFO_INDX,
    ENTITY_NFIFO_INDX,
    ENTITY_TFIFO_INDX,
    ENTITY_FORK_INDX,
    ENTITY_ICMP_INDX,
    ENTITY_CONSTANT_INDX,
    ENTITY_BRANCH_INDX,
    ENTITY_END_INDX,
    ENTITY_START_INDX,   
    ENTITY_WRITE_MEMORY_INDX,
    ENTITY_SOURCE_INDX,
    ENTITY_SINK_INDX,
    ENTITY_MUX_INDX,
    ENTITY_CTRLMERGE_INDX,
    ENTITY_LSQ_INDX,
    ENTITY_MC_INDX,
    ENTITY_SEL_INDX,
    ENTITY_DISTRIBUTOR_INDX,
    ENTITY_SELECTOR_INDX,
    ENTITY_MAX
};



#endif
