/* This is the OOPS driver for the Structured Modelling Language (SML) */

#include <cassert>
#include "ExpandedModel.h"
#include "AmplsolverCalls.h"
#include "OOPSBlock.h"
//#include "asl_pfgh.h"
extern "C" {
#include "oops/oops.h"
#include "oops/OopsInterface.h"
//#include "oops/MatrixSparseSimple.h"
#include "oops/CallBack.h"
}
//#define asl cur_ASL

/** Wrapper round the Algebras A and Q. Return value for generateSML() */
typedef struct SMLReturn_st {
  Algebra *A;
  Algebra *Q;
} SMLReturn;


/* This NodeId is replaced by OOPSBlock which is a proper class carrying the 
   full information about a node in the OOPS Algebra Tree */

/** This is the identifier needed to fill in a block of the constraint 
 *  matrix */
//typedef struct NodeId_st {
//  const char *amplfile;
//  int nvar;
//  int *lvar;
//} NodeId;

/** This is the identifier needed to fill in a block of the Hessian matrix */
typedef struct NodeIdQ_st {
  NlFile *nlfile;  //< the *.nl-file that carries the information about this
  int nrowvar;        //< number of rows that this node has
  int *lrowvar;       //< list of row indices
  int ncolvar;        //< number of columns that this node has
  int *lcolvar;       //< number of column indices
} NodeIdQ;

SMLReturn *generateSML(ExpandedModel *root);
void FillRhsVector(Vector *vb);
void FillObjVector(Vector *vb);
void FillUpBndVector(Vector *vb);

FILE *globlog = NULL;

void
SML_OOPS_driver(ExpandedModel *root)
{
  Algebra *AlgAug;
  Vector *vb, *vc, *vu;


  PARALLEL_CODE(
                int    InitPar  = InitLippPar(argc, argv);
		)
  printout = stdout;
  
  SMLReturn *Pb = generateSML(root);
  hopdm_opt_type *Opt = NewHopdmOptions();
  
  AlgAug = OOPSSetup(Pb->A, Pb->Q);


  
  // FIXME: should the stuff below be included in OOPSSetup? 
  //        that would require OOPSSetup to return vectors as well

  vb = NewVector(Pb->A->Trow, "vb");
  vc = NewVector(Pb->A->Tcol, "vc");
  vu = NewVector(Pb->A->Tcol, "vu");

  VectorFillCallBack(vc, FillObjVector);
  VectorFillCallBack(vb, FillRhsVector);
  VectorFillCallBack(vu, FillUpBndVector);

  {
    FILE *mout = fopen("mat.m","w");
    PrintMatrixMatlab(mout, Pb->A, "A");
    PrintMatrixMatlab(mout, Pb->Q, "Q");
    PrintVectorMatlab(vb, mout, "b");
    PrintVectorMatlab(vc, mout, "c");
    fclose(mout);
  }

  Vector *vx, *vy, *vz;
  primal_dual_pb *Prob;
  hopdm_prt_type *Prt = NewHopdmPrt(3);

  vx = NewVector(Pb->A->Tcol, "vx");
  vy = NewVector(Pb->A->Trow, "vy");
  vz = NewVector(Pb->A->Tcol, "vz");
  Prob = NewPDProblem(AlgAug, vb, vc, vu, vx, vy, vz);
  hopdm (stdout, Prob, Opt, Prt);
  
}

/* ==========================================================================
Here comes the generation with all subroutines
=========================================================================== */

Algebra *createA(ExpandedModel *A);
Algebra *createBottom(ExpandedModel *diag, ExpandedModel *offdiag);
Algebra *createRhs(ExpandedModel *diag, ExpandedModel *offdiag);
Algebra *createQ(ExpandedModel *A);
Algebra *createBottomQ(ExpandedModel *diag, ExpandedModel *offdiag);
Algebra *createRhsQ(ExpandedModel *diag, ExpandedModel *offdiag);
void SMLCallBack(CallBackInterfaceType *cbi);
void SMLCallBackQ(CallBackInterfaceType *cbi);

/* --------------------------------------------------------------------------
generateSLM
--------------------------------------------------------------------------- */

SMLReturn *
generateSML(ExpandedModel *root)
{
  SMLReturn *Ret = (SMLReturn*)calloc(1, sizeof(SMLReturn));

  Ret->A = createA(root);
  Ret->Q = createQ(root);
  
  return Ret;
}

