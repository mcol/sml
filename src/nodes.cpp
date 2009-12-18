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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include "nodes.h"
#include "sml.tab.h"
#include "AmplModel.h"
#include "ModelComp.h"    // for WITHARG
#include "GlobalVariables.h"

using namespace std;

static bool logCreate = false;
extern int n_indexing;
extern SyntaxNodeIx *list_of_indexing[20];

int SyntaxNode::use_global_names=0;
AmplModel *SyntaxNode::default_model =NULL;
string StageNodeNode::node = "";
string StageNodeNode::stage = "";

/* --------------------------------------------------------------------------
addItemToListNew
-------------------------------------------------------------------------- */
SyntaxNode *SyntaxNode::push_back(SyntaxNode *newitem)
{
  /* extend the size of the node by one */
  SyntaxNode **newvalues = new SyntaxNode*[nval + 1];
  int i;

  for (i=0;i<nval;i++){
    newvalues[i] = values[i];
  }
  newvalues[nval] = newitem;
  nval++;
  delete[] values;
  values = newvalues;
  return this;
}
/* --------------------------------------------------------------------------
addItemToListBeg
-------------------------------------------------------------------------- */
SyntaxNode *SyntaxNode::push_front(SyntaxNode *newitem)
{
  /* extend the size of the node by one */
  SyntaxNode **newvalues = new SyntaxNode*[nval + 1];
  int i;

  for (i=0;i<nval;i++){
    newvalues[i+1] = values[i];
  }
  newvalues[0] = newitem;
  nval++;
  delete[] values;
  values = newvalues;
  return this;
}

/* --------------------------------------------------------------------------
addItemToListOrCreate
-------------------------------------------------------------------------- */
/** A 'List' is a SyntaxNode of opCode COMMA or ' ' with a variable number
 *  of arguments. 
 *  This function takes (a possibly existing) list and adds an item to it.
 *
 *  Both the list and the item can be NULL:
 *  - if the item is NULL then the old list is simply returned;
 *  - if the list is NULL then a list with opCode 'oc' is created from the
 *    single item that is passed.
 */

ListNode *
addItemToListOrCreate(int oc, ListNode *list, SyntaxNode *newitem)
{
  if(!newitem) return list;

  if (list){
    assert(oc==list->opCode);
    return (ListNode*) list->push_back(newitem);
  }else{
    assert(oc==' '||oc==COMMA);
    return new ListNode(oc, newitem);
  }
}

/* --------------------------------------------------------------------------
SyntaxNode::print()
--------------------------------------------------------------------------- */
/** Recursively prints the expression rooted at the current node in the
 *  expression tree.
 *
 *  @note use_global_names influences how nodes of type IDREF are printed.
 */
ostream&
operator<<(ostream&s, const SyntaxNode *node) {
   if(node == NULL) return s;
   return node->put(s);
}

ostream&
operator<<(ostream&s, const SyntaxNode &node) {
   return node.put(s);
}

ostream& SyntaxNode::put(ostream&s) const {
  const SyntaxNodeIDREF *onidref;
  static int level=0;

  if(this == NULL) return s;

  SyntaxNode::iterator i = this->begin();
  /*if(s!=cout) {
     for(int j=0; j<level; ++j) cout << " ";
     if(level!=0) cout << "-";
     level++;
     cout << "here " << this->opCode << "(" << node << ")\n";
  }*/

  switch (this->opCode)
    {
    case 0:          s << **i;                           break;
      /* these are lots of simple binary operators */
    case ID:
                     //s << (const char*)*i;               break;
      cerr << "ID put bad." << endl;
      throw exception();
      break;
    case ' ':        
      if(this->nval>1) s << **(i++);
      s << ' ' << **i;
      break;
    case DOT:
      s << **i << ".";
      s << **(++i);
      break;
    case COMMA:
      s << **i;
      for(++i;i!=this->end();++i) {
         s << "," << (**i);
      }
      break;
    case DIFF:
      if(this->nval>1) s << **(i++);
      s << " diff " <<  **i;
      break;
    case CROSS:
      if(this->nval>1) s << **(i++);
      s << " cross " <<  **i;
      break;
    case DOTDOT:
      if(this->nval>1) s << **(i++);
      s << " .. " <<  **i;
      break;
    case SUM:
      s << "sum " << **i;
      s << **(++i);
      break;
    case MAX:
      s << "max " << **i;
      s << **(++i);
      break;
    case MIN:
      s << "min " << **i;
      s << **(++i);
      break;
    case EXPECTATION:
      s << "Exp( " << **i << ")";
      break;
    case LAST:
      s << "last( " << **i << ")";
      break;
    case FIRST:
      s << "first( " << **i << ")";
      break;
      // -------------------------functions f(..) --------------------------
    case ORD:
      s << "ord" << **i;
      break;
      // -------------------------terminals --------------------------
    case ORDERED:    s << " ordered";                    break;
    case SYMBOLIC:   s << " symbolic";                   break;
    case DETERMINISTIC: s << " deterministic";           break;
      /* these are lots of simple unary operators */
    case WITHIN:
      s << "within " << **i;
      break;
    case LSBRACKET:
      if (this->nval==1){
         s << "[" << **i << "]";
      } else {
         s << **i << "[";
         s << **(++i) << "]";
      }
      break;
    case LBRACE:
      s << "{" << *i << "}";
      break;
    case LBRACKET:
      s << "(" << **i << ")";
      break;
    case COLON:
      if(this->nval>1) s << **(i++);
      s << ":" <<  **i;
      break;
    case IF:
      s << "if " << **i;
      s << " then " << **(++i);
      if(this->nval==3) s << " else " << **(++i);
      break;
    case IDREF:
    case IDREFM:
      if(!(onidref = (const SyntaxNodeIDREF*)(this))) {
         cerr << "Cast of node to SyntaxNodeIDREF failed!\n";
         exit(1);
      }
      s << onidref;
      break;
    case -99: // template<class T> ValueNode
      cerr << "FAIL(-99)";
      throw exception();
      break;
    default: 
      s << endl;
      cerr << "Unknown opcode " << this->opCode << "\n";
      cerr << ".nval = " << this->nval << "\n";
      for(; i!=this->end(); ++i) {
         cerr << "val[" << **i << "]\n";
      }
      throw exception();
      exit(1);
    }
  if(s!=cout) level--;
  return s;
}

