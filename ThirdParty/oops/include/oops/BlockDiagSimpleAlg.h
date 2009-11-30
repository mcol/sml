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
 *  @file BlockDiagSimpleAlg.h
 *
 *  @author Andreas Grothey
 *
 *  Definition of BlockDiagSimpleMatrix.
 *
 */

#ifndef BLOCKDIAGSIMPLEALG_H
#define BLOCKDIAGSIMPLEALG_H

#include "oops/Algebra.h"

/* BlockDiagSimpleAlgebra:

   Defines a simple Diagonal Algebra

   A   = [D_1          ]
         [    ...      ]
         [        D_n  ]

 */  

/** A BlockDiagSimpleMatrix */
typedef struct {

  /** Array of diagonal algebras */
  Algebra  **D;

  /** Number of blocks */
  int      nb_block;

  /** Number of rows */
  int      nb_row;

  /** Number of columns */
  int      nb_col;
  /** First column of each block in the matrix (array of nb_block+1 entries) */
  int      *col_beg;

  /** First row of each block in the matrix (array of nb_block+1 entries) */
  int      *row_beg;

  /** Name of the matrix */
  char     *name;
  
} BlockDiagSimpleMatrix;

Algebra *
NewAlgebraBlockDiag(const int nb_block, Algebra **D, const char *name);

BlockDiagSimpleMatrix *
NewBlockDiagSimpleMatrix(const int nb_block, Algebra **D, const char *name);

Algebra *
NewBlockDiagSimpleAlgebra(BlockDiagSimpleMatrix *M);

#endif /* BLOCKDIAGSIMPLEALG_H */
