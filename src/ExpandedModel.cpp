/* (c) 2008,2009 Jonathan Hogg and Andreas Grothey, University of Edinburgh
 *
 * This file is part of SML.
 *
 * SML is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, using version 3 of the License.
 *
 * SML is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */
#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "ExpandedModel.h"
#include "GlobalVariables.h"
#include "AmplsolverCalls.h"
#include "AmplModel.h"


list<string> ExpandedModel::pathToNodeStack;
const bool log_EM = false;

ExpandedModel::ExpandedModel(AmplModel *src_model) :
   nLocalVars(-1), nLocalCons(-1), localVarInfoSet(false), pvar(NULL), 
   dvar(NULL), prow(NULL), drow(NULL), src(src_model)
{
  // Note: Setup of model is largely done in AmplModel::createExpandedModel()

  //printf("ExpandedModel: Called default constructor\n");
  //exit(1);
}


/* --------------------------------------------------------------------------
ExpandedModel::setLocalVarInfo
-------------------------------------------------------------------------- */
/** This routine identifies the indices of the local variables in the 
 *  *.nl file that is associated with this ExpandedModel node.
 * 
 * The routine compares the variable name stubs stored in localVarDef
 * (which are set up from the constructor using the corresponding flat
 * model (AmplModel) and the instance (defined by appropriate values
 * for the indexing variables), with the names of the variables
 * defined in the *.nl file (obtained by reading the corresponding
 * *.col file).
 *
 * The routine sets the nLocalVar, listLocalVar, nLocalCons fields of
 * the object.
 *
 * @note The method works by comparing all stubs with all names in the
 * *.col file, this uses a lot of string comparisons and is likely
 * very inefficient.
 */
void 
ExpandedModel::setLocalVarInfo()
{

  if (GlobalVariables::prtLvl>=2)
    cout << "setLocalVarInfo(): " << model_file << endl;
  // FIXME: pretty unelegant to have to do n*m comparisons

  list<string> colfilelist;
  list<int> listColIx;

  if (localVarInfoSet) {
    return;
  }else{
    localVarInfoSet=true;
  }

  // ------- read the names of constraints defined in this NlFile ------------
  // These have been written in the correct order
  ifstream fin((model_file+".row").c_str());

  if (!fin) {
    cerr << "Cannot open row name file: "+model_file+".row" << endl;
    exit(1);
  }
  
  list<SymbolTable::Entry> objList = getObjList();
  string model_name = getName();
  if(model_name!="root") model_name = model_name.substr(5); // skip "root_"
  while(!fin.eof()){
    string line;
    getline(fin, line);
    listOfConNames.push_back(line);
    // Trim away common characters between model name and var from varname
    unsigned int j = 0;
    if(getName()!="root" && line!="") {
      for(j=0; j<model_name.size() && j<line.size(); ++j) {
        if(model_name.at(j)!=line.at(j)) break;
      }
      if(j<line.size() && line.at(j)=='_') j++;
    }
    // skip empty line
    if(line.substr(j) == "") continue;
    // skip dummy row
    if(line.substr(j,5) == "dummy") {
       if(line.size()==j+5) continue;
       if(line.at(j+5)=='[') continue;
    }
    // skip any objectives
    bool isObj = false;
    for(list<SymbolTable::Entry>::const_iterator i=objList.begin(); i!=objList.end(); ++i) {
       if(line.substr(j,i->id.size()) == i->id) {
          if(line.size()==j+i->id.size()) {
             isObj=true;
             break;
          }
          if(line.at(j+i->id.size()=='[')) {
             isObj=true;
             break;
          }
       }
    }
    if(isObj) continue;
    listOfLocalConNames.push_back(line.substr(j));
  }

  fin.close();
  fin.clear(); // Recommended by Marco
  
  if (GlobalVariables::prtLvl>=2)
    cout << "Read " << listOfConNames.size() << " lines from file " << 
       model_file << ".row" << endl;

  // Now read vars - these actually need matching to correct order

  fin.open((model_file+".col").c_str());

  if (!fin) {
    cerr << "Cannot open column name file: "+model_file+".col" << endl;
    exit(1);
  }
  
  while(!fin.eof()){
    string line;
    getline(fin, line);
    colfilelist.push_back(line);
  }

  fin.close(); // Added by Marco, probably not strictly required
               // as it should occour in destructor.
  
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

        // Trim away common characters between model name and var from varname
        unsigned int j = 0;
        if(getName()!="root" && *p!="") {
          for(j=0; j<model_name.size() && j<(*p).size(); ++j) {
             if(model_name.at(j)!=(*p).at(j)) break;
          }
          if(j<(*p).size() && (*p).at(j)=='_') j++;
        }
        listOfLocalVarNames.push_back((*q).substr(j));
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
ExpandedModel::getLocalVarNames
-------------------------------------------------------------------------- */
/** Returns the names of variables local to this node.
 *
 *  @return Names of the local variables.
 */
const list<string>&
ExpandedModel::getLocalVarNames() 
{
  if (!localVarInfoSet){
    setLocalVarInfo();
    localVarInfoSet=true;
  }
  return listOfLocalVarNames;
}

/* --------------------------------------------------------------------------
ExpandedModel::getLocalConNames
-------------------------------------------------------------------------- */
/** Returns the names of constraints local to this node.
 *
 *  @return Names of local constraints.
 */
const list<string>&
ExpandedModel::getLocalConNames() 
{
  if (!localVarInfoSet){
    setLocalVarInfo();
    localVarInfoSet=true;
  }
  return listOfLocalConNames;
}

/* --------------------------------------------------------------------------
ExpandedModel::getNLocalVars
-------------------------------------------------------------------------- */
/** Returns the number of variables local to this node
 *
 *  @return Number of local variables.
 */
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
/** Returns the number of constraints local to this node.
 *
 *  @return Number of local constraints.
 */
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
/** Recursively prints out the content of this node plus any of its children.
 *  Used only for debugging.
 */
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
  
  for(unsigned int i=0; i<children.size(); ++i){
    ExpandedModel *em = (ExpandedModel*) children.at(i);
    em->print();
  }
}


