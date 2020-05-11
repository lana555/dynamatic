
set_project .
set_top_file stencil_2d.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



