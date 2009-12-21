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

#include <vector>
#include <string>
#include "ModelComp.h"
#include "StochModel.h"

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
 public:
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
  std::vector<std::string> *stagenames;

  /** StochModel that this belongs to */
  StochModel *stochmodel;

  /* ======================== methods =================================== */
  StochModelComp();  //!< constructor that sets everything to default values

  //! Constructor
  StochModelComp(const std::string& id_, compType type,
                 SyntaxNode *indexing, SyntaxNode *attrib);

  //! Transcribe a StochModelComp in a StochModel into a ModelComp 
  ModelComp *transcribeToModelComp(AmplModel *current_model,
                                   const std::string& nodedummy,
                                   const std::string& stagedummy,
                                   const int level);

  //! Shallow copy, only copies pointers
  StochModelComp *clone();
};
