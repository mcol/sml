/* This file is part of OOPS.
 *
 * OOPS is (c) 2003-2009 Jacek Gondzio and Andreas Grothey, 
 *                       University of Edinburgh
 *
 * OOPS is distributed in a restricted form in the hope that it will be a useful
 * example of what can be done with SML, however it is NOT released under a free
 * software lisence.
 *
 * You may only redistribute this version of OOPS with a version of SML. You
 * may not link OOPS with code which is not part of SML lisenced under the
 * LGPL v3.
 *
 * You may NOT modify, disassemble, or otherwise reverse engineer OOPS.
 *
 * OOPS is distributed WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
/**
 *  @file Vector.h
 *
 *  Definition of Vector.
 */

#ifndef VECTOR_H
#define VECTOR_H

#include "oops/Tree.h" 
#include "oops/DenseVector.h"

typedef double (*KrnFct) (double);
typedef double (*KrnFct2) (double, double, double*);
typedef double (*KrnFct3) (double, double, double, double*);
typedef double (*KrnFct4) (double, double, double, double, double*);
typedef double (*KrnFct5) (double, double, double, double, double, double*);
typedef void (*KrnFctGen) (double**, int, double*);

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

typedef enum {notcomputed_proc=-3, computed_proc=-1, Contrib} CompStatus;
typedef enum {Exact_st=-1,  NoStat=0, Contr_st=1} ContStatus; 

/*----------------------------------------------------------------------------
        Functions defined on the anonymous vector structure
----------------------------------------------------------------------------*/

/** A structured Vector */
typedef struct Vector {

  /** Node corresponding to this subvector */
  Tree *node;

  /** Array of all subvectors (same at all nodes) */
  struct Vector **subvectors;

  /** Memory for this node (not always allocated) */
  DenseVector *dense;

  /** If dense is allocated here (not true for GetPrimalDualParts */
  int dense_alloc;
    
#ifdef WITH_MPI
  /* CompStatus    proc;   */       /* not used */
  ContStatus       sum_stat;   /* One of NoStat, Exact_st, Contr_st 
				   Needed for CopyToDenseVector 
				   Exact_st: Shared nodes exact on all proc
				   Contrib_st:  "     are sum of all proc */
#endif

} Vector;

typedef void (*VectorCallBackFunction) (Vector *);

/*-----------------------------------------------------------------------------
        Macros
-----------------------------------------------------------------------------*/
#define OuterProdVector(outpd, NonzCC,CompCol,c)  OuterProdDenseVector (GetDenseVectorFromVector (outpd), NonzCC, CompCol, c)

#define NbOfSubVect(v)  ((v->node)->nb_sons)
#define SetEjVector(ej, j) SetCompValueVector(ej, (j), 1.);
#define SetMinusEjVector(ej, j)  SetCompValueVector (ej, j, -1.)
#define GetDenseVectorFromVector(v)  v->subvectors[(v->node)->index]->dense
#define FuncToDenseVector(x,y,f) FuncToDenseVectorStatic(x, y->elts- x->node->begin, f)
/* FIXME: can only be called for local||above nodes */
#define FuncToDenseBcstVector(x,y,f) FuncToDenseBcstVector_(x, y->elts- x->node->begin, f)
#define SubVector(v,i) v->subvectors[v->node->sons[i]->index]

/** true if memory allocated to a node above */
#define MEMORY_ABOVE(Node)          (Node->above != NULL)

/** true if not memory allocated to a node above */
#define MEMORY_NOT_ABOVE(Node)      (Node->above == NULL)

/** true if memory local to this node */
#define MEMORY_LOCAL(Node)          (Node->local == 1)

/** true if memory not local to this node */
#define MEMORY_NOT_LOCAL(Node)      (Node->local == 0)

/** returns the parent vector that holds the memory for this node */
#define FATHER_VECTOR(Vector, Node) (Vector->subvectors[Node->above->index])

/** returns the subvector of Vector, identified by node */
#define SUBVECTOR(Vector, Node)     (Vector->subvectors[Node->index])

/*-----------------------------------------------------------------------------
        Prototypes
-----------------------------------------------------------------------------*/

typedef Vector *(*NewvectorType) (Tree *, const char *);

/* ------------------- constructors and destructors ---------------------- */

extern
NewvectorType NewVector ;
/* set to NewVectorParcel: Allocate & initialise Vector following tree:
      local nodes          have dense allocated
      local & above nodes  have dense point to subvetor of above vector
      non-local nodes      have dense = null 
     also sets proc = computed_proc, sum_stat = NoStat everywhere
*/

