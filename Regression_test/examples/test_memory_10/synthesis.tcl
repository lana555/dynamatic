
set_project .
set_top_file test_memory_10.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



