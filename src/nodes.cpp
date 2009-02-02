#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include "nodes.h"
#include "sml.tab.h"
#include "ampl.h"   // for findKeywordinTree
#include "AmplModel.h"
#include "model_comp.h"    // for WITHARG

static bool logCreate = false;

int opNode::use_global_names=0;
AmplModel *opNode::default_model =NULL;
string opNode::node = "";
string opNode::stage = "";

/* ==========================================================================
 opNode
=========================================================================== */
/* this defines an object opNode :
   opNode is a node in an expression tree. 
   
   FIXME: 
   Should the opNode structure carry a flag that indicates what meaning is 
   has within the grammar?
   That way we could have routines like "isIndexing", "isSimpleSet" that
   may be helpful for error detection

*/

//opNodeID *new opNodeID(void *val) {
//   opNodeID *newOp;
//   
//   printf("creating unary opID:\n");
//   newOp = new opNodeID();
//   newOp->opCode = ID;
//   newOp->values = (void **) malloc(1*sizeof(void *));
//   newOp->values[0] = (void *) val;
//   newOp->nval = 1;
//
//   return newOp;
//}

/*relNode *newRel(int relCode, opNode *lval, opNode *rval) {
   relNode *newRel;

   newRel = (relNode *) malloc(sizeof(relNode));
   newRel->relCode = relCode;
   newRel->lval = lval;
   newRel->rval = rval;

   return newRel;
   }*/

/*void freeOpNode(opNode *target) {
   if(target == NULL) {
      fprintf(stderr, "Attepted to free a NULL opNode\n");
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
	freeOpNode((opNode*)target->values[1]);
      case '!':
	freeOpNode((opNode*)target->values[0]);
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
newIndexNode
--------------------------------------------------------------------------- */
indexNode *
newIndexNode(opNode *node){
  indexNode *ixn = (indexNode *)calloc(1, sizeof(indexNode));
  ixn->next = NULL;
  ixn->value = node;
  return ixn;
}

/* --------------------------------------------------------------------------
addItemToListNew
-------------------------------------------------------------------------- */
opNode *
addItemToListNew(opNode *list, opNode *newitem)
{
  /* extend the size of the node by one */
  opNode **newvalues = (opNode **)calloc(list->nval+1, sizeof(opNode *));
  int i;

  for (i=0;i<list->nval;i++){
    newvalues[i] = list->values[i];
  }
  newvalues[list->nval] = newitem;
  list->nval++;
  free(list->values);
  list->values = newvalues;
  return list;
}
/* --------------------------------------------------------------------------
addItemToListBeg
-------------------------------------------------------------------------- */
opNode *
addItemToListBeg(opNode *newitem, opNode *list)
{
  /* extend the size of the node by one */
  opNode **newvalues = (opNode **)calloc(list->nval+1, sizeof(opNode *));
  int i;

  for (i=0;i<list->nval;i++){
    newvalues[i+1] = list->values[i];
  }
  newvalues[0] = newitem;
  list->nval++;
  free(list->values);
  list->values = newvalues;
  return list;
}

/* --------------------------------------------------------------------------
addItemToListOrCreate
-------------------------------------------------------------------------- */
/** A 'List' is an opNode of opCode COMMA or ' ' with a variable number
 *  of arguments. 
 *  This function takes (a possibly existing) list and adds an item to it
 *  Both the list and the item can be NULL:
 *  - if the item is NULL then the old list is simply returned.
 *  - if the list is NULL then a list with opCode 'oc' is created from the
 *    single item that is passed
 */

opNode *
addItemToListOrCreate(int oc, opNode *list, opNode *newitem)
{
  if (list==NULL){
    if (newitem==NULL){
      return NULL;
    }else{
      return new opNode(oc, newitem);
    }
  }else{
    assert(oc==list->opCode);
    if (newitem==NULL) return list;
    
    /* extend the size of the node by one */
    opNode **newvalues = (opNode **)calloc(list->nval+1, sizeof(opNode *));
    int i;

    for (i=0;i<list->nval;i++){
      newvalues[i] = list->values[i];
    }
    newvalues[list->nval] = newitem;
    list->nval++;
    free(list->values);
    list->values = newvalues;
    return list;
  }
}


