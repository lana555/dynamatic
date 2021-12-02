#ifndef HLSCOVERIFICATION_H
#define HLSCOVERIFICATION_H

#include <string>
#include <vector>

#include "VerificationContext.h"

using namespace std;

namespace hls_verify {
    const string COVER_CMD = "cover";

    bool run_coverification(vector<string> argV);

    bool compare_c_and_vhdl_outputs(const VerificationContext& ctx);
}

#endif