ostream& StageNodeNode::put(ostream &s) const {
   if(value_!="") return s << value_;

   switch(opCode) {
      case STAGE: s << StageNodeNode::stage;    break;
      case NODE:  s << StageNodeNode::node;     break;
      default:
         cerr << "StageNodeNode::put called badly!" << endl;
         throw exception();
   }
   return s;
}

ostream& OpNode::put(ostream &s) const {
  if(left) s << left;
  
  switch(opCode) {
    case '+':
    case '-':
    case '*':
    case '/':
       s << (char) opCode;
       break;
    case ASSIGN:  s << "=";   break;
    case GE:      s << ">=";  break;
    case GT:      s << ">";   break;
    case LE:      s << "<=";  break;
    case LT:      s << "<";   break;
    case EQ:      s << "==";  break;
    case NE:      s << "!=";  break;
    case IN:      s << " in "; break;
    case DEFINED: s << ":=";  break;
    case POWER:   s << "**"; break;
    default:
      cerr << "Unknown opCode for OpNode: " << opCode << endl;
      exit(1);
  }

  s << right;

  return s;
}

ostream& SyntaxNodeIDREF::put(ostream& s) const {

   switch(opCode) {
   case IDREF:
      if (nval<0) {
	      // as yet unset IDREF
         s << "IDREF";
      } else if (SyntaxNode::use_global_names) {
	      s << getGlobalNameNew(ref, this, default_model, WITHARG);
      } else {
         /* this is the new ID processor */
         if(nval==0) {
            s << ref->id;
         } else {
            SyntaxNode::iterator i=begin();
            s << ref->id << "[" << **i;
            for(++i; i!=end(); ++i){
               s << "," << **i;
            }
            s << "]";
         }
      }
      break;
   case IDREFM:
      /* this is the new ID processor (for submodels) */
      // ??? is this correct
      s << ref->id;
      break;
   default:
      cerr << "In fn SyntaxNodeIDREF::put bu not an IDREF or IDREFM\n";
      exit(1);
   }

   return s;
}

string
SyntaxNode::print()
{
   ostringstream ost;
   ost << (*this);
   return ost.str();
}

void 
SyntaxNode::dump(ostream &fout)
{
  fout << print_SyntaxNodesymb(this) << "\n";
}

string
print_SyntaxNodesymb(SyntaxNode *node)
{
  ValueNode<long> *inode;
  ValueNode<double> *dnode;
  ostringstream  ost;

  if (node==NULL){
    return "NULL";
  }
  if (node->opCode==ID){
    return "(ID T)";
  }
  if ((inode = dynamic_cast<ValueNode<long> *>(node))){
    string temp = "T:";
    temp += inode->getValue();
    return temp;
  }
  if ((dnode = dynamic_cast<ValueNode<double> *>(node))){
    string temp = "T:";
    temp += dnode->getValue();
    return temp;
  }

  // start new version
  // print node symbol
  switch (node->opCode)
  {
  case IDREF: {
    SyntaxNodeIDREF *onir= dynamic_cast<SyntaxNodeIDREF*>(node);
    if (onir==NULL) {
      cerr << "Some IDREF node still not SyntaxNodeIDREF\n";
      exit(1);
    }
    ModelComp *mc = onir->ref;
    ost << "IDREF(" << (void*)node << ":" << mc->id << "(" << (void*) mc <<"))";
  }
  break;
  case ASSIGN: ost << "ASSIGN"; break;
  case IN:     ost << "IN"; break;
  case SUM:    ost << "SUM"; break;
  case LBRACE: ost << "LBR{"; break;
  case LBRACKET:ost << "LBR("; break;
  case COMMA:  ost << "COMMA"; break;
  case COLON:  ost << "COLON"; break;
  case '+':    ost << "\"+\""; break;
  case '*':    ost << "\"*\""; break;
  default:
    ost << "\"" << node->opCode << "\"";
  }
  ost << "(";
  for(SyntaxNode::iterator i=node->begin(); i!=node->end(); ++i){
    if(i!=node->begin()) ost << ", ";
    ost << print_SyntaxNodesymb(*i);
  }
  ost << ")";
  
  return ost.str();
}
/* ==========================================================================
SyntaxNode Methods to follow
============================================================================*/
// constructors:

