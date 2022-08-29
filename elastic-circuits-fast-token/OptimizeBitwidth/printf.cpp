#include "OptimizeBitwidth/printf.h"

size_t my_printf(llvm::raw_fd_ostream& stream, const char* format) { stream << format; }
