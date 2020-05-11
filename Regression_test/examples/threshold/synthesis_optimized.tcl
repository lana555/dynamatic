
set_project .
set_top_file threshold.cpp
synthesize -verbose
set_period 5
optimize
write_hdl

exit



