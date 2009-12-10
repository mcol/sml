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
 *  @file Tree.h
 *
 *  Definition of Tree.
 */

#ifndef INCTREE_H
#define INCTREE_H

typedef enum {HostNotSet=0, HostSingle, HostAllShared, HostComplex} host_stat_type;

struct Algebra;

#include <stdio.h>
#include "oops/parutil.h"


#define   ON_EVERY_HOST         -1
#define   ID_TREE_NOT_SET       -99


/** A Tree */
class Tree {
 public: 

  /** Array of child nodes */
  Tree **sons;

  /** Number of child nodes */
  int nb_sons;

  /** Initial index covered by this node */
  int begin;

  /** Last index covered by this node */
  int end;

  /** A number associated with each node of the tree */
  int index;

  /** Total number of nodes = max(index) +1  (only root) */
  int nb_nodes;


  /** = 1 if dense vector information is available at this node.
      Set for all leaves in serial mode, local leaves in parallel mode */
  bool local;

  /** Pointer to next local node above this node  
      if the vector is a subvector of a local vector,
      this is where dense info can be found
      NOT SUPPORTED (?)
      null otherwise */
  Tree *above;
  
  /** Pointer to corresponding  diagonal node on the Algebra Tree
   *  When the Tree is defined this pointer points back to the Algebra Tree  
   *  node that generated this node in the tree.
   *  It is used to give further identification for a node in the tree
   *  (which is used by the Vector CallBack functions in the setup 
   *  procedure); */
  struct Algebra *nodeOfAlg;

#ifdef WITH_MPI
    /* There are three ways of parallel communications in OOPS:

       - MPI_Allreduce on all the processors:
            This is done for MAX and MINs in hopdm_qp.c and also for
            SUM in Vector.ddotPar to sum up the processors contributions
	    to the dot-product. In this case ddotPar makes sure that
	    contributions of shared nodes are only couted once (i.e.
	    are only performed on ROOT processor)

       - bcast_type (for HostSingle nodes):
            Defines a stencil over all elements in a big double[] array
	    that are local to this processor below (and including) the
	    current node. 
	    => Used in CopyToDenseVector: All processors broadcast their local
	       elements once the Vector has been converted to DenseVector
	    => Only used in problem construction and for debugging purposes

       - share_first/share_last (forHostAllShared nodes):
            pointers to an (invisible) depth-first list of all HostAllShared
            nodes. *(share_first), *(share_first+1),...,*(share_last)
	    are all HostAllShared nodes below (and including) the current
	    node (same for all processors).
	    => Used in ReduceVector: to do an operation (usually SUM) on
	       all HostShared nodes.
    */
    int id;    /* processor assigned to this node (if local and HostSingle)*/
    int id_first; /* if HostAllShared or not local: first and last processor */
    int id_last;  /*    that share this node                                 */
    MPI_Comm comm; /* communicator that has all the processors from id_first
		      to id_last in it: 
		      if id_first=id_last=id    then comm = MPI_COMM_SELF
		      if id_first=0,id_last=N-1 then comm = MPI_COMM_WORLD   */
#ifndef NOOLD
    short *single_first; /* pointer to list of HostSingle nodes on this 
			    processor and below (and including) current node */
    short *single_last; /* pointer to last element of list of HostSingle nodes 
		   on this processor and below (and including) current node. */
#endif
    short *share_first; /* pointer to list of HostShared nodes 
			   below (and including) current node. */
    short *share_last; /* pointer to last element of list of HostShared nodes 
			  below (and including) current node.  */
    int   share_sz;    /* total number of elements in HostShared nodes 
			  below (and including) current node  */

    host_stat_type host_stat; /* one of HostNotSet, HostSingle, HostAllShared
	    HostSingle: this node (and all below) belongs to one proc 
			id must be set in this case 
	    HostShared: this node (all all below) are shared
	                same info everywhere, or needs to be gathered
	    HostNotSet: none of the above (node will still split)	*/

    MPI_Datatype *bcast_type;   /* [nb_proc] Stencil over dense double array 
	     of all elements in HostSingle nodes local to processor i, 
	     below (and including) current node. */
#ifndef NOOLD
    MPI_Datatype reduce_type;  /* Stencil over dense double array of 
		 all elements in HostShared nodes 
		 below (and including) current node. */
    int bufsize;     /* size of the buffer for Allreduce (NOT USED) */
#endif
#endif
    /* reduce type is never used (should be used in 
            Vector2/CopyToDenseReduceVector called by Vector2/CopyToDenseVector
	    but not implemented)
       bcast_type is used in Vector2/CopyToDenseVector!  
                          and tree/DoubleBcast (which itself is redundant)*/
    /* single_* are never used
       share_* are used in Vector2/ReduceVector which IS USED */

    
    /* ---------------------- constructors/destructors --------------------- */

