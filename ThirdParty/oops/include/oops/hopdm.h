/* This file is part of OOPS.
 *
 * OOPS is (c) 2003-2009 Jacek Gondzio and Andreas Grothey, 
 *                       University of Edinburgh
 *
 * OOPS is distributed in a restricted form in the hope that it will be a useful
 * example of what can be done with SML, however it is NOT released under a free
 * software lisence.
 *
 * You may only redistribute this version of OOPS with a version of SML. You
 * may not link OOPS with code which is not part of SML lisenced under the
 * LGPL v3.
 *
 * You may NOT modify, disassemble, or otherwise reverse engineer OOPS.
 *
 * OOPS is distributed WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
/**
 *  @file hopdm.h
 *
 *  @author Andreas Grothey
 *
 */
#ifndef HOPDM_H
#define HOPDM_H

#include "oops/DenseVector.h"
#include "oops/Vector.h"
#include "oops/LogiVector.h"
#include "oops/Algebra.h"
#include "oops/GlobalOpt.h"

/* temporary definition while usage of hopdm_opt_type still exists */
#define hopdm_opt_type HopdmOptions

/**
 *  Parameters that control the behaviour of the solver.
 */
typedef struct {

  /** Use the supplied starting point */
  int use_start_point;

  /** Use Warm Start Heuristic */
  int use_wsh;
  int nb_beg_center_it;
  int nb_end_center_it;
  int use_presolve;
  int ret_adv_center;
  double ret_adv_center_gap;
  int nb_ret_adv_center_it;
  int ret_path;
  GlobalOpt *glopt;
  
} hopdm_opt_type;

/**
 *  Parameters that control what is printed out.
 */
typedef struct {
   int PrtFact;
   int PrtIRSlv;
   int PrtInit;
   int PrtDTheta;
   int PrtPushV;
   int PrtResid;
   int PrtComPr;
   int Prthopdm;
   int PrthoDir;
   int PrtMaxS;
   int PrtMakeS;
   int PrtTT;
} hopdm_prt_type;

/**
 *  Information returned from the solver.
 */
typedef struct {

  double val;

  /** Return code from the solver:
      -  0: OK;
      -  1: infeasible;
      -  2: unbounded;
      -  3: inconsistent bounds;
      -  4: excess iters;
      - 99: other problem */
  int ifail;

  /** Number of iterations carried out */
  int iters; 
  int found_adv_center;   /* -1: not required, 0: NO, 1:YES */

  /** Infeasibilities at end of solve */
  double err_b, err_c, err_u;
  double gap;

} hopdm_ret;

/**
 *  Information on the primal-dual problem.
 */
typedef struct {

  /** Algebra */
  Algebra *AlgAug;

  /** Right-hand side vector */
  Vector *b;

  /** Objective coefficients */
  Vector *c;

  /** Upper bounds */
  Vector *u;

  /** Lower bounds */
  Vector *l;

  /** Solution vectors and vectors used for warmstart */
      Vector *x, *cx;
      Vector *y, *cy;
      Vector *z, *cz;
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
      int is_feas;
      double obj_fact;
  /* to check that problem has been defined by constructor                  */
      int set_by_constr; 
  /* for unmodified problem: is_feas=0, obj_fact = 1.0
     for unmodified problem: is_feas=1, obj_fact = 0.0                      */

} primal_dual_pb;

/* A nicer name */
typedef primal_dual_pb PDProblem;

/**
 *  Inverse representation of the system matrix.
 *
 *  Define everything that is needed for the inverse representation of the
 *  system matrix: I.e. all arguments to IterRefSolve for example.
 */
typedef struct
{
  Algebra *AlgAug;
  Vector *vtheta;
  Vector *vthetay;
  Vector *vpdRegTh;
  Vector *vpdRegx;
  Vector *vpdRegy;

} InverseRep;

InverseRep *
NewInverseRep(Algebra *AlgAug, Vector *vtheta, Vector *vthetay, 
	      Vector *pdRegTh, Vector *pdRegx, Vector *pdRegy);

/**
 *  A primal-dual point.
 *
 *  This struct holds the information about a primal dual point (x,y,z,s,w)
 *  or a primal-dual direction.
 *  Components s, w are not used if no upper bounded variables are present.
 */
