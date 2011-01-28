# This file is (c) 2011 Marco Colombo, University of Edinburgh.
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty.

#
# Newsvendor model written in SML (different formulation)
#

set TIME ordered := 1..2;
set NODES;
param Parent{NODES} symbolic;
param Probs{NODES};

param SellingPrice;
param InitBuyCost;
param ExtraBuyCost;
param Demand{NODES};
param MaxInitBuy;

var initBuy >= 0;

maximize Objective:
  -initBuy * InitBuyCost;

block sml stochastic using (nd in NODES, Parent, Probs, TIME): {

  stages {1}: {

    var slack >= 0;

    subject to InitialBuyingLimit:
      initBuy + slack = MaxInitBuy;
  }

  stages {2}: {

    var extraBuy >= 0;
    var unsold >= 0;

    subject to DemandSatisfaction:
      initBuy + extraBuy = Demand[nd] + unsold;

    maximize Objective:
      Demand[nd] * SellingPrice - extraBuy * ExtraBuyCost;
  }
}
