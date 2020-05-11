
set_project .
set_top_file loop_array.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