SyntaxNode::SyntaxNode(SyntaxNode &src) :
   opCode(src.opCode), nval(src.nval), values(src.values) {}

SyntaxNode::SyntaxNode (int code, SyntaxNode *val1, SyntaxNode *val2, SyntaxNode* val3) :
   opCode(code), nval(0), values(NULL)
{
   if(val1) nval++;
   if(val2) nval++;
   if(val3) nval++;

   if(nval) values = new SyntaxNode*[nval];

   int i = 0;
   if(val1) values[i++] = val1;
   if(val2) values[i++] = val2;
   if(val3) values[i++] = val3;

   if (logCreate) cout << "created " << nval << "-ary op: " << opCode << "\n";
}

SyntaxNode::~SyntaxNode() {

  for (int i = 0; i < nval; ++i)
    delete values[i];
  delete[] values;
}

/* --------------------------------------------------------------------------
SyntaxNode *SyntaxNode::deep_copy()
---------------------------------------------------------------------------- */
SyntaxNode *
SyntaxNode::deep_copy()
{
  SyntaxNode *newn = new SyntaxNode(opCode);

  if (opCode==IDREF || opCode==IDREFM){
    cerr << "IDREF SyntaxNodes need to be cloned differently\n";
    exit(1);
  }
  newn->nval = nval;
  if (nval>0)
    newn->values = new SyntaxNode*[nval];

  /* Values are copied depending on the type of the SyntaxNode */
  /* ID/IDREF/INT_VAL/FLOAT_VAL/IDREFM are treated differently */
  if (opCode==ID){
    cerr << "Called deep_copy for ID" << endl;
    throw exception();
    /*assert(nval==1);
    newn->values[0] = strdup((char *)values[0]);
    return newn;*/
  }
  
  for(int i=0;i<nval;i++)
    newn->values[i] = values[i]->deep_copy();
  return newn;
}
/* --------------------------------------------------------------------------
SyntaxNode *SyntaxNode::clone()
---------------------------------------------------------------------------- */
SyntaxNode *
SyntaxNode::clone()
{
  SyntaxNode *newn = new SyntaxNode(opCode);

  if (opCode==IDREF){
    cerr << "IDREF SyntaxNodes need to be cloned differently\n";
    exit(1);
  }
  newn->nval = nval;
  newn->values = new SyntaxNode*[nval];
  for(int i=0;i<nval;i++)
    newn->values[i] = values[i];
  return newn;
}

/* --------------------------------------------------------------------------
char *SyntaxNode::printDummyVar()
---------------------------------------------------------------------------- */
/* Assumes that the current SyntaxNode is the dummy variable in an indexing 
   expression: that is it, is either ID or LBRACKET (COMMA (ID1 .. IDn)) */

string
SyntaxNode::printDummyVar()
{
  if (opCode==ID){
    return this->print();
  }else{
    SyntaxNode *list;
    // this must be LBRACKET
    if (opCode!=LBRACKET){
      cerr << "printDummyVar: dummy var must be ID or (ID1,..,IDn)\n";
      cerr << "current opCode is "+opCode;
      exit(1);
    }
    list = (SyntaxNode*)values[0];
    if (list->opCode==ID) return list->print();
    if (list->opCode!=COMMA){
      cerr << "printDummyVar: dummy var must be ID or (ID1,..,IDn)\n";
      cerr << "current opCode is "+list->opCode;
      exit(1);
    }
    return list->print();
  }	
}

