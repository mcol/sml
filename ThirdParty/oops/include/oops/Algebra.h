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
 *  @file Algebra.h
 *
 *  @author Andreas Grothey
 *
 */

#ifndef ALGEBRA_H
#define ALGEBRA_H

#include "oops/Vector.h"
#include "oops/LogiVector.h"
#include "oops/SparseVector.h"
#include "oops/StrSparseVector.h"
#include "oops/ParAlgList.h"

#ifdef WITH_MPI
#include "oops/Delay.h"
#endif

#define MAXNAME   200


/*=============================================================================
   struct Algebra:
=============================================================================*/
typedef enum {NotAType=0, 
	      BlckAng_type, 
	      SparseSimpleMatrix_type, 
	      SparseAugMatrix_type,
	      FakeAlgebra_type,
	      RnkCor_type,
	      DenseMatrix_type,
              BlockDiagSimple_type,
	      BlockDiag_type,
              DblBordDiag_type,
              BlockRow_type,
              BlockCol_type,
              BlockSparse_type,
              BlockDense_type,
              DblBordDiagSimple_type,
              DenseSimpleMatrix_type, 
              DiagonalSimpleMatrix_type, 
              DenseSimpleAugMatrix_type, 
              LastType
} AlgebraType;

typedef enum {

  /** Value not initialized */
  UnInit=0,

  /** The matrix is completely defined */
  Terminal=1,

  /** The matrix involves other matrices */
  Intermediate=2

} AlgProc;

enum {done=1, not_done=0};

/** Augmented system */
typedef struct {

  /* for the following, no actual allocation happens */
  struct Algebra *A;
  struct Algebra *B;
  struct Algebra *Q;

  /* these will have to be freed */
  double *diag;
  int *mapA;
  int *mapQ;

} AugSystemType;

/** Pointer to a Matrix object: its exact implementation depends on the Type */
typedef void* AlgMatrix;

typedef void (*AlgProdFunc)
     (struct Algebra*, Vector*, Vector*, int add, double factor);
typedef void (*AlgPrint)
     (FILE*, AlgMatrix, const char*);
typedef void (*AlgComputeAXAt)
     (AlgMatrix, Vector*, AlgMatrix);
typedef void (*AlgZeroCols)
     (struct Algebra*, LogiVector *cols);
typedef void (*AlgSetDiag)
     (struct Algebra*, double fact, Vector *diag);
typedef void (*AlgDelCols)
     (struct Algebra*, int*);
typedef void (*AlgScale)
     (AlgMatrix, double*, double*, double*);
typedef void (*AlgScaleVec)
     (AlgMatrix, double*, Vector*, Vector*);
typedef void (*AlgComputeCholesky)
     (struct Algebra*, FILE*, Vector*, Vector*, double reginit);
typedef void (*AlgSolveCholesky)
     (struct Algebra*, Vector*, Vector*, FILE*);
typedef void (*AlgSolveSparseCholesky)
     (struct Algebra*, StrSparseVector*, Vector*);
typedef void (*AlgSolveTriangular)
     (struct Algebra*, Vector*, Vector*, FILE*);
typedef void (*AlgSolveTriangularPart)
     (struct Algebra*, Vector*, Vector*, int first_last);
typedef void (*AlgSolveTriangularDPart)
     (struct Algebra*, Vector*, Vector*, int first, int last);
typedef void (*AlgSolveTriangEj)
     (AlgMatrix, int, Vector*, FILE*);
typedef void (*AlgInverseTriangL)
     (AlgMatrix, SparseVector**, FILE*);
typedef void (*AlgGetColumn)
     (AlgMatrix, int, Vector*);
typedef void (*AlgGetDiag)
     (struct Algebra*, Vector*);
typedef void (*AlgFreeAlgebra)
     (AlgMatrix);
typedef struct Algebra* (*AlgBuildAAt)
     (struct Algebra*);
typedef int (*AlgCountCol)
     (AlgMatrix, int);
typedef void (*AlgGetSparseColumn) 
     (AlgMatrix, int, int, int*, int*, double*);

typedef void (*AlgGetStrSparseColumn) 
     (AlgMatrix *, int col, StrSparseVector*);

typedef void (*AlgSolveSparseL)
     (struct Algebra*, StrSparseVector*, StrSparseVector*);

typedef void (*AlgSolveSparsePart)
     (struct Algebra*, StrSparseVector*, StrSparseVector*,
      int first, int last);

typedef int (*AlgSetStructure)
     (struct Algebra*, int, int, int, Tree*, Tree*, int);

