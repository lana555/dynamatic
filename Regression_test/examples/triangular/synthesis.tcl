
set_project .
set_top_file triangular.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