/* ----------------------------------------------------------------------------
ExpandedModel::getNzJacobianOfIntersection
---------------------------------------------------------------------------- */
/** Returns the number of nonzeros in the Jacobian of the matrix defined
 *  by the intersection of the local constraints in this model with the
 *  local variables of another (or the same) model.
 *
 *  @param emcol_
 *         The model w.r.t. whose local variables the Jacobian should be
 *         evaluated. This parameter can be NULL, in which case the method
 *         works on the intersection of the local constraints with the local
 *         variables (a "diagonal" block).
 *
 *  @return The number of nonzeros in the given part of the Jacobian.
 */
int 
ExpandedModel::getNzJacobianOfIntersection(ExpandedModelInterface *emcol_)
{
  int nvar;
  int *lvar;
  int nz;
  ExpandedModel *emcol = static_cast< ExpandedModel* > (emcol_);

  if (emcol==NULL) emcol = this;
  
  // need to find the indices of the variables local to emcol in the
  // nlfile given by this model.

  nvar = emcol->getNLocalVars();
  lvar = new int[nvar];
  
  // find the indices of the variable local to emcol in the nlfile belonging
  // to this node.
  nlfile->findIxOfLocalVarsInNlFile(emcol, lvar);

  // and ask AMPL to evaluate the Jacobian structure
  nz = nlfile->getNoNonzerosAMPL(nvar, lvar);

  delete [] lvar;

  return nz;
}

