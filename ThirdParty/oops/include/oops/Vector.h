/* This file is part of OOPS.
 *
 * OOPS is (c) 2003-2009 Jacek Gondzio and Andreas Grothey, 
 *                       University of Edinburgh
 *
 * OOPS is distributed in a restricted form in the hope that it will be a useful
 * example of what can be done with SML, however it is NOT released under a free
 * software license.
 *
 * You may only redistribute this version of OOPS with a version of SML. You
 * may not link OOPS with code which is not part of SML licensed under the
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

class Vector;
typedef enum {notcomputed_proc=-3, computed_proc=-1, Contrib} CompStatus;
typedef enum {Exact_st=-1,  NoStat=0, Contr_st=1} ContStatus; 

#include "oops/Tree.h" 
#include "oops/LogiVector.h" 
#include "oops/DenseVector.h"



#ifdef REDUNDANT
typedef double (*KrnFct2) (double, double, double*);
typedef double (*KrnFct3) (double, double, double, double*);
typedef double (*KrnFct4) (double, double, double, double, double*);
typedef double (*KrnFct5) (double, double, double, double, double, double*);
typedef void (*KrnFctGen) (double**, int, double*);
#endif

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
#define MEMORY_LOCAL(Node)          (Node->local == true)

/** true if memory not local to this node */
#define MEMORY_NOT_LOCAL(Node)      (Node->local == false)

/** returns the parent vector that holds the memory for this node */
#define FATHER_VECTOR(Vector, Node) (Vector->subvectors[Node->above->index])

/** returns the subvector of Vector, identified by node */
#define SUBVECTOR(Vector, Node)     (Vector->subvectors[Node->index])



/*
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif
*/

/**
 * Vector class: This is the abstract class for all Vector type structures 
 * that are defined on a Tree. Implementing classes are Vector, LogiVector,
 * StrSparseVector.
 *
 */
class Vector
{
 public:
  typedef double (*KrnFct) (double);
  typedef void (*VectorCallBackFunction) (Vector *);
  typedef void (*VectorCallBackFunction2) (Vector *, Vector *);


  /** Node corresponding to this subvector */
  Tree *node;

  /** Array of all subvectors (same at all nodes) */
  Vector **subvectors;

  /** Memory for this node (not always allocated) */
  DenseVector *dense;

  /** If dense is allocated here (not true for GetPrimalDualParts */
  bool dense_alloc;

  /** Indicator if this is the root of the allocation: 
      on this node ->subvector is allocated (and needs to be freed) */
  bool is_root;

  /** The name of the vector */
  const char *name;
    
#ifdef WITH_MPI
  /* CompStatus    proc;   */       /* not used */
  ContStatus       sum_stat;   /* One of NoStat, Exact_st, Contr_st 
				   Needed for CopyToDenseVector 
				   Exact_st: Shared nodes exact on all proc
				   Contrib_st:  "     are sum of all proc */
#endif

  /*---------------------------------------------------------------------------
    Methods
  ---------------------------------------------------------------------------*/

  /* ------------------- constructors and destructors ---------------------- */

  //typedef Vector *(*NewvectorType) (Tree *, const char *);

  Vector();
  
  Vector(Tree *T, const char *name);
  /* set to NewVectorParcel: Allocate & initialise Vector following tree:
     local nodes          have dense allocated
     local & above nodes  have dense point to subvetor of above vector
     non-local nodes      have dense = null 
     also sets proc = computed_proc, sum_stat = NoStat everywhere
  */

  Vector(Tree *T, const char *name, double *dense);
  /* Similar to NewVector(Parcel):
     but memory for the actual entries (inside the DenseVector parts)  
     is taken from the double array 'dense'
  */

  /** Free recursively the memory allocated for the Vector */
  ~Vector ();

  Vector *
    getPrimalDualParts(Tree *Atree, int *map);
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


  /* ---------- methods for simple manipulation of Vectors ---------------- */
  
  /** Physically set all elements of the vector to 0 */
  void setZero();

  /** Physically set all elements of the vector to a given value */
  void setDoubleValue(const double a);

