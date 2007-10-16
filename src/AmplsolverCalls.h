#ifndef AMPLSOLVERCALLS_H
#define AMPLSOLVERCALLS_H

/* the asl_pfgh.h file in amplsolver globally redefines list
   => this is a separate file that provides all the calls to the
      amplsolver library (and that cannot use c++ lists

*/

#include <string>
using namespace std;

/** @class NlFile 
 *  @brief
 *  This object represents a *.nl file. It includes convenient routines
 *  to read data from the *.nl file. 
 * 
 *  The main reason for having the class is that the use of amplsolver 
 *  (AMPL's nl-file reading library) requires the inclusing of asl.h
 *  which defines lots of global variables with inconvenient names 
 *  like 'list'. This way, only this class has to avoid name clashes
 *  @attention This class only stores the filename as a reference, it
 *             therefore reopens the *.nl file everytime a method is called
 *             It would probably be better to open the file once and then
 *             keep a pointer to amplsolver's ASL structure. However it
 *             is not clear if this would work (due to global variables) 
 */ 
class NlFile {
 public:
  string nlfilename; //< Filename of *.nl file without *.nl extension
  int ncol;          //< # constraints defined in this file
  int nrow;          //< # variables defined in this file
  int nzH;           //< # Hessian nonzeros 
  int nzA;           //< # Jacoabian nonzeros (defined in this file)
  // -------------------------- methods ------------------------------------
  NlFile(string nlfilename);

  /** return number of variables defined in this *.nl file */
  int getNoVariables();

  /** return number of constraints defined in this *.nl file */
  int getNoConstraints();

  /** return number of Hessian entries defined in this *.nl file */
  int getNoHessianEntries();

  /** return objective Hessian structure in this *.nl file.
   *  Assumes that the Hessian is constant (pass in x=0).
   *  Only returns the Hessian of the objective.
   */
  void getHessianStructure(int *colbeg, int *rownbs);

  /** return objective Hessian entries in this *.nl file.
   *  Assumes that the Hessian is constant (pass in x=0).
   *  Only returns the Hessian of the objective.
   */
  void getHessianEntries(int *colbeg, int *rownbs, double *el);

  /** common method that opens the *.nl file and reads the scalar values
   *  n_con, n_var, nnz
   *  FIXME: when the NlFile includes a pointer to asl then this methods
   *         is redundant.
   */
  void readCommonScalarValues();
};

int getNoNonzerosAMPL(string nlfilename, int nvar, int *lvar);

void fillSparseAMPL(string nlfilename, int nvar, int *lvar, 
		      int *colbeg, int *collen, int *rownbs, double *el);

void getRhsAMPL(string nlfilename, int nvar, int *lvar, double *elts);
void getObjAMPL(string nlfilename, int nvar, int *lvar, double *elts);
void getBndAMPL(string nlfilename, int nvar, int *lvar, double *elts);

#endif
