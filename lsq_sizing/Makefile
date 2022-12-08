# 
# Makefile for lsq_sizing
#
# Author: Jiantao Liu <jianliu@student.ethz.ch>

CROSS_COMPILE ?=

AS			= $(CROSS_COMPILE)as
CC			= $(CROSS_COMPILE)g++
CD			= cd
LD			= $(CC)
CPP			= $(CC) -E
AR			= $(CROSS_COMPILE)ar
NM			= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK			= awk
KALLSYMS	= scripts/kallsyms
PERL		= perl
CHECK		= sparse
DOXYGEN		= doxygen

APP = lsq_sizing

SRCDIR=./src
OBJDIR=./src
BINDIR=./bin
LIBDIR=./src
DOCSDIR=./docs

DEFINES=${DEFINE1} ${DEFINE2} ${DEFINE3}

CFLAGS=`pkg-config libgvc --cflags` -std=c++14 -Wall -fPIC -Wno-sign-compare -Wno-unused-function -O3
LFLAGS=`pkg-config libgvc --libs`
IDIR=$(LIBDIR)

$(BINDIR)/$(APP) :: \
		$(shell mkdir -p $(BINDIR)) \
		$(SRCDIR)/main.o $(SRCDIR)/Dataflow.o $(SRCDIR)/DFnetlist.o $(SRCDIR)/DFlib.o $(SRCDIR)/DFnetlist_BasicBlocks.o \
		$(SRCDIR)/DFnetlist_channel_width.o \
		$(SRCDIR)/DFnetlist_check.o $(SRCDIR)/DFnetlist_Connectivity.o \
		$(SRCDIR)/DFnetlist_MG.o $(SRCDIR)/DFnetlist_lsq_param.o $(SRCDIR)/DFnetlist_path_finding.o $(SRCDIR)/DFnetlist_read_dot.o $(SRCDIR)/DFnetlist_write_dot.o
		$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS) \

$(SRCDIR)/DFnetlist.o :: $(SRCDIR)/DFnetlist.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/Dataflow.o :: $(SRCDIR)/Dataflow.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFlib.o :: $(SRCDIR)/DFlib.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlsit_BasicBlocks.o :: $(SRCDIR)/DFnetlsit_BasicBlocks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_channel_width.o :: $(SRCDIR)/DFnetlist_channel_width.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_check.o :: $(SRCDIR)/DFnetlist_check.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_Connectivity.o :: $(SRCDIR)/DFnetlist_Connectivity.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
$(SRCDIR)/DFnetlist_MG.o :: $(SRCDIR)/DFnetlist_MG.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_lsq_param.o :: $(SRCDIR)/DFnetlist_lsq_param.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_path_finding.o :: $(SRCDIR)/DFnetlist_path_finding.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_read_dot.o :: $(SRCDIR)/DFnetlist_read_dot.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_write_dot.o :: $(SRCDIR)/DFnetlist_write_dot.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DFnetlist_BasicBlocks.o :: $(SRCDIR)/DFnetlist_BasicBlocks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/main.o :: $(SRCDIR)/main.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
	
docs :: 
	$(CD) $(DOCSDIR) && \
	$(DOXYGEN) $(SWIFT_API_DOXY) && \
	$(DOXYGEN) $(SWIFT_STRUCTURE_DOXY) &&\
	$(DOXYGEN) $(SWIFT_SAMPLE_DOXY)

clean ::
	rm -rf $(BINDIR)/* $(OBJDIR)/*.o $(LIBDIR)/*.o





