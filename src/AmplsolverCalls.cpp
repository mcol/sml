/* the asl_pfgh.h file in amplsolver globally redefines list
   => this is a separate file that provides all the calls to the
      amplsolver library (and that cannot use c++ lists

*/

#include <string>
#include "AmplsolverCalls.h"
#include "asl_pfgh.h"
#define asl cur_ASL

using namespace std;


/* ===========================================================================
constructors
============================================================================ */
NlFile::NlFile(string nlfilename)
{
  this->nlfilename = nlfilename;
  ncol = -1;
  nrow = -1;
  nzH = -1;
  nzA = -1;
}
/* ===========================================================================
methods
============================================================================ */
void 
NlFile::readCommonScalarValues()
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::readScalar: (%-30s): ",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  ncol = n_var;
  nrow = n_con;
  nzA = nzc;

  printf("(%dx%d): %d nz\n",n_con, n_var,nzc);
  ASL_free((ASL**)&asl);

}

/* ----------------------------------------------------------------------------
NlFile::getNoConstraints
---------------------------------------------------------------------------- */
int
NlFile::getNoConstraints(){
  if (nrow>=0) return nrow;
  
  readCommonScalarValues();
  return nrow;
}

/* ----------------------------------------------------------------------------
NlFile::getNoVariabless
---------------------------------------------------------------------------- */
int
NlFile::getNoVariables(){
  if (ncol>=0) return ncol;
  
  readCommonScalarValues();
  return ncol;
}

/* ----------------------------------------------------------------------------
NlFile::getNoHessianEntries
---------------------------------------------------------------------------- */
int
NlFile::getNoHessianEntries()
{
  if (nzH>=0) return nzH;

  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getNzHess:  (%-30s): ",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  nzH = sphsetup(/* nobj=*/0, /*ow =*/1, /*y=*/0, /*uptri=*/0); 

  ASL_free((ASL**)&asl);
  printf("%d\n",nzH);
  return nzH;
}
/* ----------------------------------------------------------------------------
NlFile::getHessianStructure
---------------------------------------------------------------------------- */
void
NlFile::getHessianStructure(int *colbegH, int *rownbsH)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getHessStr: (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  nzH = sphsetup(/* nobj=*/0, /*ow =*/1, /*y=*/0, /*uptri=*/0); 
  // this should have setup the sputinfo fields

  for(int i=0;i<=ncol;i++) colbegH[i] = sputinfo->hcolstarts[i];
  for(int i=0;i<nzH;i++) rownbsH[i] = sputinfo->hrownos[i];
  // FIXME: need to copy these since sputinfo will be deallocated by the
  //        call to ASL_free. If we have te asl pointer part of the
  //        class, then we can just keep this information in memory
  //        and pass a pointer back to the caller.

  ASL_free((ASL**)&asl);
}
/* ----------------------------------------------------------------------------
NlFile::getHessianEntries
---------------------------------------------------------------------------- */
void
NlFile::getHessianEntries(int *colbegH, int *rownbsH, double *eltsH)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getHessian: (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  nzH = sphsetup(/* nobj=*/0, /*ow =*/1, /*y=*/0, /*uptri=*/0); 
  // this should have setup the sputinfo fields
  sphes(eltsH, /*nobj=*/0, NULL, NULL); 

  for(int i=0;i<=ncol;i++) colbegH[i] = sputinfo->hcolstarts[i];
  for(int i=0;i<nzH;i++) rownbsH[i] = sputinfo->hrownos[i];

  // FIXME: need to copy these since sputinfo will be deallocated by the
  //        call to ASL_free. If we have te asl pointer part of the
  //        class, then we can just keep this information in memory
  //        and pass a pointer back to the caller.

  ASL_free((ASL**)&asl);
}



/* ----------------------------------------------------------------------------
getNoNonzerosAMPL
---------------------------------------------------------------------------- */
/** Returns the number of nonzeros for a (vertical) slice of the
 *  constraint matrix (Jacobian) defined in this file.
 *
 *  @param nvar Number of variables (columns) in the slice
 *  @param lvar The indices of the variables in the slice
 *  @return Number of nonzeros in the slice of the jacobian
 *
 *  For the part of the problem defined by the intersection of all the
 *  constraints in the *.nl file and the variables given by nvar, lvar
 *  this routine will return the nonzeros in the Jacobian.
 */
