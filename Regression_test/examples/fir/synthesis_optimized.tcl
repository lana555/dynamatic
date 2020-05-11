
set_project .
set_top_file fir.cpp
synthesize -verbose
set_period 3
optimize
write_hdl

exit



