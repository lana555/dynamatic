
set_project .
set_top_file if_loop_3.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



