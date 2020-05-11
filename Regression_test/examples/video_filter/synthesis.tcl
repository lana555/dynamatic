
set_project .
set_top_file video_filter.cpp
synthesize -simple-buffers=true -verbose
#cdfg
write_hdl

exit



