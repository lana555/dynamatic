set(CMAKE_BUILD_TYPE Debug)

add_library(FrequencyDataGatherPass MODULE FrequencyDataGatherPass.cpp)
# target_link_libraries(MyCFGPass LLVMOptimizeBitWidth)

target_compile_features(FrequencyDataGatherPass PRIVATE cxx_range_for cxx_auto_type)
SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
