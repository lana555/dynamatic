#include <fstream>
#include <sstream>

#include <boost/regex.hpp>

#include "CAnalyser.h"
#include "HlsLogging.h"
#include "Utilities.h"

namespace hls_verify {
    static string LOG_TAG = "CAnalyzer";
    
    /**
     * Trying to parse a function parameter declaration assuming it is of form  "data_type param_name[array_length]".
     * @param str
     * @param param
     * @return true if successful
     */
    bool CAnalyser::parse_as_array_type(const string& str, CFunctionParameter& param) {
        boost::smatch m;
        boost::regex e("^\\s*(\\w+)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*((\\[\\s*(\\d+)\\s*\\])+)\\s*$");
        
        
        // m[0] whole pattern, m[1] param type, m[2] param name, m[3] array length
        
        if (boost::regex_search(str, m, e)) {
            param.parameter_name = m[2];
            param.is_pointer = true;
            param.is_return = false;
            
            string arrLenStr = m[3].str();
            vector<string> dimensions = split(arrLenStr.substr(1, arrLenStr.length() - 2), "[]");
            param.array_length = 1;
            param.dims = vector<int>();
            for (auto it = dimensions.begin(); it != dimensions.end(); it++) {
                int len = 0;
                if (it->substr(0, 2) == "0x") {
                    len = stoi(*it, nullptr, 16);
                } else {
                    len = stoi(*it, nullptr, 10);
                }
                param.dims.push_back(len);
                param.array_length *= len;
            }
            cout << "array length " << param.array_length << endl;
            
            param.immeadiate_type = m[1];
            if (param.immeadiate_type.substr(0, 3) == "in_") {
                param.is_input = true;
                param.is_output = false;
            } else if (param.immeadiate_type.substr(0, 4) == "out_") {
                param.is_input = false;
                param.is_output = true;
            } else if (param.immeadiate_type.substr(0, 6) == "inout_") {
                param.is_input = true;
                param.is_output = true;
            } else {
                log_err(LOG_TAG, "Unable to identify IO type for parameter " + param.parameter_name);
                return false;
            }
            
            log_inf(LOG_TAG, "Parameter identified: " + param.parameter_name + " of type " + param.immeadiate_type + "[" + to_string(param.array_length) + "].");
            return true;
        }
        return false;
    }
    
    /**
     * Trying to parse a function parameter declaration assuming it is of form  "data_type * param_name".
     * @param str
     * @param param
     * @return true if successful
     */
    bool CAnalyser::parse_as_pointer_type(const string& str, CFunctionParameter& param) {
        boost::smatch m;
        boost::regex e("^\\s*(\\w+)\\s*\\*\\s*(\\w+)\\s*$");
        
        //m[0] whole pattern, m[1] param type, m[2] param name
        
        if (boost::regex_search(str, m, e)) {
            param.parameter_name = m[2];
            param.is_pointer = true;
            param.is_return = false;
            
            param.array_length = 1;
            param.dims = vector<int>();
            param.dims.push_back(1);
            
            param.immeadiate_type = m[1];
            if (param.immeadiate_type.substr(0, 3) == "in_") {
                param.is_input = true;
                param.is_output = false;
            } else if (param.immeadiate_type.substr(0, 4) == "out_") {
                param.is_input = false;
                param.is_output = true;
            } else if (param.immeadiate_type.substr(0, 6) == "inout_") {
                param.is_input = true;
                param.is_output = true;
            } else {
                log_err(LOG_TAG, "Unable to identify IO type for parameter " + param.parameter_name);
                return false;
            }
            log_inf(LOG_TAG, "Parameter identified: " + param.parameter_name + " of type " + param.immeadiate_type + "[" + to_string(param.array_length) + "].");
            return true;
        }
        return false;
    }
    
