#ifndef SETELH
#define SETELH

#include <string>
#include <assert.h>
#include "nodes.h"

/** @class SetElement
 *  This class describes an element of a set. It is basically an array of
 *  strings (char*) together with a size.
 */

class SetElement {
 public:
  const int n;
  string *val;
  /* ----------------------------- methods -------------------------------*/

  SetElement(int n=0, char **val=NULL);
  SetElement(int n, IDNode **val);
  ~SetElement();

  bool operator()(const SetElement el1, const SetElement el2) const;

  char* toCharA();

};
#endif