  /** Set the value of component v[j] */
  void setComponent(const int j, const double value);

  /** Get the value of component v[j] */
  double getComponent(const int j);

  /** mark the Vector as all zeros (for efficiency) */
  void markZero ();

  /** remove the zero flag from the Vector */
  void unmarkZero ();

  /** Check that the setting of dense->is_zero is correct */
  void checkZeroMark();

  /** Copy the appropriate DenseVectors (using blas function dcopy) */
  void copy (Vector *targetV);

  /** x += ay */ 
  void addVector(const double a, Vector *y);

  /** x = ay + bz */
  void setAypbz(const double a, Vector *y, const double b, Vector *z);

  /** x = 1./(x+reg) */
  void invertReg(const double reg);

  /** Take the absolute value of each element in the vector: x = |x| */
  void absValue ();

  /** Add a scalar to each element of the vector */
  void addScalar(const double a);

  /** Multiply each element of the vector by a scalar */
  void multScalar(const double a);

  /** x = max(x, y), element wise */
  void maxInPlace (Vector *y);

  /** x = min(x, y), element wise */
  void minInPlace (Vector *y);

  /** Return the number of nonzeros in the Vector */
  int countNz();

  /** Set elements to uniformly distributed random numbers in [0,1] */
  void setRandom();


  /* ------------- general functions operating on vectors ---------------- */

  /** do x = x.*f(y) (or x = x.*y is f==null) */
  void multiplyCompwise(Vector *y, KrnFct f);

  /** do x = x.*f(y) (or x = x./y is f==null) */
  void divideCompwise (Vector *y, KrnFct f);

  /** do x += a * y.*z   */
  void addKroneckProd(const double a, Vector *y, Vector *z);

  /** do x += a * y./z   */
  void addInvKroneckProd(const double a, Vector *y, Vector *z);

  /** x = 1./x */
  void invert();

  /** do x = f(x) */
  void applyFunc (KrnFct f);

  /* ------------- special functions operating on a Vector ----------------- */

  /** x = y/(1 + y*z) */
  void setTheta (Vector *y, Vector *z);

  /* ---------------------------- print functions ------------------------ */

  /** print Vector to screen */
  void print ();

  /** print Vector to File */
  void printFile (FILE *out);

  /** Prints the vector *V to file out in matlab readable form: name = [...];*/
  void printMatlab(FILE *out, const char *name);



/* ------------ functions that need some degree of parallelism ----------- */
  
  /** returns scalar product x'y, x,y need to have same tree-structure 
      also works in parallel: HostShared only done on root, then summed */
  double ddotPar (Vector *y);

  /** Finds InfNorm of vector and position */
  double infNorm(int *j);
  //FIXME: each proc returns its own max, position is wrong */

  /** Finds min(abs(x(i)) of vector and position */
  double minInfNorm(int *j, const double bnd);
  // FIXME: each proc returns its own min, position is wrong 

  /** Find the maximal component of the vector */
  double maxComp(int *j);
  // FIXME: each proc returns its own min, position is wrong 

  /** Find the minimal component of the vector */
  double minComp(int *j);
  // FIXME: each proc returns its own min, position is wrong 

  /* ------------------ Vector/DenseVector functions ------------------ */


  /** Distribute DenseVector among the appropriate local/above nodes */
  void copyFromDense (DenseVector *);

  /** Copy from the appropriate local||above nodes of Vector to DenseVector
   * in parallel case do a Bcast/Reduce as necessary, so that all processors
   * have the complete vector (FIXME: the copying is done twice here!!!) */
  void copyToDense(DenseVector *);

  /** Compares x to y and reports infnorm of difference: ONLY FOR TESTING */
  void compareToDenseVector(DenseVector *y);

  /** set sum_stat = Exact_st (Only Parallel) */
  void setExact();


#ifdef WITH_MPI
  void copyToDenseBcastVector (DenseVector *y);
  /* Called from CopyToDenseVector */
  /* FIXME: why only if (x->node->host_stat!=HostSingle)? */

#ifdef REDUNDANT
  void copyToDenseReduceVector(DenseVector *y);
  /* NOT IMPLEMENTED */
#endif

