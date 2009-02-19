OOPSHOME = /home/s0455378/src/oops
AMPLSOLVER = /home/s0455378/src/ampl/solvers/sys.i686.Linux
METIS_DIR = /home/s0455378/src/metis-4.0

AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS =
#AM_LEXFLAGS += -d
AM_CFLAGS = -g
AM_CXXFLAGS = -g -I../../src -I$(AMPLSOLVER) -I$(OOPSHOME)/include -gstabs

bin_PROGRAMS = smloops
smloops_SOURCES = wrapper.cpp OOPSBlock.cpp sml-oops.cpp
smloops_LDADD = 	../../src/libsml.a -L$(AMPLSOLVER) -lamplsolver \
						-L$(OOPSHOME)/lib -loops -llapack \
              		-L$(METIS_DIR) -lmetis -lgfortran -lfl -ldl