/* ----------------------------------------------------------------------------
ExpandedModel::getJacobianOfIntersection
---------------------------------------------------------------------------- */
/** Returns the Jacobian of the matrix defined by the intersection of
 *  the local constraints in this model with the local variables of
 *  another (or the same) model in sparse matrix format.
 *
 *  @param[in] emcol_
 *             The model w.r.t. whose local variables the Jacobian should be
 *             evaluated. This parameter can be NULL, in which case the method
 *             works on the intersection of the local constraints with the
 *             local variables (a "diagonal" block).
 *  @param[out] colbeg
 *             Column starts of the Jacobian.
 *  @param[out] collen
 *             Column lengths of the Jacobian (not returned if NULL on call).
 *  @param[out] rownbs
 *             Row indices of nonzeros entries.
 *  @param[out] el
 *             Values of the nonzero entries.
 *
 *  @note Parameters colbeg, collen, rownbs, el are assumes to be of
 *  appropriate dimensions before the method is called. Namely
 *  colbeg[n+1], collen[n], rownbs[nz], el[nz] (n=number of columns,
 *  nz=number nonzeros). The number of nonzeros in this section of the
 *  Jacobian can be obtained from a call to
 *  getNzJacobianInintersection.
 */
void 
ExpandedModel::getJacobianOfIntersection(ExpandedModelInterface *emcol_, int *colbeg,
					 int *collen, int *rownbs, double *el)
{
  int nvar;
  int *lvar;
  ExpandedModel *emcol = static_cast< ExpandedModel* > (emcol_);

  if (emcol==NULL) emcol = this;
  
  // need to find the indices of the variables local to emcol in the
  // nlfile given by this model.

  nvar = emcol->getNLocalVars();
  lvar = new int[nvar];
  
  // find the indices of the variable local to emcol in the nlfile belonging
  // to this node.
  nlfile->findIxOfLocalVarsInNlFile(emcol, lvar);

  // and ask AMPL to evaluate the Jacobian structure
  nlfile->fillSparseAMPL(nvar, lvar, colbeg, collen, rownbs, el);

  delete [] lvar;
}

/* -------------------------------------------------------------------------
ExpandedModel::getRowLowBounds
-------------------------------------------------------------------------- */
/** Return the vector of lower bounds for the constraints in this model.
 *
 *  @param[out] elts
 *              The lower bounds on the constraints.
 *
 *  The method is simply a wrapper around NlFile::getRowLowBoundsAMPL.
 */
void
ExpandedModel::getRowLowBounds(double *elts)
{
  nlfile->getRowLowBoundsAMPL(elts);
}

/* -------------------------------------------------------------------------
ExpandedModel::getRowUpBounds
-------------------------------------------------------------------------- */
/** Return the vector of upper bounds for the constraints in this model.
 *
 *  @param[out] elts
 *              The upper bounds on the constraints.
 *
 *  The method is simply a wrapper around NlFile::getRowUpBoundsAMPL.
 */
void
ExpandedModel::getRowUpBounds(double *elts)
{
  nlfile->getRowUpBoundsAMPL(elts);
}

/* -------------------------------------------------------------------------
ExpandedModel::getObjGradient
-------------------------------------------------------------------------- */
/** Return the gradient of the objective defined in this model with respect 
 *  to the local variables.
 * 
 *  @param[out] elts
 *              The objective gradient.
 */
void
ExpandedModel::getObjGradient(double *elts)
{
  int *lvar = new int[nLocalVars];
  nlfile->findIxOfLocalVarsInNlFile(this, lvar);
  //for (int i=0;i<nLocalVars;i++) printf("EM:%d %d\n",i,lvar[i]);

  for(int i=0;i<nLocalVars;i++) elts[i] = 0.;
  nlfile->getObjAMPL(nLocalVars, lvar, elts);
  delete [] lvar;
}

/* -------------------------------------------------------------------------
ExpandedModel::getColLowBounds
-------------------------------------------------------------------------- */
/** Return the lower bounds of the local variables defined in this model.
 * 
 *  @param[out] elts
 *              The variable lower bounds.
 */
void
ExpandedModel::getColLowBounds(double *elts)
{
  int *lvar = new int[nLocalVars];
  nlfile->findIxOfLocalVarsInNlFile(this, lvar);

  for(int i=0;i<nLocalVars;i++) elts[i] = 0.;
  nlfile->getColLowBoundsAMPL(nLocalVars, lvar, elts);
  delete [] lvar;
}

