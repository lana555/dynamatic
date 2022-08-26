set(CMAKE_BUILD_TYPE Debug)

set(libsrc
    AddComp.cpp
    AddPhi.cpp
    AddBridges.cpp
    AddCtrl.cpp
    Nodes.cpp
    SanityChecker.cpp
    Pragmas.cpp
    ComponentsTiming.cpp
    Memory.cpp 
    PrintDot.cpp
    Bitwidth.cpp
    dbgPrints.cpp
	RemoveRedunBranches.cpp
	Quine_McCluskey.cpp
	Shannon_Expansion.cpp
	AddInj.cpp
	new_RemoveRedunComp.cpp
	AddBuffers_naive.cpp
    AddSuppress.cpp
    ConvertGSA.cpp
    SuppressLoopProd.cpp
)

add_library(ElasticPass MODULE ${libsrc})

target_compile_options(ElasticPass PUBLIC -g3 -O0 -std=c++11 -pedantic -Wall -fPIC)
set(CMAKE_CXX_FLAGS "-fno-rtti -fPIC")
