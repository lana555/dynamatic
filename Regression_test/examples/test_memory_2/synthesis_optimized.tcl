
set_project .
set_top_file test_memory_2.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



