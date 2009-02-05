#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include "nodes.h"
#include "sml.tab.h"
#include "AmplModel.h"
#include "ModelComp.h"    // for WITHARG
#include "GlobalVariables.h"

static bool logCreate = false;
extern int n_indexing;
extern SyntaxNodeIx *list_of_indexing[20];

int SyntaxNode::use_global_names=0;
AmplModel *SyntaxNode::default_model =NULL;
string SyntaxNode::node = "";
string SyntaxNode::stage = "";

/* ==========================================================================
 SyntaxNode
=========================================================================== */
/* this defines an object SyntaxNode :
   SyntaxNode is a node in an expression tree. 
   
   FIXME: 
   Should the SyntaxNode structure carry a flag that indicates what meaning is 
   has within the grammar?
   That way we could have routines like "isIndexing", "isSimpleSet" that
   may be helpful for error detection

*/

//SyntaxNodeID *new SyntaxNodeID(void *val) {
//   SyntaxNodeID *newOp;
//   
//   printf("creating unary opID:\n");
//   newOp = new SyntaxNodeID();
//   newOp->opCode = ID;
//   newOp->values = (void **) malloc(1*sizeof(void *));
//   newOp->values[0] = (void *) val;
//   newOp->nval = 1;
//
//   return newOp;
//}

/*relNode *newRel(int relCode, SyntaxNode *lval, SyntaxNode *rval) {
   relNode *newRel;

   newRel = (relNode *) malloc(sizeof(relNode));
   newRel->relCode = relCode;
   newRel->lval = lval;
   newRel->rval = rval;

   return newRel;
   }*/

/*void freeOpNode(SyntaxNode *target) {
   if(target == NULL) {
      fprintf(stderr, "Attepted to free a NULL SyntaxNode\n");
      return;
   }

   switch(target->opCode) {
      case POWER:
      case ELLIPSE:
      case LOGICAL_OR:
      case LOGICAL_AND:
      case '+':
      case '-':
      case '*':
      case '/':
	freeOpNode((SyntaxNode*)target->values[1]);
      case '!':
	freeOpNode((SyntaxNode*)target->values[0]);
         break;
      default:
         fprintf(stderr, "Unknown opCode %i. Possible Memory leak.\n", 
               target->opCode);
         target->values = NULL;
   }
   if(target->values != NULL) free(target->values);
   free(target);
}*/

/* --------------------------------------------------------------------------
addItemToListNew
-------------------------------------------------------------------------- */
SyntaxNode *SyntaxNode::push_back(SyntaxNode *newitem)
{
  /* extend the size of the node by one */
  SyntaxNode **newvalues = (SyntaxNode **)calloc(nval+1, sizeof(SyntaxNode *));
  int i;

  for (i=0;i<nval;i++){
    newvalues[i] = values[i];
  }
  newvalues[nval] = newitem;
  nval++;
  free(values);
  values = newvalues;
  return this;
}
/* --------------------------------------------------------------------------
addItemToListBeg
-------------------------------------------------------------------------- */
SyntaxNode *SyntaxNode::push_front(SyntaxNode *newitem)
{
  /* extend the size of the node by one */
  SyntaxNode **newvalues = (SyntaxNode **)calloc(nval+1, sizeof(SyntaxNode *));
  int i;

  for (i=0;i<nval;i++){
    newvalues[i+1] = values[i];
  }
  newvalues[0] = newitem;
  nval++;
  free(values);
  values = newvalues;
  return this;
}

/* --------------------------------------------------------------------------
addItemToListOrCreate
-------------------------------------------------------------------------- */
/** A 'List' is an SyntaxNode of opCode COMMA or ' ' with a variable number
 *  of arguments. 
 *  This function takes (a possibly existing) list and adds an item to it
 *  Both the list and the item can be NULL:
 *  - if the item is NULL then the old list is simply returned.
 *  - if the list is NULL then a list with opCode 'oc' is created from the
 *    single item that is passed
 */

