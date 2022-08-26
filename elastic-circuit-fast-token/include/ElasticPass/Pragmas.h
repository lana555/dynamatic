#pragma once

#include <string>
#include <vector>

void pragmas(int rem, std::string filename);
// using namespace std;

struct PragmaDetails {
    int type; ///< 0 for func, 1 for var, 2 for loop, 3 for operation
    std::string
        name; ///< Name of variable or function as it appears in the IR, "LOOP" in case of loop
    std::string info; ///< The information that was needed to be passed
};

std::string get_pragma_loop(std::string loop_name); // returns relevant info
bool get_pragma_generic(
    std::string gen); // returns true when the generic pragma being searched for is present
std::string get_pragma_func(std::string func_name);         // returns relevant info
std::string get_pragma_op(std::string line_label, char op); // returns relevant info
std::string get_pragma_var(std::string data_type, std::string var,
                           std::string arr); // datatype needs to have the non array datatype, arr
                                             // needs to have dimensions for array and 0 for others