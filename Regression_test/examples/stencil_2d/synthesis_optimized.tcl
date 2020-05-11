
set_project .
set_top_file stencil_2d.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



