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
 *  @file DenseVector.h
 *
 *  Definition of DenseVector.
 *
 *  DenseVector is a simple implementation of a dense vector.
 *  It is used mainly as a substructure of Vector.
 */

#ifndef DENSEVECTOR_H
#define DENSEVECTOR_H
#include <stdio.h>

#ifndef NAME_FCT
#define NAME_FCT
typedef char*   (*name_fct) (int);
typedef void*   (*mem_fct) (size_t, size_t);
typedef void    (*del_fct) (void *);
#endif

/** A dense Vector */
typedef struct {

  /** Dimension of the vector */
  int dim;

  /** Memory allocated here (instead of pointing into someone else's memory) */
  short mem_alloc;

  /** Flag for a zero vector */
  short is_zero;

  /** Dense array of elements */
  double *elts;

  /** Name of the vector */
  char *name;

} DenseVector;

/* --------------------- constructors/destructors -------------------------- */

/** Create a DenseVector, allocate the space and unset the zero flag */
DenseVector *
NewDenseVector(const int dim, const char *name);

/** Create a DenseVector, using space starting at mem for elements */
DenseVector *
NewDenseVectorMem(const int dim, char *name, double* mem);

/** Create a DenseVector that is a portion (begin, begin+dim) of another 
    DenseVector sharing its memory */
DenseVector *
SubDenseVector(DenseVector *Big, const int begin, const int dim);

#ifdef OBSOLETE
/** Create a DenseVector that is a portion of another DenseVector sharing
    its memory (memory for the actual DenseVector structure is taken from mem)
    NEVER USED: REDUNDANT */
DenseVector *
SubDenseMemVector(DenseVector *Big, const int begin, const int dim, char *mem);
#endif /* OBSOLETE */

/** Create a dense vector by reading it from a file */
DenseVector*
ReadDenseVector(FILE *f);

/** Free the space allocated for a DenseVector (elts is freed only if mem_alloc
    is set) */
void
FreeDenseVector (DenseVector *V);

/* ----------------------- print/write methods ----------------------------- */

/** Write DenseVector to a file in the format needed for ReadDenseVector */
int 
WriteDenseVector(DenseVector *x, FILE *f);

/** Wrapper to call one of the other PrintDenseVector routines */
void
PrintDenseVector(FILE *out, DenseVector *V, const char *format, name_fct name);

/** Print DenseVector 10 entries per line */
void
PrintDense2Vector(FILE *out, DenseVector *V, const char *format, name_fct name,
		  const int begin);

/** Print DenseVector 5 entries per line */
void
PrintDense3Vector(DenseVector *V, const char *format);

/** Print DenseVector 1 entry per line */
void
PrintDense4Vector(DenseVector *V, const char *format, FILE *out);


/* -------------------------- other methods -------------------------------- */

void
OuterProdDenseVector(DenseVector* outpd, int *NonzCC, double* CompCol, 
                   double **c);
/* Calculates c -= outpd*outpd' (only upper triangle): 
   NonzCCm, CompCol are workspace of dim (nonz in outpd) */

void
daxpyDenseVector(DenseVector* x, DenseVector*y, const double a);
/* y += a*x */

#ifdef OBSOLETE
void
SetDenseVector(DenseVector* V, int dim, double *elts);
/* Never used: doesn't work: should wrap a DenseVector around elts[] */
#endif /* OBSOLETE */

#endif