    /**
     * Trying to parse a function parameter declaration assuming it is of form  "data_type * param_name".
     * @param str
     * @param param
     * @return true if successful
     */
    bool CAnalyser::parse_as_simple_type(const string& str, CFunctionParameter& param) {
        boost::smatch m;
        boost::regex e("^\\s*(\\w+)\\s*(\\w+)\\s*\\s*$");
        
        //m[0] whole pattern, m[1] param type, m[2] param name
        
        if (boost::regex_search(str, m, e)) {
            param.parameter_name = m[2];
            param.is_pointer = false;
            param.is_return = false;
            
            param.array_length = 0;
            
            param.immeadiate_type = m[1];
            if (param.immeadiate_type.substr(0, 3) == "in_") {
                param.is_input = true;
                param.is_output = false;
            } else if (param.immeadiate_type.substr(0, 4) == "out_") {
                param.is_input = false;
                param.is_output = true;
            } else if (param.immeadiate_type.substr(0, 6) == "inout_") {
                param.is_input = true;
                param.is_output = true;
            } else {
                log_err(LOG_TAG, "Unable to identify IO type for parameter " + param.parameter_name);
                return false;
            }
            log_inf(LOG_TAG, "Parameter identified: " + param.parameter_name + " of type " + param.immeadiate_type + ".");
            return true;
        }
        return false;
    }
    
    /**
     * Trying to parse a function parameter and identify its attributes.
     * @param str
     * @param param
     * @return true if successful
     */
    bool CAnalyser::param_from_string(const string& str, CFunctionParameter& param) {
        log_inf(LOG_TAG, "Parsing parameter " + str);
        
        if (parse_as_array_type(str, param)) return true;
        if (parse_as_pointer_type(str, param)) return true;
        if (parse_as_simple_type(str, param)) return true;
        
        log_err(LOG_TAG, "Unable to parse parameter from string \"" + str + "\".");
        
        return false;
    }
    
    /**
     * Finds the actual type of a user defined type. (Does not work with pointer types. E.g., typedef int *int_ptr;)
     * @param cSrc C source code to search for the typedef
     * @param type User defined type for which the actual type should be found
     * @return actual primitive type of parameter 'type'
     */
    string CAnalyser::get_actual_type(const string& cSrc, string type) {
        boost::smatch m;
        boost::regex e("typedef\\s+(\\w+(\\s+\\w+)*)\\s+" + type + "\\s*;");
        
        while (boost::regex_search(cSrc, m, e)) {
            type = m[1].str();
            e = boost::regex("typedef\\s+(\\w+(\\s+\\w+)*)\\s+" + type + "\\s*;");
        }
        
        istringstream stream(type);
        string result;
        stream >> result;
        string word;
        while (stream >> word) {
            result = result + " " + word;
        }
        return result;
    }
    
    /**
     * Returns the bit width for a given premitive type.
     * @param type
     * @return number of bits needed to represent a value of type 'type'
     */
    int CAnalyser::get_bit_width(const string& type) {
        
        if (type == "char") return 8;
        if (type == "signed char") return 8;
        if (type == "unsigned char") return 8;
        
        if (type == "short") return 16;
        if (type == "short int") return 16;
        if (type == "signed short") return 16;
        if (type == "signed short int") return 16;
        if (type == "unsigned short") return 16;
        if (type == "unsigned short int") return 16;
        
        if (type == "int") return 32;
        if (type == "unsigned int") return 32;
        if (type == "signed int") return 32;
        if (type == "long") return 32;
        if (type == "unsigned long") return 32;
        if (type == "signed long") return 32;
        if (type == "long int") return 32;
        if (type == "unsigned long int") return 32;
        if (type == "signed long int") return 32;
        if (type == "unsigned") return 32;
        if (type == "signed") return 32;
        
        if (type == "long long") return 64;
        if (type == "unsigned long long") return 64;
        if (type == "signed long long") return 64;
        if (type == "long long int") return 64;
        if (type == "unsigned long long int") return 64;
        if (type == "signed long long int") return 64;
        
        if (type == "float") return 32;
        if (type == "double") return 64;
        if (type == "long double") return 128;
        
        return -1;
    }
    
