set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DTESTS)
add_definitions(-DSTATS)
#set or unset this parameter depending usage of vectorization during optimization passes
add_definitions(-DNO_VECTORIZATION)

include_directories(${LLVM_INCLUDE_DIRS}/polly)

set (libsrc IndexAnalysis.cpp MemElemInfo.cpp TokenDependenceInfo.cpp MemUtils.cpp)

add_library(LLVMMemElemInfo MODULE ${libsrc})
target_compile_options(LLVMMemElemInfo PUBLIC -g3 -O0 -std=c++11 -pedantic -Wall -fPIC)

SET(CMAKE_CXX_FLAGS "-fno-rtti")
