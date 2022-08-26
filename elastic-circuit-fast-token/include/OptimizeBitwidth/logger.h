#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <sstream>

// Specializes std ostream to accept llvm strings
//avoids the pain of using .str() / .data() each time
inline std::ostream& operator<<(std::ostream& os, const llvm::Twine& str) {
    return os << str.str();
}

// belong only to logger.h
static inline void my_printf(llvm::raw_ostream& stream, const char* format);
template <typename T, typename... Args>
static void my_printf(llvm::raw_ostream& stream, const char* format, T val, Args... args);

/*
 * Logger class for convenient logging.
 * Usage : logger.log("Print var %, another var %", var1, var2);
 * Why to use : you don't need to bother yourself with %d, %s and cie specifiers.
 */
class Logger {
public:
    inline explicit Logger() = default;

    inline void setStream(llvm::raw_ostream* os) { mOs = os; }
    inline llvm::raw_ostream* getStream() { return mOs; }

    // sets the prefix to prepend to the logs
    inline void setPrefix(const std::string& prefix) { mPrefix = prefix; }

    // to temporarly ignore some logs 
    inline void setEnabled(bool enable) { mEnabled = enable; }
    inline void enable() { mEnabled = true; }
    inline void disable() { mEnabled = false; }

    // indent the logs, make them more readable by creating a hierarchy
    inline void pushIndent() { mIndent += 1; }
    inline void popIndent() { mIndent = std::max(mIndent - 1, 0); }
    inline void resetIndent() { mIndent = 0; }

    template <typename... Args> 
    void log(const char* format, Args... args) const {
        if (mEnabled && mOs != nullptr) {
            *mOs << mPrefix;

            for (int i = 0 ; i < mIndent ; ++i)
                *mOs << ' ' << ' ';
            
            my_printf(*mOs, format, args...);

            *mOs << '\n';
        }
    }

private:
    llvm::raw_ostream* mOs = nullptr;
    std::string mPrefix = "";
    int mIndent = 0;
    bool mEnabled = true;
};



// printf with arguments
template <typename T, typename... Args>
void my_printf(llvm::raw_ostream& stream, const char* format, T val, Args... args) {
    while (*format != '\0') {
        if (*format == '%') {
            ++format;
            // % escape : print '%%' as '%'
            if (*format == '%') {
                stream << *format;
                ++format;
            }
            else {
                stream << val; 
                my_printf(stream, format, args...); // recursively call my_printf, with one less arg
                return; // return here, we don't want to continue the loop after the call
            }
        }
        else {
            stream << *format;
            ++format;
        }
    }
}

// bottom of the recursion : no args left
inline void my_printf(llvm::raw_ostream& stream, const char* format) {
    stream << format;
}

#endif