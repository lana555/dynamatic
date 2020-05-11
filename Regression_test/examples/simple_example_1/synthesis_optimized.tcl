
set_project .
set_top_file simple_example_1.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



