AM_YFLAGS = -d
#AM_YFLAGS += -t -v
AM_LEXFLAGS = 
#AM_LEXFLAGS += -d
AM_CFLAGS = -g 
AM_CXXFLAGS = -g -I$(AMPLSOLVER) -I$(OOPSHOME)/include -gstabs

# for parallel uncomment this
#CXXFLAGS += -DWITH_MPI -DMPI_PATH="\"mpi.h\"" 
#CXX = mpic++

lib_LIBRARIES = libsml.a

libsml_a_SOURCES = sml.tab.ypp sml.yy.lpp symtab.cpp nodes.cpp backend.cpp \
				  AmplModel.cpp ModelComp.cpp ExpandedModel.cpp sml.cpp \
				  AmplsolverCalls.cpp StochModel.cpp \
				  StochModelComp.cpp data.tab.ypp data.yy.lpp Set.cpp \
				  CompDescrParam.cpp SetElement.cpp SetNode.cpp

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
