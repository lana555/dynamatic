#include <dirent.h>
#include <unistd.h>

#include <cmath>
#include <fstream>
#include <sstream>

#include "HlsLogging.h"
#include "Utilities.h"

const string LOG_TAG = "UTIL";

namespace hls_verify {
    
    bool TokenCompare::compare(const string& token1, const string& token2) const{
        return token1 == token2;
    }
    
    IntegerCompare::IntegerCompare(bool is_signed) : is_signed(is_signed){
        
    }
    
    
    bool IntegerCompare::compare(const string& token1, const string& token2) const{
        long long i1;
        long long i2;
        string t1 = token1.substr(2);
        string t2 = token2.substr(2);
        while(t1.length() < t2.length()){
            t1 = ((t2[0] >= '8' && is_signed) ? "f" : "0") + t1;
        }
        while(t2.length() < t1.length()){
            t2 = ((t1[0] >= '8' && is_signed) ? "f" : "0") + t2;
        }
        
        //        stringstream is1(token1);
        //        stringstream is2(token2);
        //        is1 >> hex >> i1;
        //        is2 >> hex >> i2;
        
        //        cout << "t1 " << t1 << " t2 " << t2 << endl;
        return (t1 == t2);
    }
    
    FloatCompare::FloatCompare(float threshold) : threshold(threshold) {
        
    }
    
    bool FloatCompare::compare(const string& token1, const string& token2) const {
        unsigned int i1;
        unsigned int i2;
        stringstream is1(token1);
        stringstream is2(token2);
        is1 >> hex >> i1;
        is2 >> hex >> i2;
        //printf("%s %s %f %f %f %x %x\n", token1.c_str(), token2.c_str(), threshold, (*((float *) &i1)), (*((float *) &i1)), i1, i2);
        float diff = abs(*((float *)&i1) - *((float *) &i2));
        return (diff < threshold);
    }
    
    DoubleCompare::DoubleCompare(double threshold) : threshold(threshold) {
        
    }
    
    bool DoubleCompare::compare(const string& token1, const string& token2) const{
        unsigned long long i1;
        unsigned long long i2;
        stringstream is1(token1);
        stringstream is2(token2);
        is1 >> hex >> i1;
        is2 >> hex >> i2;
        double diff = abs(*((double *) &i1) - *((double *) &i2));
        return (diff < threshold);
    }
    
    
    string extract_parent_directory_path(string file_path) {
        for (int i = file_path.length() - 1; i >= 0; i--) {
            if (file_path[i] == '/') {
                return file_path.substr(0, i);
            }
        }
        return ".";
    }
    
    
    string get_application_directory() {
        char result[MAX_PATH_LENGTH];
        int count = readlink("/proc/self/exe", result, MAX_PATH_LENGTH);
        return string(result, (count > 0) ? count : 0);
    }
    
    bool compare_files(const string& refFile, const string& outFile, const TokenCompare * token_compare) {
        ifstream ref(refFile.c_str());
        ifstream out(outFile.c_str());
        if (!ref.is_open()) {
            log_err(LOG_TAG, "Reference file does not exist: " + refFile);
            return false;
        }
        if (!out.is_open()) {
            log_err(LOG_TAG, "Output file does not exist: " + outFile);
            return false;
        }
        string str1, str2;
        int tn1, tn2;
        while (ref >> str1) {
            out >> str2;
            if (str1 == "[[[runtime]]]"){
                if(str2 == "[[[runtime]]]"){
                    continue;
                }else{
                    log_err("COMPARE", "Token mismatch: [" + str1 + "] and [" + str2 + "] are not equal.");
                    return false;
                }
            }
            if (str1 == "[[[/runtime]]]") {
                break;
            }
            if (str1 == "[[transaction]]") {
                ref >> tn1;
                out >> tn2;
                if (tn1 != tn2) {
                    log_err("COMPARE", "Transaction number mismatch!");
                    return false;
                }
                continue;
            }
            if (str1 == "[[/transaction]]") {
                continue;
            }
            if (!token_compare->compare(str1, str2)) {
                log_err("COMPARE", "Token mismatch: [" + str1 + "] and [" + str2 + "] are not equal (at transaction id " + to_string(tn1) + ").");
                return false;
            }
        }
        return true;
    }
    
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\f");
        if (string::npos == first) {
            return str;
        }
        size_t last = str.find_last_not_of(" \t\n\r\f");
        return str.substr(first, (last - first + 1));
    }
    
    vector<string> split(const string& str, const string& delims) {
        std::size_t current, previous = 0;
        current = str.find_first_of(delims);
        vector<string> cont;
        while (current != std::string::npos) {
            cont.push_back(trim(str.substr(previous, current - previous)));
            previous = str.find_first_not_of(delims, current);
            current = str.find_first_of(delims, previous);
        }
        cont.push_back(trim(str.substr(previous, current - previous)));
        return cont;
    }
    
    int get_number_of_transactions(const string& input_path) {
        int result = 0;
        ifstream inf(input_path.c_str());
        string line;
        while (getline(inf, line)) {
            istringstream iss(line);
            string dt;
            iss >> dt;
            if (dt == "[[[/runtime]]]") break;
            if (dt.substr(0, 15) == "[[transaction]]") {
                result = result + 1;
            }
        }
        inf.close();
        return result;
    }
    
    bool execute_command(const string& command) {
        int status = system(command.c_str());
        if (status != 0) {
            log_err(LOG_TAG, "Execution failed for command [" + command + "]");
        }
        return (status == 0);
    }
    
    void add_header_and_footer(const string& filename) {
        ifstream fileIn(filename);
        string content((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());
        fileIn.close();
        ofstream fileOut(filename);
        fileOut << "[[[runtime]]]" << endl << content << "[[[/runtime]]]" << endl;
        fileOut.close();
    }
    
    vector<string> get_list_of_files_in_directory(const string& directory, const string& extension){
        vector<string> result;
        DIR * dirp = opendir(directory.c_str());
        struct dirent * dp;
        if (dirp != NULL) {
            while ((dp = readdir(dirp)) != NULL) {
                string temp(dp->d_name);
                if (temp.length() >= extension.length() && temp.substr(temp.length() - extension.length(), extension.length()).compare(extension) == 0)
                    result.push_back(string(dp->d_name));
            }
            closedir(dirp);
        }
        return result;
    }
    
}

