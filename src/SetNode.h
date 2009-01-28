#ifndef SETNODE_H
#define SETNODE_H

#include <cassert>
#include <vector>
#include "nodes.h"
#include "sml.tab.h"

/** @class SetNode
 * This class represents a set in the the syntax tree.
 */
class SetNode: public opNode {
public:
   SetNode(int opCode, opNode *node1=NULL, opNode *node2=NULL) :
      opNode(opCode, node1, node2)
   {
   }
};

/** @class SimpleSet
 * This calls represents a contigous set defined using 1..T or similar
 */
class SimpleSet: public SetNode {
private:
   int lower_bound_;
   int upper_bound_;
   int interval_;

public:
   bool parsed_; // did we suceed at parsing, or do we need to use ampl on it?
   SimpleSet(opNode *bnd1, opNode *bnd2);
   vector<string> members();
};

/** @class ListSet
 * This class represents a set with explicitly enumerated members
 */
class ListSet: public SetNode {
public:
   ListSet(opNode *list) :
      SetNode(LBRACE, list)
   {
   }
};

/** @class CompositeSet
 * This class represents a set composed of other sets
 */
class CompositeSet: public SetNode {
public:
   CompositeSet(int opCode, opNode *set1, opNode *set2) :
      SetNode(opCode, set1, set2)
   {
      assert((opCode==CROSS) || (opCode==DIFF));
   }
};

#endif /* ifndef SETNODE_H */