/* --------------------------------------------------------------------------
createA
--------------------------------------------------------------------------- */
/* This routine sets up the matrix A from the ExpandedModel tree */
Algebra *
createA(ExpandedModel *A)
{
  Algebra *Alg;

  if (!A->localVarInfoSet) A->setLocalVarInfo();
  if (A->children.size()==0){
    // this is final node: read in *.nl file to get dimensions 

    
    OOPSBlock *obl = new OOPSBlock(A, A);
    //NodeId *id = new NodeId();

    //id->amplfile = (A->model_file).c_str();
    //id->nvar = A->nLocalVars;
    //id->lvar = A->listOfVars;

    if (A->nLocalCons<0){
      printf("CreateA: local block has %d constraints: Not initialised?\n",
	     A->nLocalCons);
      exit(1);
    }
    printf("Create leaf node: %s\n",(A->model_file+":"+A->model_file).c_str());
    printf("    %d x %d\n",A->nLocalCons, A->nLocalVars);
    Alg = NewAlgebraSparse(A->nLocalCons, A->nLocalVars, 
			   (A->model_file+":"+A->model_file).c_str(),
			   (CallBackFunction)SMLCallBack, obl);
      
      
    //ASL_pfgh *asl;
    //FILE *nl;
    //int err;

    //    asl = (ASL_pfgh*)ASL_alloc(ASL_read_pfgh);
    ////asl = (ASL_pfgh*)ASL_alloc(ASL_read_f);

    //printf("Opening %s\n",(A->model_file).c_str());
    ////ain = fopen((A->model_file).c_str(), "r");
    //nl = jac0dim(const_cast<char*>((A->model_file).c_str()), (A->model_file).size());
    //if (nl==NULL){
    //  printf("File not found %s\n",(A->model_file).c_str());
    //  exit(1);
    //}
    //err = pfgh_read(nl, ASL_return_read_err);
    //if (err!=0){
    //  printf("pfgh_read returns err=%d\n",err);
    //  exit(1);
    // }
    //printf("Problem dimensions are %d x %d\n",n_var, n_con);
    //exit(1);

  }else{
    /* this is a complex node, set up DblBordDiag with
       - Diagonals from children (call createA again)
       - Off-diagonals with *.nl file from children and col file from parent
       - bottom from this *.nl file and col from the children              */


    /* every child is a diagonal block */
    Algebra **D, **B, **R;
    int nblk, i;

    nblk = (A->children).size();
    D = (Algebra **)calloc(nblk+1, sizeof(Algebra *));
    B = (Algebra **)calloc(nblk, sizeof(Algebra *));
    R = (Algebra **)calloc(nblk, sizeof(Algebra *));

    for(i=0; i<nblk; i++){
      D[i] = createA((A->children).at(i));
      B[i] = createBottom(A, (A->children).at(i));
      R[i] = createRhs(A, (A->children).at(i));
    }

    /* The final D[nblk] block is defined by local constraints/variables */
    
    // this is final node: read in *.nl file to get dimensions 
    // I suspect we can just copy in the code from the leaf node case above 

    OOPSBlock *obl = new OOPSBlock(A, A);

    //NodeId *id = new NodeId();

    //id->amplfile = (A->model_file).c_str();
    //id->nvar = A->nLocalVars;
    //id->lvar = A->listOfVars;

    D[nblk] = NewAlgebraSparse(A->nLocalCons, A->nLocalVars, 
			       (A->model_file+":"+A->model_file).c_str(),
			       (CallBackFunction)SMLCallBack, obl);

    Alg = NewAlgebraDblBordDiag(nblk, B, R, D, 
				(A->model_file+":"+A->model_file).c_str()); 

  }

  return Alg;

}


