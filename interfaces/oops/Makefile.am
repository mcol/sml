OOPSHOME = /home/s0455378/src/oops
METIS_DIR = /home/s0455378/src/metis-4.0

AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS =
#AM_LEXFLAGS += -d
AM_CFLAGS = -g
AM_CXXFLAGS = -g -I../../src -I$(OOPSHOME)/include -gstabs

bin_PROGRAMS = smloops
smloops_SOURCES = wrapper.cpp OOPSBlock.cpp sml-oops.cpp
smloops_LDADD = 	../../src/libsml.a @AMPL_LIBS@ \
						-L$(OOPSHOME)/lib -loops -llapack \
              		-L$(METIS_DIR) -lmetis -lgfortran -lfl -ldl
