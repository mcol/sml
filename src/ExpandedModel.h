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
#ifndef EXPANDED_MODEL_H
#define EXPANDED_MODEL_H

/* This is the Expanded version of the AMPL model 
(as opposed to the FLAT version). 

It serves to define a tree representation of the model that has a node for
EVERY submodel (rather than just every type of submodel as the FLAT model)

It would serve as an intermediate step between the AmplModel view of
the world (which seems the right structure for a Benders type solver -
complicating constaints/variables are associated with the parent node)
and the OOPS view of the world (where complicating
variables/constraints belong to off-diagonal subblocks)

This would still be in the Benders view.

*/
#include <string>
#include <list>
#include <vector>
#include "ExpandedModelInterface.h"

using namespace std;

class AmplModel;
class NlFile;

/** 
 * @class ExpandedModel 
 * @brief A submodel (block) in the expanded model tree. It carries all the 
 * information to describe a block of the problem to the solver. 
 * It corresponds roughly to the a "diagonal" node in the OOPS algebra tree.
 * 
 * The ExpandedModel object describes a (sub)block of the model
 * together with pointers to its children (depended sub-blocks). A
 * tree of ExpandedModel objects describes the whole problem together with its
 * structure. 
 *
 * A tree of ExpandedModel objects is the means by which the problem
 * (after being parsed by SML) is communicated to a solver backend. It
 * thus forms the interface between SML and a solver.
 *
 * An ExpandedModel object has information on the variables,
 * constraints and objectives that describe this sub-block of the
 * problem as well as pointers to its children (sub-blocks).  It
 * further (through NlFile) provides routines to evaluate the
 * constraint and objectives defined in this block (and their
 * derivatives).
 *
 * Information about the sub-block is handled as a two-layer
 * structure. The first layer (represented by an ExpandedModel object)
 * provides 'static' information about the block, such as the
 * dimensions (number of local constraints and variables), list of
 * variable and constraint names and a list of children.
 *
 * The second layer (represented by an NlFile object) provides access to the 
 * 'non-static' information such as function and derivative values.
 *
 * The ExpandedModel object roughly corresponds to a "diagonal" node
 * in the OOPS Matrix tree. The difference is that complicating
 * variables/constraints belong to the parent node in the
 * ExpandedModel tree, whereas in the OOPS matrix tree, they would
 * belong to the final diagonal child node.
 *
 *  The ExpandedModel tree is the natural representation for a (Benders)
 *  decomposition solver, where complicating variables/constraints do 
 *  indeed belong to the master problem.
 *
 * @details 
 * The ExpandedModel tree is created (constructed) from the (flat)
 * AmplModel tree by expanding the indexing sets associated with every
 * AmplModel object. From the AmplModel it gets passed a list of local
 * variable and constraint name stubs (in ExpandedModel::localVarDef -
 * consisting of the global entity name together with the first part
 * (identifying the particular node) of the indexing parameters). The
 * ExpandedModel object is also passed the corresponding *.nl file
 * (which provides only the local constraints, but not just the local
 * variables, but all the variables that are refered to in the local
 * constraints). The constructor then compares the (alphanumeric) list
 * of variable name stubs with the variables defined in the *.nl file
 * (from the corresponding *.col file).  to generate the list of local
 * variables (ExpandedModel::listOfVarNames), and their indices within
 * the *.nl file (ExpandedModel::listOfVars). This information is then
 * used by the routines in ExpandedModel and NlFile to provide an
 * interface into the problem to the solver.
 *
 * @todo The Amplsolver interface routines (currently accessed through
 * the field nlfile) should be made accessible from this object (by
 * providing some wrapper routines)
 */
class ExpandedModel : public ExpandedModelInterface {

 private:
  int nLocalVars;  //!< number of local variables
  int nLocalCons;  //!< number of locl constraints

  /** Indicator if information on local variables 
   *  (nLocalVars, listLocalVars, nLocalCons) has been obtained by comparing 
   *  the localVarDef's with the *.col file
   */
  bool localVarInfoSet;     //!< indicator, if local variable info has been set

  /* Store solutions */
  double *pvar, *dvar;
  double *prow, *drow;

 public:

  // local information:
  string model_file; //!< name of the *.nl file that describes the problem data

  /** The NlFile object associated with model_file. 
   *  Provides interface to routines that evaluate objective and constraint 
   *   functions.   */
  NlFile *nlfile;   

  //list of constraints
  // FIXME: can we assume that all constraints in *.nl belong to this node?
  //list of variables
  //FIXME: in which form? regex? numbers?
  // BOTH: regex at the start. Info on numbers can be generated by a 
  //       class method
 
  //! list of global names of local variables 
  list<string> listOfVarNames; 
 
  //! list of global names of local constraints
  list<string> listOfConNames; 

  //! indices of local variables in the corresponding *.nl file */
  int *listOfVars;        

  list<string> localVarDef;  //!< the locally applicable variable declarations
  
  /** A stack of block instances that encode the current path through the
   * ExpandedModel tree during the construction phase                     */
  static list<string> pathToNodeStack;   //!< stack of instance names

  // -------------------------- methods ------------------------------------
  // public:

  ExpandedModel();     //< constructor

  //! Recursively print contents of this instance
  void print();
  
  //! Return nb local vars.
  int getNLocalVars();    

  //! Return names of local vars.
  const list<string>& getLocalVarNames();

  //! Return nb local cons.
  int getNLocalCons();

  const std::list<std::string>& getLocalConNames();

  //! Returns the nonzeros in the Jacobian of a section of the model.
  int getNzJacobianOfIntersection(ExpandedModelInterface *emcol);

  //! Returns the nonzeros in the Jacobian of a section of the model.
  void getJacobianOfIntersection(ExpandedModelInterface *emcol, int *colbeg,
				 int *collen, int *rownbs, double *el);

  //! Returns the vector of lower bounds for the constraints in this model
  void getRowLowBounds(double *elts);

  //! Returns the vector of upper bounds for the constraints in this model
  void getRowUpBounds(double *elts);

  //! Returns the vector of lower bounds for the local variables in this model
  void getColLowBounds(double *elts);

  //! Returns the vector of upper bounds for the local varables in this model
  void getColUpBounds(double *elts);

  //! Returns the objective gradient for the local model w.r.t. local vars
  void getObjGradient(double *elts);

  //! Upload the local variable solutions
  void setPrimalSolColumns(double *elts);

  //! Upload the local variable duals (multipliers on bounds)
  void setDualSolColumns(double *elts);

  //! Upload the local constraints slacks
  void setPrimalSolRows(double *elts);

  //! Upload the local constraints duals (multipliers on constraints)
  void setDualSolRows(double *elts);

  int findIxOfLocalVarsInNlFile(NlFile *nlf, int *lvar);

  //! Returns unique name of this block
  string getName() const { return model_file; }

  //! Outputs the solution to the supplied stream, at given indent
  void outputSolution(ostream &out, int indent=0);

 private: 
  //! sets nLocalVar, listOfVars, nLocalCons, listOfVarNames.
  void setLocalVarInfo(); 

};

#endif
