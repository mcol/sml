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
#include "Set.h"
#include <map>
#include <cstdio>
#include <string>
#include <assert.h>
#include "data.tab.h"
#include "misc.h"

/* ---------------------------------------------------------------------------
Set::Set(SyntaxNode *list)
---------------------------------------------------------------------------- */
/*! Constructs the Set from a list of set elements given as a tree of SyntaxNodes
 *
 *  @param list a description of the set elements as read in from the data file
 *
 * This constructor assumes that the parameter list describes the set elements
 * in the following format:
 * - The top node is of type SyntaxNode.opCode=' ', 
 *    SyntaxNode::nval=[\#items in the list]
 * - Each child describes one element of the set and is of either of the 
 *   forms
 *   + SyntaxNode.opCode=ID|INT_VAL|FLOAT_VAL, SyntaxNode.values[0] = 
 *   + (...,...,..) represented as SyntaxNode.opCode=LBRACKET with one
 *     child of type COMMA. 
 *     The COMMA SyntaxNode is a list, with number of entries equal to the 
 *     dimension of the set and each element of type opCode=ID 
 *     (carrying the actual description of the set element).
 */
Set::Set(const ListNode &list):
  elements()
{
  SyntaxNode *item;

  assert(list.opCode==' ');
  
  // have a look at the first item to get the dimension of the set
  item = list[0];
  if (item->opCode==ID||item->opCode==-99){
    this->dim_ = 1;
  }else{
    // otherwise this needs to be an element of form (.., .., ..)
    assert(item->opCode==LBRACKET);
    item = *(item->begin());
    assert(item->opCode==COMMA);
    this->dim_ = item->nchild();
  }
  
  // then place all the elements on the set
  for(SyntaxNode::iterator i=list.begin(); i!=list.end(); ++i){
    item = *i;
    // and do some checking that all elements have the same dimension
    //    this->elements.push_back(item);
    if (dim_==1) {
      char** array = (char**)calloc(1, sizeof(char*));
      array[0] = item->getValue();
      add(SetElement(1,array));
    }else{
      char **array = (char**)calloc(dim_, sizeof(char*));
      assert(item->opCode==LBRACKET);
      item = (SyntaxNode*)*(item->begin());
      assert(item->opCode==COMMA);
      if (dim_==item->nchild()){
        int j = 0;
        for(SyntaxNode::iterator k=item->begin(); k!=item->end(); ++k){
          SyntaxNode *idnd = (SyntaxNode*)*k;
          assert(idnd->opCode==ID);
          array[j++] = (char*)*(idnd->begin());
        }
        add(SetElement(dim_, array));
        //this->elements.push_back(array);
      }else{
        cerr << "First element in set has dim=" << dim_ << " later element '"
           << (SyntaxNode*)*i << "' has dim=" << item->nchild() << "\n";
        exit(1);
      }
    }
  }
}


/* ---------------------------------------------------------------------------
Set::size();
---------------------------------------------------------------------------- */
int
Set::size()
{
  return elements.size();
}

/* ---------------------------------------------------------------------------
Set::dim();
---------------------------------------------------------------------------- */
int
Set::dim()
{
  return dim_;
}


/* ---------------------------------------------------------------------------
Set::printToString
---------------------------------------------------------------------------- */
string 
Set::printToString()
{
  map<SetElement,int,SetElement>::iterator iter;
  string str="";
  for( iter = elements.begin(); iter != elements.end(); ++iter ) {
    if (iter!=elements.begin()) str += " ";
    // iter is of type 'pair*'
    SetElement element = iter->first;
    int pos = iter->second;
    str += (to_string(pos)+":"+element.val[0]);
  }
  return str;

}


/* ---------------------------------------------------------------------------
Set::add(SetElement newel)
---------------------------------------------------------------------------- */
void
Set::add(SetElement newel)
{
  int n = elements.size();
  elements.insert(pair<SetElement, int>(newel, n));
}

/* ---------------------------------------------------------------------------
Set::findPos(string *el)
---------------------------------------------------------------------------- */
int
Set::findPos(SetElement el)
{ 
  map<SetElement, int, SetElement>::iterator iter = elements.find(el);
  if( iter != elements.end() ) {
    return iter->second;
  }else{
    printf("ERROR: Set.cpp: Trying to find element '%s' in Set %s\n",
           el.toCharA(), printToString().c_str());
    printf("Element %s is not in Set\n",el.toCharA());
    exit(1);
    return -1;
  }
}