  /** Do the MPI_Op op on all HostAllShared nodes of Vector x */
  void reduceVector (MPI_Op op, MPI_Comm comm);

#endif

  /** ------------------- Static functions on Vectors --------------------- */

  /* ----------------- Special functions on vectors ------------------------ */


  /** ddot = (x+a*dx)'* (y+b*dy) */
  static double ddotLinCombPar (Vector *x, Vector *y, Vector *dx, Vector *dy, 
				double a, double b);

  /** dp = (x+a1p dX)(z+a1d dZ) 
      dp_i < 1e-1barr => r_xz_i = barr - dp_i
      dp_i > 1e+1barr => r_xz_i = -5*barr
      rhs_x_i = -r_xz_i/x_i */
  static void GondzioCorr(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		  Vector *r_xz, Vector *rhs_x,
		  const double barr, const double a1p, const double a1d);

  /** GondzioCorrTarget:
   *    dp = (x+a1p dX)(z+a1d dZ) 
   *    dp < 1e-1*t[i]*barr => r_xz_i = t[i]*barr - dp
   *    dp > 1e+1*t[i]*barr => r_xz_i = -5*t[i]*barr
   *    rhs_x_i = -r_xz_i/x_i */
  static void
    GondzioCorrTarget(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		  Vector *r_xz, Vector *rhs_x, Vector *target,
		  const double barr, const double a1p, const double a1d);

  /** Chooser function between GondzioCorr/GondzioCorrTarget */
  static void
    GondzioCorrVector(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		  Vector *r_xz, Vector *rhs_x, Vector *target,
		  const double barr, const double a1p, const double a1d); 

  /** dp = (x+a1p dX)(z+a1d dZ) 
      dp_i < 1e-1barr => r_xz_i = barr - dp_i
      rhs_x_i = -r_xz_i/x_i */
  static void 
    GondzioCorrOnlySmall(Vector *x, Vector *dX, 
				Vector *z, Vector *dZ,
			       Vector *r_xz, Vector *rhs_x, const double barr,
			   const double a1p, const double a1d);

  /** x[i]*z[i] < barr => min(x,z) = barr/max(x,z) */
  static void
    BalanceKronProd(Vector *x, Vector *y, const double barr);



  /* ------- Functions that can only be used in a special setting ---------- */

  /** true if both nodes local or serial (not parallel)
   *  Used by Algebra.c: DoItAlg */
  bool comp_node(Vector *y);

  void fillCallBack(VectorCallBackFunction f);

  static void 
    CallBack2(Vector *x, Vector *y, VectorCallBackFunction2 f);

  /* ================ Functions that use a LogiVector ====================== */

  /** Copy the elements where l is set */
  void copyToWhere(Vector *y, LogiVector *l);
  /* y[i] = x[i] if l[i] */

  /* x += a*y where l is set */
  void addVectorWhere(const double a, Vector *y, LogiVector *l);

  /* x=a*y+b*z where l is set */
  void setAypbzWhere(const double a, Vector *y, const double b, Vector *z,
		     LogiVector *l);

  /* x(i) *= f(y(i)) or x(i) *= y(i) where l is set */
  void multiplyCompwiseWhere(Vector *y, KrnFct f, LogiVector *l);

  /* x(i) = x(i)*f(y(i)) or x(i) = x(i)/y(i) where l is set */
  void divideCompwiseWhere(Vector *y, KrnFct f, LogiVector *l);

  /* x(i) += a * y(i)*z(i) where l is set */
  void addKroneckProdWhere(double a, Vector *y, Vector *z, LogiVector *l);

  /* x(i) += a * y(i)/z(i) where l is set */
  void addInvKroneckProdWhere(const double a, Vector *y, Vector *z, LogiVector *l);

  /* v[i] = a if l[i] is set */
  void setValueWhere(const double a, LogiVector *l);

