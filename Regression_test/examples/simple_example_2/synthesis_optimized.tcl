
set_project .
set_top_file simple_example_2.cpp
synthesize -verbose
set_period 3
optimize
write_hdl

exit



