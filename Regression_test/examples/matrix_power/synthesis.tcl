
set_project .
set_top_file matrix_power.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