typedef struct
{
  /** x-component */
  Vector *x;

  /** y-component */
  Vector *y;

  /** z-component */
  Vector *z;

  /** s-component (only if upper bounds exist, otherwise NULL) */
  Vector *s;

  /** w-component (only if upper bounds exist, otherwise NULL) */
  Vector *w;

  /** combined xy-Vector (if needed, otherwise NULL) */
  Vector *xy;

} PDPoint;

/* ----------------------------------------------------------------------------
   Stubs for OOPS functions that can be called from outside hopdm_qp.c
---------------------------------------------------------------------------- */
void
MaxStep(FILE *out, PDPoint *pdPoint, PDPoint *pdDir,
	double *alphaX, double *alphaZ, double *alphaS, double *alphaW,
	Algebra *AlgA, LogiVector *vwhere_u, LogiVector *vwhere_l, Vector *vstep,
	hopdm_prt_type *Prt);

void
CompPDRes(FILE *out, Algebra * AlgA, Algebra *AlgQ,
	  Vector *vb, Vector *vc, Vector *vu, PDPoint *pdPoint,
	  Vector *vxib, Vector *vxic, Vector *vxiu,
	  double *err_b, double *err_c, double *err_u, 
	  LogiVector *vwhere_u, hopdm_prt_type *Prt);

int
DefTheta(FILE *out, Vector *vx, Vector *vs, Vector *vz, Vector *vw,
	 Vector *vtheta, LogiVector *vwhere_u, LogiVector *vwhere_l, hopdm_prt_type *Prt);

void
pdFactor (FILE *out, Algebra *AlgAug, Vector *vtheta, Vector *vthetay, 
	  Vector *vpdRegTh, Vector *vpdRegx, Vector *vpdRegy, 
	  hopdm_prt_type *Prt);

void
IterRefSolveNew (FILE *out, InverseRep *ivr, int *Alarm,
		 Vector *vrhs_x, Vector *vrhs_y, 
		 Vector *vdel_xy, Vector *vdel_x, Vector *vdel_y,
		 hopdm_prt_type *Prt, Vector *vNwrk3, Vector *vMwrk3,
		 double ireps);

/** Compute higher-order primal-dual directions. */
void
HopdmDir(FILE *out, InverseRep *ivr, const int iDir, const int onlyCent,
	 const double oldbarr, const double barr, double AlphaP, double AlphaD,
	 Vector *vxib, Vector *vxic, Vector *vxiu,
	 Vector *vXrhs_x, Vector *target,
	  PDPoint *pdPoint, PDPoint *pdPredDir, PDPoint *pdNewDir,
	  LogiVector *vwhere_u, LogiVector *vwhere_l, hopdm_prt_type *Prt,
	 Vector *vNw1, Vector *vMw1, const double ireps);

/** Solve an LP with the higher-order primal-dual method. */
hopdm_ret*
hopdm(FILE *out, primal_dual_pb *P, hopdm_opt_type *opt, hopdm_prt_type *Prt);

/** Solve an LP with the higher-order primal-dual method. */
hopdm_ret*
SolveOops(primal_dual_pb* pb);

/** Allocate space for for a primal_dual_pb structure. */
primal_dual_pb*
NewPDProblem(Algebra *AlgAug, Vector *b, Vector *c, Vector *u,
	     Vector *x, Vector *y, Vector *z);

/** Free the space allocated for the primal_dual_pb structure. */
void
FreePDProblem(primal_dual_pb *P);

/** Set up a PDPoint structure */
PDPoint*
NewPDPoint(Vector *x, Vector *y, Vector *z, Vector *s, Vector *w, Vector *xy);

/** Free the space allocated for the PDPoint structure */
void
FreePDPoint(PDPoint *P);

/** Allocate space for for an hopdm_opt_type structure. */
hopdm_opt_type*
NewHopdmOptions(void);

/** Free the space allocated for the hopdm_opt_type structure. */
void
FreeHopdmOptions(hopdm_opt_type *opt);

/** Allocate space for for an hopdm_prt_type structure and set the
    print level. */
hopdm_prt_type*
NewHopdmPrt(const int level);

#endif
