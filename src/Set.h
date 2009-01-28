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
 public:
  //int n; //!< number of elements in set
  int dim; //!< A set can be mutidimensional

  /** The actual elements: each element is 
   *   - string* for a one dimensional set, 
   *   - string[] for a multidimensional set 
   * look at http://www.cppreference.com/cppmap/map_constructors.html

   */
  map<SetElement, int, SetElement> elements; //!< The elements of the set

  // ---------------- constructors -----------------
  Set(); //!< default constructor

  Set(opNode *list_of_els); //!< construct a set from a list of elements in an opNode

  // ---------------- methods -----------------

  /** print a list of set elements to a string */
  int size(); //!< return size of Set
  int findPos(SetElement);  //!<find position of element in set
  void add(SetElement);     //!< add element to set
  string *getAt(int i);   //!< get a certain element of the set

  // virtal printToString inherited from CompDescr
  string printToString(); //!< print to string  
};

#endif