    /** Allocate and set a node in the tree */
    Tree(int begin = -1, int end = -1, int nbsons = 0);
    
#ifdef REDUNDANT
    Tree *
      NewDenseTree (int nb_sons, int mainbeg, int mainend, int *begin, int *end);
    /* Allocates (sons/bcast_type) and initialises 
       (nb_sons/begin/end/index/local)
       this node and all its sons (but no further) */
    /* REDUNDANT: used in msnd/MultflowSparse (red), drivers/test, 
                  lpsolver/hmain*/
    
    Tree *
      NewTreeOfTree (int nb_sons, Tree **Ti, int begin, int end);
    /* NOT USED: Allocates (sons/NOT bcast_types) and initialises 
       (nb_sons/begin/end/local/index) this node which has trees Ti as sons */
#endif /* REDUNDANT */

    /* frees sons arrary and node itself recursively 
       DOES NOT free bcast_type array */
    ~Tree();

    /* ----------------------- Utility functions --------------------------- */

    /** Print a Tree recursively depth-first, from front */
    void print(FILE *out, const char *name);
    
    
    /** Write the Tree to a file in format required by ReadTree() */
    int writeToFile(FILE*);

    /** Generate a Tree by reading it from a file */
    static Tree* readFromFile(FILE *f);
    /* Reads tree from file: beg, end, local, index, nb_sons
       depth first, from front */
    
    void check();
    /* check that division of indices (begin/end) matches in the tree */
    /* uses asserts and warns if node has zero block (begin==end) */

    /** Check whether two trees are identical, by recursively comparing
	nb_sons, begin, end. */
    bool isIdentical(Tree *T2);

    /* --------------------- Initialization functions ---------------------- */

    void setIndex();
    /* Called with root node: set index of nodes (depth first (from back)),
       above and t->nb_nodes (only for root) */
    /* Called from Init(Par)Algebras */


    void setLeavesLocal(Tree *Parent);
    /* Sets local=1 in all leaves (and if already marked local=1) - 
       sets local=0 elsewhere) and above if a node above is local
       Called from Init(Par)Algebras */ 

#ifdef WITH_MPI
    void setProcLocal();
/* Sets local==1 nodes to local=0 if they don't live on this processor
   Assumes that LeavesAreLocalTree and AllocateProcessors have been called 
   already, i.e. fields ->local, ->id_first, ->id_last are set              */

    void setAllNodesBelow(bool local, int id, host_stat_type host_stat);
    /* Sets the ->local, ->id, ->host_stat, ->above entries of this node and 
       all nodes below: 
       Called from DelayExecute: Delay specifies processor splits on top
       level, this function sets them on all lower levels */

    void setParBcastTypes();
    /* Assumes host_stat is set in all nodes +id is set in all HostSingle nodes
       */
    /* Sets: bcast_type[nb_proc], reduce_type 
       single_first, single_last, share_first, share_last, share_sz */

    void setParBcastTypesNew();

#endif
};







#ifdef REDUNDANT
int
CorrectTree (Tree *T);
/* checks that division of indices (begin/end) matches in the tree */
/* returns 1 if correct, 0 otherwise */
/* REDUNDANT: never called, use CheckTree instead */
#endif /* REDUNDANT */

/*
static int
NbNodesTree(Tree *T) */
/* Counts number of nodes in the tree */



/* ---------------------------- others ------------------------------------- */

#ifdef OBSOLETE
void
DoubleBcast(Tree *T, double *y);
/* y is a double array shared between processors: 
   each proc is broadcasting its local bits to all other processors */
/* NOT USED ANYMORE */
#endif /* OBSOLETE */

int
comp_nodes(Tree *T1, Tree *T2);
/* true if both T1 & T2 are local */
/* Only used in BlockAngAlgebra/ComputeSumBXXBi */

#endif



/* Others: CyclicSetHost : Never Used
           LocalLeafesTree: Never Used (same as LeavesAreLocal)
	   NbNodesTree:    called by SetParBcastTypes (counts all nd in tree)
	   SetHostTree:    NeverUsed (sets T->id = host)
	   TreeDepth
 */