  /** Set each element in the first vector to be the maximum of the two vectors
      for the elements where l is set */
  void maxInPlaceWhere (Vector *y, LogiVector *l);
  /* x[i] = max(x[i], y[i]) where l[i] is set */
  
  /** Set each element in the first vector to be the minimum of the two vectors
      for the elements where l is set */
  void minInPlaceWhere (Vector *y, LogiVector *l);
  /* x[i] = min(x[i], y[i]) where l[i] is set */

  double ddotParWhere (Vector *y, LogiVector *l);
  /* returns scalar product x'y, of entries that have l set */
  /* also works in parallel: HostShared only done on root, then summed */
  
  double infNormWhere (int *j, LogiVector *l);
  /* Finds the infinity norm of Vector x and position of largest element 
     for those elements for which l is set
     FIXME: each proc returns its own min, position is wrong */

  double minInfNormWhere (int *j, LogiVector *l);
  /* Finds min(abs(x[i])) and position of smallest element for those elements 
     for which l is set
     FIXME: each proc returns its own min, position is wrong */

  double minCompWhere(int *j, LogiVector *l);
  /* Finds min(x[i]) and position of smallest element for those elements 
     for which l is set
     FIXME: each proc returns its own min, position is wrong */

  double maxCompWhere(int *j, LogiVector *l);
  /* Finds max(x[i]) and position of largest element for those elements 
     for which l is set
     FIXME: each proc returns its own min, position is wrong */

  /* x[i] = 1./x[i] if l[i] is set */
  void invertWhere (LogiVector *l);

  /* Sets the vector x = y/(1+y*z), which has the effect of capping x at 1/z */
  void getThetaWhere (Vector *y, Vector *z, LogiVector *l);

  /* x[i] = f(x[i]) if l[i] is set */
  void applyFuncWhere (KrnFct f, LogiVector *l);



  /* ------- static methods that involve LogiVectors -------------------- */
  /*     ddotLinCombParWhere   ddot = (x+a*dx)'* (y+b*dy) */
  static double 
    ddotLinCombParWhere(Vector *x, Vector *y, Vector *dx, Vector *dy,
			const double a, const double b, LogiVector *l);

  static void
    GondzioCorrWhere(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		     Vector *r_xz, Vector *rhs_x, const double barr,
		     const double a1p, const double a1d,
		     LogiVector *l);
  /* GondzioCorrVector:  dp = (x+a1p dX)(z+a1d dZ) 
                       dp_i < 1e-1barr => r_xz_i = barr - dp_i
                       dp_i > 1e+1barr => r_xz_i = -5*barr
                       rhs_x_i = -r_xz_i/x_i 
  */

  static void
    GondzioCorrOnlySmallWhere(Vector *x, Vector *dX, Vector *z, Vector *dZ,
			      Vector *r_xz, Vector *rhs_x, const double barr,
			      const double a1p, const double a1d,
			      LogiVector *l);
  /* GondzioCorrVector:  dp = (x+a1p dX)(z+a1d dZ) 
                       dp_i < 1e-1barr => r_xz_i = barr - dp_i
                       rhs_x_i = -r_xz_i/x_i 
  */

  /* x[i]*z[i] < barr => min(x,z) = barr/max(x,z) where l is set */
  static void
    BalanceKronProdWhere(Vector *x, Vector *y,
			 const double barr, LogiVector *l);



#ifdef REDUNDANT
void
InvPermVector (Vector *x, DenseVector *y, int *invperm);
/* Can only be called on local nodes: y[invperm[i]] = -x[i] */

void
FuncToDenseVectorStatic (Vector *x, double *y, KrnFct f);
/* called through macro FuncToDenseVector(Vector* x, DenseVector *y, f) */
/* this macro should only be called for local||above nodes */
/* y = f(x) (or y = x if f==null), but doesn't do bcast/reduce */


#endif


#ifdef REDUNDANT
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
#endif /* REDUNDANT */

#ifdef REDUNDANT
  void
    SetContrVector(Vector *x);
  /* set sum_stat = Contr_st (Only Parallel) */
#endif /* REDUNDANT */

};

#endif