Algebra *
createBottom(ExpandedModel *diag, ExpandedModel *nondiag)
{
  Algebra *Alg;
  /* This is a bottom block: 
     take the locl constraints from the diag block and
     follow tree defined from the non-diag block */

  if (nondiag->children.size()==0){

    OOPSBlock *obl = new OOPSBlock(diag, nondiag);
    
    //NodeId *id = new NodeId();

    //id->amplfile = (diag->model_file).c_str();
    //id->nvar = nondiag->nLocalVars;
    //id->lvar = nondiag->listOfVars;

    Alg = NewAlgebraSparse(diag->nLocalCons, nondiag->nLocalVars, 
			   (diag->model_file+":"+nondiag->model_file).c_str(),
			   (CallBackFunction)SMLCallBack, obl);

  }else{
    // this is going to be a BlockDense Algebra
    int nblk = nondiag->children.size();
    Algebra **B = (Algebra **)calloc(nblk+1, sizeof(Algebra *));
    
    for(int i=0;i<nblk;i++){
      B[i] = createBottom(diag, (nondiag->children).at(i));
    }

    // The right most block is made up of the diag nodes amplfile
    // and the variables from this nondiag node

    //NodeId *id = new NodeId();
    
    OOPSBlock *obl = new OOPSBlock(diag, nondiag);
    //id->amplfile = (diag->model_file).c_str();
    //id->nvar = nondiag->nLocalVars;
    //id->lvar = nondiag->listOfVars;

    B[nblk] = NewAlgebraSparse(diag->nLocalCons, nondiag->nLocalVars, 
			   (diag->model_file+":"+nondiag->model_file).c_str(), 
			       (CallBackFunction)SMLCallBack, obl);
    Alg = NewAlgebraBlockDense(1, nblk+1, B, 
		      (diag->model_file+":"+nondiag->model_file).c_str());

  }

  return Alg;
}

Algebra *
createRhs(ExpandedModel *diag, ExpandedModel *nondiag)
{
  Algebra *Alg;
  /* This is a bottom block: 
     take the local variables from the diag block and
     follow tree defined from the non-diag block */

  if (nondiag->children.size()==0){

    OOPSBlock *obl = new OOPSBlock(nondiag, diag);
    //NodeId *id = new NodeId();

    //id->amplfile = (nondiag->model_file).c_str();
    //id->nvar = diag->nLocalVars;
    //id->lvar = diag->listOfVars;

    Alg = NewAlgebraSparse(nondiag->nLocalCons, diag->nLocalVars, 
			   (nondiag->model_file+":"+diag->model_file).c_str(),
			   (CallBackFunction)SMLCallBack, obl);

  }else{
    // this is going to be a BlockDense Algebra
    int nblk = nondiag->children.size();
    Algebra **B = (Algebra **)calloc(nblk+1, sizeof(Algebra *));
    
    for(int i=0;i<nblk;i++){
      B[i] = createRhs(diag, (nondiag->children).at(i));
    }
    
    // The bottom node is made from this node's amplfile and the variables
    // defined in diag
    OOPSBlock *obl = new OOPSBlock(nondiag, diag);

    //NodeId *id = new NodeId();

    //id->amplfile = (nondiag->model_file).c_str();
    //id->nvar = diag->nLocalVars;
    //id->lvar = diag->listOfVars;

    B[nblk] = NewAlgebraSparse(nondiag->nLocalCons, diag->nLocalVars, 
			   (nondiag->model_file+":"+diag->model_file).c_str(), 
			       (CallBackFunction)SMLCallBack, obl);

    Alg = NewAlgebraBlockDense(nblk+1, 1, B, 
		       (nondiag->model_file+":"+diag->model_file).c_str());

  }

  return Alg;
}