/* --------------------------------------------------------------------------
SyntaxNode::findIDREF()
---------------------------------------------------------------------------- */
/** Find the list of all the IDREF nodes at or below the current node */
void
SyntaxNode::findIDREF()
{
  int i;

  if (opCode==IDREF){
    cout << getGlobalName((ModelComp*)this->values[0], NULL, NULL, NOARG) <<
      endl;
  }else if (opCode==ID) {
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	     values[i]->findIDREF();
      }
    }
  }
}
/* --------------------------------------------------------------------------
SyntaxNode::findIDREF(list<ModelComp> *lmc)
---------------------------------------------------------------------------- */
/** Find the list of all the IDREF nodes at or below the current node */
void
SyntaxNode::findIDREF(list<ModelComp*> &lmc)
{
  int i;

  if (opCode==IDREF){
    //printf("%s\n",getGlobalName((ModelComp*)this->values[0], 
    //				NULL, NULL, NOARG));
    lmc.push_back(((SyntaxNodeIDREF*)this)->ref);
  }else if (opCode==ID) {
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	     ((SyntaxNode*)values[i])->findIDREF(lmc);
      }
    }
  }
}
/* --------------------------------------------------------------------------
SyntaxNode::findIDREF(list<SyntaxNode *> *lnd)
---------------------------------------------------------------------------- */
/** Find the list of all the IDREF nodes at or below the current node */
void
SyntaxNode::findIDREF(list<SyntaxNode*> *lnd)
{
  int i;

  if (opCode==IDREF){
    //printf("%s\n",getGlobalName((ModelComp*)this->values[0], 
    //				NULL, NULL, NOARG));
    lnd->push_back(this);
  }else if (opCode==ID) {
    // if terminal then return
    return;
  }else if (opCode==-99) {
     throw exception();
    cerr << "BAD findIDREF(-99)\n";
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	     values[i]->findIDREF(lnd);
      }
    }
  }
}
/* --------------------------------------------------------------------------
SyntaxNode::findOpCode(int oc, list<SyntaxNode *> *lnd)
---------------------------------------------------------------------------- */
/** Find the list of all nodes with opCode==oc at or below the current node */
void
SyntaxNode::findOpCode(int oc, list<SyntaxNode*> *lnd)
{
  int i;

  if (opCode==oc){
    //printf("%s\n",getGlobalName((ModelComp*)this->values[0], 
    //				NULL, NULL, NOARG));
    lnd->push_back(this);
  }else if (opCode==ID) {
    // if terminal then return
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	     values[i]->findOpCode(oc, lnd);
      }
    }
  }
}

/* --------------------------------------------------------------------------
SyntaxNode::findModelComp()
---------------------------------------------------------------------------- */
/** Find the ModelComp (if any) refered to by this SyntaxNode.
 *
 *  @return The ModelComp only if the expression given by this SyntaxNode is
 *          an immediate reference to a ModelComp, otherwise NULL.
 */
ModelComp *SyntaxNode::findModelComp()
{
  SyntaxNode *on = this;
  while ((on->opCode==LBRACKET || on->opCode==LBRACE) && on->nval==1){
    on = on->values[0];
  }

  if (opCode==IDREF){
    SyntaxNodeIDREF *onref = dynamic_cast<SyntaxNodeIDREF*>(this);
    return onref->ref;
  }
  return NULL;
}

/* --------------------------------------------------------------------------
SyntaxNodeIx::getIndexingSet()
---------------------------------------------------------------------------- */
SyntaxNode *SyntaxNodeIx::getIndexingSet()
{
  SyntaxNode *ix = this;
  SyntaxNode *set;
  SyntaxNode *dummyVar;

  if (ix==NULL) return NULL;
  /* remove outside braces from indexing expression */
  if (ix->opCode==LBRACE) ix = *(ix->begin());
  /* assumes that the next level is the 'IN' keyword (if present) */
  if (ix->opCode==IN){
    SyntaxNode::iterator i = ix->begin();
    dummyVar = *i;
    set = *(++i);
  }else{
    dummyVar = NULL;
    set = ix;
  }
  return set;
}

/* --------------------------------------------------------------------------
SyntaxNode::getArgumentList()
---------------------------------------------------------------------------- */
/** This is for an SyntaxNode of type IDREF (and should eventually be moved
 *  to SyntaxNodeIDREF:getArgumentList()).
 *
 *  @return A comma separated list of the arguments (the bit in [..] brackets).
 */
string
SyntaxNode::getArgumentList() const
{
   const SyntaxNodeIDREF *on;
   string arglist = "";
   if (opCode!=IDREF){
      cerr << "Can only call getArgumentList for SyntaxNodes of type IDREF\n";
      exit(1);
   }

   // see if this is actually an IDREF node
   on = dynamic_cast<const SyntaxNodeIDREF*>(this);
   if (on==NULL){
      cout << "WARNING: This is an IDREF SyntaxNode not of type SyntaxNodeIDREF\n";
      if (nval>0){
         arglist += values[1]->print();
         for(int i=1;i<nval;i++){
	         arglist += ",";
	         arglist += values[1+i]->print();
         }
      }
   }else{
      if (nval>0){
         SyntaxNode::iterator i = begin();
         arglist += (*i)->print();
         for(++i;i!=end();++i){
            arglist += ",";
	         arglist += (*i)->print();
         }
      }
  }
  return arglist;
}

