#include "ExpandedModel.h"
#include "GlobalVariables.h"
#include "AmplsolverCalls.h"
#include <list>
#include <string>
#include <iostream>
#include <fstream>


list<string> ExpandedModel::pathToNodeStack;

/* --------------------------------------------------------------------------
ExpandedModel(ampl-model *model)
-------------------------------------------------------------------------- */
/* This is the constructor that takes a FLAT AmplModel as argument
   and creates an Expanded version of it */

ExpandedModel::ExpandedModel(AmplModel *root){
  /* this needs to:
     (1) copy across information on local variables/constraints
     (2) loop over all submodel declarations
         (2a) work out their cardinality
         (2b) and add them all (recursively) to this model-object
  */
  
  ExpandedModel* em = root->createExpandedModel(string("root"),string(""));

  model_file = em->model_file;
  nlfile = em->nlfile;
  localVarDef = em->localVarDef;
  //nchd = em->nchd;
  children = em->children;
  localVarInfoSet = false;

}

ExpandedModel::ExpandedModel()
{
  nLocalVars = -1;
  nLocalCons = -1;
  localVarInfoSet = false;
  //printf("ExpandedModel: Called default constructor\n");
  //exit(1);
}


/* --------------------------------------------------------------------------
ExpandedModel::setLocalVarInfo
-------------------------------------------------------------------------- */
/* This routine compares the variable list given in localVarDef with
   those listed in the corresponding *.col file and sets the 
   nLocalVar, listLocalVar, nLocalCons entries                             */

void 
ExpandedModel::setLocalVarInfo()
{

  if (GlobalVariables::prtLvl>=2)
    printf("setLocalVarInfo(): %s\n",model_file.c_str());
  // FIXME: pretty unelegant to have to do n*m comparisons

  list<string> colfilelist;
  list<int> listColIx;

  if (localVarInfoSet) {
    return;
  }else{
    localVarInfoSet=true;
  }

  // ------- read the names of columns defined in this NlFile ------------
  ifstream fin((model_file+".col").c_str());

  if (!fin) {
    cout << "Cannot open column name file: "+model_file+".col";
    exit(1);
  }
  
  while(!fin.eof()){
    string line;
    getline(fin, line);
    colfilelist.push_back(line);
  }
  
  if (GlobalVariables::prtLvl>=2)
    cout << "Read " << colfilelist.size() << " lines from file " << 
       model_file << ".col\n";

  // -------------- compare this list against the given VarDefs
  
  
  for(list<string>::iterator p=localVarDef.begin();p!=localVarDef.end();p++){
    // for all variable declarations in ExpandedModel

    int len = (*p).size();

    int cnt;
    list<string>::iterator q;
    for(q=colfilelist.begin(),cnt=0; q!=colfilelist.end(); ++q,++cnt){
      string cand(*q, 0, len);
      if (cand==(*p)) {
        if (GlobalVariables::prtLvl>=3)
          cout << "Match of " << cand << " and " << *p << "\n";
        listColIx.push_back(cnt);
        listOfVarNames.push_back(*q);
      }
      
    }
    
  }
  
  // FIXME: cannot sort and uniquify just the numerical list, since these
  //        actions would need to be mirrored on the name list
  //  listColIx.sort();
  //  listColIx.unique();
  
  //printf("Found %d variables\n",listColIx.size());
  
  nLocalVars = listColIx.size();
  listOfVars = (int*)calloc(nLocalVars, sizeof(int));
  int cnt;
  list<int>::iterator p;
  for(p=listColIx.begin(),cnt=0; p!=listColIx.end(); ++p,++cnt){
    listOfVars[cnt] = (*p);
  }
  
  // open *.nl file to get number of constraints
  
  nLocalCons = nlfile->getNoConstraints();
  //printf("Found %d constraints\n",nLocalCons);

  if (GlobalVariables::prtLvl>=1)
    cout << "setLocalVarInfo(): " << model_file << " (" << nLocalCons <<
      "x" << nLocalVars << ")\n";

}

/* --------------------------------------------------------------------------
ExpandedModel::getNLocalVars
-------------------------------------------------------------------------- */
int
ExpandedModel::getNLocalVars()
{
  if (!localVarInfoSet){
    setLocalVarInfo();
    localVarInfoSet=true;
  }
  return nLocalVars;
}

/* --------------------------------------------------------------------------
ExpandedModel::getNLocalCons
-------------------------------------------------------------------------- */
int
ExpandedModel::getNLocalCons()
{
  if (!localVarInfoSet){
    setLocalVarInfo();
    localVarInfoSet=true;
  }
  return nLocalCons;
}

/* ----------------------------------------------------------------------------
ExpandedModel::print()
---------------------------------------------------------------------------- */
void
ExpandedModel::print()
{
  cout << "EM: ------------------------------------------------------------\n";
  if (nlfile==NULL){
    cerr << "EM: No NlFile attached\n";
    exit(1);
  }
  cout << "EM: This is ExpandedModel: " <<  nlfile->nlfilename << "\n";
  cout << "EM: Nb children: " << children.size() << "\n";
  cout << "EM: Nb local variable definitions: " << localVarDef.size() << "\n";
  for(list<string>::iterator p=localVarDef.begin();p!=localVarDef.end();p++){
    cout << "EM:   " << *p << "\n";
  }
  if (!localVarInfoSet){
    cout << "EM: Further information on local variables not set\n";
  }else{
    cout << "EM: Nb local Variables: " << nLocalVars << "\n";
    for(int i=0;i<nLocalVars;i++) cout << listOfVars[i] << " ";
    cout << "\n";
    cout << "EM: Nb local Constraints: " <<  nLocalCons << "\n";
  }

  if (children.size()>0)
    cout << "EM: now list the children:\n";
  
  for(int i=0;i<children.size();i++){
    ExpandedModel *em = children.at(i);
    em->print();
  }

}
