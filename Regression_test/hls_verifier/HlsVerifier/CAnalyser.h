#ifndef CANALYSER_H
#define CANALYSER_H

#include<string>
#include<vector>

using namespace std;

namespace hls_verify {
    
    struct CFunctionParameter {
        string immeadiate_type;
        string actual_type;
        string parameter_name;
        bool is_pointer;
        bool is_input;
        bool is_output;
        bool is_return;
        bool is_float_type;
        bool is_int_type;
        int array_length;
        vector<int> dims;
        int dt_width;
    };
    
    struct CFunction {
        CFunctionParameter return_val;
        vector<CFunctionParameter> params;
        string function_name;
    };
    
    class CAnalyser {
    public:
        static bool parse_c_function(const string& cSrc, const string& fuvName, CFunction& func);
        static bool parse_as_array_type(const string& str, CFunctionParameter& param);
        static bool parse_as_pointer_type(const string& str, CFunctionParameter& param);
        static bool parse_as_simple_type(const string& str, CFunctionParameter& param);
        static bool param_from_string(const string& str, CFunctionParameter& param);
        static string get_actual_type(const string& cSrc, string type);
        static int get_bit_width(const string& type);
        static bool is_float_type(const string& type);
        static string get_preproc_output(const string& cFilePath, const string& cIncludeDir = "");
    private:
        
    };
}

#endif


