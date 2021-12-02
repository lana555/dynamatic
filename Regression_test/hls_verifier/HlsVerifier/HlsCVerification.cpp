#include <iostream>
#include <fstream>
#include <sstream>

#include "CAnalyser.h"
#include "CInjector.h"
#include "Help.h"
#include "HlsCVerification.h"
#include "HlsLogging.h"
#include "Utilities.h"

namespace hls_verify {
    const string LOG_TAG = "CVER";

    bool run_c_verification(vector<string> args) {
        if (args.size() < 3) {
            log_err(LOG_TAG, "Not enough arguments.");
            cout << get_c_verification_help_message() << endl;
            return true;
        }

        string c_tb_path = args[0];
        string c_fuv_path = args[1];
        string c_fuv_name = args[2];
        vector<string> other_c_paths;
        for (int i = 3; i < args.size(); i++) {
            other_c_paths.push_back(args[i]);
        }

        try {
            VerificationContext ctx(c_tb_path, c_fuv_path, c_fuv_name, "", other_c_paths);
            execute_c_testbench(ctx);

            check_c_testbench_outputs(ctx);

            return true;
        } catch (string error) {
            log_err(LOG_TAG, error);
            return false;
        } catch (...) {
            log_err(LOG_TAG, "Unknown error!");
            return false;
        }
    }

    void compile_c_testbench(const VerificationContext& ctx) {
        // Compile command = gcc cfile1.c cfile2.c -I includeFileDirectory -o outputfileName
        stringstream comp_cmd_builder;
        comp_cmd_builder << "gcc ";
        //comp_cmd_builder << ctx.get_c_tb_path() + " ";
        comp_cmd_builder << ctx.get_injected_c_fuv_path() + " ";
        vector<string> other_c_paths = ctx.get_other_c_src_paths();
        for (auto it = other_c_paths.begin(); it != other_c_paths.end(); it++) {
            comp_cmd_builder << *it << " ";
        }
        comp_cmd_builder << "-I" << extract_parent_directory_path(ctx.get_c_tb_path());
        comp_cmd_builder << " -o " << ctx.get_c_executable_path();

        string comp_cmd = comp_cmd_builder.str();
        log_inf(LOG_TAG, "Compiling C files [" + comp_cmd + "]");
        if (!execute_command(comp_cmd)) {
            throw string("Compilation failed!");
        }
    }

    void do_post_processing(const VerificationContext& ctx) {
        const CFunction& fuv = ctx.get_c_fuv();
        if (fuv.return_val.is_output) {
            add_header_and_footer(ctx.get_c_out_path(fuv.return_val));
        }
        for (auto it = fuv.params.begin(); it != fuv.params.end(); it++) {
            if (it->is_output) {
                add_header_and_footer(ctx.get_c_out_path(*it));
            }
            if (it->is_input) {
                add_header_and_footer(ctx.get_input_vector_path(*it));
            }
        }
    }

    void check_c_testbench_outputs(const VerificationContext& ctx) {
        const vector<CFunctionParameter>& output_params = ctx.get_fuv_output_params();
        cout << "\n--- Comparison Results ---\n" << endl;
        for (auto it = output_params.begin(); it != output_params.end(); it++) {
            bool result = compare_files(ctx.get_ref_out_path(*it), ctx.get_c_out_path(*it), ctx.get_token_comparator(*it));
            cout << "Comparison of [" + it->parameter_name + "] : " << (result ? "Pass" : "Fail") << endl;
        }
        cout << "\n--------------------------\n" << endl;
    }

    void execute_c_testbench(const VerificationContext& ctx) {
        string command;

        // Generate IO injected C fuv

        CInjector injector(ctx);

        ofstream injected_c_fuv(ctx.get_injected_c_fuv_path());
        injected_c_fuv << injector.get_injected_c_fuv();
        injected_c_fuv.close();

        // Compiling

        compile_c_testbench(ctx);

        // Cleaning-up exisiting outputs

        command = "rm -rf " + ctx.get_c_out_dir();
        log_inf(LOG_TAG, "Cleaning C output files [" + command + "]");
        execute_command(command);

        command = "rm -rf " + ctx.get_input_vector_dir();
        log_inf(LOG_TAG, "Cleaning existing input files [" + command + "]");
        execute_command(command);

        command = "mkdir -p " + ctx.get_c_out_dir();
        log_inf(LOG_TAG, "Creating C output files directory [" + command + "]");
        execute_command(command);

        command = "mkdir -p " + ctx.get_input_vector_dir();
        log_inf(LOG_TAG, "Creating input files directory [" + command + "]");
        execute_command(command);

        // Running the compiled testbench

        command = ctx.get_c_executable_path();
        log_inf(LOG_TAG, "Executing C test-bench [" + command + "]");
        if (execute_command(command)) {
            do_post_processing(ctx);
            log_inf(LOG_TAG, "C simulation finished. Outputs saved in directory "
                    + ctx.get_c_out_dir() + ".");
        } else {
            throw string("C testbench execution failed!");
        }
    }
}
