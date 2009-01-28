OOPSHOME = /home/s0455378/src/oops
AMPLSOLVER = /home/s0455378/src/ampl/solvers/sys.i686.Linux
METIS_DIR = /home/s0455378/src/metis-4.0

AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS = 
#AM_LEXFLAGS += -d
AM_CFLAGS = -g 
AM_CXXFLAGS = -I$(AMPLSOLVER) -I$(OOPSHOME)/include -gstabs

# for parallel uncomment this
#CXXFLAGS += -DWITH_MPI -DMPI_PATH="\"mpi.h\"" 
#CXX = mpic++

bin_PROGRAMS = sml

sml_SOURCES = sml.tab.ypp sml.yy.lpp symtab.cpp nodes.cpp backend.cpp \
				  AmplModel.cpp model_comp.cpp ExpandedModel.cpp \
				  AmplsolverCalls.cpp sml-oops.cpp OOPSBlock.cpp StochModel.cpp \
				  StochModelComp.cpp data.tab.ypp data.yy.lpp Set.cpp \
				  CompDescrParam.cpp SetElement.cpp SetNode.cpp
sml_LDADD 	= -L$(AMPLSOLVER) -lamplsolver -L$(OOPSHOME)/lib -loops -llapack \
				  -L$(METIS_DIR) -lmetis -lgfortran -lfl -ldl #-lefence

# Force these to get built first
BUILT_SOURCES = sml.tab.h data.tab.h

########################################################################
# YACC and LEX:
# The proper automake way is confusing and annoying, lets do it our way.
########################################################################
data.yy.cpp: data.yy.lpp
	$(SHELL) $(YLWRAP) $< lex.data.c $@ -- $(LEXCOMPILE) -Pdata

data.tab.cpp: data.tab.ypp
	$(SHELL) $(YLWRAP) $< y.tab.c $@ y.tab.h $*.h y.output $*.output -- $(YACCCOMPILE) -pdata
