#ifndef COMPDESCRPARAM_H
#define COMPDESCRPARAM_H

#include <vector>
#include <string>
#include "Set.h"
#include "CompDescr.h"

/** @class CompDescrParam
 *  This class describes a parameter: it consists of
 *   - a list of indexing sets (these might be multidimensional themselves) 
 *   - a list of parameter values (in dense format?) - 
 *         i.e. a multidimensional array
 */  
class CompDescrParam: public CompDescr{
 public:
  int nix; //!< number of indices
  int nsets; //!< number of indexing sets (different to nix) 
  Set **indices; //!< pointers to the indexing sets

  /** Total number of entries: product of the number of elements in all 
   *  indexing sets */
  int n; //!< total number of entries
  int nread; //!< number of values that are given so far

  bool is_symbolic; //!< indicates if this is a symbolic set or not
  
  double *values; //!< the array of values
  string *symvalues; //!< the array of values for symbolic params 

  // ------------------ constructors ---------------------
  
  CompDescrParam(); //!< Default Constructor

  /** Construct given data file description and model component */
  CompDescrParam(model_comp *mc, opNode *desc); //!< Construct from data

  string printToString();

 private:
  /** Service routine that processes a tree below a TOKVALUETABLELIST node */
  void processValueTableList(opNode *node, opNodeIx *ix);

};
#endif
