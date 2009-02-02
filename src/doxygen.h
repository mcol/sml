/** 
\mainpage Structured Modelling Language for OOPS 

This is the doxygen documentation for the \ref language "Structured Modelling Language (SML)"
Interface to OOPS.

\section Introduction


\ref language "SML" is a structured modelling extension to AMPL, designed to act as a
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
sml name-of-smlfile name-of-datafile
\endcode

\section sec_interface Solver Interface

SML can be interfaced with any structure exploiting solver as the backend. 

A detailed description of the \ref interface "Solver Interface" can be found here.

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

\ref stochmodel "Stochastic Programming models" defined by sblock
commands are treated specially. They will be read into a StochModel
object.

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

\note Also need to describe the data file parser and its classes
*/

/**
\page stochmodel Processing of Stochastic Programming blocks (sblocks)

An sblock definition is read in as a normal block definition (just
that it creates a StochModel object rather than an AmplModel object).
The difference is that StochModel carries information about the
parameters of the sblock (STAGES, NODES, PROBABILITY, ANCENSTOR).

The components of a StochModel are StochModelComp objects (rather than
model_comp objects). The difference here is that a STochModelComp
carries information about which stages this component belongs to and
if a component is "deterministic" (i.e. there is only one copy of it
per stage, not one copy per node)

After reading is complete (i.e. when the "end sblock" line
is read) the StochModel object is translated into a chain of
AmplModel objects.  This is done by StochModel::expandToFlatModel

\section Expansion to flat model tree 

Expansion works in two passes. In the first pass the chain of
AmplModel objects is build (whose components are still StochModelComp
objects) In the second pass the StochModelComp objects are translated
to model_comp objects (that is references to StochModelComp objects
are resolved to references to model_comp objects and special
stochastic programming expressions such as Exp, node, stage are
translated).

The two passes are necessary since model components can refer to other
components up or down the tree. Hence the re-resolving of references
from StochModelComp to model_comp can only be done once the AmplModel
tree is complete.

In detail the steps in the expansion procedure are as follows

PASS 1:
 - Expand the STAGES set for the StochModel (StochModel::expandStages)
   and for all its components (StochModel::expandStagesOfComp) 
   This is done by setting up an ampl script that is processed by ampl and
   whose output is read in again
 - Create an AmplModel for each element in STAGES. Add a clone of all
   StochModelComp object that should be included in this stage to the model.
   Also add a model_comp for the next stage down and its indexing expression
   to the AmplModel. The indexing
   expression is of the form
   \code
     set indS0 :={this_nd in NODES:A[this_nd]=="root"};
     block S1{ix0 in indS0}
     ...
       set indS1 :={this_nd in NODES:A[this_nd]==ix0};
       block S2{ix1 in indS1}
   \endcode
   Here indS0 is a new model_comp that is added to the AmplModel before the
   model_comp representing the subblock of the next stage.

PASS2:
  - StochModel::_transcribeComponents: recursively call 
    StochModelComp::transcribeToModelComp for all components of all AmplModels
    within the chain.
    StochModelComp::transcribeToModelComp will
    - create a deep copy of the StochModelComp
    - find all IDREF nodes in dependecy and resolve them w.r.t  AmplModel chain
      (also resolving 'ancestor' references)
    - find all STAGE/NODE nodes and translate them
    - find all EXP expressions and translate them
      (EXP constraints are moved up to the correct level in the AmplModel chain
       Actually they are queued to be moved later to not mess up the recursion)
  - AmplModel::applyChanges
     Apply queued moves of model components (originating from Exp constraints)
  - Finally the dependency lists model_comp::dependencies are rebuilt 
    (root->reassignDependencies)

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


/**
\page language The Structured Modelling Language (SML)

\section block Blocks (Submodels)

The main extension of SML over AMPL is the introduction of the keyword
'block':
\code
 set COMMODITIES;
 block Net{k in COMMODITIES}: 
    var Flow{ARCS} >=0;
    ...
 end block;
\endcode
A block groups a set of model declarations (set, var, param, subject
to, minimize/maximize) together. These can be repeated over an
indexing set (as any AMPL entity can). A block is a natural
representation of a subproblem in AMPL.

Blocks can be nested. Several blocks can be defined on the
same level, thus creating a tree of blocks.

All entities within the block are local variables to this block. They
are all repeated over the indexing set indicated in the block
command. The above piece of code actually defines a variable Net_Flow:
\code
 var Net_Flow{k in COMMIDITIES, ARCS} >=0;
\endcode

Blocks can also be defined with the alternative syntax
\code
 set COMMODITIES;
 block Net{k in COMMODITIES}:{
    var Flow{ARCS} >=0;
    ...
 }
\endcode

\subsection Scoping 

From within the block all model components that were defined in the
block and its ancestor blocks can be used in definitions. Model
components defined in a sublock can be used by need to be accessed
"through" the name of the subblock. That is from outside the block
variable Flow can be refered to as
\code
 Net[k].Flow[j];
\endcode

Model components defined in sister blocks (i.e. blocks defined on the
same level) or their child blocks cannot be used.

\section sml_sp Stochastic Programming

A stochastic programming block can be defined as
\code
 block alm stochastic using (STAGES, NODES, PARENTS, PROBS):
   ...
 end block;
\endcode
or short
\code
 sblock alm using (STAGES, NODES, PARENTS, PROBS):
   ...
 end sblock;
\endcode

Here 'alm' is the name of the block, 'STAGES' and 'NODES' are sets of
stages and nodes respectively, 'PARENTS{NODES}, PROBS{NODES}' are
parameter arrays indexed over the set NODES that give the parent
node and the conditional probability of a given node respectively.
The parent of the root node must be set to the string "null".
The PARENTS array must imply the same number of stages as are given
in the STAGES set.

@note PARENT is a (symbolic) parameter indexed over the set NODES that 
gives for every node in NODES the name of its parent node. For the root node 
it is required that PARENT[root] = "null"

There are a variety of specifiers that can be applied to model
components declared within an sblock:
<ul>
<li> Components can be declared in only some of the stages. This can be specified in two ways: 
- either by using the 'stages' keyword in the definition
  \code
    subject to Inventory{i in ASSETS} stages (STAGES diff {first(STAGES)}):
      ...
  \endcode
- or by specifying a stage block:
  \code
    set first := STAGES diff {first(STAGES)};
    stage first:
      subject to Inventory{i in ASSETS}:
        ...
    end stage
  \endcode
- also with the alternative syntax:
  \code
    set first := STAGES diff {first(STAGES)};
    stage first:{
      subject to Inventory{i in ASSETS}:
        ...
    }
  \endcode
<li> All model components declared in an sblbock are by default
     indexed over the NODES set. Some parameters or variables only have one
     value for every stage not for every node within the stage. These can
     be defined by the 'deterministic' keyword:
    \code
      param Liability deterministic, >=0;
    \endcode

  @bug This is understood by the parse, but not implemented yet in the backend

<li> It is possible to explicitely refer to the node or stage of a component (for example to refer to parameters declared outside the sblock):
\code
 param Liability{STAGES}; //an alternative deterministic parameter
 sblock alm using (STAGES, NODES, PARENT, PROBS):
   subject to CashBalance:
      sum{i in ASSETS} xs[i] =  Liability[stage] + sum{i in ASSETS} xb[i];
   ...
 end sblock;
 \endcode

 Here the constraint CashBalance is repeated over all nodes in the
 sblock, whereas the keyword 'stage' in the constraint is replaced by
 the stage of the constraint.

 
 @bug Rather than implementing this as keywords 'node'/'stage' this
 should be done by declaring dummy variables for the NODES, STAGES
 set:
\code
 sblock alm using (st in STAGES, nd in NODES, ANCESTORS, PROBS):
  ..
      ...  Liability[st] +...
 \endcode

</ul>

\section exp Expectation Constraints

Add something on how to write Expectation type constraints

\section problems Known Problems

@bug Currently *all* parameters must be global (i.e. declared in the top
level block). This should be fixed once SML understands data files.

@bug variables that are defined over higher dimensional indexing sets
*must* have a dummy variable in their definition,i.e.
\code
  set NODES; 
  set ARCS within NODES cross NODES;
  var sparecap{(i,j) in ARCS};
\endcode
This is because SML needs to create a sum over all instances of the
variables (at least for the dummy objective). Without the dummy
variable (i,j) SML has no way of knowing that the set ARCS is
2-dimensional. This should be fixed once SML understands data files.

@bug In an sblock all entities *must* have different names, even if
they are defined in different stages, i.e.
\code
  subject to CashBalance stages (TIME diff {first(TIME)}): ...
  subject to CashBalance1 stages ({first(TIME)}):...
\endcode
This can probably remedied by encoding the stages information in the
internally used global name somehow.


\page interface Solver Interface

The information about the problem and its structure as processed by
SML is stored in a tree of ExpandedModel objects. The solver can be called by

\code
SML_OOPS_driver(em)
\endcode
where 'em' is the ExpandedModel representing the root node.

@bug This behaviour should be changed. Rather we should do something like Amplsolver: 
- Either, compile SML into a library that provides a call of the form
\code
ExpandedModel *SML_process_model(char *model_file_name, char *data_file_name)
\endcode
- Or, pass the name of the solver to SML as a command line argument
\code
sml name-of-smlfile name-of-datafile name-of-solver 
\endcode
and SML would then finish with a final call of the form
\code
SML_<name-of-solver>_driver(em);
\endcode

Each ExpandedModel object represents one node of the Expanded Model
tree. It roughly corresponds to a child in a decomposition scheme
applied to solve the problem. It gives information about the variables
and constraints local to this node as well as links to any children.

It further provides an interface to AMPL that provides functions to
ask AMPL to evaluate constraint functions on this node.

*/
