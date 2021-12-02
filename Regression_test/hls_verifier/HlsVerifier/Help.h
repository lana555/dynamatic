#ifndef HELP_H
#define HELP_H

#include <string>

using namespace std;

namespace hls_verify{
    string get_general_help_message();
    string get_c_verification_help_message();
    string get_vhdl_verification_help_message();
    string get_co_verification_help_message();
}

#endif