typedef void (*AlgCopyParInfo)
     (struct Algebra*);
typedef void (*AlgFakeOutAlgebra)
     (struct Algebra*, int parent_is_faked);
typedef void (*AlgParProcess)
     (struct Algebra*);

#ifdef WITH_MPI
typedef void (*AlgAllocateProcs)
     (struct Algebra*, int first, int last, ParAlgList *alglist,
      host_stat_type type, int set_tree);
#else
typedef void (*AlgMakeFlatList)
     (struct Algebra*, ParAlgList *alglist);
#endif

typedef void (*AlgAddAugmentedSystemDiag)
     (struct Algebra*, double, Vector*, Vector*);
typedef void (*AlgMakeAQMap)
     (struct Algebra*, int *, int *);
typedef struct Algebra* (*AlgMakeAugmentedSystem)
     (struct Algebra*, struct Algebra*); 
typedef struct Algebra* (*AlgMakeAugmentedSystemUnsym)
     (struct Algebra*, struct Algebra*, struct Algebra*); 

/** Algebra */
typedef struct Algebra {

  /** pointer to Matrix object, exact implementation of Matrix depends
     on the type */
  AlgMatrix                      Matrix;

  /** Name of the Algebra */
  char                           name[MAXNAME];

  /** Algebra identifier */
  void                          *id;

  /** Index of the Algebra in consecutive numbering */
  int                            ix;

  /** Number of constraints */
  int                            nb_row;

  /** Number of variables */
  int                            nb_col;

  /** To exploit sparsity in some SolveL, SolveLt */
  int                            first_nz_row;
  int                            last_nz_row;
  int                            first_nz_col;
  int                            last_nz_col;

  /** Only used by RankCor/BuildAAt: PrimalReg[nb_col] is =1 for the
     added columns of V (this setting is used in hopdm_par!) */
  /* Used in PushVars to as flag (just existence) to allow bigger CompProds  
     Also sets different vpdRegx (which is used for 
                                  theta = theta/(1+theta*pdRegx) */
  int                           *PrimalReg;

  /** Corresponding node in row tree:
      giving structure of a column/ = structure of y vector */
  Tree                          *Trow;

  /** Corresponding node in column tree:
      giving structure of a row/ = structure of x vector  */
  Tree                          *Tcol;

  /** One of UnInit/Terminal/Intermediate */
  AlgProc                        complexity;

  /** Root matrix is 0, every further down is one bigger, set by SetStructure
      only used for debugging purposes */
  /* and in matrix/DblBordDiagAlg.c to determine whether a parallel reduce
     should be done at end of MatxVectProd: done is level==0
     THIS IS A BUG!                                                         */
  int                            level;

  /** One of NotAType/BlckAng_type/Sparse_matrix_type/FakeAlgebra_type,
      RnkCor_type/Dense_matrix_type:
      Only FakeAlgebra_type is currently tested for (and this should probably 
      be avoided: idea is Fake doesn't do anything, without testing)  */
  AlgebraType                    Type;

  /** True if the Algebra represents an augmented system structure */
  int                            is_augmented;

  /** Only allocated for Augmented System Matrix created by MakeAugSystem
      points to extra information for Augmented System */
  AugSystemType                 *AugSystem;

#ifdef WITH_MPI
  /** First processor allocated to this Algebra */
  int                             first_proc;

  /** Last processor allocated to this Algebra */
  int                             last_proc;

  /** Communicator over [first_proc,last_proc] */
  MPI_Comm                        comm;

  /** One of HostNotSet, HostSingle, HostAllShared, HostComplex:
      - HostSingle: this node (and all below) belongs to one processor
                   (id must be set in this case)
      - HostShared: this node (all all below) are shared (same info everywhere)
      - HostComplex: none of the above (node will still split) */
  host_stat_type                   host_stat;
#endif


  /* ------------------------   METHODS:    --------------------------------*/
  /* Basic Stuff : */
	AlgPrint                       Print;
  /* Print(FILE *f, AlgMatrix *M, char *format):
     prints Matrix on f using format */
        AlgProdFunc                    MatrixTimesVect;
  /* MatrixTimesVect(AlgMatrix *M, Vector *x, Vector *y, int add, 
                     double fact):  
     y = Mx; , if (add) => y += fact* Mx */
        AlgProdFunc                    MatrixTransTimesVect;
  /* MatrixTimesTransVect(AlgMatrix *M, Vector *x, Vector *y, int add,
                          double fact): 
     y = M'x, if (add) => y += fact* M'x */
        AlgFreeAlgebra                 FreeAlgebra;
  /* FreeAlgebra(AlgMatrix *M):
     Free all objects Associated with this Matrix */
        AlgGetColumn                   GetColumn;
  /* GetColumn(AlgMatrix *M, int col, Vector *x):
     copies the column col of M into x (Assuming trees aggree) */
        AlgGetColumn                   GetRow;
  /* GetRow(AlgMatrix *M, int row, Vector *x):
     copies the row-th row of M into x (Assuming trees aggree) */
        AlgGetSparseColumn             GetSparseColumn;
  /* GetSparseColumn(AlgMatrix *M, int col, int start, int *nb_nz, 
                             int *indices, double *entries):
     Gets all nonzero elements of column col and appends them to the sparse 
     information in indices[], entries[]. nb_nz is updated accordingly.
     start is an offset, the local Matrix row-indices are offset by start
     before being enterd in indices[].
     has to be called with col=0,1,2,3,..*/
  /* used by makemps.c, scale.c, drawmatrix.c */
        AlgGetSparseColumn             GetSparseRow;
  /* Analogue to GetSparseColumn: returns a row of the Matrix in 
     a sparse structure. Takes care to return the row SORTED! */
        AlgGetDiag                     GetCholDiag;
  /* Get the diagonal of the Cholesky Factor (D) as a Vector 
     Needed by the sp3CompOuterProdSchur in DblBordDiag.ComputeCholesky  */
        AlgCountCol                    CountNzCol;
  /* Counts the nonzeros in the specified col */
  /* For augmented system matrices, the possible extra diagonal element from
     qdiag is NOT counted, the one from the bottom left diagonal (ydiag) IS
     counted if ydiag is allocated! */
        AlgCountCol                    CountNzRow;
  /* Counts the nonzeros in the specified row */
        AlgGetStrSparseColumn          GetStrSparseColumn;
        AlgGetStrSparseColumn          GetStrSparseRow;
  /* GetStrSparseColumn(AlgMatrix *M, int col, StrSparseVector *v):
     copies the col column/row of M into v (Assuming trees aggree)
     has to be called with col=0,1,2,3,..*/
        AlgSetStructure                SetStructure;
  /* int SetStructure(Algebra *A, int begrow, int begcol, int level,
                      Tree *Tcol, Tree *Trow, int copy):
     recursively creates the row and columns tree.
     - sets ->Trow, ->Tcol for this matrix and all its descendents
     - links the ->sons array for its ->Tcol, ->Trow correctly
       to the trees on its child Algebras.
     - sets the ->level for this Algebra
     if (copy==1) recursively sets the ->Tcol, ->Trow node of this Algebra
     and its decendents to the passed Tcol, Trow
     begrow, begcol is the offset of this matrix.
     Returns the levels in algebra tree below this level  */
        AlgZeroCols                    ZeroColumns;
        AlgZeroCols                    ZeroRows;
  /* void ZeroColumns(Algebra *A, LogiVector *where):
     Zeroes columns indicated by where. Columns will stay in the matrix, 
     just all entries in corresponding columns set to 0 
     (needed by SQP treatment of fixed columns)                            */
        AlgSetDiag                     SetDiag;
  /* void AlgSetDiag(Algebra *A, double factor, Vector *diag):
     sets diag(A) = factor*diag(A) + diag:
     Only works for square matrices                                        */
#ifdef WITH_MPI
        int                            to_be_faked;
  /* true if Algebra (and children) should be faked out (in parallel), 
     propagated by MakeAugmentedSystem, executed by FakeOutAlgebra */ 
        int                            par_split_node;
  /* true if the Algebras split here in the parallel case, i.e.
     this Algebra is owned by all processors, but its children are
     distributed. Needed in a variety of methods to determine whether
     or not a final Bcast/Reduce is necessary */
        AlgCopyParInfo                 CopyParInfo;
  /* void CopyParInfo(Algebra *A):
     recursively copies parallel info (->host, ->id) from component nodes
     of Augmented System nodes */
#ifndef NOOLD
        AlgFakeOutAlgebra              FakeOutAlgebra;
  /* void FakeOutAlgebra(Algebra *A, int parent_is_faked):
     recursively replaces computational functions by DoNothing functions
     if parent\_is\_faked is true or ->to\_be\_faked flag is set.  
     Used for Algebras on processors where these Algebras are not present
     The Algebras are defined and left in place, but they don't do anything */
#endif
#endif
        AlgParProcess                  ParProcessAlgebra;
  /* void ParProcessAlgebra(Algebra *A):
     This is the replacement for FakeOut Algebra: recursively traverses
     down the tree of Algebras. If Algebra is not on this processor 
     (from its first_proc, last_proc entries) then replace all its
     functions by DoNothing.
     For a leaf node ON this processor, ask for data via the call
     back function and do the numerics for the SparseAugmented System */
#ifdef WITH_MPI
        AlgAllocateProcs               AllocateProcessors;
  /* void AllocateProcs(Algebra *A, int first, int last, ParAlgList *alglist
                        host_stat_type type, int set_tree):
     recursivly allocate processors to Algebras: 
     This Algebra is allocated the processor range [first, last], it then
     distributes them amongs its child Algebras. 
     If type is HostSingle or HostAllShared then the children just
     inherit the same setting from the parent.
     Sets the ->first_proc, ->last_proc, ->host_stat and ->comm fields of A
     It also produces a 'flat list' of Algebra specifications in alglist
     This routine is called for AugSys Algebras, processor allocation
     is passed on to its constituent A, Q parts 
     If set_tree is set, then processor allocations are passed on to all
     6 associated vector tree nodes (this should only be done for diagonal
     Algebra-tree nodes)                                                    */
#else
        AlgMakeFlatList               MakeFlatList;
  /* serial alternative to Allocate Processors that simply recursively 
     creates a flat-list of all Algebras                                    */
#endif
        AlgDelCols                     DeleteColumns;
  /* DeleteColumns(Algebra *A, int *indices):
     Delete those columns for which indices[i] == 1   */
        AlgDelCols                     DeleteRows;
  /* DeleteRowss(Algebra *A, int *indices):
     Delete those rows for which indices[i] == 1   */
        AlgScale                       ColumnScale;
  /* ColumnScale(AlgMatrix *M, double *mult, double *min, double *max):
     Divide all elements in col-i by mult[i]. If min/max!=NULL set them
     to smallest/biggest abs value in this column AFTER scaling */
        AlgScale                       RowScale;
  /* RowScale(AlgMatrix *M, double *mult, double *min, double *max):
     Divide all elements in row-j by mult[j]. If min/max!=NULL set them
     to smallest/biggest abs value in this row AFTER scaling */
        AlgScaleVec                    ColumnScaleVec;
  /* ColumnScaleVec(AlgMatrix *M, double *mult, Vector *min, Vector *max):
     Same as ColumnScale, just min,max are Vector's */
        AlgScaleVec                    RowScaleVec;
  /* RowScaleVec(AlgMatrix *M, double *mult, Vector *min, Vector *max):
     Same as RowScale, just min,max are Vector's */

  /* FIXME: */
  /* ColumnScale/RowScale are REDUNDANT: only used in scale.c/UnscaleLPMatrix
     and could be replaced by Vec-versions there */

  /* Only implemented for "invertible" Matrices */
#ifndef NOOLD
        AlgBuildAAt                    BuildAAt;
  /* Algebra *BuildAAt(Algebra *A):
     recursively make the Algebra structure needed to store information
     of AXAt an its Cholesky factors. Only follow down "invertible" parts
     of the tree. New Algebra has dimensions nb_row x nb_row.
     Each AAt Algebra has the same type as its corresponding A-Algebra */
	AlgComputeAXAt                 ComputeAXAt;
  /* ComputeAXAt(AlgMatrix *M, Vector *X, AlgMatrix *MMt):
     recursively fill in the structures of MMt to hold the product
     MXMt (or whatever representation of it is required for this Matrix-type */
#endif
        AlgAddAugmentedSystemDiag      AddAugmentedSystemDiag;
  /* Adds the entries set in the (primal) vector Q, to the (primal)
     diagonal (i.e. in the Q-part) of the augmented System.
     Also if given adds the entries of the (dual) Vector ydiag to the (dual) 
     diagonal (i.e. in the bottom right part of AugSys)                   
     Also prepares Augmented System for factorization: copies
     elements from A, Q into sparse AugSystem structure: elements of 
     Q are multiplied with the double obj_fact argument                 */
        AlgComputeCholesky             ComputeCholesky;
  /* ComputeCholesky(Algebra *A, FILE *out, Vector *primal_reg, 
                     Vector *dual_reg, double reginit):
     Compute the Cholesky-factors of A or an appropriate implicit 
     representation of it)
     primal_reg/dual_reg is the dynamic regularization set by this routine 
     as necessary (they are initialised to reginit - default = 0)*/  
	AlgSolveCholesky               SolveCholesky;
  /* SolveCholesky(Algebra *A, Vector *rhs, Vector *sol, FILE *out):
     Solves A*sol = rhs using the already computed factors of MMt
     Assumes that MMt has been overwritten with its Cholesky factors */
	AlgSolveSparseCholesky         SolveSparseCholesky;
  /* SolveSparseCholesky(Algebra *A, StrSparseVector *rhs, Vector *sol):
     As AolveCholesky, but rhs is StrSparseVector => exploits sparsity */
        AlgSolveTriangular             SolveL;
  /* SolveL(Algebra *A, Vector *rhs, Vector *sol, FILE *out):
     Only Solve L*sol = rhs, where LDLt are Cholesky factors of A
     Might not be implemented by some Algebras, whos implicit inverse 
     representation is too different from Cholesky factors.
     Assumes that MMt has been overwritten with its Cholesky factors */
        AlgSolveTriangularPart         SolveLPart;
  /* SolveL(Algebra *A, Vector *rhs, Vector *sol, int last):
     Solve L*sol = rhs, where LDLt are Cholesky factors of A
     The solution sol is obtained from first to last element: 
        this routine only solves to element 'last'               */
	AlgSolveTriangular             SolveD;
  /* SolveD(Algebra *A, Vector *rhs, Vector *sol, FILE *out):
     Only Solve D*sol = rhs, where LDLt are Cholesky factors of A
     Remarks as SolveL */
	AlgSolveTriangularDPart     SolveDPart;
  /* SolveD(Algebra *A, Vector *rhs, Vector *sol, int first, int last):
     Solve D*sol = rhs, where LDLt are Cholesky factors of A
     Only solves for elements 'first' to 'last'                           */
	AlgSolveTriangular             SolveLt;
  /* SolveLt(Algebra *A, Vector *rhs, Vector *sol, FILE *out):
     Only Solve Lt*sol = rhs, where LDLt are Cholesky factors of A
     Remarks as SolveL */
	AlgSolveTriangularPart         SolveLtPart;
  /* SolveLt(Algebra *A, Vector *rhs, Vector *sol, int first):
     Solve Lt*sol = rhs, where LDLt are Cholesky factors of A
     Elements of sol are obtained last to first, this stops when element
     'first' is reached                                                   */
        AlgSolveSparseL                SolveSparseL;
  /* SolveSparseL(Algebra *A, StrSparseVector *rhs, StrSparseVector *sol):
     As SolveL, but sol, rhs are StrSparseVector's => exploits sparsity */
        AlgSolveSparsePart             SolveSparseD;
  /* SolveSparseD(Algebra *A, StrSparseVector *rhs, StrSparseVector *sol,
                              int first, int last):
     As SolveD, but sol, rhs are StrSparseVector's => exploits sparsity
     only fills in entries of sol between indices first and last        */
        AlgSolveSparsePart             SolveSparseLt;
  /* SolveSparseLt(Algebra *A, StrSparseVector *rhs, StrSparseVector *sol,
                   int first int last):
     As SolveLt, but sol, rhs are StrSparseVector's => exploits sparsity
     only fills in entries of sol after first (last is ignored)         */
	AlgSolveTriangEj               SolveLtej;
  /* SolveLtej(AlgMatrix *MMt, int j, Vector *sol, FILE *out):
     Solves Lt*sol = ej (j-th unit vector)   */
        AlgInverseTriangL              InverseTriangL;
  /* InverseTriangL(AlgMatrix *MMt, SparseVector **cols, FILE *out):
     Returns the columns of L^{-1} as an array of SparseVector's 
     Actually creates the cols[i], but not cols itself(!) */
  /* Only called by BlockAng/Sp1CompOuterProdSchur called for
     Sparse Schur product in ComputeCholesky */ 
        AlgMakeAQMap                   MakeAQMap;
  /* If called in Master node (mapA==MapQ==null) allocate memory for
     mapA, mapQ (number of nodes in A, Q tree).  Everywhere set mapA[i],
     mapQ[i]. And set link from current Algebra to mapA, mapQ (all Algebras
     point to the same copy) For complicated augSys-Algebras mapA[i],
     mapQ[i] just point to their common augSys. For terminal Algebras
     (MatrixSparse, MatrixDense) MapA[i], mapQ[i] point to the correct part
     of the augSys. (It is only these parts that are needed).*/
#ifndef NOOLD
        AlgMakeAugmentedSystem         MakeAugmentedSystem;
  /* Creates a corresponding Augmented System Algebra out of A and Q 
     reusing bits of A, Q as much as possible without creating new matrices
     - just setting new pointers). Makes sure that there is space for the
     diagonal entries theta (if necessary changing the matrix in question)
     If necessary reorders the bits of A and Q to make the AugmentedSystem.
     Might need to call TransposeAlgebra to make up bits of the 
     AugmentedSystem.*/
        AlgMakeAugmentedSystemUnsym    MakeAugmentedSystemUnsym;
  /* Creates a corresponding Augmented System Algebra out of Q, B and C. 
     Converts the structure given in Q, B and C to an Augmented System 
     structure    [Q  C']
                  [B  0 ]
     which is reordered (Q and B/Q and C' are interleaved) and
     returned. Calls substructures recursively.*/
#endif
        AlgMakeAugmentedSystem         MakeAugmentedSystemNoMem;
        AlgMakeAugmentedSystemUnsym    MakeAugmentedSystemUnsymNoMem;
  /* These are No-memory versions of the routines above. For complex
     matrices these routines are identical to the ones above, but for
     leaf nodes, these routines will not allocate any memory used to
     store the reordered sparse augmented matrix, the cholesky data 
     structure the theta-diagonal or row-wise representation.
     All of this is now done in the replacement for FakeOutAlgebra
     that also fills in values for SparseMatrices                       */

} Algebra;

