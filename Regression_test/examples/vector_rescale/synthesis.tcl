
set_project .
set_top_file vector_rescale.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