int 
NlFile::getNoNonzerosAMPL(int nvar, int *lvar)
{
  /* FIXME: the convenient column wise data access is only available for
            the LP reader ASL_read_f. For ASL_read_pfgh this results in 
	    a segmentation fault due to Cgrad not being allocated.
	    (Cgrad is not allocated when A_vals is set). 
	    f_read might fall over for a QP problem? In that case we
	    need to rewrite this using the cgrad structures               */

  //ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  ASL *asl = ASL_alloc(ASL_read_f);
  int tt_nz;
  
  printf("NlFile::getNoNz   : (%-30s): ",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  A_vals = (real *)Malloc(nzc*sizeof(real)); // to say we want columnwise
                                             // representation
  //int err = pfgh_read(nl, ASL_return_read_err);
  int err = f_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  tt_nz = 0;
  for(int i=0;i<nvar;i++){
    int col = lvar[i];
    // col==-1 indicates that this column is not present in the AMPL file
    if (col>=0)
      tt_nz += A_colstarts[col+1]-A_colstarts[col];
  }

  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?
  if (tt_nz<0){
    printf("getNoNozerosAMPL returns tt_nz = %d\n",tt_nz);
    exit(1);
  }
  printf("%d\n",tt_nz);
  
  return tt_nz;

}

/* ----------------------------------------------------------------------------
fillSparseAMPL
---------------------------------------------------------------------------- */
/** Returns a (vertical) slice of the constraint matrix (Jacobian)
 *  defined in this file in (columnwise) sparse matrix format (by
 *  filling in the memory locations provided).
 *
 *  @param[in] nvar Number of variables (columns) in the slice
 *  @param[in] lvar The indices of the variables in the slice
 *  @param[out] colbeg Pointer to column starts in rownbs, el
 *  @param[out] collen Vector of column lengths
 *  @param[out] rownbs Row indices for the sparse elements
 *  @param[out] el The actual nonzero elements.
 *  
 *  For the part of the problem defined by the intersection of all the
 *  constraints in the *.nl file and the variables given by nvar, lvar
 *  this routine will return the Jacobian in (columnwise) sparse matrix format.
 */
void
NlFile::fillSparseAMPL(int nvar, int *lvar, 
		  int *colbeg, int *collen, int *rownbs, double *el)
{
  /* FIXME: the convenient column wise data access is only available for
            the LP reader ASL_read_f. For ASL_read_pfgh this results in 
	    a segmentation fault due to Cgrad not being allocated.
	    (Cgrad is not allocated when A_vals is set). 
	    f_read might fall over for a QP problem? In that case we
	    need to rewrite this using the cgrad structures               */

  //ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  ASL *asl = ASL_alloc(ASL_read_f);
  int tt_nz;
  
  printf("NlFile::fillSparse: (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  A_vals = (real *)Malloc(nzc*sizeof(real)); // to say we want columnwise
                                             // representation
  int err = f_read(nl, ASL_return_read_err);
  //int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  tt_nz = 0;
  for(int i=0;i<nvar;i++){
    int col = lvar[i];
    if (col==-1){
      // this column is not present in the AMPL file => empty column
      colbeg[i] = tt_nz;
      collen[i] = 0;
    }else{
      colbeg[i] = tt_nz;
      collen[i] = A_colstarts[col+1]-A_colstarts[col];
      for(int j=A_colstarts[col];j<A_colstarts[col+1];j++){
	rownbs[tt_nz] = A_rownos[j];
	el[tt_nz] = (double)A_vals[j];
	tt_nz++;
      }      
    }
  }
  colbeg[nvar] = tt_nz;

}


/* ----------------------------------------------------------------------------
getRowLowBoundsAMPL
---------------------------------------------------------------------------- */
/** Returns the constraint (row) lower bounds for the constraints
 *  defined in this *.nl file.
 *
 *  @param[out] elts The row lower limits as a dense vector.
 *
 */

void 
NlFile::getRowLowBoundsAMPL(double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getRhs    : (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  for(int i=0;i<n_con; i++){
    elts[i] = LUrhs[2*i];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}

/* ----------------------------------------------------------------------------
getRowUpBoundsAMPL
---------------------------------------------------------------------------- */
/** Returns the constraint (row) upper bounds for the constraints
 *  defined in this *.nl file.
 *
 *  @param[out] elts The row upper limits as a dense vector.
 *
 */

void 
NlFile::getRowUpBoundsAMPL(double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getRhs    : (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  for(int i=0;i<n_con; i++){
    elts[i] = LUrhs[2*i+1];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}

/* ----------------------------------------------------------------------------
getObjAMPL
---------------------------------------------------------------------------- */
/** Evaluates the objective gradient (linear coefficients) for a
 *  (vertical) slice of the problem stored in the *.nl file.
 *
 *  @param[in] nvar Number of variables defining the slice
 *  @param[in] lvar The indices of the variables defining the slice
 *  @param[out] elts The objective gradient vector w.r.t. the variables 
 *  defined in nvar/lvar
 *
 *  @bug This only works for linear objective functions: 
 *     No vector x at which the objective should be evaluated is passed in
 *
 *
 *  @attention This routine evaluates the second last objective
 *  function defined in the *.nl file. Standard AMPL behaviour is to
 *  evaluate the last defined objective function (unless otherwise
 *  specified). Since for the SML generated *.nl file the final
 *  objective function is the dummy objective, this routine will by
 *  default evaluate the second last objective.
 *
 *  @note SML in principle supports the definition of several
 *  objective functions. It is unclear how the user would choose
 *  them. I assume by passing some option into the 
 *  "ExpandedModel *generateSML(....)" function
 */
void 
NlFile::getObjAMPL(int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getObj    : (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  
  if (n_obj>1){
    real *c = (real *)Malloc(n_var*sizeof(real));
    real objsign = objtype[n_obj-2] ? -1. : 1; //set to -1 for maximise
    
    for(int i=0;i<n_var;i++) c[i] = 0;
    for (ograd *og = Ograd[n_obj-2];og;og=og->next)
      c[og->varno] = objsign*og->coef;
    for(int i=0;i<nvar;i++){
      int ix = lvar[i];
      elts[i] = c[ix];
    }
  }

  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}
/* ----------------------------------------------------------------------------
getColUpBoundsAMPL
---------------------------------------------------------------------------- */
/** Returns upper variable (column) bounds for a selection of the
 *  variables in the *.nl file.
 *
 *  @param[in] nvar Number of variables defining the slice
 *  @param[in] lvar The indices of the variables defining the slice
 *  @param[out] elts The upper bounds for the defined variables. 
 *
 */
 
void 
NlFile::getColUpBoundsAMPL(int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getBnd    : (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  for(int i=0;i<nvar; i++){
    int ix = lvar[i];
    elts[i] = LUv[2*ix+1];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}


/* ----------------------------------------------------------------------------
getColLowBoundsAMPL
---------------------------------------------------------------------------- */
/** Returns lower variable (column) bounds for a selection of the
 *  variables in the *.nl file.
 *
 *  @param[in] nvar Number of variables defining the slice
 *  @param[in] lvar The indices of the variables defining the slice
 *  @param[out] elts The lower bounds for the defined variables. 
 *
 */
 
void 
NlFile::getColLowBoundsAMPL(int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("NlFile::getBnd    : (%s)\n",nlfilename.c_str());
  FILE *nl = jac0dim(const_cast<char*>(nlfilename.c_str()), nlfilename.size());
  if (nl==NULL){
    printf("File not found %s\n",nlfilename.c_str());
    exit(1);
  }
  
  int err = pfgh_read(nl, ASL_return_read_err);
  if (err!=0){
    printf("pfgh_read returns err=%d\n",err);
    exit(1);
  }
  
  for(int i=0;i<nvar; i++){
    int ix = lvar[i];
    elts[i] = LUv[2*ix];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}
