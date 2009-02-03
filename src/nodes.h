#ifndef NODES_H
#define NODES_H

#include <iostream>
#include <sstream>
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
 *  There are a few special meanings of the values array depending on the
 *  type of node (i.e. the value of opCode).
 *  @bug This should probably be implemented by deriving subclasses, however
 *       an attempt for ID nodes resulted in problems with dynamic_casts
 *
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
  friend opNode* find_var_ref_in_context(AmplModel *context, opNode *ref);
  friend char *print_opNodesymb(opNode *node);
 protected:
 public: 
  int nval;      //!< number of arguments 
  int opCode;    //!< ID CODE of this node (a list can be found in ampl.tab.h 
  
  /** For opCode==ID, there is usually one argument, this is a (char*) to the
   *  name of the entity. If there are two arguments the second argument is
   *  an (int*) stating the ancestor information as set by ancestor(1).ID in
   *  stochastic programming */
  opNode **values; //!< list of arguments 

  /* FIXME: not sure if these two should be part of the class. They are 
     global variables that affect the print() method */
  /** if use_global_names is set then the print() method will print out
   *  model component names as global names                               */
  static int use_global_names; 
  static AmplModel *default_model;
  static string node;  //< current replacement string for the 'node' keyword
  static string stage; //< current replacement string for the 'stage' keyword

 public:
  // ------------------------ methods -----------------------------------
  // Constructors
  opNode();
  opNode(int opCode, opNode *val1=NULL, opNode *val2=NULL, opNode *val3=NULL);
  opNode(opNode &src);
  // Destructor
  virtual ~opNode();

  /** for nodes that are indexing expressions, get the set that is indexed over
   * FIXME: move this to opNodeIx, also indexing expressions are more compl. */
  opNode *getIndexingSet();

  string print();              // recursive printing of expression

  /** for nodes that represent values (ID, INT_VAL, FLOAT_VAL), this returns
      the value of this node as a c_string */
  virtual char *getValue();  //!< Returns value of ID, INT_VAL and FLOAT_VAL node

  void dump(ostream &fout); //!< diagnostic printing

  string printDummyVar();      // node is a dummy var -> remove (..) 

  /** return comma separated list of arguments for IDREF nodes */
  string getArgumentList() const;  //< return comma separated list of arguments 

  /** find all IDREF's below current node and print to screen */
  void findIDREF();           

  /** find all IDREFs below current node */
  virtual void findIDREF(list<model_comp*> &lmc);

  /** find all IDREF nodes below current node */
  virtual void findIDREF(list<opNode*> *lnd);

  /** find all nodes of opCode oc below current node */
  virtual void findOpCode(int oc, list<opNode*> *lnd);

  /** find model_comp (if it exitst) refered to by this opNode:
   *  If the expression given by this opNode is an immediate reference to
   *  a model_comp then return that. Otherwise return NULL
   */
  model_comp *findModelComp();
  
  //! returns the 'double' represented by this node (if of type INT/FLOAT_VAL)
  virtual double getFloatVal();

  opNode &merge(const opNode &src);

  int nchild() const { return nval; }

  class Iterator {
     private:
      opNode **ptr;

     public:
      Iterator(opNode **p) { ptr=p; }
      ~Iterator() {}

      Iterator& operator=(const Iterator &other) {
         ptr = other.ptr;
         return (*this);
      }
      bool operator==(const Iterator &other) {
         return (other.ptr == ptr);
      }
      bool operator!=(const Iterator &other) {
         return (other.ptr != ptr);
      }
      Iterator& operator++() {
         ptr++;
         return(*this);
      }
      Iterator operator++(int) {
         Iterator tmp = *this;
         ++(*this);
         return tmp;
      }

      opNode* operator*() const {
         return *ptr;
      }

      opNode** operator&() const {
         return ptr;
      }
  };

  virtual Iterator begin() const { return Iterator(values); }
  virtual Iterator end() const { return Iterator(values+nval); }
  
  /** creates a deep_copy of the nodes: opNodes pointed to are recreated 
   *  as well. 
   *  Non-opNode objects pointed to are not recreated, here just pointers
   *  are copied (->ref in the case of an opNodeIDREF object)
   *  The int/double entries pointed to by INT_VAL/FLOAT_VAL opNodes *are*
   *  recreated
   */
  virtual opNode *deep_copy();

  /** creates a copy of the node, reusing the pointer in the current node */
  virtual opNode *clone();
  //  virtual void foo();

  virtual ostream& put(ostream&s) const;
  virtual opNode *push_back(opNode *newitem);
  virtual opNode *push_front(opNode *newitem);
};

