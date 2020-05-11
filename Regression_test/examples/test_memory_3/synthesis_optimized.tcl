
set_project .
set_top_file test_memory_3.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



