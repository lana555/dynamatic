#ifndef HLSCVERIFICATION_H
#define HLSCVERIFICATION_H

#include <string>
#include <vector>

#include "VerificationContext.h"

using namespace std;

namespace hls_verify {
    const string CVER_CMD = "cver";

    /**
     * Run C verification.
     * @param args remaining arguments to cver
     * @return true if the function executes normally, false otherwise.
     */
    bool run_c_verification(vector<string> args);

    /**
     * Execute the C testbench of the given verification context.
     * @param ctx verification context
     */
    void execute_c_testbench(const VerificationContext& ctx);

    /**
     * Compile the C testbench of the given verification context
     * @param ctx verification context
     */
    void compile_c_testbench(const VerificationContext& ctx);
    
    /**
     * For every data file (input/output), add opening and closing tags
     * @param ctx verification context
     */
    void do_post_processing(const VerificationContext& ctx);
    
    /**
     * Compares all generated C testbench outputs against references.
     * @param ctx verification context
     */
    void check_c_testbench_outputs(const VerificationContext& ctx);
}

#endif

