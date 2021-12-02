#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>

#define MAX_PATH_LENGTH 5000

using namespace std;

namespace hls_verify {

    class TokenCompare {
    public:
        virtual bool compare(const string& token1, const string& token2) const;
    };

    class IntegerCompare : public TokenCompare {
    public:
        IntegerCompare() = default;
        IntegerCompare(bool is_signed);
        virtual bool compare(const string& token1, const string& token2) const;
        
    private:
        bool is_signed;
    };

    class FloatCompare : public TokenCompare {
    public:
        FloatCompare() = default;
        FloatCompare(float threshold);
        virtual bool compare(const string& token1, const string& token2) const;
    private:
        float threshold;
    };

    class DoubleCompare : public TokenCompare {
    public:
        DoubleCompare() = default;
        DoubleCompare(double threshold);
        virtual bool compare(const string& token1, const string& token2) const;
    private:
        double threshold;
    };

    /**
     * Extracts the parent directory from a file path.
     * @param file_path path of the file.
     * @return parent directory path or "." if @file_path has no directory separators.
     */
    string extract_parent_directory_path(string pathName);

    /**
     * Get the directory of the executable file.
     * @return the directory of the executable file.
     */
    string get_application_directory();

    /**
     * Compares two data files using a given token comparator.
     * @param refFilePath path of the reference data file
     * @param outFile path of the other data file
     * @param token_compare the comparator to be used for comparing two tokens
     * @return true if all comparisons succeed, false otherwise.
     */
    bool compare_files(const string& refFilePath, const string& outFile, const TokenCompare * token_compare);

    /**
     * Trims the leading and trailing white spaces.
     * @param str the string to be trimmed
     * @return trimmed string
     */
    string trim(const string& str);

    /**
     * Splits the given string using one of the delimiters.
     * @param str the string to be splitted
     * @param delims the list of delimitters
     * @return a vector of strings which are the parts after splitting.
     */
    vector<string> split(const string& str, const string& delims = " \t\n\r\f");

    /**
     * Get the number of transactions in a data file.
     * @param input_path path of the data file
     * @return the number of transactions in the given file.
     */
    int get_number_of_transactions(const string& input_path);

    /**
     * Execute the given command in a shell.
     * @param command the command to be executed
     * @return true if execution returns 0, false otherwise.
     */
    bool execute_command(const string& command);

    /**
     * Add tag [[[runtime]]] at the begining of file @filename, and add tag 
     * [[[/runtime]]] at the end of file @filename.
     * @param filename path of the file to be modified
     */
    void add_header_and_footer(const string& filename);
    
    /**
     * Get a list of file paths in the given directory that has the given extension.
     * @param directory the path of the directory 
     * @param extension extension of the required files including '.' character
     * @return a vector of strings denoting the paths of the files in @directory that has extension @extension.
     */
    vector<string> get_list_of_files_in_directory(const string& directory, const string& extension="");

}

#endif
