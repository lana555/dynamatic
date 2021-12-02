#include <fstream>

#include <boost/regex.hpp>

#include "CInjector.h"
#include <iostream>
using namespace std;

namespace hls_verify {
    
    int find_end_brace_index(const string& cFuv, int startIndex) {
        int openCount = 1;
        for (int i = startIndex + 1; i < cFuv.length(); i++) {
            if (cFuv[i] == '{') openCount++;
            else if (cFuv[i] == '}') openCount--;
            if (openCount == 0) {
                return i;
            }
        }
        return cFuv.length() - 1;
    }
    
    CInjector::CInjector(const VerificationContext& ctx) : ctx(ctx) {
        
        const CFunction& func = ctx.get_c_fuv();
        // add stdio.h include and get preprocessor output
        ifstream cFuvSrcFileIn(ctx.get_c_fuv_path());
        string cFuv((istreambuf_iterator<char>(cFuvSrcFileIn)), istreambuf_iterator<char>());
        cFuvSrcFileIn.close();
        ofstream cFuvSrcFileOut(ctx.get_c_fuv_path() + ".tmp.c");
        cFuvSrcFileOut << "#include <stdio.h>" << endl << cFuv;
        cFuvSrcFileOut.close();
        
        cFuv = CAnalyser::get_preproc_output(ctx.get_c_fuv_path() + ".tmp.c");
        system(("rm " + ctx.get_c_fuv_path() + ".tmp.c").c_str());
        
        boost::smatch m;
        boost::regex e;
        
        string prefix;
        string head;
        string body;
        string suffix;
        
        stringstream out;
        
        
        e = boost::regex("^\\s*" + func.return_val.immeadiate_type + "\\s*" + func.function_name + "\\s*\\([^\\)]*\\)\\s*\\{");
        boost::regex_search(cFuv, m, e);
        
        if (m.empty()) return;
        
        
        int funcBe = m.position() + m.length();
        int funcEn = find_end_brace_index(cFuv, funcBe);
        
        prefix = cFuv.substr(0, m.position());
        head = m[0];
        body = cFuv.substr(funcBe, funcEn - funcBe);
        suffix = cFuv.substr(funcEn, cFuv.length() - funcEn);
        
        if (func.return_val.actual_type == "void") {
            body = body + "\n\treturn;\n";
        }
        
        
        out << prefix;
        out << endl << "int hlsv_transaction_id = -1;" << endl << endl;
        out << head << endl << endl;
        out << get_variable_declarations(func);
        out << get_file_io_code_for_input(func);
        
        e = boost::regex("return([^;]*);");
        while (boost::regex_search(body, m, e)) {
            out << body.substr(0, m.position()) << endl;
            out << get_file_io_code_for_output(func, string(m[1]));
            body = body.substr(m.position() + string(m[0]).length());
        }
        
        out << body;
        out << suffix;
        
        injectedFuvSrc = out.str();
    }
    
    string CInjector::get_variable_declarations(const CFunction& func) {
        stringstream out;
        if (func.return_val.actual_type != "void") {
            out << "\t" << func.return_val.immeadiate_type + " hlsv_return;" << endl;
            out << "\tFILE * hlsv_of_return;" << endl;
        }
        for (int i = 0; i < func.params.size(); i++) {
            if (func.params[i].is_input) {
                out << "\tFILE * hlsv_if_" << func.params[i].parameter_name << ";" << endl;
            }
        }
        for (int i = 0; i < func.params.size(); i++) {
            if (func.params[i].is_output) {
                out << "\tFILE * hlsv_of_" << func.params[i].parameter_name << ";" << endl;
            }
        }
        out << endl << "\tint hlsv_i = 0;" << endl;
        
        int max_dim = 0;
        for(int i = 0; i < func.params.size(); i++){
            if(func.params[i].is_pointer && func.params[i].dims.size() > max_dim){
                max_dim = func.params[i].dims.size();
            }
        }
        for(int i = 0; i < max_dim; i++){
            out << endl << "\tint hlsv_i" << i << " = 0;" << endl;
        }
        
        return out.str();
    }
    
    string CInjector::get_file_io_code_for_input(const CFunction& func) {
        stringstream out;
        out << endl << "\thlsv_transaction_id++;" << endl << endl;
        for (int i = 0; i < func.params.size(); i++) {
            if (func.params[i].is_input) {
                out << get_file_io_code_for_input_param(func.params[i]);
            }
        }
        return out.str();
    }
    
    string array_print_code(const CFunctionParameter& param, string file_name_prefix) {
        stringstream out;
        for (int d = 0; d < param.dims.size(); d++) {
            out << string(d, '\t') << "\tfor(hlsv_i" << d << " = 0; hlsv_i" << d << " < " << param.dims[d] << "; hlsv_i" << d << "++){" << endl;
        }
        out << string(param.dims.size(), '\t');
        // TODO: edit the following line for non integer types
        if (param.is_float_type) {
            if (param.dt_width == 32) {
                out << "\tfprintf(" << file_name_prefix << param.parameter_name << ", \"0x%08x\\n\", *((unsigned int *)&" << param.parameter_name;
                for (int d = 0; d < param.dims.size(); d++) {
                    out << "[hlsv_i" << to_string(d) << "]";
                }
                out << "));" << endl;
            } else {
                out << "\tfprintf(" << file_name_prefix << param.parameter_name << ", \"0x%08llx\\n\", *((unsigned long long *)&" << param.parameter_name;
                for (int d = 0; d < param.dims.size(); d++) {
                    out << "[hlsv_i" << to_string(d) << "]";
                }
                out << "));" << endl;
            }
        } else {
            out << "\t\tfprintf(" << file_name_prefix << param.parameter_name << ", \"0x%08llx\\n\", (long long)" << param.parameter_name;
            for (int d = 0; d < param.dims.size(); d++) {
                out << "[hlsv_i" << to_string(d) << "]";
            }
            out << ");" << endl;
        }
        for (int d = 0; d < param.dims.size(); d++) {
            out << string(param.dims.size() - d, '\t') << "\t}" << endl;
        }
        return out.str();
    }
    
