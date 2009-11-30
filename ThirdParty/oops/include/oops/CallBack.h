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
 *  @file CallBack.h
 *
 *  Definition of CallBackInterface.
 *
 *  CallBackInterface is used by an Algebra to ask the constructor for its
 *  Sparse data.
 *
 *  The Algebra is constructed with an id of a form decided by the constructor,
 *  this is passed as a void* to the Algebra.
 *
 *  When the algebra is asking for its data, it sets up a CallBackInterfaceType
 *  object with ->id set to the id passed by the constructor.
 *
 *  If ->row_nbs, ->col_beg, ->col_len are not NULL these arrays are filled 
 *  with the Sparse matrix data, otherwise just the number of nonzeros is 
 *  returned in ->nz.
 */

#ifndef CALLBACK_H
#define CALLBACK_H

typedef struct {

  /* INPUT: */
  void *id;
  int max_nz;

  /* OUTPUT */
  int nz;
  int *row_nbs;
  int *col_beg;
  int *col_len;
  double *element;

} CallBackInterfaceType;


typedef void (*CallBackFunction) (CallBackInterfaceType *cbi);

void 
CallBackVoid(CallBackInterfaceType *cbi);

#endif /* CALLBACK_H */
