
set_project .
set_top_file bicg.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



