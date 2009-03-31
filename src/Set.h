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
#ifndef SET_H
#define SET_H

#include <vector>
#include <map>
#include <string>
#include "nodes.h"
#include "CompDescr.h"
#include "SetElement.h"

/** @class Set
 *  This class describes a set: it mainly consists of a list of set elements
 *
 *  The element of the sets are described by the vector<string*> elements
 *  Each entry of elements is an array of strings (dimension of the array
 *  is the dimension of the set)
 *
 */
class Set: public CompDescr{
 private:
  /** The actual elements: each element is 
   *   - string* for a one dimensional set, 
   *   - string[] for a multidimensional set 
   * look at http://www.cppreference.com/cppmap/map_constructors.html
   */
  map<SetElement, int, SetElement> elements; //!< The elements of the set

  //! Dimension: a set can be multidimensional
  int dim_;

  // ---------------- methods -----------------
  void add(SetElement);     //!< add element to set

  // virtal printToString inherited from CompDescr
  string printToString(); //!< print to string  
 public:
  Set(const ListNode &list_of_els); //!< construct a set from a list of elements in an SyntaxNode
  int size(); //!< return size of Set

  //! Return the dimension of the set
  int dim();

  int findPos(SetElement);  //!<find position of element in set
};

#endif