/* --------------------------------------------------------------------------
createQ
--------------------------------------------------------------------------- */
/* This routine sets up the matrix Q from the ExpandedModel tree */
Algebra *
createQ(ExpandedModel *A)
{
  Algebra *Alg;

  if (!A->localVarInfoSet) A->setLocalVarInfo();
  if (A->children.size()==0){
    // this is final node: read in *.nl file to get dimensions 

    NodeIdQ *id = new NodeIdQ();

    id->nlfile = A->nlfile;
    id->nrowvar = A->nLocalVars;
    id->lrowvar = A->listOfVars;
    id->ncolvar = A->nLocalVars;
    id->lcolvar = A->listOfVars;

    Alg = NewAlgebraSparse(A->nLocalVars, A->nLocalVars, 
			   ("Q"+A->model_file+":"+A->model_file).c_str(),
			   (CallBackFunction)SMLCallBackQ, id);
      
      
  }else{
    // Ok, should really test whether there is a DblBordDiagMatrix needed
    // or if a BlockDiagMatrix will do:
    // - Are there any cross products in the objective bewteen the 
    //   variables local to this node and the variables defined in its 
    //   children?

    /* If there are cross-products then the variables of the children
       that are refered to are part of the AMPL model at this node, but
       they are not part of the nvar/lvar list of local variables 

       AMPL will give us 
         sputinfo->hcolstarts
	 sputinfo->hrownos
       for the Hessian defined in this model file. This could be either
       upper triangle only or the full matrix

       We can scan this for cross products (i.e. go through the columns
       corresponding to the local variables are see if there are any entries
       in rows corresponding to children). 
       If so then set up a DblBordDiagMatrix and let the child work out
       which of these entries are his.
       The child gets passed both ExpandedModels (for itself and the parent)
       The objective however MUST be included in the parent part

    */
    
    /* this is a complex node, set up DblBordDiag with
       - Diagonals from children (call createQ again)
       - Off-diagonals with *.nl file from children and col file from parent
       - bottom from this *.nl file and col from the children              */


    int nzH = (A->nlfile)->getNoHessianEntries();
    int colH = (A->nlfile)->getNoVariables(); 
    int *colbegH = (int *)malloc((colH+1)*sizeof(int));
    int *rownbsH = (int *)malloc(nzH*sizeof(int));
    (A->nlfile)->getHessianStructure(colbegH, rownbsH);

    // now scan to see of there are any off-diagonal entries

    // set marker array that indicates the local variables
    int *marker = (int*)calloc(colH, sizeof(int));
    for(int i=0;i<A->nLocalVars;i++) marker[A->listOfVars[i]]=1;

    bool foundCross = false;
    for(int i=0;i<A->nLocalVars;i++){
      int ix = A->listOfVars[i];
      // and scan through the Hessian structure of this row
      for(int j=colbegH[ix];j<colbegH[ix+1];j++){
	int row = rownbsH[j];
	if (marker[row]==0){
	  // this is an entry in a row that does not belong to this node
	  // (but presumably to a child node)
	  foundCross = true;
	}
      }
    }
    
    if (foundCross){
      /* every child is a diagonal block */
      Algebra **D, **B, **R;
      int nblk, i;
      
      nblk = (A->children).size();
      D = (Algebra **)calloc(nblk+1, sizeof(Algebra *));
      B = (Algebra **)calloc(nblk, sizeof(Algebra *));
      R = (Algebra **)calloc(nblk, sizeof(Algebra *));
      
      for(i=0; i<nblk; i++){
	D[i] = createQ((A->children).at(i));
	B[i] = createBottomQ(A, (A->children).at(i));
	R[i] = createRhsQ(A, (A->children).at(i));
      }

      /* The final D[nblk] block is defined by local constraints/variables */
    
      // this is final node: read in *.nl file to get dimensions 
      // I suspect we can just copy in the code from the leaf node case above 

      NodeIdQ *id = new NodeIdQ();

      id->nlfile = A->nlfile;
      id->ncolvar = A->nLocalVars;
      id->lcolvar = A->listOfVars;
      id->nrowvar = A->nLocalVars;
      id->lrowvar = A->listOfVars;

      D[nblk] = NewAlgebraSparse(A->nLocalVars, A->nLocalVars, 
				 ("Q"+A->model_file+":"+A->model_file).c_str(),
				 (CallBackFunction)SMLCallBackQ, id);

      Alg = NewAlgebraDblBordDiag(nblk, B, R, D, 
			     ("Q"+A->model_file+":"+A->model_file).c_str()); 

    }else{ // Not foundCross => setup BlockDiagMatrix
      /* every child is a diagonal block */
      Algebra **D;
      int nblk, i;
      
      nblk = (A->children).size();
      D = (Algebra **)calloc(nblk+1, sizeof(Algebra *));
      
      for(i=0; i<nblk; i++){
	D[i] = createQ((A->children).at(i));
      }

      /* The final D[nblk] block is defined by local constraints/variables */
    
      // this is final node: read in *.nl file to get dimensions 
      // I suspect we can just copy in the code from the leaf node case above 

      NodeIdQ *id = new NodeIdQ();

      id->nlfile = A->nlfile;
      id->ncolvar = A->nLocalVars;
      id->lcolvar = A->listOfVars;
      id->nrowvar = A->nLocalVars;
      id->lrowvar = A->listOfVars;

      D[nblk] = NewAlgebraSparse(A->nLocalVars, A->nLocalVars, 
				 ("Q"+A->model_file+":"+A->model_file).c_str(),
				 (CallBackFunction)SMLCallBackQ, id);

      Alg = NewAlgebraBlockDiag(nblk+1, D, 
			("Q"+A->model_file+":"+A->model_file).c_str()); 
      
    }
  }

  return Alg;

}

