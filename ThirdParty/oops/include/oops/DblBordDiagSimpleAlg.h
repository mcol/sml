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
 *  @file DblBordDiagSimpleAlg.h
 *
 *  @author Andreas Grothey
 *
 *  Definition of DblBordDiagSimpleMatrix.
 *
 */

#ifndef DBLBORDDIAGSIMPLEALG_H
#define DBLBORDDIAGSIMPLEALG_H


/* Forward declarations */
class Algebra;
class Tree;


/* DblBordDiagAlgebra:

   Defines a simple Double Bordered Algebra

   A   = [D_1          C_1  ]
         [    ...           ]
         [        D_n  C_n  ]
	 [B_1 ... B_n  D_n+1]

 */  


/** A DblBordDiagSimpleMatrix */
typedef struct {

  /** Array of nb_block+1 diagonal algebras */
  Algebra  **D;

  /** Array of nb_block bottom border algebras */
  Algebra  **B;

  /** Array of nb_block right border algebras */
  Algebra  **C;

  /** Number of blocks */
  int      nb_block;

  /** Number of rows */
  int      nb_row;

  /** Number of columns */
  int      nb_col;

  /** First column of each block in the matrix (array of nb_block+2 entries) */
  int      *col_beg;

  /** First row of each block in the matrix (array of nb_block+2 entries) */
  int      *row_beg;

  /** Name of the matrix */
  char     *name;
  
} DblBordDiagSimpleMatrix;

Algebra *
NewAlgebraDblBordDiag(const int nb_block, Algebra **B, Algebra **C,
		      Algebra **D, const char *name);

DblBordDiagSimpleMatrix *
NewDblBordDiagSimpleMatrix(const int nb_block, Algebra **B, Algebra **C,
			   Algebra **D, const char *name);

Algebra *
NewDblBordDiagSimpleAlgebra(DblBordDiagSimpleMatrix *M);

int
DblBordDiagSimpleSetStructure(Algebra *A, int begrow, int begcol,
			      const int level, Tree *Tcol, Tree *Trow,
			      const int copy);

Algebra *
DblBordDiagSimpleMakeAugmentedSystem(Algebra *A, Algebra *Q);

Algebra *
DblBordDiagSimpleMakeAugmentedSystemNoMem(Algebra *A, Algebra *Q);

#endif /* DBLBORDDIAGSIMPLEALG_H */
