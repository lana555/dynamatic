vlib work
vmap work work
project new . simulation work modelsim.ini 0
project open simulation
project addfile ../VHDL_SRC/hls_verify_histogram_tb.vhd
project addfile ../VHDL_SRC/simpackage.vhd
project addfile ../VHDL_SRC/two_port_RAM.vhd
project addfile ../VHDL_SRC/single_argument.vhd
project calculateorder
project compileall
eval vsim histogram_tb
run -all
exit
