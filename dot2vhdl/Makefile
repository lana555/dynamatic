# 
# Makefile : dot2vhdl
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

APP = dot2vhdl

SRCDIR=./src
OBJDIR=./src
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



$(BINDIR)/$(APP) :: $(SRCDIR)/table_printer.o $(SRCDIR)/dot_parser.o  $(SRCDIR)/vhdl_writer.o $(SRCDIR)/lsq_generator.o $(SRCDIR)/checks.o $(SRCDIR)/vivado_if.o $(SRCDIR)/reports.o \
			$(SRCDIR)/string_utils.o $(SRCDIR)/sys_utils.o \
			$(SRCDIR)/$(APP).o
	$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS)

$(SRCDIR)/table_printer.o :: $(SRCDIR)/table_printer.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/dot_parser.o :: $(SRCDIR)/dot_parser.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/vhdl_writer.o :: $(SRCDIR)/vhdl_writer.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/lsq_generator.o :: $(SRCDIR)/lsq_generator.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/vivado_if.o :: $(SRCDIR)/vivado_if.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/checks.o :: $(SRCDIR)/checks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/reports.o :: $(SRCDIR)/reports.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/string_utils.o :: $(SRCDIR)/string_utils.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/sys_utils.o :: $(SRCDIR)/sys_utils.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/$(APP).o :: $(SRCDIR)/$(APP).cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

clean ::
	rm -rf $(BINDIR)/* $(OBJDIR)/*.o
