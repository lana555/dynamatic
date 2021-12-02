#include <sstream>

#include "Help.h"

namespace hls_verify {

    string get_general_help_message() {
        ostringstream ss;
        ss << "Usage 1:\n\thlsverifier cver <c_testbench_path> <c_duv_path> <c_fuv_name>" << endl;
        ss << "Usage 2:\n\thlsverifier vver <c_testbench_path> <vhdl_entitiy_name>" << endl;
        ss << "Usage 3:\n\thlsverifier cover <c_testbench_path> <c_duv_path> <vhdl_entitiy_name>" << endl << endl;
        ss << "Note:\n\tAll C source files should be in the same subdirectory." << endl << endl;
        ss << "\tAssumes hlsverifier is run from a subdirectory (called HLS_VERIFY), which \n"
                "\tis in the same level as the subdirectories for C sources (C_SRC) and the \n"
                "\tvhdl sources (VHDL_SRC)." << endl << endl;
        ss << "\tAlso assumes that the golden references are in a directory called REF_OUT in\n"
                "\tthe same level.\n" << endl;
        return ss.str();
    }

    string get_c_verification_help_message() {
        return "Usage 1:\n\thlsverifier cver <c_testbench_path> <c_duv_path> <c_fuv_name>";
    }

    string get_vhdl_verification_help_message() {
        return "Usage:\n\thlsverifier vver <c_testbench_path> <vhdl_entitiy_name>";
    }

    string get_co_verification_help_message() {
        return "Usage:\n\thlsverifier cover <c_testbench_path> <c_duv_path> <vhdl_entitiy_name>";
    }

}