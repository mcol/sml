#ifndef BACKEND_H
#define BACKEND_H

#include "nodes.h"
#include "ampl.h"
#include <list>
#include <vector>
/* this struct stores an indexing expression in an easy to modify form:
   the add_index below will be rendered as
   val(dummyVar) in val(set) 
     or just
   val(set)
     if dummyVar is NULL
*/

/** \brief An entry on the indexing expressions stack.

    add_index implements a stack of applicable indexing expressions:
    for processing the model. Indexing expressions on the stack come from
    block definitions that have been passed.
    
    Indexing expressions are stored separately by dummy variable part
    (the bit before the 'in' keyword) and set part (the bit after the
    'in' keyword)

    \attention this stack could be implemented as its own class:
               these two would become static class variables 
    \bug this is fairly dumb at the moment: it cannot deal with 
          - multiple dimensions {i in SET1,j in SET2} 
          - SET valued expressions: {i in SET1 cross SET2} 
          - conditions:    {(i,j) in SET1:i<j}
          .
         opNodeIx does provide all these. Should replace the stack of
	 add_index objects by a stack of opNodeIx objects
*/
typedef struct add_index_st { 
  opNode *dummyVar;     //!< an opNode representing the dummy variable expr */
  opNode *set;          //!< an opNode representing the set */
} add_index;

/* some global variables that change the behaviour of some printing routines */
//extern int n_addIndex;           /* number and list of indexing expressions */
//extern add_index *l_addIndex[5];  /* to add to all statements */
extern vector <list <add_index*>* > l_addIndex;

void do_stuff(AmplModel *model);


#endif