SyntaxNode *
addItemToListOrCreate(int oc, SyntaxNode *list, SyntaxNode *newitem)
{
  if(!newitem) return list;

  if (list){
    assert(oc==list->opCode);
    return list->push_back(newitem);
  }else{
    return new SyntaxNode(oc, newitem);
  }
}


#if 0
/* --------------------------------------------------------------------------
addItemToList
-------------------------------------------------------------------------- */
/* a list of values is given by an SyntaxNode with 
    ->opCode one of {COMMA, }
    ->nval   #of items on the list
   the items on the list are given as a linked list of indexNode items
    ->values[0]   first item on list
    ->values[1]   last item on list
 
  'addItemToList' wraps newitem up in an indexNode object and adds that
  to the list represented by list
 */


SyntaxNode *
addItemToList(SyntaxNode *list, SyntaxNode *newitem)
{
  indexNode *last = (indexNode *)list->values[1];
  indexNode *newnode = newIndexNode(newitem);
  int i;
  indexNode *tmp;

  printf("Add to list: \n");
  printf("Current list: opCode= %d, nval = %d\n",list->opCode, list->nval);
  tmp = (indexNode *)(list->values[0]);
  for(i=0;i<list->nval;i++){
    char *tmpbuf = print_SyntaxNodesymb(tmp->value);
    printf("> %s\n", tmpbuf);
    free(tmpbuf);
    tmp = tmp->next;
  }

  list->nval++;
  last->next = newnode;
  list->values[1]=newnode;

  //printf("In addItemToList: %s\n",print_SyntaxNodesymb(list));
  return list;
}
#endif

//retType *
//newRetType(SyntaxNode *node, SyntaxNode *indexing, AmplModel *context)
//{
// retType *ret = (retType*)calloc(1, sizeof(retType));
//  
//  ret->node = node;
//  ret->indexing = indexing;
//  ret->context = context;
//}

