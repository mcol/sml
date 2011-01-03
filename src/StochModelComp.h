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

#ifndef STOCHMODELCOMP_H
#define STOCHMODELCOMP_H

#include "ModelComp.h"
#include "StochModel.h"
#include <string>
#include <vector>

class SyntaxNode;

/** @class StochModelComp
 *  The class describes an entity in a stochastic model.
 *
 *  The class stores information that is read in from a stochastic block
 *  It is equivalent to the ModelComp class, the only difference is that
 *  is also stores an expression corresponding to the applicable stageset
 *  and a possible deterministic attribute.
 *
 *  This component is repeated over all nodes belonging to stages that are
 *  listed in the stageset.
 */
class StochModelComp: public ModelComp {

 private:
  /* FIXME: No good idea, hides the AmplModel *model in ModelComp */
  //StochModel *model; 

  /** The following field are only used for components in stochastic blocks
   *  Usually all stochastic block components are repeated over all nodes
   * in the scenario tree:
   * A deterministic component only varies over stages, not over nodes */
  bool is_deterministic;  //!< if component is deterministic

  /** Set of stages in which component is present.
   *  stageset is a SyntaxNode giving a set expression to be expanded by AMPL */
  SyntaxNode *stageset;

  /** List of stages in which this component is present.
   *  stagenames is the expanded list of stage set members. */
  std::vector<std::string> stagenames;

 public:

  /** StochModel that this belongs to */
  StochModel *stochmodel;

  /* ======================== methods =================================== */

  //! Constructor that sets everything to default values
  StochModelComp(const std::string& id);

  //! Constructor
  StochModelComp(const std::string& id_, compType type,
                 SyntaxNode *indexing, SyntaxNode *attrib);

  //! Transcribe a StochModelComp in a StochModel into a ModelComp 
  ModelComp *transcribeToModelComp(AmplModel *current_model,
                                   const std::string& nodedummy,
                                   const std::string& stagedummy,
                                   const int level);

  //! Shallow copy, only copies pointers
  StochModelComp *clone() const;

  //! Set the stage set
  void setStageSet(SyntaxNode *stageSet) { stageset = stageSet; }

  //! Retrieve the stage set
  const SyntaxNode* getStageSet() const { return stageset; }

  //! Set whether this component varies only over the stages
  void setDeterministic(bool det) { is_deterministic = det; }

  //! Append a stage name
  void addStageName(const std::string& name);

  //! Get a reference to the vector of stage names
  const std::vector<std::string>& getStageNames() const { return stagenames; }

};

#endif /* STOCHMODELCOMP_H */
