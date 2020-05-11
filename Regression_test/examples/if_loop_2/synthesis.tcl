
set_project .
set_top_file if_loop_2.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



