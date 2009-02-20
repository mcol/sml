AMPLSOLVER = /home/s0455378/src/ampl/solvers/sys.i686.Linux

AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS =
#AM_LEXFLAGS += -d
AM_CFLAGS = -g
AM_CXXFLAGS = -g -I../../src -I$(AMPLSOLVER) -I$(OOPSHOME)/include -gstabs

bin_PROGRAMS = smlmps
smlmps_SOURCES = sml-mps.cpp wrapper.cpp
smlmps_LDADD = 	../../src/libsml.a -L$(AMPLSOLVER) -lamplsolver -lfl -ldl
