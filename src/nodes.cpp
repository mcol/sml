#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include "nodes.h"
#include "ampl.tab.hpp"
#include "ampl.h"       // for findKeywordinTree
#include "AmplModel.h"
#include "model_comp.h" // for WITHARG

static bool logCreate = false;

int opNode::use_global_names=0;
AmplModel *opNode::default_model =NULL;
string opNode::node = "";
string opNode::stage = "";

char *strcat2(char *s1, char *s2);
char *print_stropstr(opNode *node, char *buffer);
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



/* ----------------------------------------------------------------------------
 newBinOp
---------------------------------------------------------------------------- */
/* Binary Operator is everything that takes two argument.
   these are the classical binary: *, +, -, /, but also
   sum/max/min (first op=index, second op=expression)
*/   



opNode *newTertOp(int opCode, void *val1, void *val2, void *val3) {
   opNode *newOp;

   if (logCreate) printf("creating tertiary op: %d\n",opCode);
   newOp = new opNode();
   newOp->opCode = opCode;
   newOp->values = (void **) malloc(3*sizeof(void *));
   newOp->values[0] = (void *) val1;
   newOp->values[1] = (void *) val2;
   newOp->values[2] = (void *) val3;
   newOp->nval = 3;

   return newOp;
}
opNode *newBinOp(int opCode, void *lval, void *rval) {
   opNode *newOp;

   if (logCreate) printf("creating binary op: %d\n",opCode);
   newOp = new opNode();
   newOp->opCode = opCode;
   newOp->values = (void **) malloc(2*sizeof(void *));
   newOp->values[0] = (void *) lval;
   newOp->values[1] = (void *) rval;
   newOp->nval = 2;

   return newOp;
}

opNode *newUnaryOp(int opCode, void *val) {
   opNode *newOp;
   
   if (logCreate) printf("creating unary op: %d\n",opCode);
   newOp = new opNode();
   newOp->opCode = opCode;
   newOp->values = (void **) malloc(1*sizeof(void *));
   newOp->values[0] = (void *) val;
   newOp->nval = 1;

   return newOp;
}

//opNodeID *newUnaryOpID(void *val) {
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

opNode *newTerm(int opCode){
  /* a terminal can be:
     - a value (INT_VAL/FLOAT_VAL/INFINITY)
     - an identifier */
    
}

/*relNode *newRel(int relCode, opNode *lval, opNode *rval) {
   relNode *newRel;

   newRel = (relNode *) malloc(sizeof(relNode));
   newRel->relCode = relCode;
   newRel->lval = lval;
   newRel->rval = rval;

   return newRel;
   }*/

opNode *newInd(){
}

