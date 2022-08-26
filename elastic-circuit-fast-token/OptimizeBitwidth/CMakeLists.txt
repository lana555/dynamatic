set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DTESTS)
add_definitions(-DSTATS)
#set or unset this parameter depending usage of vectorization during optimization passes
add_definitions(-DNO_VECTORIZATION)
set (libsrc OptimizeBitwidth.cpp info.cpp deps.cpp)

add_library( LLVMOptimizeBitWidth MODULE ${libsrc})

target_compile_options(LLVMOptimizeBitWidth PUBLIC -g3 -O0 -std=c++11 -pedantic -Wall -fPIC)
SET(CMAKE_CXX_FLAGS "-fno-rtti")
