
set_project .
set_top_file kernel_3mm.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



