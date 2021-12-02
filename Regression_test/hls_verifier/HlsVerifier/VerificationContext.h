#ifndef VERIFICATIONCONTEXT_H
#define VERIFICATIONCONTEXT_H

#include <string>
#include <vector>
#include <map>

#include "CAnalyser.h"
#include "Utilities.h"

using namespace std;

namespace hls_verify {

    class Properties {
    public:
        static const string KEY_MODELSIM_DIR;
        static const string KEY_VHDL_SRC_DIR;
        static const string KEY_VHDL_OUT_DIR;
        static const string KEY_INPUT_DIR;
        static const string KEY_C_SRC_DIR;
        static const string KEY_C_OUT_DIR;
        static const string KEY_MODELSIM_DO_FILE;
        static const string KEY_REF_OUT_DIR;
        static const string KEY_HLSVERIFY_DIR;
        static const string KEY_FLOAT_COMPARE_THRESHOLD;
        static const string KEY_DOUBLE_COMPARE_THRESHOLD;
        static const string DEFAULT_MODELSIM_DIR;
        static const string DEFAULT_VHDL_SRC_DIR;
        static const string DEFAULT_VHDL_OUT_DIR;
        static const string DEFAULT_INPUT_DIR;
        static const string DEFAULT_C_SRC_DIR;
        static const string DEFAULT_C_OUT_DIR;
        static const string DEFAULT_MODLELSIM_DO_FILE;
        static const string DEFAULT_REF_OUT_DIR;
        static const string DEFAULT_HLSVERIFY_DIR;
        static const string DEFAULT_FLOAT_COMPARE_THRESHOLD;
        static const string DEFAULT_DOUBLE_COMPARE_THRESHOLD;

        Properties();
        Properties(const string& properties_file_name);

        string get(const string& key) const;
    private:
        map<string, string> properties;
    };

    class VerificationContext {
    public:
        VerificationContext(string c_tb_path, string c_fuv_path, string c_fuv_function_name, string vhdl_duv_entity_name, vector<string> other_c_src_paths);

        string get_c_tb_path() const;
        string get_c_fuv_path() const;
        vector<string> get_other_c_src_paths() const;
        string get_c_fuv_function_name() const;
        string get_vhdl_duv_entity_name() const;

        string get_injected_c_fuv_path() const;
        string get_c_executable_path() const;
        string get_vhdl_testbench_path() const;

        string get_base_dir() const;
        string get_hls_verify_dir() const;
        string get_vhdl_src_dir() const;

        string get_c_out_dir() const;
        string get_ref_out_dir() const;
        string get_vhdl_out_dir() const;
        string get_input_vector_dir() const;

        string get_c_out_path(const CFunctionParameter& param) const;
        string get_ref_out_path(const CFunctionParameter& param) const;
        string get_vhdl_out_path(const CFunctionParameter& param) const;
        string get_input_vector_path(const CFunctionParameter& param) const;

        string get_modelsim_do_file_name() const;

        const TokenCompare * get_token_comparator(const CFunctionParameter& param) const;

        CFunction get_c_fuv() const;
        vector<CFunctionParameter> get_fuv_output_params() const;
        vector<CFunctionParameter> get_fuv_input_params() const;
        vector<CFunctionParameter> get_fuv_params() const;

        
        bool use_addr_width_32;
    private:
        Properties properties;
        CFunction fuv;
        string c_tb_path;
        string c_fuv_path;
        std::vector< string> other_c_src_paths;
        string c_fuv_function_name;
        string vhdl_duv_entity_name;
        TokenCompare defaultComparator;
        IntegerCompare signedIntComparator;
        IntegerCompare unsignedIntComparator;
        FloatCompare floatComparator;
        DoubleCompare doubleComparator;
    };

}
#endif 
