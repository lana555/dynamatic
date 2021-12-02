cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "g++")

project(resource_minimization)

SET(CMAKE_EXE_LINKER_FLAGS)

include_directories(src)
include_directories(src/DFnetlist)


add_executable(resource_minimization
		src/DFnetlist/Dataflow.cpp
		src/DFnetlist/Dataflow.h
		src/DFnetlist/DFlib.cpp
		src/DFnetlist/DFnetlist_BBcycles.cpp
		src/DFnetlist/DFnetlist_buffers.cpp
		src/DFnetlist/DFnetlist_channel_width.cpp
		src/DFnetlist/DFnetlist_check.cpp
		src/DFnetlist/DFnetlist_Connectivity.cpp
		src/DFnetlist/DFnetlist.cpp
		src/DFnetlist/DFnetlist.h
		src/DFnetlist/DFnetlist_MG.cpp
		src/DFnetlist/DFnetlist_Optimize.cpp
		src/DFnetlist/DFnetlist_read_dot.cpp
		src/DFnetlist/DFnetlist_write_dot.cpp
		src/DFnetlist/DFnetlsit_BasicBlocks.cpp
		src/DFnetlist/ErrorManager.h
		src/DFnetlist/FileUtil.h
		src/DFnetlist/MILP_Model.h
		src/DFnetlist/SetUtil.h
        src/main.cpp
        src/Minimization.cpp
        src/MyPort.cpp
        src/MyPort.h
        src/MyBlock.cpp
        src/MyBlock.h
        src/MyChannel.cpp
        src/MyChannel.h
        src/resource_sharing.cpp
        src/bb_graph_reader.cpp
        src/bb_graph_reader.h
        src/DisjointSet.h
       	src/DisjointSet.cpp
        src/MarkedGraph.h
        src/MarkedGraph.cpp
        src/ControlPathAnalysis.h
        src/ControlPathAnalysis.cpp
        src/BuffersUtil.h
        src/BuffersUtil.cpp
        )


INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(gvc REQUIRED libgvc libcgraph libcdt)
LINK_DIRECTORIES(${gvc_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${gvc_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${gvc_LIBRARIES})

