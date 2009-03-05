class OOPSBlock;

#include "ExpandedModelInterface.h"
#include "AmplsolverCalls.h"

/** @class OOPSBlock
 * @brief  
 * OOPSBlock is an object that directly corresponds to a node in the
 *  OOPS Algebra Tree. 
 *  
 * It is characterized by 
 *   - An ExpandedModelInterface/NlFile that gives information on the rows 
 *     in this block
 *   - A list of variable definitions (given as the start of variable names)
 *     that should be used from the NlFile.
 *  This class will do the necessary interfacing (i.e. extract a list
 *  of variable indices from the NlFile that should be used and indicate
 *  In which positions these should appear in the block
 */
class OOPSBlock {
 public:
  ExpandedModelInterface *emrow;    //!< Expanded Model giving row information
  ExpandedModelInterface *emcol;    //!< Expanded Model giving col information
  //  NlFile *nlfile;       //!< The NlFile correspoding to the ExpandedModelInterface
  int ncon;             //!< number of rows in this block
  int nvar;             //!< number of columns in this block
  //  int *lvar;            //!< list of indices into the NlFile (-1 if not decl)
  // --------------------------- methods -----------------------------------
  /** constrct an OOPS block from the cross section of two ExpandedModelInterface's
   *  @param rowmod       The model giving the row information
   *  @param colmod       The model giving the column information
   */
  OOPSBlock(ExpandedModelInterface *rowmod, ExpandedModelInterface *colmod);
};

