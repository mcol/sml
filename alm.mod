# This is an attempt at the ALM model to demonstrate the 
# stochastic programming part of SML
#
# REMARKS:
#
#  - stochastic, deterministic are attributes of variables/parameters
#  - do we need stochastic/deterministic constraints/sets?
#  - keyword 'in' in variable/constraint definition to declare timestages
#  - Exp and x(-1,i) functions as in Hochreiter: if Exp is used in constraints
#    the timestage parameter is not needed (as it is clear from the context)
#  - keyword 'using' to specify tree information
#  - do not use 'sblock alm{t in STAGES}' since this implies a repetition 
#    of blocks on the same level, rather than nesting

set ASSETS;
set TIME ordered;
set NODES;
param A{NODES} symbolic;
param CP{NODES};

# these should really be declared within the alm block and not global
# => need to be able to understand data file and write partial data files
param ret{NODES, ASSETS};
param InitCash;
param tc;     
param lambda;
var mu>=0;
param Liability{TIME};

sblock alm using (TIME, NODES, A, CP):
  # all param/var are 'stochastic' by default
  # stochastic var/param are implicitly indexed over NODES
  # deterministic var/param are implicitly indexed over TIME

#  param Liability deterministic;  #how would these be set in data file?
#  param ret{ASSETS};              #how would the indexing over nodes work:
#                                  # as vector or tree?

  var xh{ASSETS} >=0;
  var xs{ASSETS} >=0;
  var xb{ASSETS} >=0;

  subject to Inventory{i in ASSETS} stages (TIME diff {first(TIME)}):
      xh[i] = ret[node,i]*xh(-1; i) + xb[i] - xs[i];
  subject to CashBalance stages (TIME diff {first(TIME)}):
      (1-tc)*sum{i in ASSETS} xs[i] = 
	Liability[stage] + (1+tc)*sum{i in ASSETS} xb[i];
  subject to CashBalance1 stages ({first(TIME)}):
      (1+tc)*sum{i in ASSETS} xh[i] = InitCash - Liability[stage];
# not clear if the one- or two-argument version of Exp should be used
# currently use one argument version, two arg version would give more
# flexibility (but harder to implement)
#  subject to ExpCons stages {first(TIME)}:
#      (1-tc)*Exp(sum{i in ASSETS}xh[i], last(TIME)) = mu;
  subject to ExpCons stages {last(TIME)}:
      (1-tc)*Exp(sum{i in ASSETS}xh[i]) = mu;
#  maximize FinalWealth:  mu - lambda*(Exp(xh*xh, last(TIME))-mu*mu);
  maximize FinalWealth stages {last(TIME)}:
	(1-tc)*Exp(sum{i in ASSETS}xh[i]);
end sblock;


#-----------------------------------------------------------------------------
# Tree data is indexed over the nodes of the tree
# set NODES;
# set TIME;
# param A{NODES};  #ancestor of every node
# param CP{NODES}; #conditional probability of every node
#
# if we do this there is some redundancy: TIME
# BUT 'TIME' is referred to in the constraint/variable declarations
#
# param ret{NODES};
# param Liability{TIME};

#PROBLEM: this might be far to much data to specify
#         should we have some tree construction functions?
#         Tree(TIME, BranchFactor, ...)?