/* --------------------------------------------------------------------------
SyntaxNode::print()
--------------------------------------------------------------------------- */
/* Thie routine recursively prints the expression routed at the current
   node in the expression tree.

   IN: use_global_names influences how nodes of type IDREF are printed

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

  SyntaxNode::Iterator i = this->begin();
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
    case -100: // FIXME: was originally a NODE or STAGE but now contains an ID.
      s << *i;
      break;
    case NODE:
      s << SyntaxNode::node;
      break;
    case STAGE:
      s << SyntaxNode::stage;
      break;
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
    case IN:
      s << **i << " in ";
      s << **(++i);
      break;
    case GE:
      if(this->nval>1) s << **(i++);
      s << ">=" <<  **i;
      break;
    case GT:
      if(this->nval>1) s << **(i++);
      s << ">" <<  **i;
      break;
    case LE:
      if(this->nval>1) s << **(i++);
      s << "<=" <<  **i;
      break;
    case LT:
      if(this->nval>1) s << **(i++);
      s << "<" <<  **i;
      break;
    case EQ:
      if(this->nval>1) s << **(i++);
      s << "==" <<  **i;
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
    case '+':
      s << **i;
      s << "+" << **(++i);
      break;
    case '-':
      if(this->nval>1) s << **(i++);
      s << "-" <<  **i;
      break;
    case '*':
      s << **i << "*";
      s << **(++i);
      break;
    case '/':
      s << **i << "/";
      s << **(++i);
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
      s << "{" << **i << "}";
      break;
    case LBRACKET:
      s << "(" << **i << ")";
      break;
    case ASSIGN:
      if (this->nval==1){
         s << "=" << **i;
      }else{
         s << *(SyntaxNode*)*i;
         s << "=" << **(++i);
      }
      break;
    case DEFINED:
      if(this->nval!=1) {
         cerr << "':=' used as binary operator?\n";
         exit(1);
      }
      s << ":=" << **i;
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
    case -99: // tempalte<class T> ValueNode
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
            SyntaxNode::Iterator i=begin();
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

char *
print_SyntaxNodesymb(SyntaxNode *node)
{
  int i;
  ValueNode<long> *inode;
  ValueNode<double> *dnode;

  if (node==NULL){
    return strdup("NULL");
  }
  if (node->opCode==ID){
    return strdup("(ID T)");
  }
  if ((inode = dynamic_cast<ValueNode<long> *>(node))){
    string temp = "T:";
    temp += inode->getValue();
    return strdup(temp.c_str());
  }
  if ((dnode = dynamic_cast<ValueNode<double> *>(node))){
    string temp = "T:";
    temp += dnode->getValue();
    return strdup(temp.c_str());
  }

  // start new version
  // print node symbol
  int retsize=0;
  char *buffer;
  char *symb;
  char **arg;
  switch (node->opCode)
  {
  case IDREF: {
    SyntaxNodeIDREF *onir= dynamic_cast<SyntaxNodeIDREF*>(node);
    if (onir==NULL) {
      cerr << "Some IDREF node still not SyntaxNodeIDREF\n";
      exit(1);
    }
    ModelComp *mc = onir->ref;
    char buffer3[40]; 
    sprintf(buffer3, "IDREF(%p:%s(%p))",node, mc->id, mc);
    //return strdup(buffer3);
    symb = strdup(buffer3);
  }
  break;
  case ASSIGN: symb = strdup("ASSIGN"); break;
  case IN:     symb = strdup("IN"); break;
  case SUM:    symb = strdup("SUM"); break;
  case LBRACE: symb = strdup("LBR{"); break;
  case LBRACKET:symb = strdup("LBR("); break;
  case COMMA:  symb = strdup("COMMA"); break;
  case COLON:  symb = strdup("COLON"); break;
  case '+':    symb = strdup("\"+\""); break;
  case '*':    symb = strdup("\"*\""); break;
  default:
    symb = (char*)malloc(7*sizeof(char));
    sprintf(symb, "\"%d\"",node->opCode);
  }
  retsize +=strlen(symb)+3; // allow space for \0 and ()
  if (node->nval>0)
    arg = (char**)calloc(node->nval, sizeof(char*));
  for(i=0;i<node->nval;i++){
    arg[i] = print_SyntaxNodesymb(node->values[i]);
    retsize += strlen(arg[i])+2;
  }
  
  buffer = (char*)calloc(retsize, sizeof(char));
  strcpy(buffer, symb);
  free(symb);
  strcat(buffer, "(");
  for(i=0;i<node->nval;i++){
    strcat(buffer, arg[i]);
    if (i<node->nval-1) strcat(buffer, ",");
    free(arg[i]);
  }
  if (node->nval>0) free(arg);
  strcat(buffer, ")");
  return buffer;
	 
	 
}
/* ==========================================================================
SyntaxNode Methods to follow
============================================================================*/
// constructors:

SyntaxNode::SyntaxNode()
{
  opCode = -1;
  nval = -1;
  values = NULL;
}

SyntaxNode::SyntaxNode(SyntaxNode &src) :
   nval(src.nval), opCode(src.opCode), values(src.values) {}

SyntaxNode::SyntaxNode (int code, SyntaxNode *val1, SyntaxNode *val2, SyntaxNode* val3) :
   opCode(code), nval(0), values(NULL)
{
   if(val1) nval++;
   if(val2) nval++;
   if(val3) nval++;

   if(nval) values = (SyntaxNode **) malloc(nval*sizeof(SyntaxNode *));

   int i = 0;
   if(val1) values[i++] = val1;
   if(val2) values[i++] = val2;
   if(val3) values[i++] = val3;

   if (logCreate) cout << "created " << nval << "-ary op: " << opCode << "\n";
}

SyntaxNode::~SyntaxNode() {
   if(values) free(values);
}


