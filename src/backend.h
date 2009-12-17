/* (c) 2008,2009 Jonathan Hogg and Andreas Grothey, University of Edinburgh
 *
 * This file is part of SML.
 *
 * SML is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, using version 3 of the License.
 *
 * SML is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */
#ifndef BACKEND_H
#define BACKEND_H

#include "nodes.h"
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
         SyntaxNodeIx does provide all these. Should replace the stack of
	 add_index objects by a stack of SyntaxNodeIx objects
*/
typedef struct add_index_st { 
  SyntaxNode *dummyVar;     //!< an SyntaxNode representing the dummy variable expr */
  SyntaxNode *set;          //!< an SyntaxNode representing the set */
} add_index;

/* some global variables that change the behaviour of some printing routines */
//extern int n_addIndex;           /* number and list of indexing expressions */
//extern add_index *l_addIndex[5];  /* to add to all statements */
extern vector <list <add_index*>* > l_addIndex;

void process_model(AmplModel *model, const char *datafilename);
#endif
