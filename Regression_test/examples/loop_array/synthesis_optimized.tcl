
set_project .
set_top_file loop_array.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



