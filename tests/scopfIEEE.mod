set     Buses ;
set     ExistingGeneratorName ;
set     LineName ;
set     Contingencies within LineName;

param   MaxFlow0 {LineName} >= 0;
param   Reactance0 {LineName} >= 0;
param   StartBus {LineName} symbolic; 
param   EndBus {LineName} symbolic; 

param   V0 = 1;

param   Location {ExistingGeneratorName} symbolic;   
param   MinGen {ExistingGeneratorName} >= 0;
param   MaxGen {g in ExistingGeneratorName} >= MinGen[g];
param   Cost {ExistingGeneratorName} >=0;

param   Demand {Buses} >= 0;
param   abl {b in Buses, l in LineName} := (if b = StartBus[l] then -1 else (if b = EndBus[l] then 1 else 0));
#param   abl {b in Buses, l in LineName}; 

var power{g in ExistingGeneratorName} >= MinGen[g], <= MaxGen[g];
var extrag{ Buses } >=-100000, <=100000;

#set ESet:= aabb;
#block trysml(k in {Contingencies union ESet}) :{

block trysml{hk in Contingencies}:

    set linediff := LineName diff {hk};
    
    var flow{l in linediff} >=-MaxFlow0[l], <=MaxFlow0[l];
    var delta{Buses} >=-3.1415, <=3.1415;
    
    subject to KirVol{l in linediff}:
        flow[l] = -V0^2/Reactance0[l]* sum{b in Buses} abl[b,l]*delta[b];
    
    subject to sumdelta:
        sum{b in Buses} delta[b] = 1;
    
    subject to KCL{b in Buses}:
      ( sum{g in ExistingGeneratorName: Location[g] == b} (power[g])) + ( sum{l in linediff} abl[b,l]*flow[l] ) + extrag[b] = Demand[b] ;  

end block;


subject to gb{b in Buses}:
    extrag[b] = 0;

minimize Total_Cost:  sum {g in ExistingGeneratorName} Cost[g]*power[g];

