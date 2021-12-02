#ifndef HLSVHDLTB_H
#define HLSVHDLTB_H

#include <string>
#include <vector>

#include "CAnalyser.h"
#include "VerificationContext.h"

using namespace std;

namespace hls_verify {

    class MemElem {
    public:
        bool isArray;

        static string clkPortName;
        static string rstPortName;
        static string ce0PortName;
        static string we0PortName;
        static string dIn0PortName;
        static string dOut0PortName;
        static string addr0PortName;
        static string ce1PortName;
        static string we1PortName;
        static string dIn1PortName;
        static string dOut1PortName;
        static string addr1PortName;
        static string donePortName;
        static string inFileParamName;
        static string outFileParamName;
        static string dataWidthParamName;
        static string addrWidthParamName;
        static string dataDepthParamName;

        string clkSignalName;
        string rstSignalName;
        string doneSignalName;
        string ce0SignalName;
        string we0SignalName;
        string dIn0SignalName;
        string dOut0SignalName;
        string addr0SignalName;
        string ce1SignalName;
        string we1SignalName;
        string dIn1SignalName;
        string dOut1SignalName;
        string addr1SignalName;
        string inFileParamValue;
        string outFileParamValue;
        string dataWidthParamValue;
        string addrWidthParamValue;
        string dataDepthParamValue;
    };

    class Constant {
    public:
        string constName;
        string constValue;
        string constType;
        Constant(string name, string value, string type);
    };

    class HlsVhdlTb {
    public:
        HlsVhdlTb(const VerificationContext& ctx);
        string generate_vhdl_testbench();
        string get_input_filepath_for_param(const CFunctionParameter& param);
        string get_output_filepath_for_param(const CFunctionParameter& param);

    private:
        VerificationContext ctx;
        string duvName;
        string tleName;
        vector<CFunctionParameter> cDuvParams;
        vector<pair<string, string>> duvPortMap;
        vector<Constant> constants;
        vector<MemElem> memElems;

        string get_library_header();
        string get_entitiy_declaration();
        string get_architecture_begin();
        string get_constant_declaration();
        string get_signal_declaration();
        string get_memory_instance_generation();
        string get_duv_instance_generation();
        string get_duv_component_declaration();
        string get_mem_component_declration();
        string get_arg_component_declration();
        string get_common_body();
        string get_architecture_end();
        string get_output_tag_generation();
        int get_transaction_number_from_input();


        static string get_ce0_port_name_for_cParam(string& cParam);
        static string get_we0_port_name_for_cParam(string& cParam);
        static string get_data_in0_port_name_for_cParam(string& cParam);
        static string get_data_out0_port_name_for_cParam(string& cParam);
        static string get_addr0_port_name_for_cParam(string& cParam);
        static string get_ce1_port_name_for_cParam(string& cParam);
        static string get_we1_port_name_for_cParam(string& cParam);
        static string get_data_in1_port_name_for_cParam(string& cParam);
        static string get_data_out1_port_name_for_cParam(string& cParam);
        static string get_addr1_port_name_for_cParam(string& cParam);
        static string get_ready_in_port_name_for_cParam(string& cParam);
        static string get_ready_out_port_name_for_cParam(string& cParam);
        static string get_valid_in_port_name_for_cParam(string& cParam);
        static string get_valid_out_port_name_for_cParam(string& cParam);
        static string get_data_inSA_port_name_for_cParam(string& cParam);
        static string get_data_outSA_port_name_for_cParam(string& cParam);


    };
}

#endif

