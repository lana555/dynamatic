
set_project .
set_top_file matrix_power.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



