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
 *  @file LogiVector.h
 *
 *  Definition of LogiVector.
 *
 *  A LogiVector contains only 0/1 values.
 */

#ifndef LOGIVECTOR_H
#define LOGIVECTOR_H

#include "oops/Vector.h" 

/** A dense Vector consisting of 0/1 values */
typedef struct {

  /** Dimension of the vector */
  int dim;

  /** Set to 1 if memory allocated, 0 otherwise */
  short mem_alloc;

  /** Not used */
  short is_zero;

  /** Array of elements */
  short *elts;

  /** Name of the vector */
  char *name;

  /** Not used */
  int newtag;

} DenseLogiVector;

/** A structured Vector consisting of 0/1 values */ 
typedef struct LogiVector
{
  /** Node corresponding to this subvector */
  Tree *node;

  /** Array of all subvectors (same at all nodes) */
  struct LogiVector **subvectors;

  /** Memory for this node (not always allocated) */
  DenseLogiVector *dense;
    
#ifdef WITH_MPI
  /** Not used */
  CompStatus proc;

  /** Not used */
  ContStatus sum_stat;
#endif
}
LogiVector;

/*-----------------------------------------------------------------------------
        Prototypes
-----------------------------------------------------------------------------*/

/* ------------------- constructors and destructors ---------------------- */

/** Create a DenseLogiVector */
DenseLogiVector *
NewDenseLogiVector(const int dim, const char *name);

/** Create a DenseLogiVector that is a portion (begin, begin+dim) of another 
    DenseLogiVector sharing its memory */
DenseLogiVector *
SubDenseLogiVector (DenseLogiVector *V, const int begin, const int dim);

/** Create a LogiVector and allocate space in the correct subnodes */
LogiVector *
NewLogiVector(Tree *T, const char *name);

/** Free the space allocated for a LogiVector */
void
FreeLogiVector (LogiVector *V);

/** Free the space allocated for a DenseLogiVector */
void
FreeDenseLogiVector (DenseLogiVector *V);

/*---------------- Setting LogiVectors from Vectors ------------------------ */

/** Set an element of the LogiVector to 1 if the corresponding entry in the
    Vector is smaller than the given bound */
void 
SetLogiVectorFromVector(LogiVector *l, Vector *v, const double bound);
/* Sets Vector[i]<bound? Logivector[i] = 1:0;   */

/** Set an element of the LogiVector to 1 if the corresponding entry in the
    Vector is greater than the given bound */
void 
SetLogiVectorFromVectorGr(LogiVector *l, Vector *v, const double bound);
/* Sets Vector[i]>bound? Logivector[i] = 1:0;   */

/** Set an element of the LogiVector to 1 if the corresponding entry in the
    Vector v1 is is greater than the entry in Vector v2 */
void 
SetLogiVectorFromVectorComp(LogiVector *l, Vector *v1, Vector *v2);
/* Sets V1[i]>V2[i]? Logivector[i] = 1:0;   */

/* ----------- methods for simple manipulation of LogiVectors -------------- */

/** Return the number of elements set to 1 (works also in parallel) */
int 
CountLogiVector(LogiVector *l);

/** Set the LogiVector l2 to the values in l1 */
void
CopyLogiVector (LogiVector *l1, LogiVector *l2);

/** Set the LogiVector l1 if l2 AND l3 */
void
AndLogiVector (LogiVector *l1, LogiVector *l2, LogiVector *l3);

/** Negate the LogiVector */
void
NotLogiVector (LogiVector *l);

/** Print the LogiVector node by node, one entry per line */
void
PrintLogiVector (LogiVector *l);

/* -------- methods to operate on Vectors depending on LogiVectors --------- */

/** Copy the elements where l is set */
void
CopyVectorWhere(Vector *x, Vector *y, LogiVector *l);
/* y[i] = x[i] if l[i] */

void
daxpyVectorWhere(Vector *x, Vector *y, const double a, LogiVector *l);
/* y += a*x where l is set */

void
daypbzVectorWhere(Vector *x, Vector *y, Vector *z,
		  const double a, const double b, LogiVector *l);
/* x=a*y+b*z where l is set */

void
KroneckProdVectorWhere(Vector *x, Vector *y, KrnFct f, LogiVector *l);
/* x(i) = x(i)*f(y(i)) or x(i) = x(i)*y(i) where l is set */

