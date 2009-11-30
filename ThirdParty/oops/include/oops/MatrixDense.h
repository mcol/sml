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
 *  @file MatrixDense.h
 *
 *  Definition of DenseMatrix.
 *
 *  @author Andreas Grothey
 *
 */

#ifndef MATRIXDENSE_H
#define MATRIXDENSE_H

#include <stdio.h>
#include "oops/DenseVector.h"
#include "oops/Vector.h"
#include "oops/Algebra.h"

/* MatrixDense supports two kinds of Dense Matrix:
    - Augmented System, quasi-definite 
    - positive definite

Matrix ->v[][] gets overwritten with its Cholesky factors
    - AugSys: L, D, Lt stored in matrix
           SolveL, SolveD, SolveLt
    - PosDef: L, Lt stored in matrix, diagonal stores sqrt_diag
          only SolveL, SolveLt
*/

/** Types of dense matrix */
typedef enum {
  DTNotAType = 0,
  AugSystem_type,
  PosDef_type 
} DenseType;

typedef struct
{
  int mA, nA;        /* dimensions of original matrix (making up Aug) */
  int *perm;         /* pointer to list 0, 1, 2, 3,   n for use in qnmfct */
}
DenseCholeskyData;

/** A dense matrix */
typedef struct {
 	double **v;
	int  nb_row;
	int  nb_col;
        char *name;
        DenseType MtxType;
        DenseCholeskyData *Chol;
} DenseMatrix;


DenseMatrix *
NewDenseMatrix(const int nb_row, const int nb_col, const char *name);

DenseMatrix*
NewDenseAugMatrix(const int nb_row, const int nb_col,
		  const int nA, const char *name);

void
PrtDenseMtx(FILE *out, DenseMatrix *A, const char *elt_format);

void
DenseMtxVectProd (Algebra *A, Vector *x, Vector *y);

void
DenseMtxTrVectProd (Algebra *A, Vector *x, Vector *y);

void
DenseComputeAXAt (DenseMatrix *A, Vector *X, DenseMatrix *AAt);

void
DenseComputeCholeskyUpper (Algebra* A, FILE *out, double* primal_reg, 
			   double* dual_reg);

void
DenseSolveL(Algebra *A, Vector *rhs, Vector *sol, FILE* out);

void
DenseSolveLt(Algebra *A, Vector *rhs, Vector *sol, FILE* out);

void
SetToZero(DenseMatrix* M);

void
DensePrintLargestElement(Algebra *A);

void 
PlusBBt(DenseMatrix* B, DenseVector* X, DenseMatrix* C);

#ifdef WITH_MPI
void
SchurSumDense(DenseMatrix *M, MPI_Comm comm);
#endif

void 
DenseMtxFree(DenseMatrix *M);

Algebra* 
NewAlgMatDense(DenseMatrix *M, const char *name);

void DensePlusBBt(DenseMatrix* B, DenseVector* X, DenseMatrix* C);

void DensePrintMatlab (DenseMatrix *M);

DenseCholeskyData*
NewDenseCholeskyData(const int n, const int m);

#endif /* MATRIXDENSE_H */
