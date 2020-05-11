
set_project .
set_top_file memory_loop.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