/** Merges the values list of src into that of this object.
 *
 *  The items from src are prepended to this object's values.
 */
SyntaxNode &SyntaxNode::merge(const SyntaxNode &src) {
   SyntaxNode **newvalues = new SyntaxNode*[src.nval + nval];

   SyntaxNode **ptr = newvalues;
   for(int i=0;i<src.nval;i++)
      *(ptr++) = src.values[i];
   for(int i=0;i<nval;i++)
      *(ptr++) = values[i];
   nval += src.nval;
   delete[] values;
   values = newvalues;

   return (*this);
}


/* ==========================================================================
SyntaxNodeix Methods to follow
============================================================================*/
SyntaxNodeIx::SyntaxNodeIx(SyntaxNode *on) :
   SyntaxNode(*on)
{
  qualifier = NULL;
  ncomp = 0;
  sets = NULL;
  sets_mc = NULL;
  dummyVarExpr = NULL;
  done_split = 0;
  splitExpression();
}

/* ---------------------------------------------------------------------------
SyntaxNodeIx::printDiagnostic
-----------------------------------------------------------------------------*/
void
SyntaxNodeIx::printDiagnostic(ostream &fout)
{
  if (!done_split) splitExpression();
  fout << "qualifier: " << qualifier << "\n";
  fout << "number of indexing expressions: " << ncomp << "\n";
  for(int i=0;i<ncomp;i++){
    fout << i << ": dummyVar: " << dummyVarExpr[i] << "\n";
    fout << i << ": set     : " << sets[i] << "\n";
  }
}

/* ---------------------------------------------------------------------------
SyntaxNodeIx::getListDummyVars
-----------------------------------------------------------------------------*/
list<SyntaxNode *>
SyntaxNodeIx::getListDummyVars()
{
  list<SyntaxNode *> l;
  
  for(int i=0;i<ncomp;i++){
    SyntaxNode *dv = dummyVarExpr[i];
    // a dummy var expression is either ID/IDREF or (ID1,...IDn)
    if (dv->opCode==ID||dv->opCode==IDREF){
      l.push_back(dv);
    }else if(dv->opCode==LBRACKET){
      dv = *(dv->begin());
      if (dv->opCode!=COMMA){
	     cerr << "A dummy variable expression is either ID or (ID1,...ID2)\n";
	     cerr << "Given expression: " << dv << "\n";
	     exit(1);
      }
      ListNode *dvl = static_cast<ListNode *>(dv);
      for(ListNode::literator j=dvl->lbegin(); j!=dvl->lend(); ++j){
	     l.push_back(*j);
      }
    }else{
      cerr << "A dummy variable expression is either ID or (ID1,...ID2)\n";
      cerr << "Given expression: " << dv << "\n";
      exit(1);
    }
  }
  return l;
}

/* ---------------------------------------------------------------------------
SyntaxNodeIx::splitExpression
-----------------------------------------------------------------------------*/
/* sets up the ->set, ->dummyVar components of the class 

 A general indexing expression can be of the form

    {(i,j) in ARCS, k in SET: i>k} 
*/
void SyntaxNodeIx::splitExpression()
{
  SyntaxNode *tmp, *tmp2;
  int i;

  if (done_split) return;
  done_split=1;

  if (opCode!=LBRACE){
    cerr << "Error in splitExpression: Indexing Expression must start with {\n";
    cerr << "     " << this;
    exit(1);
  }

  tmp = *(this->begin());
  // discard the colon (if there is one present: only interested in lhs) 
  if (tmp->opCode==COLON) {
    SyntaxNode::iterator tj = tmp->begin();
    tmp = *tj; // step down tree
    qualifier = *(++tj); // qualifier from previous node
  }else{
    qualifier = NULL;
  }
  /* this should now be a comma separated list */
  if (tmp->opCode==COMMA){
    ListNode *tmpl = static_cast<ListNode*>(tmp);
    ncomp = tmp->nchild();
    this->sets = new SyntaxNode*[ncomp];
    this->sets_mc = (ModelComp**)calloc(ncomp, sizeof(ModelComp*));
    this->dummyVarExpr = new SyntaxNode*[ncomp];
    i=0;
    for(ListNode::literator ti=tmpl->lbegin(); ti!=tmpl->lend(); ++ti, ++i){
      tmp2 = findKeywordinTree(*ti, IN);
      /* everything to the left of IN is a dummy variables */
      if (tmp2){
        SyntaxNode::iterator j = tmp2->begin();
	     dummyVarExpr[i] = *j;
	     sets[i] = *(++j);
      } else {
	/* just set, but no dummyVar given */
	     dummyVarExpr[i] = NULL;
	     sets[i] = *ti;
      }
      /* try to find ModelComp of the set expression, 
	 If it doesn't exist create */
      if (ModelComp *mc = sets[i]->findModelComp()){
	     sets_mc[i] = mc;
      }else{
	     sets_mc[i] = new ModelComp("dummy", TSET, NULL, sets[i]);
      }
    }
  }else{
    ncomp = 1;
    this->sets = new SyntaxNode*[1];
    this->sets_mc = (ModelComp**)calloc(1, sizeof(ModelComp*));
    this->dummyVarExpr = new SyntaxNode*[1];
    tmp2 = findKeywordinTree(tmp, IN);
    if (tmp2){
      SyntaxNode::iterator tj = tmp2->begin();
      dummyVarExpr[0] = *tj;
      sets[0] = *(++tj);
    } else {
      /* just set, but no dummyVar given */
      dummyVarExpr[0] = NULL;
      sets[0] = tmp;
    }
    /* try to find ModelComp of the set expression, 
       If it doesn't exist create */
    if (ModelComp *mc = sets[0]->findModelComp()){
      sets_mc[0] = mc;
    }else{
      sets_mc[0] = new ModelComp("dummy", TSET, NULL, sets[0]);
    }
  }
}

