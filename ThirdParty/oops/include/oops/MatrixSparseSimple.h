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
 *  @file MatrixSparseSimple.h
 *
 *  @author Andreas Grothey
 *
 *  Definition of SparseSimpleMatrix.
 *
 */

#ifndef SPARSESIMPLEMATRIX_H
#define SPARSESIMPLEMATRIX_H

#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>  
#include "oops/Vector.h"
#include "oops/Algebra.h"
#include "oops/CallBack.h"


/** Row wise sparsity structure */
typedef struct {

  /** Row links: next element in same row, or -1 (mx_nb_el) */
  int *links;

  /** Row headers: first element in this row (nb_row+1) */
  int *headers;

  /** Column indices (mx_nb_el) */
  int *indices;

  /** Number of nonzeros in this row (nb_row) */
  int *row_len;

} row_st;

/** A SparseSimpleMatrix */
typedef struct {

  /** Number of rows */
  int nb_row;

  /** Number of columns */
  int nb_col;

  /** Number of nonzero elements */
  int nb_el;

  /** Size of element arrays */
  int max_nb_el;

  /** Array of nonzero elements */
  double *element;               

  /** Row indices */
  int *row_nbs;

  /** Column begin */
  int *col_beg;

  /** Column length */
  int *col_len;

  /** Row-wise sparsity structure */
  row_st *rows;

  /** Name of the matrix */
  char *name;

  /** Whether the row-wise structure has been initialized */
  int row_wise;

  /** The starting index of the arrays */
  int startnb;

  /** Function to call to fill in Sparse data (if deferred) */
  CallBackFunction cbf;

} SparseSimpleMatrix;

/*========================== A useful macro  ===============================*/
#define forall_elt(A, row_pt, col) \
  for (col = 0; col < A->nb_col; col++)\
      for (row_pt = A->col_beg[col];\
           row_pt < A->col_beg[col] + A->col_len[col];\
           row_pt++)

SparseSimpleMatrix*
NewSparseZeroMatrix(const int row_nbs_sz, const int col_nbs_sz);

/** Define a SparseSimpleMatrix but don't allocate any memory for it */
SparseSimpleMatrix*
NewSparseMatrixNoMem(const int row_nbs_sz, const int col_nbs_sz,
		     CallBackFunction f, const char *name);

/** Allocate and define a SparseSimpleMatrix */
SparseSimpleMatrix*
NewSparseMatrix(const int row_nbs_sz, const int col_nbs_sz,
		const int element_sz, const char *name);

/** Constructor wrapper for an Algebra representing a SparseSimpleMatrix
    with given dimensions and CallBackFunction */
Algebra *
NewAlgebraSparse(int nrow, int ncol, const char *name, 
		 CallBackFunction f, void *id);

/** Free the space allocated for a SparseSimpleMatrix */
void
FreeSparseMatrix (SparseSimpleMatrix* N);

/** Allocate a row_wise structure */
void
row_wise (SparseSimpleMatrix * A);

/** Free a row_wise structure */
void
free_row_wise (SparseSimpleMatrix * A);

int
resize (SparseSimpleMatrix * A, int new_size);

Algebra *
NewSparseSimpleAlgebra(SparseSimpleMatrix * M);

void
PrtSparseMtxMatlab(FILE *out, SparseSimpleMatrix * A, const char *name);

/** Add two sparse matrices, the second multiplied by the scalar f */
void 
AddSparseSimpleMatrix(SparseSimpleMatrix *A, SparseSimpleMatrix *B, double f);

#ifdef WITH_MPI
void
SchurSumSparse(SparseSimpleMatrix *M, MPI_Comm comm);
#endif

#endif /* SPARSESIMPLEMATRIX_H */
