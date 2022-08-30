#pragma once

#define ICMP_NAME "icmp"
#define ADD_NAME "add"
#define SUB_NAME "sub"
#define MUL_NAME "mul"
#define SEXT_NAME "sext"
#define LOAD_NAME "load"
#define STORE_NAME "store"
#define LSQ_LOAD_NAME "lsq_load"
#define LSQ_STORE_NAME "lsq_store"
#define MERGE_NAME "phi_n"
#define GETPTR_NAME "getelementptr"
#define FADD_NAME "fadd"
#define FSUB_NAME "fsub"
#define FMUL_NAME "fmul"
#define UDIV_NAME "udiv"
#define SDIV_NAME "sdiv"
#define FDIV_NAME "fdiv"
#define FCMP_NAME "fcmp"
#define PHIC_NAME "phiC"
#define ZDC_NAME "zdc" // zero-delay component, fake component to force the delay to 0

#define MC_NAME "mc"
#define FORK_NAME "fork"
#define RET_NAME "ret"
#define BRANCH_NAME "branch"
#define END_NAME "end"

#define AND_NAME "and"
#define OR_NAME "or"
#define XOR_NAME "xor"
#define SHL_NAME "shl"
#define ASHR_NAME "ashr"
#define LSHR_NAME "lshr"
#define SELECT_NAME "select"

#define MUX_NAME "phi"


#include <string>

enum {
    ICMP_INDX,
    ADD_INDX,
    SUB_INDX,
    MUL_INDX,
    SEXT_INDX,
    LOAD_INDX,
    STORE_INDX,
    LSQ_LOAD_INDX,
    LSQ_STORE_INDX,
    MERGE_INDX,
    GETPTR_INDX,
    FADD_INDX,
    FSUB_INDX,
    FMUL_INDX,
    UDIV_INDX,
    SDIV_INDX,
    FDIV_INDX,
    FCMP_INDX,
    PHIC_INDX,
    ZDL_INDX,
 //   MEMC_INDX,
    FORK_INDX,
    RET_INDX, 
    BRANCH_INDX,
    END_INDX,
    AND_INDX,
    OR_INDX,
    XOR_INDX,
    SHL_INDX,
    ASHR_INDX,
    LSHR_INDX,
    SELECT_INDX,
    MUX_INDX,
    CMP_MAX,
};

enum {
    BITSIZE_1_INDX,
    BITSIZE_2_INDX,
    BITSIZE_4_INDX,
    BITSIZE_8_INDX,
    BITSIZE_16_INDX,
    BITSIZE_32_INDX,
    BITSIZE_64_INDX,
};

enum{
	DATA,
	VALID,
	READY,
	VR,
	CV,
	CR,
	VC,
	VD,
};

static const std::string components_name[] = {
    ICMP_NAME,  ADD_NAME,   SUB_NAME,    MUL_NAME,  SEXT_NAME, LOAD_NAME,
    STORE_NAME, LSQ_LOAD_NAME, LSQ_STORE_NAME, MERGE_NAME, GETPTR_NAME,
    FADD_NAME, FSUB_NAME, FMUL_NAME, UDIV_NAME,  SDIV_NAME,
    FDIV_NAME,   FCMP_NAME, PHIC_NAME, ZDC_NAME, 
//MC_NAME, 
FORK_NAME, 
RET_NAME, 
BRANCH_NAME, 
END_NAME,

AND_NAME,
OR_NAME,
XOR_NAME,
SHL_NAME,
ASHR_NAME,
LSHR_NAME,
SELECT_NAME,
MUX_NAME

};

static const float datapath_delay[CMP_MAX + 1][10] = {
    {0.784, 0.966, 1.485, 1.342, 1.410, 1.530, 1.770}, // ICMP
    {0.784, 0.989, 1.377, 1.333, 1.453, 1.693, 2.173}, // ADD
    {0.784, 0.989, 1.377, 1.333, 1.453, 1.693, 2.173}, // SUB
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // MUL
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // SEXT
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // load
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // store
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // lsq_load
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // lsq_store
    {0.366, 0.366, 0.366, 0.366, 0.366, 0.366, 0.366}, // merge
    {2.966, 2.966, 2.966, 2.966, 2.966, 2.966, 2.966}, // get_pointer
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // fadd
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // fsub
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // fmul
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // udiv
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // sdiv
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // fdiv
    {0.966, 0.966, 0.966, 0.966, 0.966, 0.966, 0.966}, // fcmp
    {0.166, 0.166, 0.166, 0.166, 0.166, 0.166, 0.166}, // phic
//    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // Memc
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // Fork
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // Ret
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // Branch
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // End
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // 
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000}, // cmp_none
};

static const int datapath_latency[CMP_MAX + 1][10] = {
    {0, 0, 0, 0, 0, 0, 0},        // ICMP
    {0, 0, 0, 0, 0, 0, 0},        // ADD
    {0, 0, 0, 0, 0, 0, 0},        // SUB
    {4, 4, 4, 4, 4, 4, 4},        // MUL
    {0, 0, 0, 0, 0, 0, 0},        // SEXT
    {2, 2, 2, 2, 2, 2, 2},        // load
    {0, 0, 0, 0, 0, 0, 0},        // store
    {5, 5, 5, 5, 5, 5, 5},        // lsq_load
    {0, 0, 0, 0, 0, 0, 0},        // lsq_store
    {0, 0, 0, 0, 0, 0, 0},        // merge
    {0, 0, 0, 0, 0, 0, 0},        // get_pointer
    {10, 10, 10, 10, 10, 10, 10}, // fadd
    {10, 10, 10, 10, 10, 10, 10}, // fsub
    {6, 6, 6, 6, 6, 6, 6},        // fmul
    {36, 36, 36, 36, 36, 36, 36}, // udiv
    {36, 36, 36, 36, 36, 36, 36}, // sdiv
    {30, 30, 30, 30, 30, 30, 30}, // fdiv
    {2, 2, 2, 2, 2, 2, 2},        // fcmp
    {0, 0, 0, 0, 0, 0, 0},        // phic
// {0, 0, 0, 0, 0, 0, 0},// Memc
 {0, 0, 0, 0, 0, 0, 0},// Fork
 {0, 0, 0, 0, 0, 0, 0},// Ret
 {0, 0, 0, 0, 0, 0, 0},// Branch
 {0, 0, 0, 0, 0, 0, 0},// End
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
 {0, 0, 0, 0, 0, 0, 0},//
    {0, 0, 0, 0, 0, 0, 0},        // cmp_none
};

#define ROUTING_DELAY_0 1.0
#define ROUTING_DELAY_10 1.1
#define ROUTING_DELAY_20 1.2
#define ROUTING_DELAY_30 1.3
#define ROUTING_DELAY_40 1.4

extern float get_component_delay(std::string component, int datasize, std::string serial_number, int mode);
extern int get_component_latency(std::string component, int datasize, std::string serial_number);
