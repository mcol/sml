AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS =
#AM_LEXFLAGS += -d
AM_CFLAGS = -g
AM_CXXFLAGS = -g -I../../src -gstabs

bin_PROGRAMS = smlmps
smlmps_SOURCES = sml-mps.cpp sml-mps.h wrapper.cpp
smlmps_LDADD = 	../../src/libsml.a @AMPL_LIBS@ -lfl -ldl
