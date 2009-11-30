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
 *  @file ParAlgList.h
 *
 *  @author Andreas Grothey
 *
 */

#ifndef PARALGLIST_H
#define PARALGLIST_H

/** List of parallel algebras */
typedef struct ParAlgList {

  /** Maximum number of Algebras that can be put in the list */
  int max_nb_alg;

  /** Number of Algebras already in the list */
  int nalg;

  /** List of Algebras */
  struct Algebra **A;

} ParAlgList;


/** Constructor */
ParAlgList *NewParAlgList(const int n);

/** Free the space allocated by the structure */
void FreeParAlgList(ParAlgList *alglist);

#endif /* PARALGLIST_H */
