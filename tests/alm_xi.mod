# First ALM model with Stochastic dominance constraints
# written in SML
# 30/05/2008

param T;
set ASSETS;
set TIME ordered := 0..T;
#set TIME ordered, := 0..T;
set NODES;
set BENCHMARK; # set of benchmarks

param Parent{NODES} symbolic; # parent of nodes
param Price{ASSETS}; # prices of assets
param Return{ASSETS,NODES}; # returns of assets at each node
param Vbenchmk{BENCHMARK}; # values of benchmarks
param Bench2nd{BENCHMARK}; # 2nd order SD values of benchmarks
param Gama; # transaction fee

param Init; # initial wealth
param Probs{NODES}; # probability distribution of nodes

var slack2{BENCHMARK, TIME} >=0;

block alm stochastic using(nd in NODES,Parent, Probs, st in TIME):{

  var x_hold{ASSETS} >=0;
  var risk{BENCHMARK} >=0;
#  var wealth >=0;
  var cash >=0;

  stages (1..T) :{
    var x_sold{ASSETS} >=0;
    var x_bought{ASSETS} >=0;
  }

  stages {0} :{
#    var slackStartBudget >=0;
    subject to StartBudget:
      (1+Gama)*sum{j in ASSETS} x_hold[j]*Price[j] + cash = Init;
  }

  stages (1..T) :{
    var slack1{BENCHMARK} >=0;
    subject to CashBalance:
      cash + (1+Gama) * sum{j in ASSETS} Price[j] * x_bought[j] = 
        ancestor(1).cash + (1-Gama) * sum{j in ASSETS} Price[j] *
          x_sold[j];
    subject to Inventory{j in ASSETS}:
      x_hold[j] =
        (1+Return[j,nd])*ancestor(1).x_hold[j]+x_bought[j]-x_sold[j];
    subject to StochasticDominance1{l in BENCHMARK}:
      sum{j in ASSETS}Price[j]*x_hold[j]+cash-Vbenchmk[l] = -risk[l] + slack1[l];
    subject to StochasticDominance2{l in BENCHMARK}:
      Exp(risk[l]) = Bench2nd[l] - slack2[l,st];
  }

  stages {T}:{
    var wealth >=0;
    subject to wealthcs:  wealth = sum{j in ASSETS} Price[j]*x_hold[j]+cash;
    maximize objFunc: Exp(wealth);
  }
}
      
