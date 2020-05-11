
set_project .
set_top_file bicg.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



