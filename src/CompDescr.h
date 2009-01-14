#ifndef COMPDESCR
#define COMPDESCR

#include <string>
/** @class CompDescr
 *  This is a superclass for all Component Description classes such as
 *  Set, CompDescrParam
 *  Its main purpose is to provide a type (other than void*) that can be
 *  used in the ->value field of model_comp
 */

class CompDescr{
 public:
  virtual string printToString() {};
};

#endif
