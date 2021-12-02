#include "VerificationContext.h"
#include "HlsLogging.h"

namespace hls_verify {

    const string Properties::KEY_MODELSIM_DIR = "MSIM_DIRECTORY";
    const string Properties::KEY_VHDL_SRC_DIR = "VHDL_SRC_DIRECTORY";
    const string Properties::KEY_VHDL_OUT_DIR = "VHDL_OUT_DIRECTORY";
    const string Properties::KEY_INPUT_DIR = "INPUT_FILE_DIRECTORY";
    const string Properties::KEY_C_SRC_DIR = "C_SRC_DIRECTORY";
    const string Properties::KEY_C_OUT_DIR = "C_OUT_DIRECTORY";
    const string Properties::KEY_MODELSIM_DO_FILE = "MODELSIM_DO_FILE_NAME";
    const string Properties::KEY_REF_OUT_DIR = "REF_OUT_DIRECTORY";
    const string Properties::KEY_HLSVERIFY_DIR = "HLS_VERIFY_DIR";
    const string Properties::KEY_FLOAT_COMPARE_THRESHOLD = "FLOAT_COMPARE_THRESHOLD";
    const string Properties::KEY_DOUBLE_COMPARE_THRESHOLD = "DOUBLE_COMPARE_THRESHOLD";
    const string Properties::DEFAULT_MODELSIM_DIR = "MGC_MSIM";
    const string Properties::DEFAULT_VHDL_SRC_DIR = "VHDL_SRC";
    const string Properties::DEFAULT_VHDL_OUT_DIR = "VHDL_OUT";
    const string Properties::DEFAULT_INPUT_DIR = "INPUT_VECTORS";
    const string Properties::DEFAULT_C_SRC_DIR = "C_SRC";
    const string Properties::DEFAULT_C_OUT_DIR = "C_OUT";
    const string Properties::DEFAULT_MODLELSIM_DO_FILE = "simulation.do";
    const string Properties::DEFAULT_REF_OUT_DIR = "REF_OUT";
    const string Properties::DEFAULT_HLSVERIFY_DIR = "HLS_VERIFY";
    const string Properties::DEFAULT_FLOAT_COMPARE_THRESHOLD = "1";
    const string Properties::DEFAULT_DOUBLE_COMPARE_THRESHOLD = "0.000000000001";

    Properties::Properties() {
        properties[KEY_MODELSIM_DIR] = DEFAULT_MODELSIM_DIR;
        properties[KEY_VHDL_SRC_DIR] = DEFAULT_VHDL_SRC_DIR;
        properties[KEY_VHDL_OUT_DIR] = DEFAULT_VHDL_OUT_DIR;
        properties[KEY_INPUT_DIR] = DEFAULT_INPUT_DIR;
        properties[KEY_C_SRC_DIR] = DEFAULT_C_SRC_DIR;
        properties[KEY_C_OUT_DIR] = DEFAULT_C_OUT_DIR;
        properties[KEY_MODELSIM_DO_FILE] = DEFAULT_MODLELSIM_DO_FILE;
        properties[KEY_REF_OUT_DIR] = DEFAULT_REF_OUT_DIR;
        properties[KEY_HLSVERIFY_DIR] = DEFAULT_HLSVERIFY_DIR;
        properties[KEY_FLOAT_COMPARE_THRESHOLD] = DEFAULT_FLOAT_COMPARE_THRESHOLD;
        properties[KEY_DOUBLE_COMPARE_THRESHOLD] = DEFAULT_DOUBLE_COMPARE_THRESHOLD;
    }

    Properties::Properties(const string& properties_file_name) : Properties() {
        throw string("Not implemented: " + string(__FILE__) + ":" + to_string(__LINE__));
    }

    string Properties::get(const string& key) const {
        if (properties.count(key)) {
            //return properties.
            return properties.at(key);
            //return properties[key];
        } else {
            throw "Key \"" + key + "\" not found.";
        }
    }

    VerificationContext::VerificationContext(string c_tb_path, string c_fuv_path,
            string c_fuv_function_name, string vhdl_duv_entity_name, vector<string> other_c_src_paths) :
    c_tb_path(c_tb_path), c_fuv_path(c_fuv_path), c_fuv_function_name(c_fuv_function_name),
    vhdl_duv_entity_name(vhdl_duv_entity_name), other_c_src_paths(other_c_src_paths), properties() {
        string preprocessed_c_tb = CAnalyser::get_preproc_output(get_c_tb_path(), extract_parent_directory_path(get_c_tb_path()));
        CAnalyser::parse_c_function(preprocessed_c_tb, get_c_fuv_function_name(), fuv);
        defaultComparator = TokenCompare();
        unsignedIntComparator = IntegerCompare(false);
        signedIntComparator = IntegerCompare(true);
        floatComparator = FloatCompare(stof(properties.get(Properties::KEY_FLOAT_COMPARE_THRESHOLD)));
        doubleComparator = DoubleCompare(stod(properties.get(Properties::KEY_FLOAT_COMPARE_THRESHOLD)));
        
    }

    string VerificationContext::get_c_tb_path() const {
        return c_tb_path;
    }