/*----------------------------------------------------------------------------
SyntaxNodeIx::hasDummyVar
---------------------------------------------------------------------------- */
/** Sees if the indexing Expression given by SyntaxNodeIx defines the 
 *  dummy variable given by name.
 *
 *  @param name
 *         The name of the dummy variable to look for.
 *  @return The ("ID") SyntaxNode representing the dummy Variable (if found) or
 *          NULL (if not found)
 */
SyntaxNode *SyntaxNodeIx::hasDummyVar(const string& name) {

  int i;
  SyntaxNode *ret = NULL;

  for(i=0;i<ncomp;i++){
    SyntaxNode *tmp = dummyVarExpr[i];
    if (!tmp) continue; // no dummy var, just a set.

    // this is either ID or (ID,   ,ID)
    if (tmp->opCode==ID){
      IDNode *tmpid = (IDNode *) tmp;
      if (logCreate) cout << "Found dummy variable: " << tmpid->name << "\n";
      if (name == tmpid->name)
        ret = tmp;
    }else{
      /* This is a multidimensional dummy variable: */
      assert(tmp->opCode==LBRACKET);
      tmp = *(tmp->begin());
      ListNode *tmpl = static_cast<ListNode*>(tmp);
      // and this should be a comma separated list
      assert(tmpl);
      assert(tmpl->opCode==COMMA);
      for(ListNode::literator j=tmpl->lbegin(); j!=tmpl->lend(); ++j){
        // items on the list should be ID
        assert((*j)->opCode==ID);
        IDNode *tmp2 = (IDNode *) *j;
        if (logCreate)
           cout << "Found dummy variable: " << tmp2->name << "\n";
        if (name == tmp2->name)
          ret = tmp2;
      }
    }
  }
  return ret;
}

/*----------------------------------------------------------------------------
SyntaxNodeIx::deep_copy
---------------------------------------------------------------------------- */
/** Makes a recursive copy of this node that uses all new data structures.
 *
 *  SyntaxNodeIDREF nodes will also be duplicated, however they will point
 *  to the original ModelComp's (rather than duplicates of them).
 */
SyntaxNodeIx *
SyntaxNodeIx::deep_copy()
{
  SyntaxNodeIx *onix = new SyntaxNodeIx(opCode);
  
  onix->nval = nval;
  onix->values = new SyntaxNode*[nval];
  for(int i=0;i<nval;i++)
    onix->values[i] = values[i]->deep_copy();
  
  // deep_copy is a virtual function, so qualifier->deep_copy is not defined
  // when qualifier==NULL
  if (qualifier) onix->qualifier = qualifier->deep_copy();
    
  onix->ncomp = ncomp;
  onix->sets = new SyntaxNode*[ncomp];
  onix->dummyVarExpr = new SyntaxNode*[ncomp];
  memset(onix->dummyVarExpr, 0, ncomp*sizeof(SyntaxNode*));
  
  for(int i=0;i<ncomp;i++){
    onix->sets[i] = sets[i]->deep_copy();
    if (dummyVarExpr[i]) onix->dummyVarExpr[i] = dummyVarExpr[i]->deep_copy();
  }
  return onix;
}


/* ===========================================================================
SyntaxNodeIDREF methods
============================================================================ */
/* --------------------------------------------------------------------------
SyntaxNodeIDREF::SyntaxNodeIDREF(ModelComp *r)
---------------------------------------------------------------------------- */
SyntaxNodeIDREF::SyntaxNodeIDREF(ModelComp *r, SyntaxNode *val1) :
  SyntaxNode(IDREF, val1), ref(r), stochparent(0) {}

SyntaxNodeIDREF::SyntaxNodeIDREF(int opCode, ModelComp *r) :
  SyntaxNode(opCode), ref(r), stochparent(0) 
{
   assert(opCode==IDREF||opCode==IDREFM);
}

