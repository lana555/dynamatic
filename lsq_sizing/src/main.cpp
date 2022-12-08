#include <iostream>
#include "ErrorManager.h"
#include "MILP_Model.h"
#include "Dataflow.h"
#include <sstream>
#include <fstream>
#include <regex>
#include <dirent.h>
#include <map>

using namespace std;
using namespace Dataflow;

using vecParams = vector<string>;

string exec;

string command;

// Functions and structs definition

struct user_input {
    string graph_name;
    int case_selection;
};

void clear_input(user_input& input) {
    input.graph_name = "dataflow";
    input.case_selection = 0;
}

void print_input(const user_input& input) {
    string selected_case = "";

    // Check the selected case
    selected_case  = input.case_selection ? "worst_case" : "best_case";

    cout << "\n================================================" << endl;
    cout << "                    LSQ-Sizing" << endl;
    cout << "dataflow graph name: " << input.graph_name << endl;
    cout << "case-selection: " << selected_case << endl;
    cout << "================================================" << endl;
}

void parse_user_input(const vecParams& params, user_input& input) {
    clear_input(input);
    regex name_regex("(-filepath=)(.*)");
    regex case_regex("(-case)(.*)");
    for (auto param: params) {
        if (regex_match(param, case_regex)) {
            input.case_selection = atof(param.substr(param.find("=") + 1).c_str());
        } else if (regex_match(param, name_regex)) {
            input.graph_name = param.substr(param.find("=") + 1);
        } else {
            cout << "[ERROR] " << param << " is invalid argument" << endl;
            assert(false);
        }
    }
}

//function used to generate the II log file for the best case scenario
//this function assumes that the dynamatic log is in the same path from where this function has been called 
void generate_best_case_II_log(const user_input& input){

    int best_case  = input.case_selection ? 0 : 1;
    if(best_case == 0){
        return;
    }

    string log_output = input.graph_name +"_best_II.log";
    cout << "Generating log file " << log_output << endl;

    string dynamatic_log = "";
    regex dyn_log_reg("dynamatic_[0-9]+-[0-9]+-[0-9]+\.log");

    struct dirent *file;
    DIR *directory = opendir(".");
    if (directory != NULL){
        file = readdir(directory);
        while(file != NULL){
            if(regex_match(file->d_name, dyn_log_reg))
                dynamatic_log = file->d_name;
            file = readdir(directory);
        }
    }else{
        cout << "[ERROR] Opening the current directory" << endl;
        assert(false);
    }

    if(dynamatic_log == ""){
        cout << "[ERROR] Dynamatic log not found in this path" << endl;
        assert(false);
    }

    string throughputs[100];

    string grep_cmd = "grep 'Throughput for MG' " +dynamatic_log;
    FILE* grep_output = popen(grep_cmd.c_str() , "r");

    if(grep_output == NULL){
        cout << "[ERROR] File " << dynamatic_log << " not found" << endl;
        assert(false);
    }
    
    char buff [52];
    string th;
    int mg_id;
    regex id_reg("MG [0-9]+ in");
    regex num_reg("[0-9]+");
    regex th_reg("MG [0-9]+: [0-9]+\.[0-9]+");
    regex float_reg("[0-9]+\.[0-9]+");

    while(fgets(buff, sizeof(buff), grep_output) != NULL){
	smatch match;

	//identify MG id
	auto line = string(buff);
	regex_search( line, match, id_reg);
	line = string(match[0]);
	regex_search( line , match, num_reg);
        mg_id = stoi( string(match[0]) );

	//identify throughput
	line = string(buff);
	regex_search( line, match, th_reg);
	line = string(match[0]);
	regex_search( line , match, float_reg);
        th = string(match[0]);

        throughputs[mg_id] = th;

    }
    pclose(grep_output);

    ofstream write(log_output);

    for(string th: throughputs){
	if(th != "" & th != "\n")
	    write << th << endl;
    }

    write.close();

}


int lsq_sizing(const vecParams& params) {
    // Parse and print the input
    user_input input{};
    parse_user_input(params, input);
    print_input(input);

    //Generate best case II log file if the best case has been selected by the used
    generate_best_case_II_log(input);

    // Read the dot file for the flow of lsq_sizing
    cout << "Read the dot file after buffer placement" << endl;
    cout << endl;
    string suffix = "_optimized.dot"; //"_graph_buf.dot"
    DFnetlist DF(input.graph_name + suffix, input.graph_name + "_bbgraph_buf.dot");

    if (DF.hasError()) {
        cerr << DF.getError() << endl;
        return 1;
    }

    // Read again the file for outputing new dot files after the lsq_sizing flow
    DFnetlist DF_post_buffer(input.graph_name + suffix , input.graph_name + "_bbgraph_buf.dot");

    cout << "Compute the size of the lsq in example " << input.graph_name << endl;
    cout << endl;

    DF.buildAdjMatrix(1, input.graph_name, DF_post_buffer, input.case_selection);

    return 0;
}

int main(int argc, char *argv[]) {

    // Return the last part of the path, function name
    exec = basename(argv[0]);

    if (argc != 3) {
        cout << "Please provide the following args:\n\t-filepath=<path_to_dot_file>\n\t-case=selected_case (0 for best case scenario / 1 for worst case scenario)" << endl;
        return 0;
    }

    // Store the params
    vecParams params;
    for (int i = 1; i < argc; ++i) params.push_back(argv[i]);

    return lsq_sizing(params);

}



