
set_project .
set_top_file example.c
synthesize -use-lsq=false -simple-buffers=true 
cdfg
write_hdl
exit