/* --------------------------------------------------------------------------
SyntaxNodeIDREF *SyntaxNodeIDREF::deep_copy()
---------------------------------------------------------------------------- */
SyntaxNodeIDREF*
SyntaxNodeIDREF::deep_copy()
{
  SyntaxNodeIDREF *newn = new SyntaxNodeIDREF(opCode);

  newn->nval = nval;
  if (nval>0){
    newn->values = new SyntaxNode*[nval];
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i]->deep_copy();
  }

  // this is a ModelComp that needs to be cloned as well
  //newn->ref = ref->clone();
  newn->ref = ref;
  newn->stochparent = stochparent;

  return newn;
}

/* --------------------------------------------------------------------------
SyntaxNodeIDREF *SyntaxNodeIDREF::clone()
---------------------------------------------------------------------------- */
SyntaxNodeIDREF *
SyntaxNodeIDREF::clone()
{
  SyntaxNodeIDREF *newn = new SyntaxNodeIDREF(opCode);

  newn->nval = nval;
  if (nval>0){
    newn->values = new SyntaxNode*[nval];
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i];
  }
  newn->ref = ref;
  //  newn->ref = ref->clone();
  newn->stochparent = stochparent;

  return newn;
}

IDNode::IDNode(const string& new_name, long new_stochparent) :
   SyntaxNode(ID), name(new_name), stochparent(new_stochparent) {}

ostream& ListNode::put(ostream& s) const {
   literator i = lbegin();
   if(i==lend()) return s;

   char sep = ',';
   if(opCode==' ') sep = ' ';

   s << *i;
   for(++i; i!=lend(); ++i)
      s << sep << *i;

   return s;
}

void IDNode::findOpCode(int oc, list<SyntaxNode*> *lnd) {
   if(oc==ID) lnd->push_back(this);
}

ListNode *ListNode::deep_copy() {
   ListNode *copy = new ListNode(opCode);

   for(literator i=lbegin(); i!=lend(); ++i)
      copy->push_back((*i)->deep_copy());

   return copy;
}

ListNode *ListNode::clone() {
   ListNode *copy = new ListNode(opCode);

   for(literator i=lbegin(); i!=lend(); ++i)
      copy->push_back(*i);

   return copy;
}

ListNode::ListNode(int opCode, SyntaxNode *val1, SyntaxNode *val2) :
   SyntaxNode(opCode, val1, val2) 
{
   if(val1) push_back(val1);
   if(val2) push_back(val2);
}

OpNode::OpNode(int opCode, SyntaxNode *op1, SyntaxNode *op2) :
   SyntaxNode(opCode, op1, op2), left(NULL)
{
   assert(op1);
   if(!op2) {
      // Unary operation eg -1, op sign sits to left of operand
      right = op1;
   } else {
      // Binary operation
      left = op1;
      right = op2;
   }
}

OpNode *OpNode::deep_copy() {
   if(left) {
      return new OpNode(opCode, left->deep_copy(), right->deep_copy());
   } else {
      return new OpNode(opCode, right->deep_copy());
   }
}

OpNode *OpNode::clone() {
   if(left) {
      return new OpNode(opCode, left, right);
   } else {
      return new OpNode(opCode, right);
   }
}

/* ----------------------------------------------------------------------------
findKeywordinTree
---------------------------------------------------------------------------- */
/** Traverses down the tree and returns the topmost reference to the keyword
 *  in the Tree.
 */
SyntaxNode *
findKeywordinTree(SyntaxNode *root, int oc)
{
   if (root->opCode==oc) return root;

   SyntaxNode *found, *res;
   found = NULL;
   for(SyntaxNode::iterator i=root->begin(); i!=root->end(); ++i) {
      res = findKeywordinTree((SyntaxNode*)*i, oc);
      if(res && found) {
         cerr << "Found keyword " << oc << "at least twice in " << root << "\n";
         exit(1);
      }
      found = res;
   }
   return found;
}

/* ---------------------------------------------------------------------------
find_var_ref_in_context
---------------------------------------------------------------------------- */
/** This routine does the work of putting together dot'd variable names
 *  'root' is a SyntaxNode of type ID that points to the left hand part
 *  of the dot'd expression parsed so far. 'ref' is the new part that
 *  should be added.
 *
 *  @param context
 *         A pointer to the current AmplModel that defines the scope in which
 *         the ID expressions should be resolved.
 *  @param ref
 *         A pointer to an expression that evaluates to a ModelComp: this can
 *         be given by an ID a dotted expression ID.ID or a reference to a
 *         parent stage (in StochProg) such as ID(-1;...).
 *         It can also carry an indexing expressinon ID[.,.,.] in which case
 *         the indexing is attached to the returned IDREF node.
 *
 *  @return An SyntaxNode of type IDREF that points to the correct ModelComp.
 *
 *  @bug Should return a SyntaxNodeIDREF*.
 *
 *  A SyntaxNode of type IDREF looks like this
 *       ->opCode = IDREF;
 *       ->nval = # of arguments
 *       ->values[0] = pointer to entity in model list
 *       ->values[1 - n] = arguments 
 */
