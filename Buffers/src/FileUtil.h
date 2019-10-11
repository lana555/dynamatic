#ifndef _FILEUTIL_H__
#define _FILEUTIL_H__

#include <fstream>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <string>

/**
 * @class FileUtil
 * @author Jordi Cortadella
 * @date 05/01/19
 * @file FileUtil.h
 * @brief Class to handle few utilities for files.
 */
class FileUtil
{
public:

    /**
     * @brief Creates a temporary file name with the format "prefix".XXXXXXsuffix.
     * This format is used with mkstemp to create a temporary file.
     * @param prefix Prefix of the file.
     * @param suffix Suffix of the file.
     * @return The file name.
     */
    static std::string createTempFilename(const std::string& prefix, const std::string& suffix = "") {
        char fname[1000];
        strcat(strcpy(fname, prefix.c_str()), ".XXXXXX");
        if (not suffix.empty()) strcat(fname, suffix.c_str());
        int file = mkstemps(fname, suffix.size());
        if (file == -1) return "";
        return fname;
    }

    /**
     * @brief Deletes a temporary file name.
     * @param filename The name of the file.
     */
    static void deleteTempFilename(const std::string& filename) {
        unlink(filename.c_str());
    }

    /**
     * @brief Writes a string into a file.
     * @param s The string to be written.
     * @param filename The name of the file (if empty, the string is written into cout).
     * @param error In case of error, the message is stored in this parameter.
     * @return True if successful, and false otherwise.
     * @note error is only modified in case an error is produced.
    */
    static bool write(const std::string& s, const std::string& filename, std::string& error) {
        if (filename.empty()) {
            std::cout << s;
            return true;
        }

        std::ofstream f(filename);
        if (not f.good()) {
            error = "Error when opening the output file " + filename + ".";
            return false;
        }

        f << s;
        f.close();
        return true;
    }
};

#endif // _FILEUTIL_H__