/* --------------------------------------------------------------------------
SyntaxNode *SyntaxNode::deep_copy()
---------------------------------------------------------------------------- */
SyntaxNode *
SyntaxNode::deep_copy()
{
  SyntaxNode *newn = new SyntaxNode();

  if (opCode==IDREF || opCode==IDREFM){
    cerr << "IDREF SyntaxNodes need to be cloned differently\n";
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0)
    newn->values = (SyntaxNode **)calloc(nval, sizeof(SyntaxNode *));

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
  SyntaxNode *newn = new SyntaxNode();

  if (opCode==IDREF){
    cerr << "IDREF SyntaxNodes need to be cloned differently\n";
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  newn->values = (SyntaxNode **)calloc(nval, sizeof(SyntaxNode *));
  for(int i=0;i<nval;i++)
    newn->values[i] = values[i];
  return newn;
}

/* --------------------------------------------------------------------------
char *SyntaxNode::getFloatVal()
---------------------------------------------------------------------------- */
/* Returns the double value represented by this SyntaxNode
   Assumes that the current SyntaxNode is either INT_VAL or FLOAT_VAL */

double 
SyntaxNode::getFloatVal()
{
  if (opCode!=ID){
    cerr << "Attempting to call getFloatVal for an SyntaxNode not of type"
       "INT_VAL/FLOAT_VAL/ID\n";
    exit(1);
  }
  cerr << "Badness IDNode::getFloatVal?" << endl;
  throw exception();
  return atof((char*)values[0]);
}

/* --------------------------------------------------------------------------
SyntaxNode *SyntaxNode::getValue()
---------------------------------------------------------------------------- */
/* Returns the value of the SyntaxNode as a c_string
   Assumes that the SyntaxNode in question is ID, INT_VAL, FLOAT_VAL */
char *
SyntaxNode::getValue()
{
  cerr << "Attempting to call getValue on something funny." << endl;
  throw exception();
  if (opCode!=ID){
    cerr << "Attempting to call getValue for an SyntaxNode not of type ID/INT_VAL/FLOAT_VAL\n";
    exit(1);
  }
  return (char*)values[0];
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
/* find the list of all the IDREF nodes at or below the current node */
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
/* find the list of all the IDREF nodes at or below the current node */
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
/* find the list of all the IDREF nodes at or below the current node */
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
/* find the list of all nodes with opCode==oc at or below the current node */
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
/* find the ModelComp (if any) refered to by this SyntaxNode 
 * Only return the ModelComp if the expression given by this SyntaxNode is an
 * immediate reference to a ModelComp. Otherwise return NULL
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
SyntaxNode::getIndexingSet()
---------------------------------------------------------------------------- */
SyntaxNode *SyntaxNode::getIndexingSet()
{
  SyntaxNode *ix = this;
  SyntaxNode *set;
  SyntaxNode *dummyVar;

  if (ix==NULL) return NULL;
  /* remove outside braces from indexing expression */
  if (ix->opCode==LBRACE) ix = (SyntaxNode*)ix->values[0];
  /* assumes that the next level is the 'IN' keyword (if present) */
  if (ix->opCode==IN){
    dummyVar = ix->values[0];
    set = ix->values[1];
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
 *  to SyntaxNodeIDREF:getArgumentList()):
 *  returns a comma separated list of the arguments (the bit in [..] brackets)
 *
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
         SyntaxNode::Iterator i = begin();
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
 * The items from src are prepended to this object's values
 */
SyntaxNode &SyntaxNode::merge(const SyntaxNode &src) {
   SyntaxNode **newvalues = (SyntaxNode **)calloc(src.nval+nval,sizeof(SyntaxNode *));

   SyntaxNode **ptr = newvalues;
   for(int i=0;i<src.nval;i++)
      *(ptr++) = src.values[i];
   for(int i=0;i<nval;i++)
      *(ptr++) = values[i];
   nval += src.nval;
   free(values);
   values = newvalues;

   return (*this);
}


//void SyntaxNode::foo(){}

/* ==========================================================================
SyntaxNodeix Methods to follow
============================================================================*/
SyntaxNodeIx::SyntaxNodeIx()
{
  qualifier = NULL;
  ncomp = 0;
  sets = NULL;
  sets_mc = NULL;
  dummyVarExpr = NULL;
  done_split = 0;
}

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
      for(SyntaxNode::Iterator j=dv->begin(); j!=dv->end(); ++j){
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
  
  tmp = this->values[0];
  // discard the colon (if there is one present: only interested in lhs) 
  if (tmp->opCode==COLON) {
    qualifier = tmp->values[1];
    tmp = tmp->values[0];
  }else{
    qualifier = NULL;
  }
  /* this should now be a comma separated list */
  if (tmp->opCode==COMMA){
    ncomp = tmp->nchild();
    this->sets = (SyntaxNode**)calloc(ncomp, sizeof(SyntaxNode*));
    this->sets_mc = (ModelComp**)calloc(ncomp, sizeof(ModelComp*));
    this->dummyVarExpr = (SyntaxNode**)calloc(ncomp, sizeof(SyntaxNode*));
    for(i=0;i<ncomp;i++){
      tmp2 = findKeywordinTree((SyntaxNode*)tmp->values[i], IN);
      /* everything to the left of IN is a dummy variables */
      if (tmp2){
	     dummyVarExpr[i] = tmp2->values[0];
	     sets[i] = tmp2->values[1];
      } else {
	/* just set, but no dummyVar given */
	     dummyVarExpr[i] = NULL;
	     sets[i] = tmp->values[i];
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
    this->sets = (SyntaxNode**)calloc(1, sizeof(SyntaxNode*));
    this->sets_mc = (ModelComp**)calloc(1, sizeof(ModelComp*));
    this->dummyVarExpr = (SyntaxNode**)calloc(1, sizeof(SyntaxNode*));
    tmp2 = findKeywordinTree(tmp, IN);
    if (tmp2){
      dummyVarExpr[0] = tmp2->values[0];
      sets[0] = tmp2->values[1];
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
 *  dummy variable given by name 
 *
 *  @param name The name of the dummy variable to look for
 *  @return The ("ID") SyntaxNode representing the dummy Variable (if found) or
 *          NULL (if not found)
 *
 */

SyntaxNode *SyntaxNodeIx::hasDummyVar(const char *const name)
{
  int i;
  SyntaxNode *ret = NULL;
  SyntaxNode *tmp;

  for(i=0;i<ncomp;i++){
    tmp = dummyVarExpr[i];
    if (!tmp) continue; // no dummy var, just a set.

    // this is either ID or (ID,   ,ID)
    if (tmp->opCode==ID){
      IDNode *tmpid = (IDNode *) tmp;
      if (logCreate) cout << "Found dummy variable: " << tmpid->name << "\n";
      if (strcmp(name, tmpid->name.c_str())==0)
        ret = tmp;
    }else{
      /* This is a multidimensional dummy variable: */
      assert(tmp->opCode==LBRACKET);
      tmp = tmp->values[0];
      // and this should be a comma separated list
      assert(tmp->opCode==COMMA);
      for(SyntaxNode::Iterator j=tmp->begin(); j!=tmp->end(); ++j){
        // items on the list should be ID
        assert((*j)->opCode==ID);
        IDNode *tmp2 = (IDNode *) *j;
        if (logCreate)
           cout << "Found dummy variable: " << tmp2->name << "\n";
        if (strcmp(name, tmp2->name.c_str())==0)
          ret = tmp2;
      }
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------
SyntaxNodeIx::deep_copy
---------------------------------------------------------------------------- */
/** Makes a recursive copy of this node that uses all new data structures
 *  SyntaxNodeIDREF nodes will also be duplicated, however they will point
 *  to the original ModelComp's (rather than duplicates of them)
 */

SyntaxNodeIx *
SyntaxNodeIx::deep_copy()
{
  SyntaxNodeIx *onix = new SyntaxNodeIx();
  
  onix->opCode = opCode;
  onix->nval = nval;
  onix->values = (SyntaxNode **)calloc(nval, sizeof(SyntaxNode *));
  for(int i=0;i<nval;i++)
    onix->values[i] = values[i]->deep_copy();
  
  // deep_copy is a virtual function, so qualifier->deep_copy is not defined
  // when qualifier==NULL
  if (qualifier) onix->qualifier = qualifier->deep_copy();
    
  onix->ncomp = ncomp;
  onix->sets = (SyntaxNode**)calloc(ncomp, sizeof(SyntaxNode*));
  onix->dummyVarExpr = (SyntaxNode**)calloc(ncomp, sizeof(SyntaxNode*));
  
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
SyntaxNodeIDREF::SyntaxNodeIDREF(ModelComp *r) :
  SyntaxNode(IDREF), ref(r), stochparent(0) {}

/* --------------------------------------------------------------------------
SyntaxNodeIDREF *SyntaxNodeIDREF::deep_copy()
---------------------------------------------------------------------------- */
SyntaxNodeIDREF*
SyntaxNodeIDREF::deep_copy()
{
  SyntaxNodeIDREF *newn = new SyntaxNodeIDREF();

  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0){
    newn->values = (SyntaxNode **)calloc(nval, sizeof(SyntaxNode *));
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
  SyntaxNodeIDREF *newn = new SyntaxNodeIDREF();

  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0){
    newn->values = (SyntaxNode **)calloc(nval, sizeof(SyntaxNode *));
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i];
  }
  newn->ref = ref;
  //  newn->ref = ref->clone();
  newn->stochparent = stochparent;

  return newn;
}

IDNode::IDNode(const char *const new_name, long new_stochparent) :
   SyntaxNode(ID), name(new_name), stochparent(stochparent) {}
IDNode::IDNode(const string new_name, long new_stochparent) :
   SyntaxNode(ID), name(new_name), stochparent(new_stochparent) {}

ostream& ListNode::put(ostream& s) const {
   iterator i = list.begin();
   if(i==list.end()) return s;

   s << *i;
   for(++i; i!=list.end(); ++i)
      s << "," << *i;

   return s;
}

ListNode *ListNode::deep_copy() {
   ListNode *copy = new ListNode();

   for(iterator i=begin(); i!=end(); ++i)
      copy->push_back((*i)->deep_copy());

   return copy;
}

ListNode *ListNode::clone() {
   ListNode *copy = new ListNode();

   for(iterator i=begin(); i!=end(); ++i)
      copy->push_back(*i);

   return copy;
}

ListNode::ListNode(SyntaxNode *val1, SyntaxNode *val2) :
   SyntaxNode(COMMA, val1, val2) 
{
   if(val1) push_back(val1);
   if(val2) push_back(val2);
}


/* ----------------------------------------------------------------------------
findKeywordinTree
---------------------------------------------------------------------------- */
/* this routine traverses down the tree and returns the top most reference to 
   the keyword in the Tree */

SyntaxNode *
findKeywordinTree(SyntaxNode *root, int oc)
{
   if (root->opCode==oc) return root;

   SyntaxNode *found, *res;
   found = NULL;
   for(SyntaxNode::Iterator i=root->begin(); i!=root->end(); ++i) {
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
 * @param ref A pointer to an expression that evaluates to a ModelComp
 *            this can be given by an ID a dotted expression ID.ID
 *            or a reference to a parent stage (in StochProg) such as 
 *            ID(-1;...).
 *            It can also carry an indexing expressinon ID[.,.,.] in
 *            which case the indexing is attached to the returned IDREF node
 * @param context A pointer to the current AmplModel that defines the scope
 *                in which the ID expressions should be resolved
 * @return An SyntaxNode of type IDREF that points to the correct ModelComp
 * @bug Should return an SyntaxNodeIDREF* 
 *
 *  An SyntaxNode of type IDREF looks like this
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
   SyntaxNode *tmp, *argNode;
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
      SyntaxNode::Iterator i = ref->begin();
      idNode = (IDNode*)*i;
      argNode = *(++i);
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
   tmp = find_var_ref_in_indexing(idNode->name.c_str());
   if (tmp) {
      if (GlobalVariables::logParseModel) 
         cout << idNode->name << " is matched by dummy var in " << *tmp << "\n";
      return ref;
   }

   // try to find a match in the local context
   ret = find_var_ref_in_context_(context, idNode);

   if (argNode){
      if (GlobalVariables::logParseModel)
         cout << "Adding argument list to node: " << *argNode << "\n";
      free(idNode->values); // jdh - what does this do?
      ret->values = argNode->values;
      ret->nval = argNode->nval;
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
      argNode->values=NULL;
      argNode->nval = 0;
   } else {
      ret->nval = 0;
   }
  
   ret->stochparent = stochparent;
   return ret;
}

SyntaxNodeIDREF*
find_var_ref_in_context_(AmplModel *context, IDNode *ref)
{
   ModelComp *thismc;
   SyntaxNodeIDREF *ret;
  
   for(list<ModelComp*>::iterator p = context->comps.begin();
         p!=context->comps.end();p++){
      thismc = *p;
      if (strcmp(ref->name.c_str(), thismc->id)==0){
         /* this is a match */
         if (GlobalVariables::logParseModel){
            cout << "Found Match: " << ref->name << " refers to ";
            cout << nameTypes[thismc->type] << "\n";
            cout << "    " << thismc->id << "\n";
            cout << "       " << *(thismc->indexing) << "\n";
            cout << "       " << *(thismc->attributes) << "\n";
         }

         ret = new SyntaxNodeIDREF();
         ret->ref = thismc;
         ret->opCode = IDREF;
         if (thismc->type==TMODEL){
            ret->opCode = IDREFM;
         }
         return ret;
      }
      //thismc = thismc->next;
   }

  
   //thismc = context->vars;
   //for(i=0;i<context->n_vars;i++){
   //  if (strcmp(name, thismc->id)==0){
   //    /* this is a match */
   //    printf("Found Match: %s refers to variable\n",name);
   //    printf("    %s\n",thismc->id);
   //    printf("       %s\n",print_SyntaxNode(thismc->indexing));
   //    printf("       %s\n",print_SyntaxNode(thismc->attributes));
   //    ref->values[0] = (void*)thismc;
   //    ref->opCode = IDREF;
   //    return ref;
   //  }
   //  thismc = thismc->next;
   //}
   //thismc = context->cons;
   //for(i=0;i<context->n_cons;i++){
   //  if (strcmp(name, thismc->id)==0){
   //    /* thismc is a match */
   //    printf("Found Match: %s refers to constraint\n",name);
   //    printf("    %s\n",thismc->id);
   //    printf("       %s\n",print_SyntaxNode(thismc->indexing));
   //    printf("       %s\n",print_SyntaxNode(thismc->attributes));
   //    ref->values[0] = (void*)thismc;
   //    ref->opCode = IDREF;
   //    return ref;
   //  }
   //  thismc = thismc->next;
   //}
   //thismc = context->sets;
   //for(i=0;i<context->n_sets;i++){
   //  if (strcmp(name, thismc->id)==0){
   //    /* this is a match */
   //    printf("Found Match: %s refers to set\n",name);
   //    printf("    %s\n",thismc->id);
   //    printf("       %s\n",print_SyntaxNode(thismc->indexing));
   //    printf("       %s\n",print_SyntaxNode(thismc->attributes));
   //    ref->values[0] = (void*)thismc;
   //    ref->opCode = IDREF;
   //    return ref;
   //  }
   //  thismc = thismc->next;
   //}
   //thismc = context->params;
   //for(i=0;i<context->n_params;i++){
   //  if (strcmp(name, thismc->id)==0){
   //    /* this is a match */
   //    printf("Found Match: %s refers to parameter\n",name);
   //    printf("    %s\n",thismc->id);
   //    printf("       %s\n",print_SyntaxNode(thismc->indexing));
   //    printf("       %s\n",print_SyntaxNode(thismc->attributes));
   //    ref->values[0] = (void*)thismc;
   //    ref->opCode = IDREF;
   //    return ref;
   //  }
   //  thismc = thismc->next;
   //}
   //thism = context->submodels;
   //for(i=0;i<context->n_submodels;i++){
   //  if (strcmp(name, thism->name)==0){
   //    /* this is a match */
   //    printf("Found Match: %s refers to submodel\n",name);
   //    printf("    %s\n",thism->name);
   //    //printf("       %s\n",print_SyntaxNode(thism->indexing));
   //    //printf("       %s\n",print_SyntaxNode(this->attributes));
   //    ref->values[0] = (void*)thism;
   //    ref->opCode = IDREFM;
   //    return ref;
   //  }
   //  thism = thism->next;
   //}

   /* need also to look through parent model */
   if (context->parent){
      SyntaxNodeIDREF *match = find_var_ref_in_context_(context->parent, ref);
      return match;
   }

   /* need also to look through list of local variables */

   cerr << "Could not find ref " << ref->name << " in context\n";
   exit(1);
}


/* ---------------------------------------------------------------------------
find_var_ref_in_indexing
---------------------------------------------------------------------------- */
/* scan through the current set of active indexing expressions and see if
   any of them define the dummy variable  given by 'name' 

   IN: 
    char *name                 the name of identifier to look for
    
    int n_indexing             the currently active indexing expressions
    SyntaxNode *list_of_indexing
   RETURN:
    The Indexing expression in which the name occurs 
    (or NULL if there is no match)
                                                                      */

SyntaxNode *
find_var_ref_in_indexing(const char *const name)
{
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
       
#if 0
         assert(tmp->opCode==LBRACE);
         tmp = (SyntaxNode*)tmp->values[0];
         if (tmp->opCode==COLON) tmp = (SyntaxNode*)tmp->values[0];
         /* this should now be a comma separated list */
         if (tmp->opCode==COMMA){
            for(j=0;j<tmp->nval;j++){
               tmp2 = findKeywordinTree((SyntaxNode*)tmp->values[j], IN);
               /* everything to the left of IN is a dummy variables */
               if (tmp2){
                  tmp2 = (SyntaxNode*)tmp2->values[0];
                  assert(tmp2->opCode==ID);
                  if (GlobalVariables::logParseModel)
                     printf("Found dummy variable: %s\n",tmp2->values[0]);
                  if (strcmp(name, (const char*)tmp2->values[0])==0)
                     ret = (SyntaxNode*)tmp->values[j];
     
               }
            }
         }else{
            /* if this is not a comma separated list, then look directly */
            char *tmpbuf1 = print_SyntaxNode(tmp);
            char *tmpbuf2 = print_SyntaxNodesymb(tmp);
            if (GlobalVariables::logParseModel){
               printf(">%s\n", tmpbuf1);
               printf(">%s\n", tmpbuf2);
            }
            free(tmpbuf1);
            free(tmpbuf2);
            tmp2 = findKeywordinTree(tmp, IN);
            /* everything to the left of IN is a dummy variables */
            if (tmp2){
               tmp2 = (SyntaxNode*)tmp2->values[0];
               assert(tmp2->opCode==ID||tmp2->opCode==COMMA);
               if (GlobalVariables::logParseModel)
                  printf("Found dummy variable: %s\n",tmp2->values[0]);
               if (strcmp(name, (const char*)tmp2->values[0])==0)
                  ret = tmp;
            }
         }
#endif
      }
   }
   return ret;
}
