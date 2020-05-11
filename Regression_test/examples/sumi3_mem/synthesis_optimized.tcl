
set_project .
set_top_file sumi3_mem.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



