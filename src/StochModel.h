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
#ifndef STOCHMODEL_H
#define STOCHMODEL_H

#include "AmplModel.h"
#include "SetNode.h"

/* ------------------------------------------------------------------------ */
/** @class StochModel
 *  @brief This class describes a stochastic model (block).
 *
 *  It will gather the information present in the SML model file for this
 *  block much in the same way that AmplModel does for ordinary blocks.
 *  The difference is that sets and parameters that define the 
 *  Scenario tree are associated with it. These are
 *  - stageset:    (ordered) set of stages
 *  - nodeset:     set of nodes
 *  - anc:         parameter array giving ancestor node for every node
 *  - prob:        parameter array giving conditional probability for e node
 *
 *  In principle the stochastic model block can also be repeated "horizontally"
 *  in the same manner as all other blocks by specifying an indexing 
 *  expression.
 *  The stochastic model block will be expanded at processing time into a 
 *  nested set of AmplModels. 
 */
class StochModel: public AmplModel{
 public:
  // FIXME: stage should be replaced by a Set (decribing the elements)
  SetNode *stageset;  //!< The set of STAGES
  vector <string> stagenames; //!< explicit set of STAGES 
  bool is_symbolic_stages; //!< if stage names are symbolic or numeric
  SyntaxNode *nodeset;   //!< The set of NODES 
  SyntaxNode *anc;       //!< The parameter array of ancestors
  SyntaxNode *prob;      //!< The parameter array of probabilities

  // -------------------------- methods ----------------------------------
  //! Constructor 
  StochModel(SetNode *onStages, SyntaxNode *onNodes, SyntaxNode *onAncs, 
	     SyntaxNode *onProb, AmplModel *parent);

  //! Expand the StochModel to a nested set of FlatModels 
  AmplModel *expandToFlatModel();

  //! recursive helper function for expandToFlatModel 
  void _transcribeComponents(AmplModel *current, int level);

  //! expands the STAGES set into the actual elements and stores them in stagenames 
  void expandStages();

  /** expands all STAGES set associated with StochModelComp components into 
   * the actual elements and stores them in  StochModelComp->stagenames */
  //!< expand STAGES set of all StochModelComps in this model
  void expandStagesOfComp(); 

  /** expand on AmplModel::addComp to setup stochmodel of component too */
  void addComp(ModelComp *comp);

};

#endif
