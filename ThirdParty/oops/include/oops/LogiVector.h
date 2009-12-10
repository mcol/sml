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

class LogiVector;

#include "oops/Vector.h" 

typedef double (*KrnFct) (double);

/** A dense Vector consisting of 0/1 values */
class DenseLogiVector {

 public:

  /** Dimension of the vector */
  int dim;

  /** Set to 1 if memory allocated, 0 otherwise */
  bool mem_alloc;

  /** Not used */
  bool is_zero;

  /** Array of elements */
  bool *elts;

  /** Name of the vector */
  char *name;

  /** Not used */
  int newtag;

  /* ------------------- constructors and destructors ---------------------- */
  
  /** Create a DenseLogiVector */
  DenseLogiVector(const int vdim, const char *vname);

  /** Create a DenseLogiVector that is a portion (begin, begin+dim) of another 
      DenseLogiVector sharing its memory */
  DenseLogiVector(DenseLogiVector *V, const int begin, const int vdim);
  
  /** Free the space allocated for a DenseLogiVector */
  ~DenseLogiVector ();

};

/** A structured Vector consisting of 0/1 values */ 
class LogiVector
{
 public:
  /** Node corresponding to this subvector */
  Tree *node;

  /** Array of all subvectors (same at all nodes) */
  LogiVector **subvectors;

  /** Memory for this node (not always allocated) */
  DenseLogiVector *dense;
    
  /** Indicator if this is the root of the allocation: 
      on this node ->subvector is allocated (and needs to be freed) */
  bool is_root;

#ifdef WITH_MPI
  /** Not used */
  CompStatus proc;

  /** Not used */
  ContStatus sum_stat;
#endif
  /* ------------------- constructors and destructors ---------------------- */

  LogiVector();

  /** Create a LogiVector and allocate space in the correct subnodes */
  LogiVector(Tree *T, const char *name);

  /** Free the space allocated for a LogiVector */
  ~LogiVector ();

/*---------------- Setting LogiVectors from Vectors ------------------------ */

/** Set an element of the LogiVector to true if the corresponding entry in the
    Vector is smaller than the given bound */
  void setWhereSmaller(Vector *v, const double bound);
  /* Sets Vector[i]<bound? Logivector[i] = 1:0;   */
  
  /** Set an element of the LogiVector to 1 if the corresponding entry in the
      Vector is greater than the given bound */
  void setWhereLarger(Vector *v, const double bound);
  /* Sets Vector[i]>bound? Logivector[i] = 1:0;   */

  /** Set an element of the LogiVector to 1 if the corresponding entry in the
      Vector v1 is is greater than the entry in Vector v2 */
  void setWhereLarger(Vector *v1, Vector *v2);
  /* Sets V1[i]>V2[i]? Logivector[i] = 1:0;   */

  /* ---------- methods for simple manipulation of LogiVectors ------------ */

  /** Return the number of elements set to 1 (works also in parallel) */
  int count();

  /** Set the LogiVector l2 to the values in l1 */
  void copyTo(LogiVector *l2);

  /** Set the LogiVector l1 if l2 AND l3 */
  void setToAnd(LogiVector *l2, LogiVector *l3);

  /** Set the LogiVector l1 if l2 OR l3 */
  void setToOr(LogiVector *l2, LogiVector *l3);

  /** Negate the LogiVector */
  void negate();

  /** Print the LogiVector node by node, one entry per line */
  void print();
};


#endif
