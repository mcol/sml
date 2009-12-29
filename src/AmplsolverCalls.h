/* (c) 2008,2009 Jonathan Hogg and Andreas Grothey, University of Edinburgh
 *
 * This file is part of SML.
 *
 * SML is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, using version 3 of the License.
 *
 * SML is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */
#ifndef AMPLSOLVERCALLS_H
#define AMPLSOLVERCALLS_H

#include <string>
#include <map>
#include <assert.h>
#include "ExpandedModel.h"

/* the asl_pfgh.h file in amplsolver globally redefines list
   => this is a separate file that provides all the calls to the
      amplsolver library (and that cannot use c++ lists
*/

class IndexListValue {
 public:
  int nvar;
  int *lvar;
  int count;

  /** Constructor */
  IndexListValue(int new_nvar = -1, int *new_lvar = NULL, int new_count =  -1) :
    nvar(new_nvar), lvar(new_lvar), count(new_count) {}

  /** Destructor */
  ~IndexListValue() {
   delete[] lvar;
  }

};


/** @class NlFile 
 *  This object represents a *.nl file: it is associated with
 *  an ExpandedModel object and provides routines to access the *.nl
 *  file through the amplsolver library.
 *
 *  @note The main reason for having the class is that the use of
 *  amplsolver (AMPL's nl-file reading library) requires the inclusion
 *  of asl.h which defines lots of global variables with inconvenient
 *  names like 'list'. This way, only this class has to avoid name
 *  clashes.
 *
 *  @attention This class only stores the filename as a
 *  reference, it therefore reopens the *.nl file every time a method
 *  is called. It would probably be better to open the file once and
 *  then keep a pointer to amplsolver's ASL structure. However it is
 *  not clear if this would work (due to global variables).
 * 
 *  @bug The Hessian routines are not tested. The interface should
 *  probably change as well (i.e. pass in a list of variables w.r.t
 *  which the Hessian should be evaluated.
 */ 
class NlFile {
  friend class ExpandedModel;

  //! Filename of *.nl file without *.nl extension
  std::string nlfilename;
  int ncol;          //!< # constraints defined in this file
  int nrow;          //!< # variables defined in this file
  int nzH;           //!< # Hessian nonzeros 
  int nzA;           //!< # Jacoabian nonzeros (defined in this file)

  /** The NlFile defines constraints that span over several column
   * blocks. Typically only the intersection with one of these blocks
   * needs to be avaluates. For this we need to know which columns in
   * the NlFile belong to a given column block (given by an
   * ExpandedModel). This is stored in terms of this map: for every
   * ExpandedModel it gives an array of indices of the corresponding
   * columns.
   *
   * @note Might want to be able to look up dimension and list
   * (nval/lval). In this case we need to store a structure of
   * (nval/lval) items on the map
   *
   * @note This information is needed in various places and is
   * computed fairly ineffciently (by O(n^2) string comparisons). In
   * order not to redo work, any index list that is computed is stored
   * on this map. The computation is done by
   * findIxOfLocalVarsInNlFile, which will check if the required list
   * is already on the map.
   *
   * @bug findIxOfLocalVarsInNlFile should probably become a member of
   * this class.
   *
   * @note I am not sure if this is the correct place to store this map.
   */
  std::map<ExpandedModel*, IndexListValue*> indexList;
  // -------------------------- methods ------------------------------------
 public:

  /** Constructor */
  NlFile(const std::string& name);

  /** Destructor */
  ~NlFile();

 private:

  /** Return the number of constraints defined in this *.nl file */
  int getNoConstraints();

  /** common method that opens the *.nl file and reads the scalar values
   *  n_con, n_var, nnz
   *  FIXME: when the NlFile includes a pointer to asl then this methods
   *         is redundant.
   */
  void readCommonScalarValues();
  int getNoNonzerosAMPL(int nvar, const int *lvar);

  void fillSparseAMPL(int nvar, const int *lvar,
		      int *colbeg, int *collen, int *rownbs, double *el);
  void getRowLowBoundsAMPL(double *elts);
  void getRowUpBoundsAMPL(double *elts);
  void getObjAMPL(int nvar, int *lvar, double *elts);
  void getColLowBoundsAMPL(int nvar, int *lvar, double *elts);
  void getColUpBoundsAMPL(int nvar, int *lvar, double *elts);
  int findIxOfLocalVarsInNlFile(ExpandedModel *em, int *lvar);

  // Here follow stuff for which wrapper routines in ExpandedModel do not
  // exist yet.
 public:
  // this is here since createQ accesses it directly
  /** Return the number of variables defined in this *.nl file */
  int getNoVariables();

  /** Return the number of Hessian entries defined in this *.nl file */
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

};

#endif