Vector *
NewVectorFromArray(Tree *T, const char *name, double *dense);
/* Similar to NewVector(Parcel):
   but memory for the actual entries (inside the DenseVector parts)  
   is taken from the double array 'dense'
*/

/** Free recursively the memory allocated for the Vector */
void
FreeVector (Vector *V);

Vector *
GetPrimalDualVectorParts(Vector *augSysVector, Tree *Atree, int *map);
/* Creates a vector for access to the primal (dual) part of an augmented
   System vector: 
      augSysVector : the Vector that separate primal/dual access is needed for
      Atree        : the tree corresponding to the primal (dual) structure
      map          : map[i] is the node in the augSysVector that corresponds
                     to the i-th node in the Atree.
   The method follows down Atree to create a new Vector analoguous to
   NewVector. BUT instead of creating new DenseVectors for every node 
   it just creates a pointer to the corresponding node of augSysVector

   This way the Vector can be accessed through either the augSys structure
   or the primal/dual structure, the memory behind the two strucutures is
   the same */


/* ----------- methods for simple manipulation of Vectors ----------------- */

/** Physically set all elements of the vector to 0 */
void
ZeroVector (Vector *v);

/** Physically set all elements of the vector to a given value */
void
SetVectorToDouble(Vector *v, const double a);

/** Set the value of component v[j] */
void
SetCompValueVector(Vector *v, const int j, const double value);

/** Get the value of component v[j] */
void
GetCompValueVector(Vector *v, const int j, double *value);

void
MarkZero (Vector *x);
void
UnMarkZero (Vector *x);
/* Set dense->is_zero = 0/1 for all local||above nodes */

/** Check that the setting of dense->is_zero is correct */
void
CheckIsZeroVector(Vector *x, const char *name);

/** Copy the appropriate DenseVectors (using blas function dcopy) */
void
CopyVector (Vector *x, Vector *y);

void
daxpyVector(Vector *x, Vector *y, const double a);
/* y = y + ax */ 

void
daypbzVector(Vector *x, Vector *y, Vector *z, const double a, const double b);
/* x = ay + bz */

void
invertVector(Vector *y, const double reg);
/* y = 1./(y+reg) */

/** Take the absolute value of each element in the vector */
void
AbsVector (Vector *x);
/* x = |x| */

/** Add a scalar to each element of the vector */
void
AddScalarVector(Vector *x, const double a);

/** Multiply each element of the vector by a scalar */
void
MultScalarVector(Vector *x, const double a);

/** Set the first vector to be the maximum between the two vectors
    (element-wise) */
void
MaxVector (Vector *x, Vector *y);

/** Set the first vector to be the minimum between the two vectors
    (element-wise) */
void
MinVector (Vector *x, Vector *y);

/** Return the number of nonzeros in the Vector */
int 
CountNzVector(Vector *x);

/** Set the vector elements to uniformly distributed random numbers in [0,1] */
void
SetRandomVector(Vector *x);

/* ------------------ Special functions on vectors ------------------------ */

void
GondzioCorrVector(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		  Vector *r_xz, Vector *rhs_x, Vector *target,
		  const double barr, const double a1p, const double a1d);
/* dp = (x+a1p dX)(z+a1d dZ) 
   dp_i < 1e-1barr => r_xz_i = barr - dp_i
   dp_i > 1e+1barr => r_xz_i = -5*barr
   rhs_x_i = -r_xz_i/x_i */

void
GondzioCorrVectorOnlySmall(Vector *x, Vector *dX, Vector *z, Vector *dZ,
			   Vector *r_xz, Vector *rhs_x, const double barr,
			   const double a1p, const double a1d);
/* dp = (x+a1p dX)(z+a1d dZ) 
   dp_i < 1e-1barr => r_xz_i = barr - dp_i
   rhs_x_i = -r_xz_i/x_i */

void
BalanceKronProdVector(Vector *x, Vector *y, const double barr);
/* x[i]*z[i] < barr => min(x,z) = barr/max(x,z) */


void
getThetaVector (Vector *x, Vector *y, Vector *z);
/* x = y/(1 + y*z) */

/* ------------- general functions operating on vectors ---------------- */

void
KroneckProdVector (Vector *x, Vector *y, KrnFct f);
/* do x = x.*f(y) (or x = x.*y is f==null) */

void
InvKroneckProdVector (Vector *x, Vector *y, KrnFct f);
/* do x = x.*f(y) (or x = x./y is f==null) */

void
daxpyKroneckProdVector(Vector *x, Vector *y, Vector *z, const double a);
/* do x += a * y.*z   */

