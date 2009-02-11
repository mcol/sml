#ifndef DATANODES_H
#define DATANODES_H

#include "nodes.h"
#include "data.tab.h"

/** Wraps a set template with an object list */
class SetSpec : public SyntaxNode {
  public:
   const SyntaxNode *tmpl;
   const ListNode *list;

  public:
   SetSpec(const SyntaxNode *new_tmpl, const ListNode *new_list) :
      SyntaxNode(TOKSETSPEC), tmpl(new_tmpl), list(new_list) {}
};

#endif
