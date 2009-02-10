#ifndef SETNODE_H
#define SETNODE_H

#include <cassert>
#include <vector>
#include "nodes.h"
#include "CompDescr.h"
#include "sml.tab.h"

/** @class SetNode
 * This class represents a set in the the syntax tree.
 */
class SetNode: public SyntaxNode, public CompDescr {
public:
   SetNode(int opCode, SyntaxNode *node1=NULL, SyntaxNode *node2=NULL) :
      SyntaxNode(opCode, node1, node2) {}
};

/** @class SimpleSet
 * This calls represents a contigous set defined using 1..T or similar
 */
class SimpleSet: public SetNode {
private:
   int lower_bound_;
   SyntaxNode *lbc_;
   int upper_bound_;
   SyntaxNode *ubc_;
   int interval_;

public:
   bool parsed_; // did we suceed at parsing, or do we need to use ampl on it?
   SimpleSet(SyntaxNode *bnd1, SyntaxNode *bnd2);
   vector<string> members(AmplModel &context);
};

/** @class ListSet
 * This class represents a set with explicitly enumerated members
 */
class ListSet: public SetNode {
public:
   ListSet(SyntaxNode *list) :
      SetNode(LBRACE, list) {}
};

/** @class CompositeSet
 * This class represents a set composed of other sets
 */
class CompositeSet: public SetNode {
public:
   CompositeSet(int opCode, SyntaxNode *set1, SyntaxNode *set2) :
      SetNode(opCode, set1, set2)
   {
      assert((opCode==CROSS) || (opCode==DIFF));
   }
};

#endif /* ifndef SETNODE_H */
