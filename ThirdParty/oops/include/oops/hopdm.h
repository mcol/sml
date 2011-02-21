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
 *  @file hopdm.h
 *
 *  @author Andreas Grothey
 *
 */
#ifndef HOPDM_H
#define HOPDM_H

#include <cstdio>


/* Forward declarations */
class Algebra;
class LogiVector;
class OopsOpt;
class StatusVector;
class Vector;


/* temporary definitions while their usage may still exist */
#define hopdm_prt_type PrintOptions


/** The amount of printing required. */
enum PrintLevelValues {

  /** Don't print anything */
  PRINT_NONE,

  /** Print one line per iteration */
  PRINT_ITER,

  /** Print one line per iteration, factorization, iterative refinement,
      and residuals */
  PRINT_INFO,

  /** Print verbosely */
  PRINT_VERBOSE,

  /** Placeholder for the last item */
  LAST_LEVEL
};


/**
 *  Parameters that control what is printed out.
 */
class PrintOptions {

 public:

  /** Constructor */
  PrintOptions(const int PrintLevel = PRINT_ITER);

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

   // private:

  /** Set the member variables for the amount of printing required */
  void setPrintLevel(const int PrintLevel);

};

/**
 *  Information returned from the solver.
 */
typedef struct {

  double val;
  double dVal;

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
  double err_b, err_c, err_u, err_l;
  double gap;

} hopdm_ret;


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
class PDPoint {
 public:
  /** x-component */
  Vector *x;

  /** y-component */
  Vector *y;

  /** t-component */
  Vector *t;

  /** z-component */
  Vector *z;

  /** s-component (only if upper bounds exist, otherwise NULL) */
  Vector *s;

  /** w-component (only if upper bounds exist, otherwise NULL) */
  Vector *w;

  /** combined xy-Vector (if needed, otherwise NULL) */
  Vector *xy;

  /* ================================================================== */
  PDPoint(): 
    x(NULL), y(NULL), t(NULL), z(NULL), s(NULL), w(NULL), xy(NULL)
  {}
  
    PDPoint(Vector *x, Vector *y, Vector *t, Vector *z, 
	    Vector *s=NULL, Vector *w = NULL, Vector *xy=NULL)
      :x(x), y(y), t(t), z(z), s(s), w(w), xy(xy)
  {}
  
  /** write the point to a Matlab file */  
  void writeMatlab(FILE *mout);

  /** write the point to a Matlab file */  
  void writeMatlab(char *filename);
};

/* ----------------------------------------------------------------------------
   Stubs for OOPS functions that can be called from outside hopdm_qp.c
---------------------------------------------------------------------------- */

void
MaxStep(FILE *out, PDPoint *pdPoint, PDPoint *pdDir,
	double *alphaT, double *alphaZ, double *alphaS, double *alphaW,
	Algebra *AlgA, LogiVector *vwhere_u, LogiVector *vwhere_l, Vector *vstep,
        PrintOptions *Prt);

void
CompPDRes(FILE *out, Algebra * AlgA, Algebra *AlgQ,
	  Vector *vb, Vector *vc, Vector *vl, Vector *vu, PDPoint *pdPoint,
	  Vector *vxib, Vector *vxic, Vector *vxiu, Vector *vxil, 
	  double *err_b, double *err_c, double *err_u, double *err_l, 
          LogiVector *vwhere_l, LogiVector *vwhere_u, PrintOptions *Prt);

int
DefTheta(FILE *out, Vector *vt, Vector *vs, Vector *vz, Vector *vw,
         Vector *vtheta, LogiVector *vwhere_u, LogiVector *vwhere_l,
         PrintOptions *Prt);

void
pdFactor (FILE *out, Algebra *AlgAug, Vector *vtheta, Vector *vthetay, 
	  Vector *vpdRegTh, Vector *vpdRegx, Vector *vpdRegy, 
          PrintOptions *Prt, OopsOpt *opt);

void
IterRefSolveNew (FILE *out, InverseRep *ivr, int *Alarm,
		 Vector *vrhs_x, Vector *vrhs_y, 
		 Vector *vdel_xy, Vector *vdel_x, Vector *vdel_y,
		 PrintOptions *Prt, Vector *vNwrk3, Vector *vMwrk3,
		 double ireps);

/** Compute higher-order primal-dual directions. */
void
HopdmDir(FILE *out, InverseRep *ivr, const int iDir, const int onlyCent,
	 const double oldbarr, const double barr, double AlphaP, double AlphaD,
	 Vector *vxib, Vector *vxic, Vector *vxiu, Vector *vxil, 
	 Vector *vXrhs_x, Vector *target,
	  PDPoint *pdPoint, PDPoint *pdPredDir, PDPoint *pdNewDir,
         LogiVector *vwhere_u, LogiVector *vwhere_l, 
	 PrintOptions *Prt, OopsOpt *opt, 
	 Vector *vNw1, Vector *vMw1, const double ireps);



/** Set up a PDPoint structure */
PDPoint*
NewPDPoint(Vector *x, Vector *y, Vector *t, Vector *z, 
	   Vector *s, Vector *w, Vector *xy);

/** Free the space allocated for the PDPoint structure */
void
FreePDPoint(PDPoint *P);


/** Allocate a PrintOptions object on the heap and set the print level.
 *  @ deprecated Use the PrintOptions object directly.
 */
PrintOptions*
NewHopdmPrt(const int level);

#endif