void
daxpyKroneckProdVectorWhere(Vector *x, Vector *y, Vector *z,
			    const double a, LogiVector *l);
/* x(i) += a * y(i)*z(i) where l is set */

void
InvKroneckProdVectorWhere(Vector *x, Vector *y, KrnFct f, LogiVector *l);
/* x(i) = x(i)*f(y(i)) or x(i) = x(i)/y(i) where l is set */

void
daxpyInvKroneckProdVectorWhere(Vector *x, Vector *y, Vector *z,
			       const double a, LogiVector *l);
/* x(i) += a * y(i)/z(i) where l is set */

void
SetVectorToDoubleWhere(Vector *v, const double a, LogiVector *l);
/* v[i] = a if l[i] is set */

void
GondzioCorrVectorWhere(Vector *x, Vector *dX, Vector *z, Vector *dZ,
		       Vector *r_xz, Vector *rhs_x, const double barr,
		       const double a1p, const double a1d,
		       LogiVector *l);
/* GondzioCorrVector:  dp = (x+a1p dX)(z+a1d dZ) 
                       dp_i < 1e-1barr => r_xz_i = barr - dp_i
                       dp_i > 1e+1barr => r_xz_i = -5*barr
                       rhs_x_i = -r_xz_i/x_i 
*/

void
GondzioCorrVectorOnlySmallWhere(Vector *x, Vector *dX, Vector *z, Vector *dZ,
				Vector *r_xz, Vector *rhs_x, const double barr,
				const double a1p, const double a1d,
				LogiVector *l);
/* GondzioCorrVector:  dp = (x+a1p dX)(z+a1d dZ) 
                       dp_i < 1e-1barr => r_xz_i = barr - dp_i
                       rhs_x_i = -r_xz_i/x_i 
*/
void
BalanceKronProdVectorWhere(Vector *x, Vector *y,
			   const double barr, LogiVector *l);
/* x[i]*z[i] < barr => min(x,z) = barr/max(x,z) where l is set */

/** Set each element in the first vector to be the maximum of the two vectors
    for the elements where l is set */
void
MaxVectorWhere (Vector *x, Vector *y, LogiVector *l);
/* x[i] = max(x[i], y[i]) where l[i] is set */

/** Set each element in the first vector to be the minimum of the two vectors
    for the elements where l is set */
void
MinVectorWhere (Vector *x, Vector *y, LogiVector *l);
/* x[i] = min(x[i], y[i]) where l[i] is set */

double
ddotVectorParWhere (Vector *x, Vector *y, LogiVector *l);
/* returns scalar product x'y, of entries that have l set */
/* also works in parallel: HostShared only done on root, then summed */

double
ddotLinCombParWhere(Vector *x, Vector *y, Vector *dx, Vector *dy,
		    const double a, const double b, LogiVector *l);
/*     ddotLinCombParWhere   ddot = (x+a*dx)'* (y+b*dy) */

void
InfNormVectorWhere (Vector *x, double *val, int *j, LogiVector *l);
/* Finds the infinity norm of Vector x and position of largest element 
   for those elements for which l is set
   FIXME: each proc returns its own min, position is wrong */

void
MinInfNormVectorWhere (Vector *x, double *val, int *j, LogiVector *l);
/* Finds min(abs(x[i])) and position of smallest element for those elements 
   for which l is set
   FIXME: each proc returns its own min, position is wrong */

void
MinCompVectorWhere(Vector *x, double *val, int *j, LogiVector *l);
/* Finds min(x[i]) and position of smallest element for those elements 
   for which l is set
   FIXME: each proc returns its own min, position is wrong */

void
MaxCompVectorWhere(Vector *x, double *val, int *j, LogiVector *l);
/* Finds max(x[i]) and position of largest element for those elements 
   for which l is set
   FIXME: each proc returns its own min, position is wrong */

void
FuncVectorWhere (Vector *x, KrnFct f, LogiVector *l);
/* x[i] = f(x[i]) if l[i] is set */

/* Sets the vector x = y/(1+y*z), which has the effect of capping x at 1/z
*/
void
getThetaVectorWhere (Vector *x, Vector *y, Vector *z, LogiVector *l);


#endif
