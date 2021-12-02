#include <iostream>
#include <string>
#include <vector>

#include "Help.h"
#include "HlsCVerification.h"
#include "HlsCoVerification.h"
#include "HlsVhdlVerification.h"

using namespace std;
using namespace hls_verify;

int main(int argc, char** argv) {
    if (argc > 1) {
        string first_arg(argv[1]);

        vector<string> remaining_args;
        for (int i = 2; i < argc; i++) {
            string temp(argv[i]);
            remaining_args.push_back(temp);
        }

        if (first_arg == CVER_CMD) {
            if (run_c_verification(remaining_args)) {
                return 0;
            } else {
                return -1;
            }
        }
        if (first_arg == VVER_CMD) {
            if (run_vhdl_verification(remaining_args)) {
                return 0;
            } else {
                return -1;
            }
        }
        if (first_arg == COVER_CMD) {
            if (run_coverification(remaining_args)) {
                return 0;
            } else {
                return -1;
            }
        }
    }

    cout << endl << "No/invalid arguments!" << endl << endl;
    cout << get_general_help_message() << endl;

    return 0;
}