/* -------------------------------------------------------------------------
ExpandedModel::getColUpBounds
-------------------------------------------------------------------------- */
/** Return the upper bounds of the local variables defined in this model.
 *
 *  @param[out] elts
 *              The variable upper bounds.
 */
void
ExpandedModel::getColUpBounds(double *elts)
{
  int *lvar = new int[nLocalVars];
  nlfile->findIxOfLocalVarsInNlFile(this, lvar);

  for(int i=0;i<nLocalVars;i++) elts[i] = 0.;
  nlfile->getColUpBoundsAMPL(nLocalVars, lvar, elts);
  delete [] lvar;
}


/* -------------------------------------------------------------------------
ExpandedModel::findIxOfLocalVarsInNlFile
-------------------------------------------------------------------------- */
/** Find the indices of the local variables of this given model in a given 
 *  nlfile.
 *
 *  @param[in] nlf
 *             The NlFile that defines local constraints and all variables
 *             that are used by these constraints.
 *  @param[out] lvar
 *             Assumed to be allocated with em->nLocalVar elements: lvar[i]
 *             is the the index of the ith local variable in em in the nlfile.
 *
 *  @return The number of matches found.
 *
 * For the full doumentation see NlFile::findIxOfLocalVarsInNlFile.
 * This method belongs logically to the NlFile class, since it calculates
 * (column) sections of the columns defined in the NlFile. However since
 * we cannot use lists in the NlFile class, the actual code is here.
 */
int
ExpandedModel::findIxOfLocalVarsInNlFile(NlFile *nlf, int *lvar)
{
  ExpandedModel *em = this;
  string nlfilename = nlf->nlfilename;
  int nvar = em->getNLocalVars();
  list<string> colfilelist;
  int count = 0; // count number of matches

  // look up if this index set has already been calculated
  if (nlf->indexList.count(em)>0){
    // we have already calculated this list
    IndexListValue *ilv = nlf->indexList[em];
    if(log_EM) cout << "<<<<<<<<<<<<<<< found IndexValue "+nlfilename+":"+em->model_file+"\n";
    assert(nvar==ilv->nvar);
    for (int i=0;i<nvar;i++){
      lvar[i] = ilv->lvar[i];
      if (lvar[i]>=0) count++;
    }
    assert(count==ilv->count);
  }else{
    if(log_EM) cout << "<<<<<<<<<<<<<<< place IndexValue "+nlfilename+":"+em->model_file+"\n";
    // we need to calculate it
    for(int i=0;i<nvar;i++) lvar[i] = -1;
    
    // ------- read the names of columns defined in this NlFile ------------
    ifstream fin((nlfilename+".col").c_str());
    
    if (!fin) {
      cout << "Cannot open column name file: "+nlfilename+".col";
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
	     nlfilename.c_str());
    }
    
    // -------------- compare this listOfVarNames against this list
    int i=0;
    for(list<string>::iterator p=em->listOfVarNames.begin();
	p!=em->listOfVarNames.end();p++){
      // (*p) is a name of a local variable. Should see if we can find this
      // in this NlFile
      int cnt=0;
      for(list<string>::iterator q=colfilelist.begin();q!=colfilelist.end();q++){
	if ((*p)==(*q)){
	  lvar[i] = cnt;
	  count++;  //increase number of matches
	  break;
	}
	cnt++;
      }
      i++;
    }
    //and place a copy on the map
    int *lvarc = new int[nvar];
    for (int i=0;i<nvar;i++) lvarc[i] = lvar[i];
    nlf->indexList[em] = new IndexListValue(nvar,lvarc,count);
  }

  return count;
}

