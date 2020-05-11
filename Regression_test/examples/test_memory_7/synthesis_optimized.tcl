
set_project .
set_top_file test_memory_7.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



