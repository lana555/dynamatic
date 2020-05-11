
set_project .
set_top_file kernel_2mm.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



