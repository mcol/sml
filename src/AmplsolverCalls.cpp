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
  
  printf("Opening %s\n",nlfilename.c_str());
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

  printf("Problem dimensions are %d x %d\n",n_var, n_con);
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
  
  printf("Opening %s\n",nlfilename.c_str());
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
  
  printf("Opening %s\n",nlfilename.c_str());
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
  
  printf("Opening %s\n",nlfilename.c_str());
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

/* ===========================================================================
Stuff that is not currently a class method
============================================================================ */



/* ----------------------------------------------------------------------------
getNoNonzerosAMPL
---------------------------------------------------------------------------- */
int 
getNoNonzerosAMPL(string nlfilename, int nvar, int *lvar)
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
  
  printf("Opening %s\n",nlfilename.c_str());
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
  return tt_nz;

}

/* ----------------------------------------------------------------------------
fillSparseAMPL
---------------------------------------------------------------------------- */
void
fillSparseAMPL(string nlfilename, int nvar, int *lvar, 
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
  
  printf("Opening %s\n",nlfilename.c_str());
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
getRhsAMPL
---------------------------------------------------------------------------- */
void 
getRhsAMPL(string nlfilename, int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("Opening %s\n",nlfilename.c_str());
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
    if (fabs(LUrhs[2*i]-LUrhs[2*i+1])>1e-6){
      printf("At the moment only support equality constraints!\n");
      printf("Bounds for c/s %d in %s: %f %f\n",i, nlfilename.c_str(),
	     LUrhs[2*i], LUrhs[2*i+1]);
      exit(1);
    }
    elts[i] = LUrhs[2*i];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}

/* ----------------------------------------------------------------------------
getObjAMPL
---------------------------------------------------------------------------- */
void 
getObjAMPL(string nlfilename, int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("Opening %s\n",nlfilename.c_str());
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
  
  real *c = (real *)Malloc(n_var*sizeof(real));

  for(int i=0;i<n_var;i++) c[i] = 0;
  for (ograd *og = Ograd[0];og;og=og->next)
    c[og->varno] = og->coef;
  for(int i=0;i<nvar;i++){
    int ix = lvar[i];
    elts[i] = c[ix];
  }

  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}
/* ----------------------------------------------------------------------------
getBndAMPL
---------------------------------------------------------------------------- */
void 
getBndAMPL(string nlfilename, int nvar, int *lvar, double *elts)
{
  ASL_pfgh *asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
  //asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);
  
  printf("Opening %s\n",nlfilename.c_str());
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
    if (fabs(LUv[2*ix])>1e-6){
      printf("At the moment only support lower variable bound of 0!\n");
      printf("Bounds for var %d in %s: %f %f\n",ix, nlfilename.c_str(),
	     LUv[2*ix], LUv[2*ix+1]);
      exit(1);
    }
    elts[i] = LUv[2*ix+1];
  }
  
  ASL_free((ASL**)&asl); // FIXME: does this really free *all* the memory?

}
