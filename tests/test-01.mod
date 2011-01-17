# This file is (c) 2011 Marco Colombo, University of Edinburgh.
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty.

set PRODUCTS;

var x{PRODUCTS} >= 0;
var slack{PRODUCTS} >= 0;
var slackTotal >= 0;

param maxProduction{PRODUCTS};
param maxTotalProduction;
param profit{PRODUCTS};

subject to maxProd{i in PRODUCTS}:
    x[i] + slack[i] = maxProduction[i];
subject to maxProdTot:
    sum{i in PRODUCTS} x[i] + slackTotal = maxTotalProduction;

maximize totalProfit:
   sum{i in PRODUCTS} profit[i] * x[i];
