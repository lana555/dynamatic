#ifndef CINJECTOR_H
#define CINJECTOR_H

#include<string>

#include "CAnalyser.h"
#include "VerificationContext.h"

using namespace std;

namespace hls_verify {

    class CInjector {
    public:
        CInjector(const VerificationContext& ctx);
        string get_injected_c_fuv();

    private:
        VerificationContext ctx;
        string injectedFuvSrc;
        string get_file_io_code_for_input_param(const CFunctionParameter& param);
        string get_file_io_code_for_output_param(const CFunctionParameter& param);
        string get_file_io_code_for_return_value(const CFunctionParameter& param, string actualReturnValue);
        string get_file_io_code_for_input(const CFunction& func);
        string get_file_io_code_for_output(const CFunction& func, string actualReturnValue);
        string get_variable_declarations(const CFunction & func);
    };
}

#endif

