#ifndef NODES_H
#define NODES_H

#include <list>
class model_comp;
using namespace std;

class AmplModel;


/** @class opNode
 *  @brief This is a node in the operator tree. 
 *
 *  All AMPL/SML expressions are broken down into a tree of operators. 
 *  The nodes of this tree are objects of type opNode.
 *
 *  Note that values[] is a list of untyped pointers. Normally these
 *  would point to further opNode structure (children of the current
 *  node). For an ID it is simply a pointer to the string containing
 *  the name
 *
 *  \bug A comma separated list is currently represented by an opNode with
 *       opCode==COMMA and values[0/1] pointing to the first and last member
 *       in a linked list of opNodes. This linked list is implemented using
 *       _indexNode objects (which is a opNode plus a pointer to next). 
 *       This is done to facilitate adding members to the list without 
 *       knowing the dimension beforehand. Better implemented by replacing
 *       the opNode::values array with a C++ list.
 */
class opNode {
 public: 
  int opCode;    //!< ID CODE of this node (a list can be found in ampl.tab.h 
  int nval;      //!< number of arguments 
  void **values; //!< list of arguments 

  /** if use_global_names is set then the print() method will print out
   *  model component names as global names                               */
  static int use_global_names; 
  static AmplModel *default_model;
  // ------------------------ methods -----------------------------------
  opNode();
  // for nodes that are indexing expressions, get the set that is indexed over
  // FIXME: move this to opNodeIx, also indexing expressions are more compl.
  opNode *getIndexingSet();
  /* FIXME: not sure if these two should be part of the class. They are 
     global variables that affect the print() method */
  char *print();              // recursive printing of expression
  char *printDummyVar();      // node is a dummy var -> remove (..) 
  void findIDREF();           // find all IDREF's below current node 
  void findIDREF(list<model_comp*> *lmc);// find all IDREFs below current node 
  opNode *clone();
};

/*  A general indexing expression can be of the form

    {(i,j) in ARCS, k in SET: i>k} 
*/
class opNodeIx : public opNode {
 public:
  opNode *qualifier;    //!< the stuff to the right of ':' (if present)
  int ncomp;            //!< number of 'dummy IN set'-type expressions
  opNode **sets;        //!< list of the sets
  opNode **dummyVarExpr;//!< list of the dummyVarExpressions
  // --------------------------- methods ----------------------------------
  opNodeIx();
  opNodeIx(opNode *on);
  opNode *hasDummyVar(char *name); 
  list<char *>* getListDummyVars(); // get list of all dummy variables 
  void splitExpression();   // to set up the set, dummyVar components
  void printDiagnostic();   // diagnostic print of class variables
  // private:            
  int done_split;           // indicates that the extra data fields have
                            // been set: qualifier/ncomp/sets/dummyVarExpr
};


/* ----------------------------------------------------------------------------
opNodeIDREF 
---------------------------------------------------------------------------- */
/* IDREF is an opNode that represents a reference to a model_component
 */
class opNodeIDREF : public opNode {
 public:
  model_comp *ref; //!< pointer to the model component referred to by this node
  opNodeIDREF *clone();
};


// typedef struct {
//   int relCode;
//   opNode *lval;
//   opNode *rval;
//} relNode;

//typedef struct {
// opNode *node;
//  opNode *indexing;
//  struct AmplModel_st *context;
//} retType;


/** \brief Linked list of opNode.

    This is a list of arguments. A comma separated list of arguments is
    represented by an opNode with 
     - opCode=COMMA, 
     - nval=#items on list
     - values[0]=first item on list (of type _indexNode)
     - values[1]=last item on list (of type _indexNode)
*/
typedef struct _indexNode {
  opNode *value;          //!< item on the linked list
  struct _indexNode *next; //!< next item on the linked list
} indexNode;

opNode *newTertOp(int opCode, void *val1, void *val2, void *val3);
opNode *newBinOp(int opCode, void *lval, void *rval);
opNode *newUnaryOp(int opCode, void *val);
//relNode *newRel(int opCode, opNode *lval, opNode *rval);
void freeOpNode(opNode *target);
//indexNode *newIndexNode(opNode *node);
opNode *addItemToList(opNode *list, opNode *newitem);
opNode *addItemToListNew(opNode *list, opNode *newitem);
//retType *newRetType(opNode *node, opNode *indexing, 
//		    struct AmplModel_st *context);
char *print_opNode(opNode *node);
char *print_opNodesymb(opNode *node);

#endif