void
daxpyInvKroneckProdVector(Vector *x, Vector *y, Vector *z, const double a);
/* do x += a * y./z   */

void
FuncVector (Vector *x, KrnFct f);
/* do x = f(x) */
void
FuncVector2 (KrnFct2 f, Vector *x, Vector *y, double *a);
void
FuncVector3 (KrnFct3 f, Vector *x, Vector *y, Vector *z, double *a);
void
FuncVector4 (KrnFct4 f, Vector *w, Vector *x, Vector *y, Vector *z,
	     double *a);
void
FuncVector5 (KrnFct5 f, Vector *v, Vector *w, Vector *x, Vector *y, Vector *z,
	     double *a);

/* do v = f(v, w, x, y, z, a)  */

void
FuncVectorGen (KrnFctGen f, int nbvec, Vector **vec, double *coeff);
/* do vec[0] = f(vec[0], ..., vec[nbvec-1], coeff[])  */


/* ---------------------------- print functions ------------------------ */

void
PrintVector (Vector *V);
void
PrintVector2 (Vector *V, FILE *out);
void
PrintVectorMatlab(Vector *V, FILE *out, const char *name);
/* Prints the vector *V to file out in matlab readable form: 
         name = [...];
 */

/* ------------ functions that need some degree of parallelism ----------- */
#ifdef OBSOLETE
double
ddotVector (Vector *x, Vector *y);
/* returns scalar product x'y, x,y need to have same tree-structure 
   only works in serial (par: each proc does its HostSingle + HostShared*/
/* Used by BlockAngAlgebra.c */
#endif

double
ddotVectorPar (Vector *x, Vector *y);
/* returns scalar product x'y, x,y need to have same tree-structure 
   also works in parallel: HostShared only done on root, then summed */
/* Used by hopdm_par.c */

double
ddotLinCombPar (Vector *x, Vector *y, Vector *dx, Vector *dy, 
		double a, double b);
/* ddot = (x+a*dx)'* (y+b*dy) */

void
InfNormVector (Vector *x, double *val, int *j);
/* Finds InfNorm of vector and position
   FIXME: each proc returns its own max, position is wrong */

void
MinInfNormVector(Vector *x, double *val, int *j, const double bnd);
/* Finds min(abs(x(i)) of vector and position
   FIXME: each proc returns its own min, position is wrong */

/** Find the minimal component of the vector */
void
MinCompVector(Vector *x, double *val, int *j);
/* FIXME: each proc returns its own min, position is wrong */

/* ------------------ Vector/DenseVector functions ------------------ */


void
CopyDenseToVector (DenseVector *, Vector *);
/* Distribute DenseVector among the appropriate local||above nodes of Vector */

void
CopyToDenseVector (Vector *, DenseVector *);
/* Copy from the appropriate local||above nodes of Vector to DenseVector
   in parallel case do a Bcast/Reduce as necessary, so that all processors
   have the complete vector (FIXME: the copying is done twice here!!!)*/

void
SetExactVector(Vector *x);
/* set sum_stat = Exact_st (Only Parallel) */

#ifdef REDUNDANT
void
SetContrVector(Vector *x);
/* set sum_stat = Contr_st (Only Parallel) */
#endif /* REDUNDANT */

#ifdef WITH_MPI
void
CopyToDenseBcastVector (Vector *x, DenseVector *y);
/* Called from CopyToDenseVector */
/* FIXME: why only if (x->node->host_stat!=HostSingle)? */

void
CopyToDenseReduceVector(Vector *x, DenseVector *y);
/* NOT IMPLEMENTED */

void
ReduceVector (Vector *x, MPI_Op op, MPI_Comm comm);
/* Do the MPI_Op op on all HostAllShared nodes of Vector x */
#endif

/* --------- Functions that can only be used in a special setting ---------- */

void
InvPermVector (Vector *x, DenseVector *y, int *invperm);
/* Can only be called on local nodes: y[invperm[i]] = -x[i] */

void
FuncToDenseVectorStatic (Vector *x, double *y, KrnFct f);
/* called through macro FuncToDenseVector(Vector* x, DenseVector *y, f) */
/* this macro should only be called for local||above nodes */
/* y = f(x) (or y = x if f==null), but doesn't do bcast/reduce */

void 
CompareVectorDenseVector(Vector *x, DenseVector *y);
/* Compares x to y and reports infnorm of difference: ONLY FOR TESTING */

int 
comp_node(Vector *x, Vector *y);
/* true if both nodes local or serial (not parallel) 
   Used by Algebra.c: DoItAlg */

void 
VectorFillCallBack(Vector *x, VectorCallBackFunction f);

#endif
