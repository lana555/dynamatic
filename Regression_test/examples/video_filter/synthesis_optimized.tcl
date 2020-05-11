
set_project .
set_top_file video_filter.cpp
synthesize -verbose
optimize -timeout=60
write_hdl

exit



