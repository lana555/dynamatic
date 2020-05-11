
set_project .
set_top_file image_resize.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



