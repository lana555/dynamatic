
set_project .
set_top_file vector_rescale.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