#if 0
/* --------------------------------------------------------------------------
addItemToList
-------------------------------------------------------------------------- */
/* a list of values is given by an opNode with 
    ->opCode one of {COMMA, }
    ->nval   #of items on the list
   the items on the list are given as a linked list of indexNode items
    ->values[0]   first item on list
    ->values[1]   last item on list
 
  'addItemToList' wraps newitem up in an indexNode object and adds that
  to the list represented by list
 */


opNode *
addItemToList(opNode *list, opNode *newitem)
{
  indexNode *last = (indexNode *)list->values[1];
  indexNode *newnode = newIndexNode(newitem);
  int i;
  indexNode *tmp;

  printf("Add to list: \n");
  printf("Current list: opCode= %d, nval = %d\n",list->opCode, list->nval);
  tmp = (indexNode *)(list->values[0]);
  for(i=0;i<list->nval;i++){
    char *tmpbuf = print_opNodesymb(tmp->value);
    printf("> %s\n", tmpbuf);
    free(tmpbuf);
    tmp = tmp->next;
  }

  list->nval++;
  last->next = newnode;
  list->values[1]=newnode;

  //printf("In addItemToList: %s\n",print_opNodesymb(list));
  return list;
}
#endif

//retType *
//newRetType(opNode *node, opNode *indexing, AmplModel *context)
//{
// retType *ret = (retType*)calloc(1, sizeof(retType));
//  
//  ret->node = node;
//  ret->indexing = indexing;
//  ret->context = context;
//}

/* --------------------------------------------------------------------------
opNode::print()
--------------------------------------------------------------------------- */
/* Thie routine recursively prints the expression routed at the current
   node in the expression tree.

   IN: use_global_names influences how nodes of type IDREF are printed

*/

ostream&
operator<<(ostream&s, const opNode *node) {
   if(node == NULL) return s;
   return node->put(s);
}

ostream&
operator<<(ostream&s, const opNode &node) {
   return node.put(s);
}

