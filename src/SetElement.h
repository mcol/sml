#ifndef SETELH
#define SETELH

#include <string>
#include <assert.h>

/** @class SetElement
 *  This class describes an element of a set. It is basically an array of
 *  strings (char*) together with a size.
 */

class SetElement {
 public:
  int n;
  char **val;
  /* ----------------------------- methods -------------------------------*/
  SetElement();

  SetElement(int n, char **val);

  bool operator()(const SetElement el1, const SetElement el2) const;

  char* toCharA();

};
#endif
