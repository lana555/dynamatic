
set_project .
set_top_file image_resize.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



