
set_project .
set_top_file gaussian.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