void freeOpNode(opNode *target) {
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
}

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
addItemToListNew(opNode *list, void *newitem)
{
  /* extend the size of the node by one */
  void **newvalues = (void **)calloc(list->nval+1, sizeof(void *));
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
addItemToListBeg(void *newitem, opNode *list)
{
  /* extend the size of the node by one */
  void **newvalues = (void **)calloc(list->nval+1, sizeof(void *));
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
      return newUnaryOp(oc, newitem);
    }
  }else{
    assert(oc==list->opCode);
    if (newitem==NULL) return list;
    
    /* extend the size of the node by one */
    void **newvalues = (void **)calloc(list->nval+1, sizeof(void *));
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

char *print_opNode(opNode *node){return (node==NULL)?strdup(""):node->print();}

char*
opNode::print()
{
  model_comp *thisc;
  AmplModel *thism;
  char *buffer;
  char *buffer2;
  int i;
  opNode *node = this;
  if (node==NULL){
    buffer = strdup("");
    return buffer;
  }

  switch (node->opCode)
    {
    case 0:
      return print_opNode((opNode*)node->values[0]);

      /* these are terminals */
    case INT_VAL:
      buffer = (char *)malloc(10);
      i = sprintf(buffer, "%d", *((int *)(node->values[0])));
      if (i>10) {
	printf("integer constant too long: %d",*(int *)(node->values[0]));
	exit(1);
      }
      return buffer;

    case FLOAT_VAL:
      buffer = (char *)malloc(15);
      i = sprintf(buffer, "%f", *(double *)(node->values[0]));
      if (i>15) {
	printf("float constant too long: %f",*(double *)(node->values[0]));
	exit(1);
      }
      return buffer;
      break;
      /* these are lots of simple binary operators */
    case NODE:
      return strdup(opNode::node.c_str());
      break;
    case STAGE:
      return strdup(opNode::stage.c_str());
      break;
    case ID:
      /* in this case the values[0] is simply a pointer to a name */
      buffer = strdup((const char*)node->values[0]);
      return buffer;
      break;
    case IDREF:
      if (nval<0) {
	// as yet unset IDREF
	return strdup("IDREF");
      }
      if (use_global_names){
	buffer = getGlobalNameNew(((opNodeIDREF*)node)->ref, node, default_model, WITHARG);
	return buffer;
      }else{
      /* this is the new ID processor */
	thisc = ((opNodeIDREF*)node)->ref;
	buffer = strdup(thisc->id);
	if (node->nval==0) return buffer;
	buffer2 = strdup("[");
	buffer = strcat2(buffer, strcat2(buffer2, print_opNode((opNode*)node->values[0])));
	for(i=1;i<node->nval;i++){
	  buffer2 = strdup(",");
	  buffer = strcat2(buffer, strcat2(buffer2, print_opNode((opNode*)node->values[i])));
	}
	buffer2 = strdup("]");
	buffer = strcat2(buffer, buffer2);
	return buffer;
      }
      break;

    case IDREFM:
      {
	/* this is the new ID processor (for submodels) */
	opNodeIDREF *onir = dynamic_cast<opNodeIDREF*>(node);
	// ??? is this correct
	thisc = (model_comp*)onir->ref;
	buffer = strdup(thisc->id);
	//thism = (AmplModel*)onir->ref;
	//buffer = strdup(thism->name);
	return buffer;
      }
      break;
    case ' ':
      buffer = strdup(" ");
      return print_stropstr(node, buffer);

    case DOT:
      buffer = strdup(".");
      return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		     print_opNode((opNode*)node->values[1]));
      
    case COMMA:
      /* a comma can have any number of arguments */
      if (node->nval==1){
	return print_opNode((opNode*)node->values[0]);
      }
      buffer = strdup(",");
      buffer2 = strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
			print_opNode((opNode*)node->values[1]));
      for(i=2;i<node->nval;i++){
	buffer2 =  strcat2(strcat2(buffer2, buffer), print_opNode((opNode*)node->values[i]));
      }
      return buffer2;

    case IN:
      buffer = strdup(" in ");
      return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		     print_opNode((opNode*)node->values[1]));

    case GE:
      buffer = strdup(">=");
      return print_stropstr(node, buffer);

    case GT:
      buffer = strdup(">");
      return print_stropstr(node, buffer);

    case LE:
      buffer = strdup("<=");
      return print_stropstr(node, buffer);

    case LT:
      buffer = strdup("<");
      return print_stropstr(node, buffer);

    case EQ:
      buffer = strdup("==");
      return print_stropstr(node, buffer);

    case DIFF:
      buffer = strdup(" diff ");
      return print_stropstr(node, buffer);

    case CROSS:
      buffer = strdup(" cross ");
      return print_stropstr(node, buffer);

    case DOTDOT:
      buffer = strdup(" .. ");
      return print_stropstr(node, buffer);

    case '+':
      buffer = strdup("+");
      return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		     print_opNode((opNode*)node->values[1]));

    case '-':
      buffer = strdup("-");
      return print_stropstr(node, buffer);

    case '*':
      buffer = strdup("*");
      return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		     print_opNode((opNode*)node->values[1]));

    case '/':
      buffer = strdup("/");
      return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		     print_opNode((opNode*)node->values[1]));

    case SUM:
      buffer = strdup("sum ");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     print_opNode((opNode*)node->values[1]));

    case MAX:
      buffer = strdup("max ");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     print_opNode((opNode*)node->values[1]));

    case MIN:
      buffer = strdup("min ");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     print_opNode((opNode*)node->values[1]));
      break;
    case EXPECTATION:
      buffer = strdup("Exp(");
      buffer2 = strdup(")");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     buffer2);
      break;
    case LAST:
      buffer = strdup("last(");
      buffer2 = strdup(")");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     buffer2);
      break;
    case FIRST:
      buffer = strdup("first(");
      buffer2 = strdup(")");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),
		     buffer2);
      break;
      // -------------------------functions f(..) --------------------------
    case ORD:
      buffer = strdup("ord");
      return strcat2(buffer, print_opNode((opNode*)node->values[0]));

      // -------------------------terminals --------------------------
    case ORDERED:
      return strdup(" ordered");
      break;
    case SYMBOLIC:
      return strdup(" symbolic");
      break;
    case DETERMINISTIC:
      return strdup(" deterministic");
      break;
      /* these are lots of simple unary operators */
    case WITHIN:
      buffer = strdup("within ");
      return strcat2(buffer, print_opNode((opNode*)node->values[0]));

    case LSBRACKET:
      buffer = strdup("[");
      buffer2 = strdup("]");
      if (node->nval==1){
	return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),buffer2);
      }else{
	return strcat2(print_opNode((opNode*)node->values[0]), 
		       strcat2(strcat2(buffer, print_opNode((opNode*)node->values[1])),buffer2));
      }

    case LBRACE:
      buffer = strdup("{");
      buffer2 = strdup("}");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),buffer2);

    case LBRACKET:
      buffer = strdup("(");
      buffer2 = strdup(")");
      return strcat2(strcat2(buffer, print_opNode((opNode*)node->values[0])),buffer2);

    case ASSIGN:
      buffer = strdup("=");
      if (node->nval==1){
	return strcat2(buffer, print_opNode((opNode*)node->values[0]));
      }else{
	return strcat2(print_opNode((opNode*)node->values[0]), 
		       strcat2(buffer, print_opNode((opNode*)node->values[1])));
      }
      break;
    case DEFINED:
      buffer = strdup(":=");
      if (node->nval==1){
	return strcat2(buffer, print_opNode((opNode*)node->values[0]));
      }else{
	printf("':=' used as binary operator?\n");
	exit(1);
	//	return strcat2(print_opNode((opNode*)node->values[0]), 
	//		       strcat2(buffer, print_opNode((opNode*)node->values[1])));
      }
      break;
    case COLON:
      buffer = strdup(":");
      if (node->nval==1){
	return strcat2(buffer, print_opNode((opNode*)node->values[0]));
      }else{
	return strcat2(print_opNode((opNode*)node->values[0]), 
		       strcat2(buffer, print_opNode((opNode*)node->values[1])));
      }
      break;

    case IF:
      if (node->nval==2){
	char *tmp1 = print_opNode((opNode*)node->values[0]);
	char *tmp2 = print_opNode((opNode*)node->values[1]);
	int len = strlen(tmp1)+strlen(tmp2)+3+6+1;
	char *ret = (char*)malloc(len);
	sprintf(ret, "if %s then %s",tmp1, tmp2);
	free(tmp1);
	free(tmp2);
	return ret;
      }else{
	// has three arguments 
	char *tmp1 = print_opNode((opNode*)node->values[0]);
	char *tmp2 = print_opNode((opNode*)node->values[1]);
	char *tmp3 = print_opNode((opNode*)node->values[2]);
	int len = strlen(tmp1)+strlen(tmp2)+strlen(tmp3)+3+6+6+1;
	char *ret = (char*)malloc(len);
	sprintf(ret, "if %s then %s else %s",tmp1, tmp2, tmp3);
	free(tmp1);
	free(tmp2);
	free(tmp3);
	return ret;
      }
      break;
    default: 
      printf("Unknown opcode %i\n",node->opCode);
      printf("->nval = %d\n",node->nval);
      for (i=0;i<node->nval;i++){
	printf("val[%d]=%s\n",i,print_opNode((opNode*)node->values[i]));
      }
      exit(1);
    }
}

