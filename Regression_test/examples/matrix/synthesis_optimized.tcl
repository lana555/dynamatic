
set_project .
set_top_file matrix.cpp
synthesize -verbose
optimize -timeout=240
write_hdl

exit



