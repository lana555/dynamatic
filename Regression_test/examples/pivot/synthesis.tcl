
set_project .
set_top_file pivot.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