/** \brief A node on the operator tree representing an indexing expression. 
 *
 *  This is node on the operator tree that represents an indexing expression
 *  A general indexing expression can be of the form
 *
 *  {(i,j) in ARCS, k in SET: i>k} 
 * 
 * it will be broken down into a list of 'dummy IN set' expressions plus an
 * optional qualifier (the condition to the right of COLON)
 */
class opNodeIx : public opNode {
 public:
  opNode *qualifier;    //!< the stuff to the right of ':' (if present)
  int ncomp;            //!< number of 'dummy IN set'-type expressions
  opNode **sets;        //!< list of the set expressions
  model_comp **sets_mc;  //!< list of model_comp for the indexing sets
  opNode **dummyVarExpr;//!< list of the dummyVarExpressions
  // private:            
  int done_split; //!< indicates that extra fields have been set: qualifier/ncomp/sets/dummyVarExpr
  // --------------------------- methods ----------------------------------
  opNodeIx();  //!< default constructor: sets everything to 0/NULL
  opNodeIx(opNode *on); //!< initialise values from an opNode

  //! Finds if the indexing expression defines a given dummy variable 
  opNode *hasDummyVar(const char *const name); 

  //! returns list of all dummy variables defined by this index'g expression 
  list<opNode *> getListDummyVars(); 

  //! set up the ->sets, ->dummyVarExpr, ->ncomp, ->qualifier components 
  void splitExpression();   
  
  //! copies node and all its subnodes into new datastructures 
  opNodeIx *deep_copy();    

  void printDiagnostic(ostream &fout);   //!< diagnostic print of class variables
};


// FIXME: opNodeID was an attempt at a subclass for ID opNodes, this should
//        directly carry information about possible ancestor settings
//        => lead to a problem with dynamic_cast, hence abondoned
//* ----------------------------------------------------------------------------
//opNodeID 
//---------------------------------------------------------------------------- */
///** opNodeID is an opNode that represents an ID. The only reason to have
// *  it separate is so that it can carry extra information (such as how
// *  many levels further up it should refer to for stoch programming)
// *  @bug: at the moment only if the "parent" information in a StochModel
// *        is set is this generated, possibly all opNode's with code ID
// *         should be of this type
// */
//
//class opNodeID : public opNode {
// public:
//  int stochparent; //< levels above this one for which the reference is
//
//  opNodeID *clone();
//};

/* ----------------------------------------------------------------------------
IDNode
---------------------------------------------------------------------------- */
/** \brief A node on the tree representing a user identifier [ie variable name]
 */

class IDNode : public opNode {
  public:
   const string name;
  
  public:
   IDNode(const char *const new_name, opNode *stochparent=NULL);
   IDNode(const string name, opNode *stochparent=NULL);
   double getFloatVal() { return atof(name.c_str()); }
   char *getValue() { return strdup(name.c_str()); }
   void findIDREF(list<model_comp*> &lmc) { return; }
   void findIDREF(list<opNode*> &lnd) { return; }
   // We never search for ID:
   void findOpCode(int oc, list<opNode*> *lnd) { return; }
   ostream& put(ostream&s) const { 
      return s << name;
   }
   opNode *deep_copy() { 
      if(values) { // we have a ancestor index
         return new IDNode(name, (*values)->deep_copy());
      } else {
         return new IDNode(name); 
      }
   }
   opNode *clone() { return deep_copy(); }
};


