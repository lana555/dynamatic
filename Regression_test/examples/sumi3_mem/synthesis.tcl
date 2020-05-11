
set_project .
set_top_file sumi3_mem.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



