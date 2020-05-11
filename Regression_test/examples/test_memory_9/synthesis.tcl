
set_project .
set_top_file test_memory_9.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



