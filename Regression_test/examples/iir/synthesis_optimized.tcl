
set_project .
set_top_file iir.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