/* -------------------------------------------------------------------------
ExpandedModel::setPrimalSolColumns
-------------------------------------------------------------------------- */
/** Upload the local variable solutions */
void ExpandedModel::setPrimalSolColumns(double *elts) {
   if (!localVarInfoSet){
      setLocalVarInfo();
      localVarInfoSet=true;
   }
   if(!pvar) {
      pvar = (double*) calloc(nLocalVars, sizeof(double));
   }
   for(double *src=elts,*dest=pvar; src<elts+nLocalVars; ++src,++dest)
      *dest = *src;
}

/* -------------------------------------------------------------------------
ExpandedModel::setDualSolColumns
-------------------------------------------------------------------------- */
/** Upload the local variable duals (multipliers on bounds) */
void ExpandedModel::setDualSolColumns(double *elts) {
   if (!localVarInfoSet){
      setLocalVarInfo();
      localVarInfoSet=true;
   }
   if(!dvar) {
      dvar = (double*) calloc(nLocalVars, sizeof(double));
   }
   for(double *src=elts,*dest=dvar; src<elts+nLocalVars; ++src,++dest)
      *dest = *src;
}

/* -------------------------------------------------------------------------
ExpandedModel::setPrimalSolRows
-------------------------------------------------------------------------- */
/** Upload the local constraints slacks */
void ExpandedModel::setPrimalSolRows(double *elts) {
   if (!localVarInfoSet){
      setLocalVarInfo();
      localVarInfoSet=true;
   }
   if(!prow) {
      prow = (double*) calloc(nLocalCons, sizeof(double));
   }
   for(double *src=elts,*dest=prow; src<elts+nLocalCons; ++src,++dest)
      *dest = *src;
}

/* -------------------------------------------------------------------------
ExpandedModel::setDualSolRows
-------------------------------------------------------------------------- */
/** Upload the local constraints duals (multipliers on constraints) */
void ExpandedModel::setDualSolRows(double *elts) {
   if (!localVarInfoSet){
      setLocalVarInfo();
      localVarInfoSet=true;
   }
   if(!drow) {
      drow = (double*) calloc(nLocalCons, sizeof(double));
   }
   for(double *src=elts,*dest=drow; src<elts+nLocalCons; ++src,++dest)
      *dest = *src;
}

/* -------------------------------------------------------------------------
ExpandedModel::outputSolution
-------------------------------------------------------------------------- */
/** Outputs the solution to the supplied stream */
void ExpandedModel::outputSolution(ostream &out, int indent) {
   if (!localVarInfoSet){
      setLocalVarInfo();
      localVarInfoSet=true;
   }

   string ind = "";
   for(int i=0; i<indent; ++i) ind += " ";
   string ind2 = ind + "  ";

   if(getName()!="root") {
      string name = getName();
      string pname = parent->getName();
      out << "\n" << ind << name.substr(pname.size()+1) << " {" << endl;
   } else {
      ind2 = ind;
   }

   if(pvar) {
      double *pptr = pvar;
      double *dptr = dvar;
      for(list<string>::const_iterator i=getLocalVarNames().begin(); 
          i!=getLocalVarNames().end(); ++i) {
        out << ind2 << left << setw(20) << *i
            << " Value = " << setw(15) << *(pptr++);
        if(dvar) out << " Reduced cost = " << *(dptr++);
        out << endl;
      }
      out << endl;
   }

   if(prow) {
      double *pptr = prow;
      double *dptr = drow;
      for(list<string>::const_iterator i=getLocalConNames().begin(); 
          i!=getLocalConNames().end(); ++i) {
        out << ind2 << left << setw(20) << *i
            << " Slack = " << setw(15) << *(pptr++);
        if(drow) out << " Dual = " << *(dptr++);
        out << endl;
      }
   }

   for(vector<ExpandedModelInterface*>::iterator i=children.begin(); 
         i<children.end(); ++i)
      (*i)->outputSolution(out, indent+(getName()=="root"?0:2));

   if(getName()!="root") out << ind << "}" << endl;
}

list<SymbolTable::Entry> ExpandedModel::getObjList() const {
   return src->getObjList();
}
