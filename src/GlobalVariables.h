/** @class GlobalVariables 
 *  @brief This class provides some static global variables
 *
 *  This class provides lgobal variables that control the level of debug
 *  printing and logging.
 *  It also stores the names of the model and data input files.
 *  The initial values for these variables is set in ampl.ypp. Any class
 *  that wishes to use this variables must include "GlobalVariables.h".
 */

class GlobalVariables{
 public:
  static char *modelfilename; //!< The name of the model file 
  static char *datafilename;  //!< The name of the data file

  /** prtLvl is 0: no printing
                1: log: just phases and statistics for every phase
		2: detailed log
		3: debugging printing
   */
  static int prtLvl; //!< The level of debugging printing to screen.
  
  static bool logParseModel; //!< Controls if the model parser should log
  //static bool logParseData;
};
