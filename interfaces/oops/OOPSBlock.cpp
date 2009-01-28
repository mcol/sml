#include "OOPSBlock.h"
#include "GlobalVariables.h"
#include <iostream>
#include <fstream>

/* ----------------------------------------------------------------------------
OOPSBlock::OOPSBlock(ExpandedModel*, list<string>*)
---------------------------------------------------------------------------- */
OOPSBlock::OOPSBlock(ExpandedModel *rowmod, ExpandedModel *colmod)
{
  /* We need to:
      - take the list of variable names from colmod (colmod->listOfVarNames)
      - compare them against the variables defined by the NlFile attached
        to rowmod (rowmod->model_file+".col")
      - colmod->listOfVarNames will give the number of columns in this block
      - need a list of indices into the NlFile for these columns
   */
  


  list<string> colfilelist;
  list<int> listColIx;

  if (GlobalVariables::prtLvl>=2){
    printf("-------------------------OOPS Block---------------------------\n");
    printf("Generate OOPSBlock: col: %s/ row: %s\n",
           colmod->model_file.c_str(), rowmod->model_file.c_str());
  }

  this->em = rowmod;
  this->nlfile = rowmod->nlfile;

  this->nvar = colmod->getNLocalVars();
  this->ncon = rowmod->getNLocalCons();
  this->lvar = (int*)malloc(nvar*sizeof(int));
  
  for(int i=0;i<nvar;i++) lvar[i] = -1;

  // ------- read the names of columns defined in this NlFile ------------
  // FIXME: Should these be remembered in the NlFile rather than needing to be
  //        read in?
  ifstream fin((rowmod->model_file+".col").c_str());

  if (!fin) {
    cout << "Cannot open column name file: "+rowmod->model_file+".col";
    exit(1);
  }
  
  string line;
  getline(fin, line);
  while(!fin.eof()){
    colfilelist.push_back(line);
    getline(fin, line);
  }
  
  if (GlobalVariables::prtLvl>=2){
    printf("Read %d lines from file %s.col\n",colfilelist.size(),
           rowmod->model_file.c_str());
  }

  // -------------- compare this listOfVarNames against this list
  int i=0;
  for(list<string>::iterator p=colmod->listOfVarNames.begin();
      p!=colmod->listOfVarNames.end();p++){
    // (*p) is a name of a local variable. Should see if we can find this
    // in this NlFile
    int cnt=0;
    for(list<string>::iterator q=colfilelist.begin();q!=colfilelist.end();q++){
      if ((*p)==(*q)){
        lvar[i] = cnt;
        break;
      }
      cnt++;
    }
    i++;
  }

  //printf("Row model: %s\n",rowmod->model_file.c_str());
  //printf("Col model: %s\n",colmod->model_file.c_str());
  //printf("Nb_row = %d\n",ncon);

  if (GlobalVariables::prtLvl>=3){
    printf("The NlFile declares these variables:\n"); 
    int cnt=0;
    for(list<string>::iterator p=colfilelist.begin();p!=colfilelist.end();p++){
      printf("%2d: %s\n",cnt, (*p).c_str());
      cnt++;
    }

    printf("The column block defines %d variables:\n",nvar);
    cnt =0;
    for(list<string>::iterator p=colmod->listOfVarNames.begin();
        p!=colmod->listOfVarNames.end();p++){
      printf(" at %2d:  %s\n",lvar[cnt], (*p).c_str());
      cnt++;
    }
  }
 
  if (GlobalVariables::prtLvl>=1){
    printf("OOPS Block: %s(rw)/%s(cl): %dx%d\n",
           rowmod->model_file.c_str(), colmod->model_file.c_str(),
           ncon, nvar);
  }
  
}
