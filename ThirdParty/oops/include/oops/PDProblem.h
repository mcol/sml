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
 *  @file PDProblem.h
 *
 *  @author Andreas Grothey
 *
 */
#ifndef PDPROB_H
#define PDPROB_H

#include "oops/OopsOpt.h"
#include "hopdm.h"
#include <cstdio>


/* Forward declarations */
class Algebra;
class StatusVector;
class Vector;


/* temporary definitions while their usage may still exist */
#define primal_dual_pb PDProblem

/* ------------------------------------------------------------------------- */
/** @class PDProblem
 * This class describes a primal-dual problem and provides methods to
 * call OOPS in various modes.
 * 
 * This class should be extended by every "application" of OOPS, which would
 * consequently inherit all the methods to call OOPS
 *
 * The class has an OopsOpt component which gets automatically initialised with
 * default options on construction. Options from the control variable file
 * oops_ct_var.dat can be read by calling .OopsOpt.readFile()
 */
class PDProblem {

 public:

  /** Algebra */
  Algebra *AlgAug;

  /** Dimensions of the problem */
  int nb_col, nb_row;

  /** Right-hand side vector */
  Vector *b;

  /** Objective coefficients */
  Vector *c;

  /** Upper bounds */
  Vector *u;

  /** Lower bounds */
  Vector *l;

  /** Status vector */
  StatusVector *stat;

  /** number of variables of various types */
  int nb_ub, nb_free;

  /** row and column names */
  char **colnames, **rownames;

  /** Solution vectors and vectors used for warmstart */
      Vector *x, *cx;
      Vector *y, *cy;
      Vector *z, *cz;
      Vector *t, *ct;
  /* These are only used for problems with bounds */
      Vector *s, *cs;
      Vector *w, *cw;

  /** Average complementarity gap corresponding to advanced point,
      also used to pass in mu of advanced starting point */
  double adv_mu;

  /** Complete primal path (if needed) */
  Vector **path;

  /** Target vector of complementarity pairs to be used for warm-start
      (if not NULL) */
  Vector *target;

  /* Modifiers for feasibility problem */
  /* OOPS_QP can solve an l_2 feasibility problem instead of the real 
     problem, or put a factor on the objective. This is set by the
     following two parameters:                                             */
  /* for unmodified problem: is_feas=0, obj_fact = 1.0
     for unmodified problem: is_feas=1, obj_fact = 0.0                      */
  int is_feas;
  double obj_fact;
  /* to check that problem has been defined by constructor                  */
  int set_by_constr; 
      
  /** The return value when the problem is solved */    
  hopdm_ret *ret;

  /** The printing options setting */
  PrintOptions PrtOpt;

  /* The options setting */
  OopsOpt Opt;

/* ------------------------------ methods ---------------------------------- */
  /** Constructor */
  PDProblem(Algebra *AlgAug = NULL,
            Vector *b =NULL, Vector *c =NULL, Vector *l =NULL, Vector *u =NULL,
            Vector *x = NULL, Vector *y = NULL, Vector *z = NULL);

  /** Destructor */
  ~PDProblem();

  /** Set the problem data (if constructed without data) */
  void setProblem(Algebra *algAug, Vector *vb, Vector *vc, 
		  Vector *vl, Vector *vu,
		  Vector *vx, Vector *vy, Vector *vt=NULL, Vector *vz=NULL,
		  Vector *vs=NULL, Vector *vw=NULL);

  /** pass in arrays with row and column names */
  void registerNames(char **clnms, char **rwnms);


  /** Solve an LP with the higher-order primal-dual method. */
  void solve();

  /** Solve an LP with the higher-order primal-dual method. */
  void solve(FILE *out);

  /** set the print options */
  void setPrintLevel(int prt);

  /** set the print options */
  void setOptions(OopsOpt &Opt);

  /** write problem in MPS format */
  void writeMPS(const char *filename, bool classic=false);

  /** write the problem data in Matlab format */
  void writeMatlab(const char *filename);

  /** do a modification step on the warmstart point */
  void doModificationStep();

  /** return success status after solve */
  int getIfail();
  
  /** return advanced center (if required) */
  PDPoint* getAdvCenter();

  /** return solution */
  PDPoint* getSolution();

 protected: 
  /** update stat vector and nb_free, nb_ub from current bounds */
  void updateStatus();

  /** Once problem data is set, put the object in a consistent state */
  void finishInitialisation();
};


/* ========================================================================= */
/* These are deprecated static methods */

/** Allocate space for a PDProblem object on the heap.
 *  @deprecated Use the PDProblem class directly instead.
 */
PDProblem*
NewPDProblem(Algebra *AlgAug, Vector *b, Vector *c, Vector *u,
	     Vector *x, Vector *y, Vector *z);

/** Free the space allocated for a PDProblem object on the heap.
 *  @deprecated Use the PDProblem class directly instead.
 */
void
FreePDProblem(PDProblem *P);

/** Solve an LP with the higher-order primal-dual method. */
hopdm_ret*
SolveOops(PDProblem* pb);

/** Solve an LP with the higher-order primal-dual method. */
hopdm_ret*
hopdm(FILE *out, PDProblem *P, OopsOpt *opt, PrintOptions *Prt);

hopdm_ret*
hopdm_std(PDProblem *P, OopsOpt *opt, PrintOptions *Prt);


hopdm_ret*
hopdm_pr(FILE *out, PDProblem *Prob,
         OopsOpt *options, PrintOptions *Prt);


#endif
