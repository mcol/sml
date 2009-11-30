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
 *  @file SparseVector.h
 *
 *  Definition of SparseVector.
 */

#ifndef SPARSEVECTOR_H
#define SPARSEVECTOR_H

#include "oops/DenseVector.h"

/** SparseVector */
typedef struct {

  /** Dimension of the vector */
  int dim;

  /** Nonzero entries */
  double *entries;

  /** Indices of the nonzero entries */
  int *indices;

  /** Number of nonzero entriess */
  int nb_entries;

  /** Maximum number of nonzero entries */
  int max_entries;

  /** Name of the vector */
  char *name;

} SparseVector;

/* Used for Sparse Calculations within the Algebras */

/* --------------------- constructors/destructors -------------------------- */

/** Create a SparseVector and allocate arrays */
SparseVector *
NewSparseVector(const int dim, const char *name, const int max_nb_el);

/** Free the memory occupied by SparseVector */
void
FreeSparseVector (SparseVector *V);

/** Resize indices/entries of the SparseVector (losing the old information) */
void 
SparseVectorResize(SparseVector *v, const int new_size);

/** Print the SparseVector using the given format for the nonzeros */ 
void
PrintSparseVector(FILE *out, SparseVector *V, const char *format);

/* ------------------ DenseVector copy functions --------------------------- */

/** Fill the SparseVector from a DenseVector (Sparse exists already) */
void
FromDenseVect (DenseVector *dense, SparseVector *sparse);

/** Copy the SparseVector to a DenseVector of same dimension
    (dense is 0'd first) */
void
ToDenseVect (DenseVector *dense, SparseVector *sparse);

/** Copy the SparseVector to a DenseVector (without zeroing it first) */
void
UnpackSparseVector (SparseVector *sparse, DenseVector* dense);

/** Create zeros in DenseVector corresponding to nonzero entries in 
    SparseVector */
void
CleanSparseVector (SparseVector *sparse, DenseVector* dense);

/* ----------------------------- others ------------------------------------ */

/** Copy the nonzeros from a SparseVector (the copy must have enough space
    allocated) */
void
CopySparseVector (SparseVector *V, SparseVector *CopyV);

/** Compute the scalar product between a sparse and a dense vector */
double
ScalPrSparseVector (SparseVector *sparse, DenseVector *dense);

void
daxpySparseDense(SparseVector *sparse, DenseVector *dense, const double a);
/* dense[i] += a*sparse[i] everywhere where sparse has entries */

#endif /* SPARSEVECTOR_H */
