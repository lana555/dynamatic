# 
# Makefile : DHLS Shell
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

APP = dhls
ANALYZE = analyze
ELABORATE = elaborate

SRCDIR=./src
OBJDIR=./src
BINDIR=./bin

#CC=g++
#CFLAGS=-O3 -Wall
#CFLAGS=-O0 -g -Wall
#CFLAGS=pthread -lm -O0 -g -static-libgcc -Wall -static -fpermissive
# DEFINE1=-DUSE_PL_MEMORY
#DEFINE2=-D_USE_TCP
DEFINES=${DEFINE1} ${DEFINE2} ${DEFINE3}

CFLAGS=-O3 -g -Wall -fpermissive $(DEFINES) 

#LFLAGS=-lpthread -lm
LFLAGS=
#-static
IDIR=$(SWIFTLIB)

all: $(BINDIR)/$(APP) $(BINDIR)/$(ANALYZE) $(BINDIR)/$(ELABORATE)

$(BINDIR)/$(APP) :: $(SRCDIR)/parser.o $(SRCDIR)/$(APP).o
	$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS)

$(BINDIR)/$(ANALYZE) :: $(SRCDIR)/analyze.o $(SRCDIR)/$(ANALYZE).o
	$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS)

$(SRCDIR)/analyze.o :: $(SRCDIR)/analyze.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(BINDIR)/$(ELABORATE) :: $(SRCDIR)/elaborate.o $(SRCDIR)/$(ELABORATE).o
	$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS)

$(SRCDIR)/elaborate.o :: $(SRCDIR)/elaborate.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/parser.o :: $(SRCDIR)/cmd_parser.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

$(SRCDIR)/$(APP).o :: $(SRCDIR)/$(APP).cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) -I $(SRCDIR)

docs :: 
	$(CD) $(DOCSDIR) && \
	$(DOXYGEN) $(SWIFT_API_DOXY) && \
	$(DOXYGEN) $(SWIFT_STRUCTURE_DOXY) &&\
	$(DOXYGEN) $(SWIFT_SAMPLE_DOXY)

clean ::
	rm -rf $(BINDIR)/* $(OBJDIR)/*.o
