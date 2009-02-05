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
 public:
  int dim; //!< A set can be mutidimensional
 private:
  Set(); //!< default constructor

  // ---------------- methods -----------------
  void add(SetElement);     //!< add element to set

  // virtal printToString inherited from CompDescr
  string printToString(); //!< print to string  
 public:
  Set(SyntaxNode *list_of_els); //!< construct a set from a list of elements in an SyntaxNode
  int size(); //!< return size of Set
  int findPos(SetElement);  //!<find position of element in set
};

#endif
