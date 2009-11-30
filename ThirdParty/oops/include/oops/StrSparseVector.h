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
 *  @file StrSparseVector.h
 *
 *  Definition of StrSparseVector.
 *
 *  This is a sparse implementation of the Vector object.
 *  Its main purpose is to copy a Vector to a StrSparseVector and then
 *  do the following operations sparse.
 *
 *  DOES NOT CURRENTLY SUPPORT PARALLELISM!
 *
 *  StrSparseVector has problem with above nodes. Nodes below local nodes
 *  currently simply DO NOT EXIST! (i.e allocating stops at local nodes).
 */

#ifndef STRSPARSEVECTOR_H
#define STRSPARSEVECTOR_H

#include "oops/DenseVector.h"
#include "oops/Vector.h"
#include "oops/SparseVector.h"


/*-----------------------------------------------------------------------------
        Functions defined on the anonymous vector structure
-----------------------------------------------------------------------------*/

/** A structured SparseVector */
typedef struct StrSparseVector {

  /** Node corresponding to this subvector */
  Tree *node;

  /** Array of all subvectors (same at all nodes) */
  struct StrSparseVector **subvectors;

  /** Memory for this node (not always allocated) */
  SparseVector *sparse;

} StrSparseVector;


/*-----------------------------------------------------------------------------
        Macros
-----------------------------------------------------------------------------*/
#define OuterProdVector(outpd, NonzCC,CompCol,c)  OuterProdDenseVector (GetDenseVectorFromVector (outpd), NonzCC, CompCol, c)
#define NbOfSubVect(v)  ((v->node)->nb_sons)
#define GetSparseFromStrSparseVector(v)  v->subvectors[(v->node)->index]->sparse
#define SubVector(v,i) v->subvectors[v->node->sons[i]->index]

/*-----------------------------------------------------------------------------
        Prototypes
-----------------------------------------------------------------------------*/

/* -------------------- constructors and destructors ----------------------- */

StrSparseVector *
NewStrSparseVector(Tree *T, const char *name);
/*  Allocate & initialise StrSparseVector following tree:
      local nodes          have sparse allocated
      above nodes          have sparse = null 
      non-local nodes      have sparse = null 
*/

void
CopySparseToDenseVector (StrSparseVector *x, DenseVector *y);
/* Copy from the appropriate local||above nodes of StrSparseVector to 
   DenseVector
   Does NOT DO reduce/bcast */

void
CopyDenseToSparseVector (DenseVector *x, StrSparseVector *y);
/* Copy DenseVector to StrSparseVector (hoping enough space allocated in
   StrSparseVectors nodes.*/

void
CopyStrSparseVectorToVector (StrSparseVector *x, Vector *y);
/* Copy StrSparseVector to Vector (actually unpacks: Vector is not zeroed) */

void
CopyVectorToStrSparseVector(Vector *x, StrSparseVector *y);
/* Copy Vector to StrSparseVector (assumes StrSparseVector is big enough) */

/** Compute the outer product V'D^{-1}V */
void
OuterProdStrSparseVector(int n, StrSparseVector **v, Vector *d, double **c);

/** Add a multiple of StrSparseVector x to Vector y */
void 
daxpyStrSparseVectorToVector(StrSparseVector *x, Vector *y, double a);

void
InfNormStrSparseVector (StrSparseVector *x, double *val, int *j);
/* NOT IMPLEMENTED YET: ONLY TEMPLATE */

/** Recursively free the memory allocated for the StrSparseVector */
void
FreeStrSparseVector (StrSparseVector *V);

/** Print a StrSparseVector */
void
PrintStrSparseVector (StrSparseVector  *y);

StrSparseVector *
NewCopy_StrSparseVector (StrSparseVector *orig);
/* Makes (new) copy of orig, reducing sizes as necessary */

void
UnpackStrSparseVector (StrSparseVector *sparse, DenseVector* dense);
/* Copy sparse to dense, overwriting only element which are listed in sparse */

void
PackStrSparseVector(StrSparseVector *v);
/* reallocate space, so that no more is allocated than needed */

void
CleanStrSparseVector (StrSparseVector *sparse, DenseVector* dense);
/* Zero all elements of dense which are listed in sparse */ 

void
ZeroStrSparseVector (StrSparseVector *v);
/* Sets sparse->nb_entries = 0 in all local||above nodes */

double
ScalPrStrSparseVector (StrSparseVector *v, DenseVector* dense);
/* returns scalar product v'dense, does NOT WORK IN PARALLEL */ 

/** Count the elements of a StrSparseVector. */
int
StrSparseVectorCountElements(StrSparseVector *x);

/** Returns the scalar product of a StrSparseVector and a Vector */
double
ddotStrSparseVectorPar (StrSparseVector *x, Vector *y);

SparseVector*
GetSparseVectorFromStrSparseVector (StrSparseVector *x);
/* Gets pointer to the SparseVector inside a local node of StrSparseVector
   FIXME: could be done by macro */

void
StrSparseVector2SparseVector(StrSparseVector *x, SparseVector *y);
/* Copy StrSparseVector to SparseVector checking that spV has enough entries */

void
PackSparseVectorFromStrSparseVector (StrSparseVector *V, SparseVector *spV);
/* Copy StrSparseVector to SparseVector hoping that spV has enough entries */
/* Does not do recduce/bcast in parallel */


#ifdef WITH_MPI
void
SharedSparse(StrSparseVector **sparse, int nb_vect);
/* NOT PROPERLY IMPLEMENTED: NOT USED */

#endif

#endif /* STRSPARSEVECTOR_H */
