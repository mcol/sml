#OOPSHOME = /home/s0455378/src/oops
#METIS_DIR = /home/s0455378/src/metis-4.0

AM_CFLAGS = -g
AM_CXXFLAGS = -g -I../../src @OOPS_INCLUDE@ -gstabs

bin_PROGRAMS = smloops
smloops_SOURCES = wrapper.cpp OOPSBlock.cpp sml-oops.cpp
smloops_LDADD = 	../../src/libsml.a @AMPL_LIBS@ \
						@OOPS_LIB@ -llapack @METIS_LIB@ -lgfortran -lfl -ldl
