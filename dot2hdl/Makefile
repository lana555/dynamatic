# 
# Makefile : dotvhdl
# 
# Description: 
# 
#  
# Rev. 0.1
# 
# Author: Andrea Guerrieri <andrea.guerrieri@epfl.ch (C) 2017
#
# Copyright: See COPYING file that comes with this distribution
#
# 

CROSS_COMPILE ?=

AS		= $(CROSS_COMPILE)as
#CC		= $(CROSS_COMPILE)gcc
CC		= $(CROSS_COMPILE)g++
CD		= cd
LD		= $(CC) -nostdlib
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
DEPMOD		= /sbin/depmod
KALLSYMS	= scripts/kallsyms
PERL		= perl
CHECK		= sparse
DOXYGEN		= doxygen

APP = dot2hdl

SRCDIRVER=./src/Verilog
SRCDIRVHD=./src/VHDL
SRCDIRSHR=./src/shared

OBJDIRVER=./obj/Verilog
OBJDIRVHD=./obj/VHDL
OBJDIRSHR=./obj/shared

BINDIR=./bin
DOCSDIR=./docs

#CC=g++
#CFLAGS=-O3 -Wall
#CFLAGS=-O0 -g -Wall
#CFLAGS=pthread -lm -O0 -g -static-libgcc -Wall -static -fpermissive
# DEFINE1=-DUSE_PL_MEMORY
#DEFINE2=-D_USE_TCP
DEFINES=${DEFINE1} ${DEFINE2} ${DEFINE3}

CFLAGS=-O3 -g -Wall -static -fpermissive $(DEFINES) 

#LFLAGS=-lpthread -lm
LFLAGS=
#-static

MD = mkdir



$(BINDIR)/$(APP) :: $(OBJDIRVHD)/table_printer.o  $(OBJDIRVHD)/vhdl_writer.o $(OBJDIRVHD)/lsq_generator.o $(OBJDIRVHD)/checks.o $(OBJDIRVHD)/eda_if.o $(OBJDIRVHD)/reports.o $(OBJDIRVHD)/string_utils.o $(OBJDIRVHD)/sys_utils.o\
		 	$(OBJDIRVER)/BranchComponent.o  $(OBJDIRVER)/BufferComponent.o $(OBJDIRVER)/ComponentClass.o $(OBJDIRVER)/ConstantComponent.o $(OBJDIRVER)/ControlMergeComponent.o $(OBJDIRVER)/dot_reader1.o $(OBJDIRVER)/EndComponent.o\
		 	$(OBJDIRVER)/FIFOComponent.o  $(OBJDIRVER)/ForkComponent.o $(OBJDIRVER)/Graph.o $(OBJDIRVER)/GraphToVerilog.o $(OBJDIRVER)/IntegerArithmeticComponent.o $(OBJDIRVER)/LoadComponent.o $(OBJDIRVER)/LSQComponent.o\
		 	$(OBJDIRVER)/LSQControllerComponent.o  $(OBJDIRVER)/MemoryComponent.o $(OBJDIRVER)/MergeComponent.o $(OBJDIRVER)/MuxComponent.o $(OBJDIRVER)/SinkComponent.o $(OBJDIRVER)/SourceComponent.o $(OBJDIRVER)/StartComponent.o\
		 	$(OBJDIRVER)/StoreComponent.o  $(OBJDIRVER)/string_utility.o\
		 	$(OBJDIRSHR)/dot_parser.o\
		 	$(OBJDIRSHR)/$(APP).o
	$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS)

$(OBJDIRVHD) ::
	$(MD) -p $(obj) $(OBJDIRVHD) $(OBJDIRVER) $(OBJDIRSHR)

#Compiling VHDL
$(OBJDIRVHD)/string_utils.o :: $(OBJDIRVHD) $(SRCDIRVHD)/string_utils.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)
	
$(OBJDIRVHD)/sys_utils.o :: $(OBJDIRVHD) $(SRCDIRVHD)/sys_utils.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/table_printer.o :: $(OBJDIRVHD) $(SRCDIRVHD)/table_printer.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/vhdl_writer.o :: $(OBJDIRVHD) $(SRCDIRVHD)/vhdl_writer.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/lsq_generator.o :: $(OBJDIRVHD) $(SRCDIRVHD)/lsq_generator.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/eda_if.o :: $(OBJDIRVHD) $(SRCDIRVHD)/eda_if.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/checks.o :: $(OBJDIRVHD) $(SRCDIRVHD)/checks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVHD)/reports.o :: $(OBJDIRVHD) $(SRCDIRVHD)/reports.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)
	
#Compiling Verilog
$(OBJDIRVER)/BranchComponent.o :: $(SRCDIRVER)/BranchComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)
	
$(OBJDIRVER)/BufferComponent.o :: $(SRCDIRVER)/BufferComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)
	
$(OBJDIRVER)/ComponentClass.o :: $(SRCDIRVER)/ComponentClass.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/ConstantComponent.o :: $(SRCDIRVER)/ConstantComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/ControlMergeComponent.o :: $(SRCDIRVER)/ControlMergeComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/dot_reader1.o :: $(SRCDIRVER)/dot_reader1.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/EndComponent.o :: $(SRCDIRVER)/EndComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/FIFOComponent.o :: $(SRCDIRVER)/FIFOComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/ForkComponent.o :: $(SRCDIRVER)/ForkComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/Graph.o :: $(SRCDIRVER)/Graph.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/GraphToVerilog.o :: $(SRCDIRVER)/GraphToVerilog.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/IntegerArithmeticComponent.o :: $(SRCDIRVER)/IntegerArithmeticComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/LoadComponent.o :: $(SRCDIRVER)/LoadComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/LSQComponent.o :: $(SRCDIRVER)/LSQComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/LSQControllerComponent.o :: $(SRCDIRVER)/LSQControllerComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/MemoryComponent.o :: $(SRCDIRVER)/MemoryComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/MergeComponent.o :: $(SRCDIRVER)/MergeComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/MuxComponent.o :: $(SRCDIRVER)/MuxComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/SinkComponent.o :: $(SRCDIRVER)/SinkComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/SourceComponent.o :: $(SRCDIRVER)/SourceComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/StartComponent.o :: $(SRCDIRVER)/StartComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/StoreComponent.o :: $(SRCDIRVER)/StoreComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/string_utility.o :: $(SRCDIRVER)/string_utility.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/StartComponent.o :: $(SRCDIRVER)/StartComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRVER)/StartComponent.o :: $(SRCDIRVER)/StartComponent.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)




#Compiling Shared Files
$(OBJDIRSHR)/dot_parser.o :: $(SRCDIRSHR)/dot_parser.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

$(OBJDIRSHR)/$(APP).o :: $(SRCDIRSHR)/$(APP).cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIRVHD) -I $(SRCDIRVER) -I $(SRCDIRSHR)

clean ::
	rm -rf $(BINDIR)/* $(OBJDIRVHD)/*.o $(OBJDIRVER)/*.o $(OBJDIRSHR)/*.o 