ostream& opNode::put(ostream&s) const {
  const opNodeIDREF *onidref;
  static int level=0;

  if(this == NULL) return s;

  opNode::Iterator i = this->begin();
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
      s << opNode::node;
      break;
    case STAGE:
      s << opNode::stage;
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
         s << *(opNode*)*i;
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
      if(!(onidref = (const opNodeIDREF*)(this))) {
         cerr << "Cast of node to opNodeIDREF failed!\n";
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

ostream& opNodeIDREF::put(ostream& s) const {
   model_comp *thisc;

   switch(this->opCode) {
   case IDREF:
      if (this->nval<0) {
	      // as yet unset IDREF
         s << "IDREF";
      } else if (opNode::use_global_names) {
	      s << getGlobalNameNew(this->ref, this, this->default_model,
            WITHARG);
      } else {
         /* this is the new ID processor */
	      thisc = this->ref;
         if(this->nval==0) {
            s << thisc->id;
         } else {
            opNode::Iterator i=this->begin();
            s << thisc->id << "[" << **i;
            for(++i; i!=this->end(); ++i){
               s << "," << **i;
            }
            s << "]";
         }
      }
      break;
   case IDREFM:
      /* this is the new ID processor (for submodels) */
      // ??? is this correct
      thisc = (model_comp*)this->ref;
      s << thisc->id;
      break;
   default:
      cerr << "In fn opNodeIDREF::put bu not an IDREF or IDREFM\n";
      exit(1);
   }

   return s;
}

string
opNode::print()
{
   ostringstream ost;
   ost << (*this);
   return ost.str();
}

void 
opNode::dump(ostream &fout)
{
  fout << print_opNodesymb(this) << "\n";
}

char *
print_opNodesymb(opNode *node)
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
    opNodeIDREF *onir= dynamic_cast<opNodeIDREF*>(node);
    if (onir==NULL) {
      cerr << "Some IDREF node still not opNodeIDREF\n";
      exit(1);
    }
    model_comp *mc = onir->ref;
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
    arg[i] = print_opNodesymb(node->values[i]);
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
opNode Methods to follow
============================================================================*/
// constructors:

opNode::opNode()
{
  opCode = -1;
  nval = -1;
  values = NULL;
}

opNode::opNode (int code, opNode *val1, opNode *val2, opNode* val3)
{
   opCode = code;

   nval = 0;
   if(val1) nval++;
   if(val2) nval++;
   if(val3) nval++;

   values = NULL;
   if(nval) values = (opNode **) malloc(nval*sizeof(opNode *));

   if(val1) values[0] = val1;
   if(val2) values[1] = val2;
   if(val3) values[2] = val3;

   if (logCreate) cout << "created " << nval << "-ary op: " << opCode << "\n";
}

opNode::~opNode() {
   if(values) free(values);
}


/* --------------------------------------------------------------------------
opNode *opNode::deep_copy()
---------------------------------------------------------------------------- */
opNode *
opNode::deep_copy()
{
  opNode *newn = new opNode();

  if (opCode==IDREF || opCode==IDREFM){
    cerr << "IDREF opNodes need to be cloned differently\n";
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0)
    newn->values = (opNode **)calloc(nval, sizeof(opNode *));

  /* Values are copied depending on the type of the opNode */
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
opNode *opNode::clone()
---------------------------------------------------------------------------- */
opNode *
opNode::clone()
{
  opNode *newn = new opNode();

  if (opCode==IDREF){
    cerr << "IDREF opNodes need to be cloned differently\n";
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  newn->values = (opNode **)calloc(nval, sizeof(opNode *));
  for(int i=0;i<nval;i++)
    newn->values[i] = values[i];
  return newn;
}

/* --------------------------------------------------------------------------
char *opNode::getFloatVal()
---------------------------------------------------------------------------- */
/* Returns the double value represented by this opNode
   Assumes that the current opNode is either INT_VAL or FLOAT_VAL */

double 
opNode::getFloatVal()
{
  if (opCode!=ID){
    cerr << "Attempting to call getFloatVal for an opNode not of type"
       "INT_VAL/FLOAT_VAL/ID\n";
    exit(1);
  }
  cerr << "Badness IDNode::getFloatVal?" << endl;
  throw exception();
  return atof((char*)values[0]);
}

/* --------------------------------------------------------------------------
opNode *opNode::getValue()
---------------------------------------------------------------------------- */
/* Returns the value of the opNode as a c_string
   Assumes that the opNode in question is ID, INT_VAL, FLOAT_VAL */
char *
opNode::getValue()
{
  cerr << "Attempting to call getValue on something funny." << endl;
  throw exception();
  if (opCode!=ID){
    cerr << "Attempting to call getValue for an opNode not of type ID/INT_VAL/FLOAT_VAL\n";
    exit(1);
  }
  return (char*)values[0];
}

/* --------------------------------------------------------------------------
char *opNode::printDummyVar()
---------------------------------------------------------------------------- */
/* Assumes that the current opNode is the dummy variable in an indexing 
   expression: that is it, is either ID or LBRACKET (COMMA (ID1 .. IDn)) */

string
opNode::printDummyVar()
{
  if (opCode==ID){
    return this->print();
  }else{
    opNode *list;
    // this must be LBRACKET
    if (opCode!=LBRACKET){
      cerr << "printDummyVar: dummy var must be ID or (ID1,..,IDn)\n";
      cerr << "current opCode is "+opCode;
      exit(1);
    }
    list = (opNode*)values[0];
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
opNode::findIDREF()
---------------------------------------------------------------------------- */
/* find the list of all the IDREF nodes at or below the current node */
void
opNode::findIDREF()
{
  int i;

  if (opCode==IDREF){
    cout << getGlobalName((model_comp*)this->values[0], NULL, NULL, NOARG) <<
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
opNode::findIDREF(list<model_comp> *lmc)
---------------------------------------------------------------------------- */
/* find the list of all the IDREF nodes at or below the current node */
void
opNode::findIDREF(list<model_comp*> &lmc)
{
  int i;

  if (opCode==IDREF){
    //printf("%s\n",getGlobalName((model_comp*)this->values[0], 
    //				NULL, NULL, NOARG));
    lmc.push_back(((opNodeIDREF*)this)->ref);
  }else if (opCode==ID) {
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	     ((opNode*)values[i])->findIDREF(lmc);
      }
    }
  }
}
/* --------------------------------------------------------------------------
opNode::findIDREF(list<opNode *> *lnd)
---------------------------------------------------------------------------- */
/* find the list of all the IDREF nodes at or below the current node */
void
opNode::findIDREF(list<opNode*> *lnd)
{
  int i;

  if (opCode==IDREF){
    //printf("%s\n",getGlobalName((model_comp*)this->values[0], 
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
opNode::findOpCode(int oc, list<opNode *> *lnd)
---------------------------------------------------------------------------- */
/* find the list of all nodes with opCode==oc at or below the current node */
void
opNode::findOpCode(int oc, list<opNode*> *lnd)
{
  int i;

  if (opCode==oc){
    //printf("%s\n",getGlobalName((model_comp*)this->values[0], 
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
opNode::findModelComp()
---------------------------------------------------------------------------- */
/* find the model_comp (if any) refered to by this opNode 
 * Only return the model_comp if the expression given by this opNode is an
 * immediate reference to a model_comp. Otherwise return NULL
 */

model_comp *opNode::findModelComp()
{
  opNode *on = this;
  while ((on->opCode==LBRACKET || on->opCode==LBRACE) && on->nval==1){
    on = on->values[0];
  }

  if (opCode==IDREF){
    opNodeIDREF *onref = dynamic_cast<opNodeIDREF*>(this);
    return onref->ref;
  }
  return NULL;
}


/* --------------------------------------------------------------------------
opNode::getIndexingSet()
---------------------------------------------------------------------------- */
opNode *opNode::getIndexingSet()
{
  opNode *ix = this;
  opNode *set;
  opNode *dummyVar;

  if (ix==NULL) return NULL;
  /* remove outside braces from indexing expression */
  if (ix->opCode==LBRACE) ix = (opNode*)ix->values[0];
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
opNode::getArgumentList()
---------------------------------------------------------------------------- */
/** This is for an opNode of type IDREF (and should eventually be moved
 *  to opNodeIDREF:getArgumentList()):
 *  returns a comma separated list of the arguments (the bit in [..] brackets)
 *
 */

string
opNode::getArgumentList() const
{
   const opNodeIDREF *on;
   string arglist = "";
   if (opCode!=IDREF){
      cerr << "Can only call getArgumentList for opNodes of type IDREF\n";
      exit(1);
   }

   // see if this is actually an IDREF node
   on = dynamic_cast<const opNodeIDREF*>(this);
   if (on==NULL){
      cout << "WARNING: This is an IDREF opNode not of type opNodeIDREF\n";
      if (nval>0){
         arglist += values[1]->print();
         for(int i=1;i<nval;i++){
	         arglist += ",";
	         arglist += values[1+i]->print();
         }
      }
   }else{
      if (nval>0){
         opNode::Iterator i = begin();
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
opNode &opNode::merge(const opNode &src) {
   opNode **newvalues = (opNode **)calloc(src.nval+nval,sizeof(opNode *));

   opNode **ptr = newvalues;
   for(int i=0;i<src.nval;i++)
      *(ptr++) = src.values[i];
   for(int i=0;i<nval;i++)
      *(ptr++) = values[i];
   nval += src.nval;
   free(values);
   values = newvalues;

   return (*this);
}


//void opNode::foo(){}

/* ==========================================================================
opNodeix Methods to follow
============================================================================*/
opNodeIx::opNodeIx()
{
  qualifier = NULL;
  ncomp = 0;
  sets = NULL;
  sets_mc = NULL;
  dummyVarExpr = NULL;
  done_split = 0;
}

opNodeIx::opNodeIx(opNode *on)
{
  opCode = on->opCode;
  nval = on->nval;
  values = on->values;

  qualifier = NULL;
  ncomp = 0;
  sets = NULL;
  sets_mc = NULL;
  dummyVarExpr = NULL;
  done_split = 0;
  splitExpression();
}

/* ---------------------------------------------------------------------------
opNodeIx::printDiagnostic
-----------------------------------------------------------------------------*/
void
opNodeIx::printDiagnostic(ostream &fout)
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
opNodeIx::getListDummyVars
-----------------------------------------------------------------------------*/
list<opNode *>
opNodeIx::getListDummyVars()
{
  list<opNode *> l;
  
  for(int i=0;i<ncomp;i++){
    opNode *dv = dummyVarExpr[i];
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
      for(opNode::Iterator j=dv->begin(); j!=dv->end(); ++j){
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
opNodeIx::splitExpression
-----------------------------------------------------------------------------*/
/* sets up the ->set, ->dummyVar components of the class 

 A general indexing expression can be of the form

    {(i,j) in ARCS, k in SET: i>k} 
*/
void opNodeIx::splitExpression()
{
  opNode *tmp, *tmp2;
  int i;

  if (done_split) return;
  done_split=1;

  if (opCode!=LBRACE){
    cerr << "Error in splitExpression: Indexing Expression must start with {\n";
    cerr << "     " << this;
    exit(1);
  }
  
  tmp = (opNode*)this->values[0];
  // discard the colon (if there is one present: only interested in lhs) 
  if (tmp->opCode==COLON) {
    qualifier = tmp->values[1];
    tmp = tmp->values[0];
  }else{
    qualifier = NULL;
  }
  /* this should now be a comma separated list */
  if (tmp->opCode==COMMA){
    ncomp = tmp->nval;
    this->sets = (opNode**)calloc(ncomp, sizeof(opNode*));
    this->sets_mc = (model_comp**)calloc(ncomp, sizeof(model_comp*));
    this->dummyVarExpr = (opNode**)calloc(ncomp, sizeof(opNode*));
    for(i=0;i<ncomp;i++){
      tmp2 = findKeywordinTree((opNode*)tmp->values[i], IN);
      /* everything to the left of IN is a dummy variables */
      if (tmp2){
	     dummyVarExpr[i] = tmp2->values[0];
	     sets[i] = tmp2->values[1];
      } else {
	/* just set, but no dummyVar given */
	     dummyVarExpr[i] = NULL;
	     sets[i] = tmp->values[i];
      }
      /* try to find model_comp of the set expression, 
	 If it doesn't exist create */
      if (model_comp *mc = sets[i]->findModelComp()){
	     sets_mc[i] = mc;
      }else{
	     sets_mc[i] = new model_comp("dummy", TSET, NULL, sets[i]);
      }
    }
  }else{
    ncomp = 1;
    this->sets = (opNode**)calloc(1, sizeof(opNode*));
    this->sets_mc = (model_comp**)calloc(1, sizeof(model_comp*));
    this->dummyVarExpr = (opNode**)calloc(1, sizeof(opNode*));
    tmp2 = findKeywordinTree(tmp, IN);
    if (tmp2){
      dummyVarExpr[0] = tmp2->values[0];
      sets[0] = tmp2->values[1];
    } else {
      /* just set, but no dummyVar given */
      dummyVarExpr[0] = NULL;
      sets[0] = tmp;
    }
    /* try to find model_comp of the set expression, 
       If it doesn't exist create */
    if (model_comp *mc = sets[0]->findModelComp()){
      sets_mc[0] = mc;
    }else{
      sets_mc[0] = new model_comp("dummy", TSET, NULL, sets[0]);
    }
  }
}


/*----------------------------------------------------------------------------
opNodeIx::hasDummyVar
---------------------------------------------------------------------------- */
/** Sees if the indexing Expression given by opNodeIx defines the 
 *  dummy variable given by name 
 *
 *  @param name The name of the dummy variable to look for
 *  @return The ("ID") opNode representing the dummy Variable (if found) or
 *          NULL (if not found)
 *
 */

opNode *opNodeIx::hasDummyVar(const char *const name)
{
  int i, j;
  opNode *ret = NULL;
  opNode *tmp;

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
      for(j=0;j<tmp->nval;j++){
        // items on the list should be ID
        opNode *tmp2;
        tmp2 = tmp->values[j];
        assert(tmp2->opCode==ID);
        if (logCreate)
           cout << "Found dummy variable: " << tmp2->values[0] << "\n";
        cerr << "Badness in hasDummyVar";
        throw exception();
        if (strcmp(name, (const char*)tmp2->values[0])==0)
          ret = tmp2;
      }
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------
opNodeIx::deep_copy
---------------------------------------------------------------------------- */
/** Makes a recursive copy of this node that uses all new data structures
 *  opNodeIDREF nodes will also be duplicated, however they will point
 *  to the original model_comp's (rather than duplicates of them)
 */

opNodeIx *
opNodeIx::deep_copy()
{
  opNodeIx *onix = new opNodeIx();
  
  onix->opCode = opCode;
  onix->nval = nval;
  onix->values = (opNode **)calloc(nval, sizeof(opNode *));
  for(int i=0;i<nval;i++)
    onix->values[i] = values[i]->deep_copy();
  
  // deep_copy is a virtual function, so qualifier->deep_copy is not defined
  // when qualifier==NULL
  if (qualifier) onix->qualifier = qualifier->deep_copy();
    
  onix->ncomp = ncomp;
  onix->sets = (opNode**)calloc(ncomp, sizeof(opNode*));
  onix->dummyVarExpr = (opNode**)calloc(ncomp, sizeof(opNode*));
  
  for(int i=0;i<ncomp;i++){
    onix->sets[i] = sets[i]->deep_copy();
    if (dummyVarExpr[i]) onix->dummyVarExpr[i] = dummyVarExpr[i]->deep_copy();
  }
  return onix;
}


/* ===========================================================================
opNodeIDREF methods
============================================================================ */
/* --------------------------------------------------------------------------
opNodeIDREF::opNodeIDREF(model_comp *r)
---------------------------------------------------------------------------- */
opNodeIDREF::opNodeIDREF(model_comp *r) :
  opNode(IDREF), ref(r), stochparent(0) {}

/* --------------------------------------------------------------------------
opNodeIDREF *opNodeIDREF::deep_copy()
---------------------------------------------------------------------------- */
opNodeIDREF*
opNodeIDREF::deep_copy()
{
  opNodeIDREF *newn = new opNodeIDREF();

  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0){
    newn->values = (opNode **)calloc(nval, sizeof(opNode *));
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i]->deep_copy();
  }

  // this is a model_comp that needs to be cloned as well
  //newn->ref = ref->clone();
  newn->ref = ref;
  newn->stochparent = stochparent;

  return newn;
}
/* --------------------------------------------------------------------------
opNodeIDREF *opNodeIDREF::clone()
---------------------------------------------------------------------------- */
opNodeIDREF *
opNodeIDREF::clone()
{
  opNodeIDREF *newn = new opNodeIDREF();

  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0){
    newn->values = (opNode **)calloc(nval, sizeof(opNode *));
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i];
  }
  newn->ref = ref;
  //  newn->ref = ref->clone();
  newn->stochparent = stochparent;

  return newn;
}

IDNode::IDNode(const char *const new_name, opNode *indexing) :
   opNode(ID, indexing), name(new_name) {if(indexing) cerr << "WINNER" << endl;}
IDNode::IDNode(const string new_name, opNode *indexing) :
   opNode(ID, indexing), name(new_name) {if(indexing) cerr << "WINNER" << endl;}
