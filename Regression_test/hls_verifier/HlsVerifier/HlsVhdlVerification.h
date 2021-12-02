#ifndef HLSVHDLVERIFICATION_H
#define HLSVHDLVERIFICATION_H

#include <string>
#include <vector>

#include "VerificationContext.h"

using namespace std;

namespace hls_verify {
    const string VVER_CMD = "vver";

    /**
     * Run VHDL verification.
     * @param args remaining arguments to vver
     * @return true if the function executes normally, false otherwise.
     */
    bool run_vhdl_verification(vector<string> args);
    
    /**
     * Generate the VHDL testbench of the given verification context.
     * @param ctx
     */
    void generate_vhdl_testbench(const VerificationContext& ctx);

    /**
     * Generate ModelSim scripts to run the generated VHDL testbench.
     * @param ctx verification context
     */
    void generate_modelsim_scripts(const VerificationContext& ctx);

    /**
     * Compares all generated VHDL testbench outputs against references.
     * @param ctx verification context
     */
    void check_vhdl_testbench_outputs(const VerificationContext& ctx);
    
    /**
     * Execute the VHDL testbench of the given verification context.
     * @param ctx verification context
     */
    void execute_vhdl_testbench(const VerificationContext& ctx);
}

#endif