SyntaxNode*
find_var_ref_in_context(AmplModel *context, SyntaxNode *ref)
{
   /* 'ref' is an SyntaxNode representing an iditem. 
      This can be either
      - a ID node where values[0] simply points to a name
      - an ID node which is actually SyntaxNodeID and has stochparent set
      - a LSBRACKET node, where values[0] is ID and values[1] is CSL
      in the second case the CSL should be added as further arguments
      to the resulting IDREF node
   */
  
   /* returns: pointer */
   SyntaxNode *tmp;
   ListNode *argNode;
   IDNode *idNode;
   SyntaxNodeIDREF *ret;
   int stochparent=0;

   /* and now scan through the whole of the local context to see if we 
      find any matches */
   /* the local context is 
       - all vars
       - all constraints
       - all objectives
       - all sets
       - all parameters
       - all submodels
       - all temporary variables (this list needs to be set up somewhere)
   */
  
   // split the expression 'ref' into an id part and an argument list
   if (GlobalVariables::logParseModel)
      cout << "find_var_ref_in_context: " << ref << "\n";

   if (ref->opCode==ID){
      idNode = (IDNode *)ref;
      argNode = NULL;
   }else{
      assert(ref->opCode==LSBRACKET||ref->opCode==LBRACKET);
      SyntaxNode::iterator i = ref->begin();
      idNode = (IDNode*)*i;
      argNode = (ListNode*) *(++i);
      assert(idNode->opCode==ID);
      assert(argNode->opCode==COMMA);
   }

   // Test if this ID node is actually of type SyntaxNodeID and if so remember
   // the value of stochparent
   {
      if (idNode->stochparent!=0){
         // there is an extra argument, which is the stochparent
         stochparent = idNode->stochparent;
      }
   }

   if (GlobalVariables::logParseModel) 
      cout << "--> search for matches of " << idNode->name << "\n";
 
   // see if this matches a dummy variable
   tmp = find_var_ref_in_indexing(idNode->name);
   if (tmp) {
      if (GlobalVariables::logParseModel) 
         cout << idNode->name << " is matched by dummy var in " << *tmp << "\n";
      return ref;
   }

   // try to find a match in the local context
   ret = context->find_var_ref_in_context(idNode);

   // ret could be NULL if it is actually a STAGE or NODE dummy variable
   if(!ret) {
      if(argNode) {
         cerr << "dummy index of stageset or nodeset and argNode=true not "
            "yet handled!" << endl;
         exit(1);
      }
      return idNode; // return something at least vaguely meaningful
   }

   if (argNode){
      if (GlobalVariables::logParseModel)
         cout << "Adding argument list to node: " << *argNode << "\n";
      //free(idNode->values); // jdh - what does this do?
      for(ListNode::literator i=argNode->lbegin(); i!=argNode->lend(); ++i)
         ret->push_back(*i);
      if (ref->opCode==LBRACKET){
         // this is old code to deal with ancestor(1).ID declarations. To go
         cerr << "Executing old code to deal with ancestor(1).ID "
            "declarations\n";
         exit(1);

         // This is a reference indexed by '(..)'. In this case we are in
         // a stoch block and the first argument refers to the stage
         //      ret->stochrecourse = (SyntaxNode*)ret->values[0];
         //for(int i=1;i<ret->nval;i++){
         //ret->values[i-1] = ret->values[i];
         //}
         //ret->nval--;
      }
      argNode->clear();
   }
  
   ret->stochparent = stochparent;
   return ret;
}

/* ---------------------------------------------------------------------------
find_var_ref_in_indexing
---------------------------------------------------------------------------- */
/** Scan through the current set of active indexing expressions and see if
 *  any of them defines the dummy variable given by 'name'.
 *
 *  @param name
 *         The name of identifier to look for.
 *
 *   int n_indexing             the currently active indexing expressions
 *   SyntaxNode *list_of_indexing
 *  @return The Indexing expression in which the name occurs (or NULL if there
 *          is no match).
 */
SyntaxNode *
find_var_ref_in_indexing(const string& name) {

   int i;
   SyntaxNodeIx *tmp;
   SyntaxNode *ret = NULL;

   for(i=0;i<n_indexing;i++){
      /* have a look at all the indexing expressions */
      /* an indexing expression is a '{' node followed by a 'Comma' node */
      tmp = list_of_indexing[i];
      if (tmp!=NULL){
         tmp->splitExpression();
         ret = tmp->hasDummyVar(name);
         if (ret) return ret;
      }
   }
   return ret;
}
