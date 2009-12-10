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
 *  @file GlobalOpt.h
 * 
 *  Global options set through the OOPS control file.
 *
 *  @author Andreas Grothey
 */

#ifndef GLOBALOPT_H
#define GLOBALOPT_H

/** Structure for the options that can be set through the OOPS control file. */
typedef struct GlobalOpt
{
  /* ----------------------- General options ---------------------------- */
  /** Printing level (for those routines that implement this)
      0=errors only, 1=log/stats, 2=details, 3=all                */
  int prt;

  /** Iteration limit */
  int iter_limit;

  /** Convergence tolerance */
  double conv_tol;

  /* ------------- Options governing the main IPM method ---------------- */
  /** Split the Affine-Scaling Direction into three components */
  int get_diff_dir;

  /** Number of iterations in which the split direction are used */
  int n_diff_dir;

  /** Only do different directions if either step size is below this level */
  double diff_dir_blocklevel;

  /** Enforce convergence of feasibility not much later than convergence
      of optimality (LAGGING).
      i.e if err_feas > 10*err_opt =>reduce barr by 0.5 (rather than 0.1) */
  int force_feas;

  /** Detect numerical problems: if gap & primal & dual feas < 1e-4, then
      go into near convergence mode: if gap increases, allow 2 iters to
      decrease by a factor of 2, else return current point */
  int det_num_prob;

  /** Cure numerical problems: as det_num_prob, but also decrease the PushVar
      amounts by a factor of 4 */
  int cure_num_prob;

  /* Initialise vpdRegx/y vector to a value (rather than 0) 
    (this is PDREG in qnmfct)                                             */
  double init_regxy; /* INIT_REGXY */

  /** Value to use for ro_reg in num_factAS (RO in qnmfact)
      default is 1e-8, try 1e-6 for stronger regularization */
  double ro_reg;    /* RO_REG */

  /** Default value to use for pdRegTh: if vPrimalReg is allocated
      1e-6/1e-10 is used. Default value here is 1e-12 */
  double default_pdregth;

  /** Always try at least one higher-order corrector (iDir=3),
      even if Mehrotra's corrector has failed */
  int always_hoc; 

  /** Use the weighted correctors technique */
  int weighted_hoc;

  /* --------------- Options affecting the Linear Algebra --------------- */
  /** Type of reordering scheme to use: 0=MMD, 1=ND (Metis) */
  int LA_reorder;

  /** Use Sparse Schur complement */
  int LA_use_sparse_schur;

  /* -------------- Options governing Warmstart Behaviour --------------- */
  /* Do Warmstart at all 
     (picked up by model building routines to save starting point and set
     hopdm->use_start_point option) */
  int WS_do;         

  /** Gap of advanced starting point to return */
  double WS_gap_adv_cen;

  /** Target mu-value of advanced starting point to return */
  double WS_target_mu;

  /** Number of additional centering iterations before returning an
      advanced point */
  int WS_ret_adv_cen_it;

  /** Number of centering iterations at beginning of IPM */
  int WS_beg_cen_it;

  /** Use a target vector of complementarity pairs in the search directions */
  int WS_target;

  /* --------- Options about finding the new advanced starting point ----- */
  /* adjust to new bounds: Implemented is strategy 1, which uses the old
                           x with the following changes:
                            - solution was <sqrt(my) from bound 
                                => set starting x same distance from new bnd
                            - solution was free but is now < sqrt(mu) from bnd
                                => set sqrt(my) away from bound              */
  int WS_adjust_to_bnds;

  /** Adjust z to reflect the change in objective, as long as z stays 
      greater than sqrt(mu)/2 */
  int WS_adjust_z;

  /** Balance complementarity products: adjust smaller of x, z to have all
      complementarity products within [mu/WS_balance_xz, mu*WS_balance_xz] */
  double WS_balance_xz;


  /* ---------- Options governing the Unblocking strategy -------------- */
  /* Do Unblocking at all */
  int WS_unblk;

  /* Do unblocking in the first 'WS_n_unblk_it' iterations */
  int WS_n_unblk_it;

  /* Only try to Unblock components which are blocking at below this level */
  double WS_unblk_lev;


  /* -------- Options governing the use of Forward Sensitivity ---------- */
  /*                       (not in use on 26/11/04)                       */
  int WS_ForwardSens_TenWorst_Analyse;
  int WS_ForwardSens_TenWorst_TakeStep;

}
GlobalOpt;

extern GlobalOpt *glopt;

/** Read the OOPS control file. */
GlobalOpt *
ReadGlobalOpt(void);

#endif