/* ----------------------------------------------------------------------------
createBottom/RhsQ
---------------------------------------------------------------------------- */
/** These two functions will setup the Rhs/Bottom blocks of the Q matrix
 *  Since Q is symmetric, these two functions should be almost identical
 *  They are called if a parent *.nl file finds cross products in the 
 *  objective (i.e. in a column belonging to a parent variable there 
 *  is a entry in a row that does not belong to the parent - but presumably
 *  to a child.
 *  This child should look at the Hessian structure setup in the parents
 *  *.nl file (passed in diag). It should scan through the columns belonging
 *  to the parents variables and see if there are any entries in rows
 *  belonging to *this* child (names of these rows can be obtained from 
 *  'offdiag' they probably need scanning against the column name file
 *  diag->model_file+".col", to get their index numbers in the parents 
 *  numbering.
 *  Also need some treatment of complicated blocks (i.e. when the child node
 *  'offdiag' itself has children) - this should jst be a matter of 
 *  setting up the appropriate BlockDense constructor in the same manner
 *  as done for the A matrix
 *
 *  FIXME: can any of this information (blocking of the Q matrix be
 *         included as part of the ExpandedModel (in order to separate
 *         frontend and backend and do as much processing in the frontend
 *         as possible?
 */

Algebra *
createBottomQ(ExpandedModel *diag, ExpandedModel *offdiag)
{
  printf("createBottomQ not implemented yet!\n");
  exit(1);
}

Algebra *
createRhsQ(ExpandedModel *diag, ExpandedModel *offdiag)
{
  printf("createBottomQ not implemented yet!\n");
  exit(1);
}

/* ---------------------------------------------------------------------------
CallBackFunction: SMLCallBack
---------------------------------------------------------------------------- */
void
SMLCallBack(CallBackInterfaceType *cbi)
{
  /* This needs to be able to fill in the local sparse nodes with the 
     information coming from the ampl file. It can be called either
     for the diagonal nodes or for the off-diagonal nodes

     It needs to know
     - name of the ampl file
     - list and number of variables to use

  */
  
  /*
   where CallBackInterface is a struct of the following form
    int nz
    int max_nz
    int *row_nbs
    int *col_beg
    int *col_len
    double *element
    void *id
  */
  
  /* id is a pointer to NodeId with components
      string amplfile;
      int nvar;
      int *lvar;
  */

  

  OOPSBlock *obl = (OOPSBlock*)cbi->id;

  //NodeId *id = (NodeId*)cbi->id;
  if (cbi->row_nbs==NULL){
    // only want number of nonzeros back
    cbi->nz = getNoNonzerosAMPL(obl->em->model_file, obl->nvar, obl->lvar);
  }else{
    // want to fill in matrices
    fillSparseAMPL(obl->em->model_file, obl->nvar, obl->lvar, cbi->col_beg,
		   cbi->col_len, cbi->row_nbs, cbi->element);
  }
  

}
/* ---------------------------------------------------------------------------
CallBackFunction: SMLCallBack
---------------------------------------------------------------------------- */
void
SMLCallBackQ(CallBackInterfaceType *cbi)
{
  /* This needs to be able to fill in the local sparse nodes with the 
     information coming from the ampl file. It can be called either
     for the diagonal nodes or for the off-diagonal nodes

     It needs to know
     - name of the ampl file
     - list and number of variables to use

  */
  
  /*
   where CallBackInterface is a struct of the following form
    int nz
    int max_nz
    int *row_nbs
    int *col_beg
    int *col_len
    double *element
    void *id
  */
  
  /* id is a pointer to NodeId with components
      string amplfile;
      int nvar;
      int *lvar;
  */

  
  /* need to know: 
     - the amplfile (better the NlFile structure 
        (the one that has the info on the objective = the diagonal file)
     - the variable list for the diagonal part
     - the variable list for the nondiagonal part
  */
  NodeIdQ *id = (NodeIdQ*)cbi->id;

  if (cbi->row_nbs==NULL){
    // only want number of nonzeros back
    NlFile *nlfile = id->nlfile;
    int nzH = nlfile->getNoHessianEntries();
    int colH = nlfile->getNoVariables(); 
    int *colbegH = (int *)malloc((colH+1)*sizeof(int));
    int *rownbsH = (int *)malloc(nzH*sizeof(int));
    nlfile->getHessianStructure(colbegH, rownbsH);
    
    // mark all the relevant rows
    int *marker = (int *)calloc(nlfile->ncol, sizeof(int));
    for(int i=0;i<nlfile->ncol;i++) marker[i] = 0;
    for(int i=0;i<id->nrowvar;i++) marker[id->lrowvar[i]] = 1;

    // and go through all relevant columns and count the number of entries
    cbi->nz = 0;
    for(int i=0;i<id->ncolvar;i++){
      int col = id->lcolvar[i];
      // scan through this column
      for(int j=colbegH[col];j<colbegH[col+1];j++){
	int row = rownbsH[j];
	if (marker[row]==1) cbi->nz++;
      }
    }
    return;
  }else{
    // only want number of nonzeros back
    NlFile *nlfile = id->nlfile;
    int nzH = nlfile->getNoHessianEntries();
    int colH = nlfile->getNoVariables(); 
    int *colbegH = (int *)malloc((colH+1)*sizeof(int));
    int *rownbsH = (int *)malloc(nzH*sizeof(int));
    double *eltsH = (double *)malloc(nzH*sizeof(double));
    nlfile->getHessianEntries(colbegH, rownbsH, eltsH);
    
    // mark all the relevant rows
    int *marker = (int *)calloc(nlfile->ncol, sizeof(int));
    for(int i=0;i<nlfile->ncol;i++) marker[i] = 0;
    for(int i=0;i<id->nrowvar;i++) marker[id->lrowvar[i]] = i+1;

    // and go through all relevant columns and copy the entries
    cbi->nz = 0;
    for(int i=0;i<id->ncolvar;i++){
      int col = id->lcolvar[i];
      cbi->col_beg[i] = cbi->nz;
      // scan through this column
      for(int j=colbegH[col];j<colbegH[col+1];j++){
	int row = rownbsH[j];
	if (marker[row]!=0) {
	  cbi->element[cbi->nz] = eltsH[j];
	  cbi->row_nbs[cbi->nz] = marker[row]-1; // to get the local numbering
	  cbi->nz++;
	}
      }
    }
    cbi->col_beg[id->ncolvar] = cbi->nz;
    for(int i=0;i<id->ncolvar;i++) 
      cbi->col_len[i] = cbi->col_beg[i+1]-cbi->col_beg[i];
    return;
  }
  

}

