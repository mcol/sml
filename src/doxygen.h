/** 
\mainpage Structure Modelling Language for OOPS 

\section Introduction

SML is a structured modelling extension to AMPL, designed to act as a
preprocessor to AMPL. The driver performs the following tasks:
- read an SML input file,
- analyse the intended problem/matrix structure,
- create a separate (plain) AMPL file for each node in the matrix tree
- process the AMPL files through AMPL (therefore creating *.nl files),
- fill the internal data structures with information about the resulting
  matrix tree.

The OOPS driver backend will create OOPS structured matrices from the
objects, and set up the appropriate call back functions that fill in
the sparse leaf nodes of the matrix tree by processing the appropriate
*.nl file.

A different backend (for example for a decomposition solver) could be
provided. The backend is independent of AMPL (just the amplsolver
library needs to be called). The frontend is dependent on AMPL.


\section Install

In the Makefile the following lines need to be set to the correct values
\code
OOPSHOME=
AMPLSOLVER=
\endcode
to point to the location of OOPS and the amplsolver (AMPL's *.nl file reader).

further the line 
\code 
LDFLAGS 
\endcode 

needs to include the correct fortran libraries for use with OOPS (this
is -lg2c for g77 and -lgfortran for gfortran)


\section Usage

In order to process an SML file say

\code
sml < name-of-smlfile
\endcode

\bug Currently the ampl scriptfile is not processed automatically. In
order to process a model one needs to say 
- sml < name-of-smlfile (which probably results in an error)
- ampl script.scr
- sml < name-of-smlfile (which does the rest of the processing)

\bug Also currently it is assumed that the *.dat file is named
"global.dat"


\section Internals

The processing of an SML file is done in three steps:
- \ref frontend "Frontend" (reading the file into the internal data structures)
- \ref scriptfile "writing out" the script file and submodel AMPL files
- Creating the \ref expanded "Expanded Model"
- \ref backend "Backend" (creating OOPS data structures from the internal SML objects)

*/

/**
\page frontend Frontend

ampl.l is the LEX/FLEX input file. It mainly consists of a list of tokens
that should be recognised in SML.

ampl.ypp is the YACC/BISON grammar file. It specifies the grammar of
SML in Backus-Naur form (BNF). Large parts of it follow the
specification of the AMPL grammar in the appendix of the AMPL book. In
YACC/BISON every "rule" returns a "value" that is computed from the
values of its components. In SML most rules return a pointer to
opNode. An opNode represents an AMPL/SML operator, and AMPL/SML
expression is thus represented as a tree of opNodes. 

The grammar processor recognizes the start and end of a block/submodel
and creates a AmplModel object for each of these. It classifies all
other lines into set/parameter/variable/constraint/objective/submodel
declarations - which are stored in a model_comp object - and attaches
them to the appropriate (current) model.

Every declaration is further divided into a name, indexing and
attribute (body) section. These are attached to the appropriate
model_comp object. The indexing and attribute expressions are
represented as a tree of opNodes. The indexing expression is actually
represented by the subobject opNodeIx, which has extra fields and
methods that allow a quick recovery of the defined dummy variables and
their dimension.

The indexing and attributes trees are postprocessed: they are
scanned for name references. Every reference is compared with the
names of model components that are currently in scope (this includes
dummy variables that are used in the indexing expression), and if
found the reference in the opNode tree is replaced by a pointer to the
appropriate model_comp (done by find_var_ref_in_context()).
\bug Currently no hashing is done in this search.

Internally all names are represented by an opNode with
opNode.opCode==ID. These are replaced by an object of subclass
opNodeIDREF with opNodeIDREF.opCode==IDREF, which carry a pointer to
the appropriate model_component.

The output of the frontend is a tree of AmplModel objects (each
representing a node of the model tree), consisting of model_comp
objects (each representing on AMPL declaration). These in turn consist
of several opNode trees representing the indexing and attribute(body)
section of the declaration
*/

/**
\page scriptfile Writing the scriptfile and submodel AMPL files

\section submodel Writing the submodel AMPL files

This is done in backend.cpp by process_model() and write_ampl_for_submodel() called from it.

For every AmplModel object a separate *.mod file is generated. The
name of the *.mod file is obtained by concatenating all the block
names on the path from the root model down to this model. Every *.mod
file contains the subset of all given model_comp declarations needed
for this block. They are obtained in the following manner:

- All set and parameter declarations in nodes *above* and including
  the current one are included. 

- All variable, constraint and objective declarations in the current
  node are included.

- All entities referenced by any included model component are also
included (there is a model_comp::dependencies list that helps this
tasks. Also there is the possibility to recursively 'tag' all
dependencies of a given model_comp object:
model_comp::tagDependencies(), and to get a list of all tagged
model_comp objects) 

- All included components are printed out in the order in which they appeared in the original AMPL/SML file. 

- Names of components and parameter list have to be changed. They are
translated into "global" names. Basically every block adds the name of
the block to the beginning of the name of every component defined
within it, and the dummy variable in the indexing expression for this
block becomes the first index of every component. For example a
variable Flow{i in ARCS} defined inside a block MCNF{j in COMM}
becomes MCNF_Flow{j in COMM, i in ARCS}. All references to Flow would
be changed from Flow[i] to Flow[j,i]

The writing out of one component of the *.mod file is done by
modified_write() Translating of local names into global names is done
by a static variable opNode::use_global_names. opNode has a
opNode::print() method that prints out the AMPL expression represented
by the tree. If opNode::use_global_names is set then all names of
IDREF nodes are replaced by their global name with the proper indexing
expression. 

A stack of applicable indexing expressions is kept in the global
variable l_addIndex/n_addIndex in backend.cpp 

The resulting *.mod file (and therefore the corresponding *.nl file
contains exactly the constraints defined in the block, but may have
more variables. Therefore in order to get the sparse matrix information 
for an OOPS Matrix tree node, some matching of variable names has to be done.

\section writescript Writing the scriptfile "script.scr"

Is done in process_model() in backend.cpp. It uses much the same logic
as the printing out of the *.mod files.


*/