    string CInjector::get_file_io_code_for_input_param(const CFunctionParameter& param) {
        stringstream out;
        out << "\thlsv_if_" << param.parameter_name << " = fopen(\"" << ctx.get_input_vector_path(param) << "\", \"a\");" << endl;
        out << "\tfprintf(hlsv_if_" << param.parameter_name << ", \"[[transaction]] %d\\n\", hlsv_transaction_id);" << endl;
        if (param.is_pointer) {
            out << array_print_code(param, "hlsv_if_");
        } else {
            if (param.is_float_type) {
                if (param.dt_width == 32) {
                    out << "\tfprintf(hlsv_if_" << param.parameter_name << ", \"0x%08x\\n\",  *((unsigned int *)&" << param.parameter_name << "));" << endl;
                    
                } else {
                    out << "\tfprintf(hlsv_if_" << param.parameter_name << ", \"0x%08llx\\n\",  *((unsigned long long *)&" << param.parameter_name << "));" << endl;
                    
                }
            } else {
                out << "\tfprintf(hlsv_if_" << param.parameter_name << ", \"0x%08llx\\n\", (long long)" << param.parameter_name << ");" << endl;
            }
        }
        out << "\tfprintf(hlsv_if_" << param.parameter_name << ", \"[[/transaction]]\\n\");" << endl;
        out << "\tfclose(hlsv_if_" << param.parameter_name << ");" << endl << endl;
        return out.str();
    }
    
    string CInjector::get_file_io_code_for_output(const CFunction& func, string actualReturnValue) {
        stringstream out;
        out << "\t{" << endl;
        for (int i = 0; i < func.params.size(); i++) {
            if (func.params[i].is_output) {
                out << get_file_io_code_for_output_param(func.params[i]);
            }
        }
        out << get_file_io_code_for_return_value(func.return_val, actualReturnValue);
        out << "\t}" << endl;
        return out.str();
    }
    
    string CInjector::get_file_io_code_for_output_param(const CFunctionParameter& param) {
        stringstream out;
        out << "\t\thlsv_of_" << param.parameter_name << " = fopen(\"" << ctx.get_c_out_path(param) << "\", \"a\");" << endl;
        out << "\t\tfprintf(hlsv_of_" << param.parameter_name << ", \"[[transaction]] %d\\n\", hlsv_transaction_id);" << endl;
        if (param.is_pointer) {
            out << array_print_code(param, "hlsv_of_");
        } else {
            if (param.is_float_type) {
                if (param.dt_width == 32) {
                    out << "\tfprintf(hlsv_of_" << param.parameter_name << ", \"0x%08x\\n\",  *((unsigned int *)&" << param.parameter_name << "));" << endl;
                    
                } else {
                    out << "\tfprintf(hlsv_of_" << param.parameter_name << ", \"0x%08llx\\n\",  *((unsigned long long *)&" << param.parameter_name << "));" << endl;
                    
                }
            } else {
                out << "\tfprintf(hlsv_of_" << param.parameter_name << ", \"0x%08llx\\n\",  (long long)" << param.parameter_name << ");" << endl;
            }
        }
        out << "\t\tfprintf(hlsv_of_" << param.parameter_name << ", \"[[/transaction]]\\n\");" << endl;
        out << "\t\tfclose(hlsv_of_" << param.parameter_name << ");" << endl << endl;
        return out.str();
    }
    
    string CInjector::get_file_io_code_for_return_value(const CFunctionParameter& param, string actualReturnValue) {
        stringstream out;
        
        if (param.actual_type == "void") {
            out << "\t\treturn;" << endl;
            return out.str();
        }
        
        out << "\t\thlsv_return = " << actualReturnValue << ";" << endl;
        
        out << "\t\thlsv_of_return = fopen(\"" << ctx.get_c_out_path(param) << "\", \"a\");" << endl;
        out << "\t\tfprintf(hlsv_of_return, \"[[transaction]] %d\\n\", hlsv_transaction_id);" << endl;
        if (param.is_float_type) {
            if (param.dt_width == 32) {
                out << "\t\tfprintf(hlsv_of_return, \"0x%08x\\n\",  *((unsigned int *)&hlsv_return));" << endl;
                
            } else {
                out << "\t\tfprintf(hlsv_of_return, \"0x%08llx\\n\",  *((unsigned long long*)&hlsv_return));" << endl;
                
            }
        } else {
            out << "\t\tfprintf(hlsv_of_return, \"0x%08llx\\n\", (long long)hlsv_return);" << endl;
        }
        out << "\t\tfprintf(hlsv_of_return, \"[[/transaction]]\\n\");" << endl;
        out << "\t\tfclose(hlsv_of_return);" << endl << endl;
        
        out << "\t\treturn hlsv_return;" << endl;
        return out.str();
    }
    
    string CInjector::get_injected_c_fuv() {
        return injectedFuvSrc;
    }
    
}

