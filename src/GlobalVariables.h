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
#ifndef GLOBALVARIABLES
#define GLOBALVARIABLES

#include <string>

/** @class GlobalVariables 
 *  This class provides some static global variables.
 *
 *  This class provides global variables that control the level of debug
 *  printing and logging.
 *  It also stores the names of the model and data input files.
 *  The initial values for these variables is set in ampl.ypp. Any class
 *  that wishes to use this variables must include "GlobalVariables.h".
 */
class GlobalVariables{
 public:

  //! Name of the data file
  static std::string datafilename;

  //! Command used for invoking ampl
  static const std::string amplcommand;

  /** prtLvl is 0: no printing
                1: log: just phases and statistics for every phase
		2: detailed log
		3: debugging printing
   */
  static int prtLvl; //!< The level of debugging printing to screen.
  
  static bool logParseModel; //!< Controls if the model parser should log
};

#endif
