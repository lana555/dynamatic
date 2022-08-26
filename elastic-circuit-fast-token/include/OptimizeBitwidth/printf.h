#ifndef PRINTF_H
#define PRINTF_H

#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <sstream>
/*
 * printf function, used by logger class.
 */
template <typename T> void printVal(llvm::raw_fd_ostream& stream, T val) { stream << val; }

size_t my_printf(llvm::raw_fd_ostream& stream, const char* format);

template <typename T, typename... Args>
void my_printf(llvm::raw_fd_ostream& stream, const char* format, T val, Args... args) {
    while (*format) {
        if (*format != '%') {
            stream.write(*format++);
        } else {
            if (*(format + 1) == '%') {
                stream.write(*format);
                format += 2;
            } else {
                printVal(stream, val);
                my_printf(stream, format + 1, args...);
                break;
            }
        }
    }
}

#endif