/**
\page expanded Creating Expanded Model

The SML file describes the problem in "flat" form: that is only one
instance of each type of block is present (and therefore only one
instance of an AmplModel object). The indexing expression that
carries information about the number of instances of a block is just
passed through but not analysed.

In the ExpandedModel tree however there is one instance of each block
for each member of the indexing set.
Since the SML interpreter does not attempt
to understand AMPL enough to generate the indexing set member list
itself, the ampl-interpreter is used for this purpose. 

\section Naming of Nodes

While nodes in the "flat" model are named by concatenating the block
names from root to the current model, in the expanded model nodes are
named by also concatenating the instance of each block (this is the
value of the dummy variable in the indexing expression in this branch). 
That is a model tree
\code
block MCNF{i in ARCS}:

 block RouteComm{j in COMM}:

 end block

end block
\endcode

results in flat tree nodes (and the AMPL submodel *.mod files) being
called root, root_MCNF and root_MCNF_RouteComm and expanded tree nodes
being called root_MCNF_A1, root_MCNF_A2, ... and
root_MCNF_RouteComm_A1_C1, root_MCNF_RouteComm_A1_C2, etc.

\section Generation of expanded model instance lists/cardinalities

Lines 

\code 
print card(<indexing-set-name>) > "<name-of-node>.crd"
display <indexing-set-name> > "<name-of-node>.set" 
\endcode 

are included in the script file for every block definition within a
given (sub)model to print out the cardinality and the members for each
indexing set. <name-of-node> is here the name of the node in the
expanded tree (including instance names). So the cardinality (and list
of members) of COMM in the example above could be different depending
on the value of the first level dummy variable i. These different
values for the second level indexing set COMM would be reflected in
the files root_MCNF_RouteComm_A1.crd/set,
root_MCNF_RouteComm_A2.crd/set, giving the cardinality and instance
names of the root_MCNF_RouteComm submodel in the root_MCNF_A1/A2
branches of the expanded model

\section Generation of the expanded model tree

The expanded model is represented as a tree of ExpandedTree
objects. The tree is generated by calling the constructor
ExpandedModel::ExpandedModel(AmplModel* ) which in turn calls
AmplModel::createExpandedModel(string, string).

*/

/**
\page backend OOPS Backend

This is done by SML_OOPS_driver() in sml-oops.cpp. A node in the OOPS
Matrix tree is represented by an OOPSBlock object. The OOPS matrices A
and Q are created in generateSML(ExpandedModel*). 

A node in the OOPS Matrix tree for A is basically represented by two
ExpandedModel objects (with their corresponding NlFile objects):

-# the first specifies the row node, i.e. the NlFile from which the constraints
should be taken,
-# the other specifies the col node, i.e. which variables should be taken.

These are the two parameters in the OOPSBlock constructor.

The specification of the variables is a bit tricky. The ExpandedModel
node specifying the column doesn't directly carry a list of relevant
expanded variable (names). All it contains is a list of variable
definitions (from the AmplModel object that spawned it) and an NlFile
object (that however contains more variable definitions than
needed). What is done is to get a list of applicable variables for
each ExpandedModel object by getting the list of variables defined in
its NlFile and comparing this against the variable declarations in the
originating AmplModel object. After obtaining this list of applicable variables
this is compared with the variables declared in the row ExpandedModel. 
\bug This part of the implementation needs a lot of comparing
strings against strings in nested loops and is quite inefficient.

This is all done in the constructor
OOPSBlock::OOPSBlock(ExpandedModel*, ExpandedModel*).

Once this is done, the setup of the OOPS Matrices is straightforward.
Each node in the OOPS matrix tree is generated with a pointer to the
corresponding OOPSBlock object being passed as the identifier (which
is subsequently used in the CallBack function)

All calls to amplsolver (the AMPL nl-file reader library) are done in
the class NlFile. This is mainly because the main amplsolver include
file "asl.h" defines many global variables, some of which clash with
C++ keywords (e.g. list). This way the use of these keywords only
needs to be avoided in NlFile.cpp

*/