    string VerificationContext::get_c_fuv_path() const {
        return c_fuv_path;
    }

    string VerificationContext::get_c_fuv_function_name() const {
        if (c_fuv_function_name.size() > 0) {
            return c_fuv_function_name;
        } else { // if function name is not set, assume it is the same as the entitiy name
            return vhdl_duv_entity_name;
        }
    }

    string VerificationContext::get_vhdl_duv_entity_name() const {
        if (vhdl_duv_entity_name.size() > 0) {
            return vhdl_duv_entity_name;
        } else { // if entitiy name is not set, assume it is the same as the function name
            return c_fuv_function_name;
        }
    }

    vector<string> VerificationContext::get_other_c_src_paths() const {
        return other_c_src_paths;
    }

    string VerificationContext::get_c_executable_path() const {
        return extract_parent_directory_path(c_fuv_path) + "/" + "hls_verify_" + get_c_fuv_function_name() + ".out";
    }

    string VerificationContext::get_vhdl_testbench_path() const {
        return get_vhdl_src_dir() + "/" + "hls_verify_" + get_vhdl_duv_entity_name() + "_tb.vhd";
    }

    string VerificationContext::get_modelsim_do_file_name() const {
        return properties.get(Properties::KEY_MODELSIM_DO_FILE);
    }

    string VerificationContext::get_injected_c_fuv_path() const {
        return extract_parent_directory_path(c_fuv_path) + "/" + "hls_verify_" + get_c_fuv_function_name() + ".c";
    }

    string VerificationContext::get_c_out_dir() const {
        return get_base_dir() + "/" + properties.get(Properties::KEY_C_OUT_DIR);
    }

    string VerificationContext::get_c_out_path(const CFunctionParameter& param) const {
        if (!param.is_output) throw string("Parameter " + param.parameter_name + " is not an output type.");
        return get_c_out_dir() + "/output_" + param.parameter_name + ".dat";
    }

    string VerificationContext::get_input_vector_dir() const {
        return get_base_dir() + "/" + properties.get(Properties::KEY_INPUT_DIR);
    }

    string VerificationContext::get_input_vector_path(const CFunctionParameter& param) const {
        if (!param.is_input) throw string("Parameter " + param.parameter_name + " is not an input type.");
        return get_input_vector_dir() + "/input_" + param.parameter_name + ".dat";
    }

    string VerificationContext::get_ref_out_dir() const {
        return get_base_dir() + "/" + properties.get(Properties::KEY_REF_OUT_DIR);
    }

    string VerificationContext::get_ref_out_path(const CFunctionParameter& param) const {
        if (!param.is_output) throw string("Parameter " + param.parameter_name + " is not an output type.");
        return get_ref_out_dir() + "/output_" + param.parameter_name + ".dat";
    }

    string VerificationContext::get_vhdl_out_dir() const {
        return get_base_dir() + "/" + properties.get(Properties::KEY_VHDL_OUT_DIR);
    }

    string VerificationContext::get_vhdl_out_path(const CFunctionParameter& param) const {
        if (!param.is_output) throw string("Parameter " + param.parameter_name + " is not an output type.");
        return get_vhdl_out_dir() + "/output_" + param.parameter_name + ".dat";
    }

    string VerificationContext::get_base_dir() const {
        return "..";
    }

    string VerificationContext::get_hls_verify_dir() const {
        return ".";
    }

    string VerificationContext::get_vhdl_src_dir() const {
        return get_base_dir() + "/" + properties.get(Properties::KEY_VHDL_SRC_DIR);
    }

    const TokenCompare * VerificationContext::get_token_comparator(const CFunctionParameter& param) const {
        if (param.is_int_type) {
            if(param.actual_type.find("unsigned") != string::npos)
                return &unsignedIntComparator;
            else
                return &signedIntComparator;
        }
        if (param.is_float_type && param.dt_width == 32) {
            return &floatComparator;
        }
        if (param.is_float_type && param.dt_width == 64) {
            return &doubleComparator;
        }
        return &defaultComparator;
    }

    CFunction VerificationContext::get_c_fuv() const {
        return fuv;
    }

    vector<CFunctionParameter> VerificationContext::get_fuv_input_params() const {
        vector<CFunctionParameter> result;
        for (auto it = fuv.params.begin(); it != fuv.params.end(); it++) {
            if (it->is_input) {
                result.push_back(*it);
            }
        }
        return result;
    }

    vector<CFunctionParameter> VerificationContext::get_fuv_output_params() const {
        vector<CFunctionParameter> result;
        if (fuv.return_val.is_output) {
            result.push_back(fuv.return_val);
        }
        for (auto it = fuv.params.begin(); it != fuv.params.end(); it++) {
            if (it->is_output) {
                result.push_back(*it);
            }
        }
        return result;
    }

    vector<CFunctionParameter> VerificationContext::get_fuv_params() const {
        vector<CFunctionParameter> result;
        //if (fuv.return_val.is_output) {
            result.push_back(fuv.return_val);
        //}
        for (auto it = fuv.params.begin(); it != fuv.params.end(); it++) {
            result.push_back(*it);
        }
        return result;
    }

}
