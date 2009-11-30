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
 *  @file BlockDenseAlg.h
 *
 *  Definition of a Block Dense Algebra.
 *
 *  A Block Dense Algebra consists of an n x m array of block matrices 
 *  (all block matrices are set).
 *  Substitutes the old BlockRow and BlockCol algebras.
 *
 *  @author Andreas Grothey
 *
 */

#ifndef BLOCKDENSEALG_H
#define BLOCKDENSEALG_H

#include "oops/Algebra.h"

typedef struct BlockDenseMatrix_st {
  int      nb_row;
  int      nb_col;

  int      nb_block;     /* total number of blocks      */
  int      nb_blk_col;   /* number of columns of blocks */
  int      nb_blk_row;   /* number of rows of blocks    */
  
  int      *start_blk_col; /* [nb_blk_col+1] ix of first col in blk-col */  
  int      *start_blk_row; /* [nb_blk_row+1] ix of first row in blk-row */  

  Algebra  **B;       /* List of Blocks (row-wise) */
  char*    name;

} BlockDenseMatrix;

/** Allocate space and initialize a BlockDenseMatrix. */
BlockDenseMatrix *
NewBlockDenseMatrix(const int nb_blk_row, const int nb_blk_col,
		    Algebra **B, const char *name);


Algebra *
NewAlgebraBlockDense(const int nb_blk_row, const int nb_blk_col,
		     Algebra **B, const char *name);

Algebra *
NewBlockDenseAlgebra(BlockDenseMatrix *M);

#endif /* BLOCKDENSEALG_H */