/* The following wraps allow a call MethodAlg(A, ...) = A->Method(A, ...)
   and only execute if DoItAlg(A,x,y) == 1
   (i.e. don't execute if node terminal, but either x or y not on this proc) */

/* CLASS POLYMORPHIC: */
int
MatrixTransTimesVectAlg(Algebra* A, Vector *x, Vector *y,
			const int add, const double fact);

int
MatrixTimesVectAlg(Algebra* A, Vector *x, Vector *y,
		   const int add, const double fact);

int
GetColumnAlg(Algebra *A, const int col, Vector *Col);

int
GetStrSparseColumnAlg(Algebra *A, const int col, StrSparseVector *Col);

int
SolveSparseLAlg(Algebra* A, StrSparseVector *x, StrSparseVector *y);

/* These are always excecuted, but still do 
                               MethodAlg(A, ...) = A->Method(A, ...) */
int 
ComputeCholeskyAlg(Algebra* A, FILE *f, Vector *primal_reg, Vector* dual_reg,
		   const double reginit);

int 
SolveLtejAlg(Algebra* A, const int j, Vector *y, FILE *f);

void
FreeAlgebraAlg(Algebra *);

Algebra*
AlgebraBuildAAt(Algebra* A);

void
PrintAlg(FILE *f, Algebra *A, const char *fmt);

