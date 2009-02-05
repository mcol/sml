#include "ModelComp.h"
#include "StochModel.h"
/** @class StochModelComp
 *  @brief The class describes an entity in a stochastic model
 *
 *  The class stores information that is read in from a stochastic block
 *  It is equivalent to the ModelComp class, the only difference is that
 *  is also stores an expression corresponding to the applicable stageset
 *  and a possible deterministic attribute
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

  /** This component is repeated over all nodes belonging to stages that 
   *  are listed in the stageset
   *  stageset is a SyntaxNode giving a set expression (to be expanded by AMPL) */
  SyntaxNode *stageset;       //!< set of stages in which component is present

  /** This component is repeated over all nodes belonging to stages that 
   *  are listed in the stageset
   *  stagenames is the expanded list of stage set members */
  vector<string> *stagenames; //!< list of stages in which component is present

  StochModel *stochmodel; //!< points to the StochModel that this belongs to
  /* ======================== methods =================================== */
  StochModelComp();  //!< constructor that sets everything to default values

  //! constructor
  StochModelComp(char *id, compType type, SyntaxNode *indexing, SyntaxNode *attrib);

  //! transcribe a StochModelComp in a StochModel into a ModelComp 
  ModelComp *transcribeToModelComp(AmplModel *current_model, int level);

  StochModelComp *clone(); //!< shallow copy. Only copy pointers.
};
