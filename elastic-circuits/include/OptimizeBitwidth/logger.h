#ifndef LOGGER_H
#define LOGGER_H

#include "printf.h"
#include <fstream>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <sstream>
/*
 * Logger class for convenient logging.
 * Usage : logger_object->log("Print var %, another var %", var1, var2);
 * Why to use : you don't need to bother yourself with %d, %s and etc. specifiers.
 */
class Logger {
public:
    void setStream(llvm::raw_fd_ostream& os) { m_os = &os; }

    void setPrefix(const std::string& prefix) { m_prefix = prefix; }

    template <typename... Args> void log(const char* format, Args... args) const {
        my_printf(*m_os, (m_prefix + format + "\n").c_str(), args...);
    }

    Logger() {}

private:
    llvm::raw_fd_ostream* m_os;
    std::string m_prefix;
};

#endif