    /**
     * Checks if a given type is a floating point type
     * @param type
     * @return true if 'type' is a floating point type
     */
    bool CAnalyser::is_float_type(const string& type) {
        if (type == "float") return true;
        if (type == "double") return true;
        if (type == "long double") return true;
        return false;
    }
    
    string CAnalyser::get_preproc_output(const string& cFilePath, const string& cIncludeDir) {
        string preProcCmd = "gcc -E " + cFilePath + " -I ";
        if (cIncludeDir.empty()) {
            preProcCmd += extract_parent_directory_path(cFilePath);
        } else {
            preProcCmd += cIncludeDir;
        }
        preProcCmd += " -o hls_preproc.tmp";
        int status = system(preProcCmd.c_str());
        if (status != 0) {
            log_err(LOG_TAG, "Preprocessing failed for " + cFilePath + ".");
        }
        ifstream t("hls_preproc.tmp");
        std::string cSrc((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
        t.close();
        system("rm hls_preproc.tmp");
        
        return cSrc;
    }
    
    bool CAnalyser::parse_c_function(const string& cSrc, const string& fuvName, CFunction& func) {
        func.function_name = fuvName;
        
        boost::smatch m;
        boost::regex e_func_dec("^\\s*((\\w+)(\\s+\\w+)*)\\s*" + fuvName + "\\s*\\(([^\\)]*)\\)\\s*;"); //\\s+
        
        // m[0] whole pattern, m[1] return type, m[2] param list
        
        if (!boost::regex_search(cSrc, m, e_func_dec)) {
            return false;
        }
        
        string returnType = m[1];
        string paramList = m[4];
        
        CFunctionParameter fuvRet;
        fuvRet.parameter_name = "end";
        fuvRet.is_pointer = false;
        fuvRet.is_return = true;
        fuvRet.is_input = false;
        fuvRet.array_length = 0;
        if (returnType != "void") {
            fuvRet.is_output = true;
            fuvRet.immeadiate_type = returnType;
            fuvRet.actual_type = get_actual_type(cSrc, fuvRet.immeadiate_type);
            fuvRet.dt_width = get_bit_width(fuvRet.actual_type);
            fuvRet.is_float_type = is_float_type(fuvRet.actual_type);
            fuvRet.is_int_type = !fuvRet.is_float_type;
            log_inf(LOG_TAG, "Return identified: " + fuvRet.parameter_name + " of type " + fuvRet.immeadiate_type + " (return value).");
        } else {
            fuvRet.is_output = false;
            fuvRet.immeadiate_type = "void";
            fuvRet.actual_type = "void";
            fuvRet.dt_width = 0;
            fuvRet.is_float_type = false;
            fuvRet.is_int_type = false;
            log_inf(LOG_TAG, "Function does not return any value.");
        }
        func.return_val = fuvRet;
        
        vector<CFunctionParameter> params;
        
        vector<string> p = split(paramList, ",");
        for (int i = 0; i < p.size(); i++) {
            CFunctionParameter param;
            bool ok = param_from_string(p[i], param);
            if (ok) {
                params.push_back(param);
            } else {
                log_err(LOG_TAG, "Function parameter \"" + p[i] + "\" could not be parsed.");
                return false;
            }
        }
        
        for (int i = 0; i < params.size(); i++) {
            cout << "i" << i << endl;
            params[i].actual_type = get_actual_type(cSrc, params[i].immeadiate_type);
            params[i].dt_width = get_bit_width(params[i].actual_type);
            params[i].is_float_type = is_float_type(params[i].actual_type);
            params[i].is_int_type = !params[i].is_float_type;
            log_inf(LOG_TAG, "Actual type of \"" + params[i].parameter_name + "\" is \"" + params[i].actual_type + "\" (" + to_string(params[i].dt_width) + " bits).");
        }
        
        func.params = params;
        
        return true;
    }
}

