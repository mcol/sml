#include "Set.h"
#include <map>
#include <string>
#include <assert.h>
#include "data.tab.h"
#include "misc.h"

/* ---------------------------------------------------------------------------
Set::Set()
---------------------------------------------------------------------------- */
Set::Set():
  dim(1),
  //n(0),
  elements()
{}

/* ---------------------------------------------------------------------------
Set::Set(opNode *list)
---------------------------------------------------------------------------- */
/*! Constructs the Set from a list of set elements given as a tree of opNodes
 *
 *  @param list a description of the set elements as read in from the data file
 *
 * This constructor assumes that the parameter list describes the set elements
 * in the following format:
 * - The top node is of type opNode.opCode=' ', 
 *    opNode::nval=[\#items in the list]
 * - Each child describes one element of the set and is of either of the 
 *   forms
 *   + opNode.opCode=ID|INT_VAL|FLOAT_VAL, opNode.values[0] = 
 *   + (...,...,..) represented as opNode.opCode=LBRACKET with one
 *     child of type COMMA. 
 *     The COMMA opNode is a list, with number of entries equal to the 
 *     dimension of the set and each element of type opCode=ID 
 *     (carrying the actual dscription of the set element).
 */
Set::Set(opNode *list):
  elements()
{
  opNode *item;

  assert(list->opCode=' ');
  //this->n = list->nval;
  
  // have a look at the first item to get the dimension of the set
  item = (opNode *)*(list->begin());
  if (item->opCode==ID||item->opCode==INT_VAL||item->opCode==FLOAT_VAL){
    this->dim = 1;
  }else{
    // otherwise this needs to be an element of form (.., .., ..)
    assert(item->opCode==LBRACKET);
    item = (opNode*)*(item->begin());
    assert(item->opCode==COMMA);
    this->dim = item->nval;
  }
  
  // then place all the elements on the set
  for(opNode::Iterator i=list->begin(); i!=list->end(); ++i){
    item = (opNode*) *i;
    // and do some checking that all elements have the same dimension
    //    this->elements.push_back(item);
    if (dim==1) {
      char** array = (char**)calloc(1, sizeof(char*));
      //string *array = (string*)calloc(1, sizeof(string));
      assert(item->opCode==ID||item->opCode==INT_VAL||item->opCode==FLOAT_VAL);
      array[0] = item->getValue();
      //this->elements.push_back(array);
      add(SetElement(1,array));
    }else{
      //string* array = new string[dim];
      char **array = (char**)calloc(dim, sizeof(char*));
      assert(item->opCode==LBRACKET);
      item = (opNode*)*(item->begin());
      assert(item->opCode==COMMA);
      if (dim==item->nval){
        int j = 0;
        for(opNode::Iterator k=item->begin(); k!=item->end(); ++k){
          opNode *idnd = (opNode*)*k;
          assert(idnd->opCode==ID);
          array[j++] = (char*)*(idnd->begin());
        }
        add(SetElement(dim, array));
        //this->elements.push_back(array);
      }else{
        cerr << "First element in set has dim=" <<dim << " later element '"
           << (opNode*)*i << "' has dim=" << item->nval << "\n";
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
    str += (to_string(pos)+":"+string(element.val[0]));
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

