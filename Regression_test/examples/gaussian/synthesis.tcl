
set_project .
set_top_file gaussian.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



