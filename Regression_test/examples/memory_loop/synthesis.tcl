
set_project .
set_top_file memory_loop.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



