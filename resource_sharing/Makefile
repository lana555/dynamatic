# 
# Makefile : resource resource_sharing
# 
# Description: 
# This Makefile generates the resource sharing binary
#  
# Rev. 1.0
# 
# Author: Andrea Guerrieri <andrea.guerrieri@epfl.ch (C) 2021
#
# Copyright: See COPYING file that comes with this distribution
#
# 

CROSS_COMPILE ?=

AS		= $(CROSS_COMPILE)as
#CC		= $(CROSS_COMPILE)gcc
CC		= $(CROSS_COMPILE)g++
CD		= cd
#LD		= $(CC) -nostdlib
LD		= $(CC)
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


APP = resource_minimization


SRCDIR=./src
OBJDIR=./src
BINDIR=./bin
LIBDIR=./src/DFnetlist
DOCSDIR=./docs
SRCDIR_DFnetlist=./src/DFnetlist

DEFINES=${DEFINE1} ${DEFINE2} ${DEFINE3}

CFLAGS=`pkg-config libgvc --cflags` -std=c++14 -Wall -fPIC -Wno-sign-compare -Wno-unused-function -O3
LFLAGS=`pkg-config libgvc --libs`
IDIR=$(LIBDIR)


$(BINDIR)/$(APP) :: \
		$(SRCDIR)/main.o $(SRCDIR_DFnetlist)/Dataflow.o $(SRCDIR_DFnetlist)/DFnetlist.o $(SRCDIR_DFnetlist)/DFlib.o $(SRCDIR_DFnetlist)/DFnetlsit_BasicBlocks.o \
		$(SRCDIR_DFnetlist)/DFnetlist_BBcycles.o $(SRCDIR_DFnetlist)/DFnetlist_buffers.o $(SRCDIR_DFnetlist)/DFnetlist_channel_width.o \
		$(SRCDIR_DFnetlist)/DFnetlist_check.o $(SRCDIR_DFnetlist)/DFnetlist_Connectivity.o \
		$(SRCDIR_DFnetlist)/DFnetlist_MG.o $(SRCDIR_DFnetlist)/DFnetlist_Optimize.o $(SRCDIR_DFnetlist)/DFnetlist_read_dot.o $(SRCDIR_DFnetlist)/DFnetlist_write_dot.o \
		$(SRCDIR)/bb_graph_reader.o $(SRCDIR)/BuffersUtil.o $(SRCDIR)/ControlPathAnalysis.o $(SRCDIR)/DisjointSet.o \
		$(SRCDIR)/MarkedGraph.o $(SRCDIR)/Minimization.o $(SRCDIR)/MyChannel.o $(SRCDIR)/MyChannel.o $(SRCDIR)/resource_sharing.o $(SRCDIR)/MyBlock.o $(SRCDIR)/MyPort.o 
		$(CC) $(CFLAGS) $? -o $@ $(LDIR) $(LFLAGS) 


$(SRCDIR_DFnetlist)/DFnetlist.o :: $(SRCDIR_DFnetlist)/DFnetlist.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
$(SRCDIR_DFnetlist)/Dataflow.o :: $(SRCDIR_DFnetlist)/Dataflow.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFlib.o :: $(SRCDIR_DFnetlist)/DFlib.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlsit_BasicBlocks.o :: $(SRCDIR_DFnetlist)/DFnetlsit_BasicBlocks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_BBcycles.o :: $(SRCDIR_DFnetlist)/DFnetlist_BBcycles.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) 

$(SRCDIR_DFnetlist)/DFnetlist_buffers.o :: $(SRCDIR_DFnetlist)/DFnetlist_buffers.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR) 

$(SRCDIR_DFnetlist)/DFnetlist_channel_width.o :: $(SRCDIR_DFnetlist)/DFnetlist_channel_width.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_check.o :: $(SRCDIR_DFnetlist)/DFnetlist_check.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_Connectivity.o :: $(SRCDIR_DFnetlist)/DFnetlist_Connectivity.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
$(SRCDIR_DFnetlist)/DFnetlist_MG.o :: $(SRCDIR_DFnetlist)/DFnetlist_MG.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_Optimize.o :: $(SRCDIR_DFnetlist)/DFnetlist_Optimize.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_read_dot.o :: $(SRCDIR_DFnetlist)/DFnetlist_read_dot.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_write_dot.o :: $(SRCDIR_DFnetlist)/DFnetlist_write_dot.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR_DFnetlist)/DFnetlist_BasicBlocks.o :: $(SRCDIR_DFnetlist)/DFnetlist_BasicBlocks.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
$(SRCDIR)/bb_graph_reader.o :: $(SRCDIR)/bb_graph_reader.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)
	
$(SRCDIR)/BuffersUtil.o :: $(SRCDIR)/BuffersUtil.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/ControlPathAnalysis.o :: $(SRCDIR)/ControlPathAnalysis.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/DisjointSet.o :: $(SRCDIR)/DisjointSet.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/MarkedGraph.o :: $(SRCDIR)/MarkedGraph.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/Minimization.o :: $(SRCDIR)/Minimization.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/MyBlock.o :: $(SRCDIR)/MyBlock.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/MyChannel.o :: $(SRCDIR)/MyChannel.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/MyPort.o :: $(SRCDIR)/MyPort.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/resource_sharing.o :: $(SRCDIR)/resource_sharing.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)

$(SRCDIR)/main.o :: $(SRCDIR)/main.cpp
	$(CC) $(CFLAGS) -c $? -o $@ -I $(IDIR)	
	
docs :: 
	$(CD) $(DOCSDIR) && \
	$(DOXYGEN) $(SWIFT_API_DOXY) && \
	$(DOXYGEN) $(SWIFT_STRUCTURE_DOXY) &&\
	$(DOXYGEN) $(SWIFT_SAMPLE_DOXY)

clean ::
	rm $(BINDIR)/* $(OBJDIR)/*.o
