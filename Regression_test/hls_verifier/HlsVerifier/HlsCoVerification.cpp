#include <iostream>

#include "Help.h"
#include "HlsCoVerification.h"
#include "HlsCVerification.h"
#include "HlsLogging.h"
#include "HlsVhdlVerification.h"
#include "Utilities.h"

namespace hls_verify {
    const string LOG_TAG = "COVER";

    bool run_coverification(vector<string> args) {
        if (args.size() < 3) {
            log_err(LOG_TAG, "Not enough arguments.");
            cout << get_co_verification_help_message() << endl;
            return true;
        }
        
        bool use_addr_width_32 = false;
        
        vector<string> temp;
        
        for(auto& arg : args){
            if(arg.size() > 0 && arg[0] == '-'){
                if(arg == "-aw32"){
                    use_addr_width_32 = true;
                } 
            } else{
                temp.push_back(arg);
            }
        }
        
        args = temp;
        
        string cTbPath = args[0];
        string cDuvPath = args[1];
        string c_fuv_function_name = args[2];
        string vhdl_duv_entity_name = c_fuv_function_name;
        if (args.size() > 3) {
            vhdl_duv_entity_name = args[3];
        }
        vector<string> other_c_paths;
        for (int i = 4; i < args.size(); i++) {
            other_c_paths.push_back(args[i]);
        }

        try {
            VerificationContext ctx(cTbPath, cDuvPath, c_fuv_function_name, vhdl_duv_entity_name, other_c_paths);
            ctx.use_addr_width_32 = use_addr_width_32;
            execute_c_testbench(ctx);
            execute_vhdl_testbench(ctx);
            bool value = compare_c_and_vhdl_outputs(ctx);
            return value;            
            //return true;
        } catch (string error) {
            log_err(LOG_TAG, error);
            return false;
        } catch (...) {
            log_err(LOG_TAG, "Unknown error!");
            return false;
        }
    }

//    void compare_c_and_vhdl_outputs(const VerificationContext& ctx) {    
      bool compare_c_and_vhdl_outputs(const VerificationContext& ctx) {
        const vector<CFunctionParameter>& output_params = ctx.get_fuv_output_params();
        cout << "\n--- Comparison Results ---\n" << endl;
        for (auto it = output_params.begin(); it != output_params.end(); it++) {
            bool result = compare_files(ctx.get_c_out_path(*it), ctx.get_vhdl_out_path(*it), ctx.get_token_comparator(*it));
            cout << "Comparison of [" + it->parameter_name + "] : " << (result ? "Pass" : "Fail") << endl;
            return result;
        }
        cout << "\n--------------------------\n" << endl;

	

    }
}
