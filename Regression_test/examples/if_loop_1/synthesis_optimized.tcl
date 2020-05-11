
set_project .
set_top_file if_loop_1.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