/* ----------------------------------------------------------------------------
opNodeIDREF 
---------------------------------------------------------------------------- */
/** \brief A node on the tree representing a reference to a model_comp. 
 *
 * IDREF is an opNode that represents a reference to a model_component
 *
 */
class opNodeIDREF : public opNode {
 public:
  model_comp *ref; //!< pointer to the model_comp referred to by this node
  /* stochrecourse was for the same purpose as stochparent, just that the
     recourse level was given as an opNode (i.e. expression to be 
     eveluated by AMPL rather than an explicit  INT_VAL */
  //?opNode *stochrecourse; //!< resourse level in stoch programming
  /** This field is only meaningful if the node represents a component
   *  in a stochastic program. In that case stochparent gives the recourse
   *  level of the component. This is the first argument in expressions
   *  such as xh(-1,i) which refers to xh[i] in the parent stage.
   */
  int stochparent; //!< levels above this one for which the reference is
  // ---------------------------- methods -----------------------------
  // constructor
  opNodeIDREF(model_comp *r=NULL);
  
  //! creates a shallow copy: points to the same components as the original
  opNodeIDREF *clone();
  
  //! creates a copy using all new datastrutures (does not duplicate ->ref)
  opNodeIDREF *deep_copy();

  ostream& put(ostream&s) const;
};

/** @class ValueNode
 * represents a value.
 */
template<class T> class ValueNode : public opNode {
  public:
   const T value;

  public:
   ValueNode(const T new_value) :
      opNode(-99), value(new_value) {}
   double getFloatVal() { return value; }
   char *getValue();
   void findIDREF(list<model_comp*> &lmc) { return; }
   void findIDREF(list<opNode*> &lnd) { return; }
   // We never search for INT_VAL or FLOAT_VAL:
   void findOpCode(int oc, list<opNode*> *lnd) { return; }
   ostream& put(ostream&s) const { return s << this->value; }
   opNode *deep_copy() { return new ValueNode<T>(value); }
   opNode *clone() { return deep_copy(); }
};
template<class T> char *ValueNode<T>::getValue() {
   ostringstream ost;
   ost << value;
   return strdup(ost.str().c_str());
}

/** @class ListNode
 * represents a comma seperated list of opNodes
 */
class ListNode: public opNode {
  public:
   typedef list<opNode *>::const_iterator iterator;

  private:
   list<opNode *> list;

  public:
   ListNode();
   iterator begin() { return list.begin(); }
   iterator end() { return list.end(); }
   ostream& put(ostream&s) const;
   ListNode *deep_copy();
   ListNode *clone();
   opNode *push_front(opNode *node) { list.push_front(node); return this; }
   opNode *push_back(opNode *node) { list.push_back(node); return this; }
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
     - nval=\#items on list
     - values[0]=first item on list (of type _indexNode)
     - values[1]=last item on list (of type _indexNode)
*/
typedef struct _indexNode {
  opNode *value;          //!< item on the linked list
  struct _indexNode *next; //!< next item on the linked list
} indexNode;

//opNodeID *newUnaryOpID(void *val);
//relNode *newRel(int opCode, opNode *lval, opNode *rval);
//void freeOpNode(opNode *target);
//indexNode *newIndexNode(opNode *node);
//opNode *addItemToList(opNode *list, opNode *newitem);
inline opNode *addItemToListNew(opNode *list, opNode *newitem) {
   return list->push_back(newitem);
}
opNode *addItemToListOrCreate(int oc, opNode *list, opNode *newitem);
inline opNode *addItemToListBeg(opNode *newitem, opNode *list) {
   return list->push_front(newitem);
}
//retType *newRetType(opNode *node, opNode *indexing, 
//		    struct AmplModel_st *context);
char *print_opNodesymb(opNode *node);

ostream& operator<<(ostream& s, const opNode &node);
ostream& operator<<(ostream& s, const opNode *node);

#endif