char *
print_stropstr(opNode *node, char *buffer){
  if (node->nval==2){
    return strcat2(strcat2(print_opNode((opNode*)node->values[0]), buffer),
		   print_opNode((opNode*)node->values[1]));
  }else{
    return strcat2(buffer, print_opNode((opNode*)node->values[0]));
  }
}

/* this is a strcat function that allocates space for the return string
   while freeing space of the other two strings */
char *strcat2(char *s1, char *s2){
  char *ret;
  ret = (char *)malloc(strlen(s1)+strlen(s2)+1);
  
  sprintf(ret,"%s%s",s1,s2);
  free(s1);
  free(s2);
  return ret;
}

void 
opNode::dump(FILE *fout)
{
  fprintf(fout, "%s\n",print_opNodesymb(this));
}

char *
print_opNodesymb(opNode *node)
{
  char *buffer;
  int i;

  if (node==NULL){
    buffer = strdup("NULL");
    return buffer;
  }
  if (node->opCode==ID){
    buffer = strdup("(ID T)");
    return buffer;
  }
  if (node->opCode==INT_VAL){
    char buffer3[20];
    assert(node->nval==1);
    sprintf(buffer3, "T:%d",(int*)node->values[0]);
    buffer = strdup(buffer3);
    return buffer;
  }
  if (node->opCode==FLOAT_VAL){
    char buffer3[20];
    assert(node->nval==1);
    sprintf(buffer3, "T:%f",(double*)node->values[0]);
    buffer = strdup(buffer3);
    return buffer;
  }

  // start new version
  // print node symbol
  int retsize=0;
  char *symb;
  char **arg;
  switch (node->opCode)
  {
  case IDREF:{
    opNodeIDREF *onir= dynamic_cast<opNodeIDREF*>(node);
    if (onir==NULL) {
      printf("Some IDREF node still not opNodeIDREF\n");exit(1);}
    model_comp *mc = onir->ref;
    char buffer3[40]; 
    sprintf(buffer3, "IDREF(%p:%s(%p))",node, mc->id, mc);
    //return strdup(buffer3);
    symb = strdup(buffer3);
    break;}
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
    arg[i] = print_opNodesymb((opNode*)node->values[i]);
    retsize += strlen(arg[i])+2;
  }
  
  buffer = (char*)calloc(retsize, sizeof(char));
  strcpy(buffer, symb);
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


/* --------------------------------------------------------------------------
opNode *opNode::deep_copy()
---------------------------------------------------------------------------- */
opNode *
opNode::deep_copy()
{
  opNode *newn = new opNode();

  if (opCode==IDREF || opCode==IDREFM){
    printf("IDREF opNodes need to be cloned differently\n");
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  if (nval>0)
    newn->values = (void **)calloc(nval, sizeof(void *));

  /* Values are copied depending on the type of the opNode */
  /* ID/IDREF/INT_VAL/FLOAT_VAL/IDREFM are treated differently */
  if (opCode==ID){
    assert(nval==1);
    newn->values[0] = strdup((char *)values[0]);
    return newn;
  }
  if (opCode==INT_VAL){
    assert(nval==1);
    newn->values[0] = (int*)calloc(1, sizeof(int));
    *((int*)(newn->values[0])) = *(int*)(values[0]);
    return newn;
  }
  if (opCode==FLOAT_VAL){
    assert(nval==1);
    newn->values[0] = (double*)calloc(1, sizeof(double));
    *((double*)(newn->values[0])) = *(double*)(values[0]);
    return newn;
  }
  
  for(int i=0;i<nval;i++)
    newn->values[i] = ((opNode*)values[i])->deep_copy();
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
    printf("IDREF opNodes need to be cloned differently\n");
    exit(1);
  }
  newn->opCode = opCode;
  newn->nval = nval;
  newn->values = (void **)calloc(nval, sizeof(void *));
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
  if (opCode!=INT_VAL && opCode!=FLOAT_VAL && opCode!=ID){
    printf("Attempting to call getFloatVal for an opNode not of type INT_VAL/FLOAT_VAL/ID\n");
    exit(1);
  }
  if (opCode==INT_VAL){
    return (double) (*(int*)values[0]);
  }else if (opCode==ID){
    return atof((char*)values[0]);
  }
  
  return *(double*)values[0];
}

/* --------------------------------------------------------------------------
opNode *opNode::getValue()
---------------------------------------------------------------------------- */
/* Returns the value of the opNode as a c_string
   Assumes that the opNode in question is ID, INT_VAL, FLOAT_VAL */
char *
opNode::getValue()
{
  char buffer[20]; 
  if (opCode!=INT_VAL && opCode!=FLOAT_VAL && opCode!=ID){
    printf("Attempting to call getValue for an opNode not of type ID/INT_VAL/FLOAT_VAL\n");
    exit(1);
  }
  if (opCode==ID){
    return (char*)values[0];
  }else if (opCode==INT_VAL){
    sprintf(buffer, "%i",*(int*)values[0]);
    return strdup(buffer);
  }else {
    sprintf(buffer, "%f",*(int*)values[0]);
    printf("Calling getValue() for a FLOAT_VAL node: %s.\n",buffer);
    printf("If this is an attempt to use a float as a set element it might cause problems\n");
    exit(1);
    return strdup(buffer);
  }
  
}
/* --------------------------------------------------------------------------
char *opNode::printDummyVar()
---------------------------------------------------------------------------- */
/* Assumes that the current opNode is the dummy variable in an indexing 
   expression: that is it, is either ID or LBRACKET (COMMA (ID1 .. IDn)) */

char *
opNode::printDummyVar()
{
  if (opCode==ID){
    return this->print();
  }else{
    opNode *list;
    // this must be LBRACKET
    if (opCode!=LBRACKET){
      cout << "printDummyVar: dummy var must be ID or (ID1,..,IDn)\n";
      cout << "current opCode is "+opCode;
      exit(1);
    }
    list = (opNode*)values[0];
    if (list->opCode==ID) return list->print();
    if (list->opCode!=COMMA){
      cout << "printDummyVar: dummy var must be ID or (ID1,..,IDn)\n";
      cout << "current opCode is "+list->opCode;
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
    printf("%s\n",getGlobalName((model_comp*)this->values[0], 
				NULL, NULL, NOARG));
  }else if (opCode==ID) {
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	((opNode*)values[i])->findIDREF();
      }
    }
  }
}
/* --------------------------------------------------------------------------
opNode::findIDREF(list<model_comp> *lmc)
---------------------------------------------------------------------------- */
/* find the list of all the IDREF nodes at or below the current node */
void
opNode::findIDREF(list<model_comp*> *lmc)
{
  int i;

  if (opCode==IDREF){
    //printf("%s\n",getGlobalName((model_comp*)this->values[0], 
    //				NULL, NULL, NOARG));
    lmc->push_back(((opNodeIDREF*)this)->ref);
  }else if (opCode==ID||opCode==INT_VAL||opCode==FLOAT_VAL) {
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
  }else if (opCode==ID||opCode==INT_VAL||opCode==FLOAT_VAL) {
    // if terminal then return
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	((opNode*)values[i])->findIDREF(lnd);
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
  }else if (opCode==ID||opCode==INT_VAL||opCode==FLOAT_VAL) {
    // if terminal then return
    return;
  }else{
    for(i=0;i<nval;i++){
      if (values[i]){
	((opNode*)values[i])->findOpCode(oc, lnd);
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
    on = (opNode*)on->values[0];
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
    dummyVar = (opNode*)ix->values[0];
    set = (opNode*)ix->values[1];
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
opNode::getArgumentList()
{
  opNodeIDREF *on;
  string arglist = "";
  if (opCode!=IDREF){
    printf("Can only call getArgumentList for opNodes of type IDREF\n");
    exit(1);
  }

  // see if this is actually an IDREF node
  on = dynamic_cast<opNodeIDREF*>(this);
  if (on==NULL){
    printf("WARNING: This is an IDREF opNode not of type opNodeIDREF\n");
    if (nval>0){
      arglist += ((opNode*)values[1])->print();
      for(int i=1;i<nval;i++){
	arglist += ",";
	arglist += ((opNode*)values[1+i])->print();
      }
    }
  }else{
    if (nval>0){
      arglist += ((opNode*)values[0])->print();
      for(int i=1;i<nval;i++){
	arglist += ",";
	arglist += ((opNode*)values[i])->print();
      }
    }
  }
  return arglist;
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
opNodeIx::printDiagnostic(FILE *fout)
{
  if (!done_split) splitExpression();
  fprintf(fout, "qualifier: %s\n",(qualifier==NULL)?"NULL":qualifier->print());
  fprintf(fout, "number of indexing expressions: %d\n",ncomp);
  for(int i=0;i<ncomp;i++){
    fprintf(fout, "%d: dummyVar: %s\n",i,(dummyVarExpr[i]==NULL)?"NULL":dummyVarExpr[i]->print());
    fprintf(fout, "%d: set     : %s\n",i,(sets[i]==NULL)?"NULL":sets[i]->print());
  }
  
}

/* ---------------------------------------------------------------------------
opNodeIx::getListDummyVars
-----------------------------------------------------------------------------*/
list<char *>*
opNodeIx::getListDummyVars()
{
  list<char *>* l = new list<char *>;
  
  for(int i=0;i<ncomp;i++){
    opNode *dv = dummyVarExpr[i];
    // a dummy var expression is either ID/IDREF or (ID1,...IDn)
    if (dv->opCode==ID||dv->opCode==IDREF){
      l->push_back(dv->print());
    }else if(dv->opCode==LBRACKET){
      dv = (opNode*)dv->values[0];
      if (dv->opCode!=COMMA){
	printf("A dummy variable expression is either ID or (ID1,...ID2)\n");
	printf("Given expression: %s\n",dv->print());
	exit(1);
      }
      for(int j=0;j<dv->nval;j++){
	opNode *dn = (opNode*)dv->values[j];
	l->push_back(dn->print());
      }
    }else{
      printf("A dummy variable expression is either ID or (ID1,...ID2)\n");
      printf("Given expression: %s\n",dv->print());
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
    printf("Error in splitExpression: Indexing Expression must start with {\n");
    printf("     %s",print_opNode(this));
    exit(1);
  }
  
  tmp = (opNode*)this->values[0];
  // discard the colon (if there is one present: only interested in lhs) 
  if (tmp->opCode==COLON) {
    qualifier = (opNode*)tmp->values[1];
    tmp = (opNode*)tmp->values[0];
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
	dummyVarExpr[i] = (opNode*)tmp2->values[0];
	sets[i] = (opNode*)tmp2->values[1];
      } else {
	/* just set, but no dummyVar given */
	dummyVarExpr[i] = NULL;
	sets[i] = (opNode*)tmp->values[i];
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
    tmp2 = findKeywordinTree((opNode*)tmp, IN);
    if (tmp2){
      dummyVarExpr[0] = (opNode*)tmp2->values[0];
      sets[0] = (opNode*)tmp2->values[1];
    } else {
      /* just set, but no dummyVar given */
      dummyVarExpr[0] = NULL;
      sets[0] = (opNode*)tmp;
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

opNode *opNodeIx::hasDummyVar(char *name)
{
  int i, j;
  opNode *ret = NULL;
  opNode *tmp;


  for(i=0;i<ncomp;i++){
    tmp = dummyVarExpr[i];
    if (tmp){ // i.e. this expression has a dummy var (and not just a set)
      // this is either ID or (ID,   ,ID)
      if (tmp->opCode==ID){
	if (logCreate) printf("Found dummy variable: %s\n",tmp->values[0]);
	  if (strcmp(name, (const char*)tmp->values[0])==0)
	    ret = (opNode*)tmp;
      }else{
	/* This is a multidimensional dummy variable: */
	assert(tmp->opCode==LBRACKET);
	tmp = (opNode*)tmp->values[0];
	// and this should be a comma separated list
	assert(tmp->opCode==COMMA);
	for(j=0;j<tmp->nval;j++){
	  // items on the list should be ID
	  opNode *tmp2;
	  tmp2 = (opNode*)tmp->values[j];
	  assert(tmp2->opCode==ID);
	  if (logCreate) printf("Found dummy variable: %s\n",tmp2->values[0]);
	  if (strcmp(name, (const char*)tmp2->values[0])==0)
	    ret = (opNode*)tmp2;
	}
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
  onix->values = (void **)calloc(nval, sizeof(void *));
  for(int i=0;i<nval;i++)
    onix->values[i] = ((opNode*)values[i])->deep_copy();
  
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
opNodeIDREF::opNodeIDREF(): 
  opNode(),
  stochparent(0),
  ref(NULL)
{}

/* --------------------------------------------------------------------------
opNodeIDREF::opNodeIDREF(model_comp *r)
---------------------------------------------------------------------------- */
opNodeIDREF::opNodeIDREF(model_comp *r): opNode()
{
  ref = r;
  stochparent = 0;
  opCode = IDREF;
  nval = 0; // this is correct if there is no subscripting expression
}

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
    newn->values = (void **)calloc(nval, sizeof(void *));
    for(int i=0;i<nval;i++)
      newn->values[i] = ((opNode*)values[i])->deep_copy();
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
    newn->values = (void **)calloc(nval, sizeof(void *));
    for(int i=0;i<nval;i++)
      newn->values[i] = values[i];
  }
  newn->ref = ref;
  //  newn->ref = ref->clone();
  newn->stochparent = stochparent;

  return newn;
}