/* ---------------------------------------------------------------------------
CallBackFunction: FillRhsVector
---------------------------------------------------------------------------- */
void
FillRhsVector(Vector *vb)
{
  Tree *T = vb->node;
  DenseVector *dense = GetDenseVectorFromVector(vb);

  Algebra *A = (Algebra*)T->nodeOfAlg; // the diagonal node that spawned this tree
  OOPSBlock *obl = (OOPSBlock*)A->id; // and its id structure

  // FIXME: should the id structure include information on the ExpandedModel
  //        as well? That way we could do some more sanity checks

  getRhsAMPL(obl->em->model_file, obl->nvar, obl->lvar, dense->elts);

}

/* ---------------------------------------------------------------------------
CallBackFunction: FillObjVector
---------------------------------------------------------------------------- */
void
FillObjVector(Vector *vc)
{
  Tree *T = vc->node;
  DenseVector *dense = GetDenseVectorFromVector(vc);

  Algebra *A = (Algebra*)T->nodeOfAlg; // the diagonal node that spawned this tree
  OOPSBlock *obl = (OOPSBlock*)A->id;        // and its id structure
  //NodeId *id = (NodeId*)A->id;        // and its id structure

  assert(obl->nvar==T->end-T->begin);
  getObjAMPL(obl->em->model_file, obl->nvar, obl->lvar, dense->elts);

}

/* ---------------------------------------------------------------------------
CallBackFunction: FillUpBndVector
---------------------------------------------------------------------------- */
void
FillUpBndVector(Vector *vu)
{
  Tree *T = vu->node;
  DenseVector *dense = GetDenseVectorFromVector(vu);

  Algebra *A = (Algebra *)T->nodeOfAlg; // the diagonal node that spawned this tree
  //NodeId *id = (NodeId*)A->id;        // and its id structure
  OOPSBlock *obl = (OOPSBlock*)A->id;        // and its id structure

  assert(obl->nvar==T->end-T->begin);
  getBndAMPL(obl->em->model_file, obl->nvar, obl->lvar, dense->elts);

}