void 
PrintMatrixMatlab(FILE *f, Algebra *A, const char *matrixname);

void 
PrintMatrixMatlabSparse(FILE *f, Algebra *A, const char *matrixname);

/* END POLYMORPHIC */

Algebra *
FakeSubstitute(Algebra *A, int A_id);

Algebra*
SparseAlgTrans(FILE *out, Algebra* A);

Algebra*
FakeAlgebra(Algebra *Source);

Algebra *
FakeParCopy(Algebra *A, int A_id);

Algebra*
NothingAlgebra(void);

Algebra *
NewAlgebra(void);
/* Creates an Algebra that has all pointers initialised to ErrFct */

AugSystemType *
NewAugSystem(void);
/* Generic Constructor for AugSystemType objects */

/** Free the memory allocated for the augmented system */
void
FreeAugSystem(AugSystemType *Aug);

void
GetStrSparseColumn(Algebra *Any, int col,  StrSparseVector *v);

void
CommonAllocateProcessors(Algebra *AlgAug, int first, int last,
			 ParAlgList *alglist, host_stat_type type,
			 int set_tree);

#ifndef WITH_MPI


Algebra *
InitAlgebras(FILE *out, Algebra* A, Algebra *Q);
#else

MPI_Comm
CreateCommunicator(const int first, const int last);

void
SetParSplitAlgebra(Algebra *A);

Algebra *
InitParAlgebras(FILE *out, Algebra* A, Algebra *Q, 
		struct delay_st *DelayA, struct delay_st *DelayQ);

#endif

Algebra *
InitAlgebrasNew(Algebra* A, Algebra *Q);

#endif /* ALGEBRA_H */
