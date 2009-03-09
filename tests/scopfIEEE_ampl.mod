set     Buses ;
set     ExistingGeneratorName ;
set     LineName ;
set     Contingencies within LineName;

param   MaxFlow0 {LineName} >= 0;
param   Reactance0 {LineName} >= 0;
param   StartBus {LineName} symbolic; 
param   EndBus {LineName} symbolic; 

param   V0 = 1;

param   Demand {Buses} >= 0;
param   abl {b in Buses, l in LineName} := (if b = StartBus[l] then -1 else (if b = EndBus[l] then 1 else 0));

param   Location {ExistingGeneratorName} symbolic;   
param   MinGen {ExistingGeneratorName} >= 0;
param   MaxGen {g in ExistingGeneratorName} >= MinGen[g];
param   Cost {ExistingGeneratorName} >=0;


var power{g in ExistingGeneratorName} >= MinGen[g], <= MaxGen[g] ;
var flow{ LineName } >=-100000, <=100000;
var flow2{ Contingencies,LineName } >=-100000, <=100000;
var delta{ Buses } >=-100000, <=100000;
var delta2{ Contingencies,Buses } >=-100000, <=100000;
var extrag{ Buses } >=-100000, <=100000;

minimize Total_Cost:  sum {g in ExistingGeneratorName} Cost[g]*power[g];


subject to FlowCon{l in LineName}:
    -MaxFlow0 [l] <=  flow[l] <= MaxFlow0 [l];

subject to FlowCon2{c in Contingencies, l in LineName}:
    -MaxFlow0 [l] <=  flow2[c, l] <= MaxFlow0 [l];
    
subject to KirVol{l in LineName}:
    flow[l] = -V0^2/Reactance0[l]* sum{b in Buses} abl[b,l]*delta[b];    

subject to KirVol2{c in Contingencies, l in LineName}:
    flow2[c,l] = (if l=c then 0 else -V0^2/Reactance0[l]* sum{b in Buses} abl[b,l]*delta2[c,b]);
  
subject to sumdelta:
    sum{b in Buses} delta[b] = 1;

subject to sumdelta2{c in Contingencies}:
    sum{b in Buses} delta2[c,b] = 1;

subject to KCL{b in Buses}:
    ( sum{g in ExistingGeneratorName} (if Location [g] = b then 1 else 0)*(power[g])) + ( sum{l in LineName} abl[b,l]*flow[l] ) + extrag[b] = Demand[b] ;  

subject to KCL2{c in Contingencies,b in Buses}:
    ( sum{g in ExistingGeneratorName} (if Location [g] = b then 1 else 0)*(power[g])) + ( sum{l in LineName} abl[b,l]*flow2[c,l] ) + extrag[b] = Demand[b] ;  

subject to gb{b in Buses}:
    extrag[b] = 0;
