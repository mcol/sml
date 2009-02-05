#ifndef NODES_H
#define NODES_H

#include <iostream>
#include <sstream>
#include <list>
class ModelComp;
using namespace std;

class AmplModel;


/** @class SyntaxNode
 *  @brief This is a node in the operator tree. 
 *
 *  All AMPL/SML expressions are broken down into a tree of operators. 
 *  The nodes of this tree are objects of type SyntaxNode.
 *
 *  Note that values[] is a list of untyped pointers. Normally these
 *  would point to further SyntaxNode structure (children of the current
 *  node). For an ID it is simply a pointer to the string containing
 *  the name
 *
 *  There are a few special meanings of the values array depending on the
 *  type of node (i.e. the value of opCode).
 *  @bug This should probably be implemented by deriving subclasses, however
 *       an attempt for ID nodes resulted in problems with dynamic_casts
 *
 *
 *  \bug A comma separated list is currently represented by an SyntaxNode with
 *       opCode==COMMA and values[0/1] pointing to the first and last member
 *       in a linked list of SyntaxNodes. This linked list is implemented using
 *       _indexNode objects (which is a SyntaxNode plus a pointer to next). 
 *       This is done to facilitate adding members to the list without 
 *       knowing the dimension beforehand. Better implemented by replacing
 *       the SyntaxNode::values array with a C++ list.
 */
class SyntaxNode {
  friend SyntaxNode* find_var_ref_in_context(AmplModel *context,
    SyntaxNode *ref);
  friend char *print_SyntaxNodesymb(SyntaxNode *node);
 protected:
 public: 
  int nval;      //!< number of arguments 
  int opCode;    //!< ID CODE of this node 
                 // (a list can be found in ampl.tab.h)
  
  /** For opCode==ID, there is usually one argument, this is a (char*) to the
   *  name of the entity. If there are two arguments the second argument is
   *  an (int*) stating the ancestor information as set by ancestor(1).ID in
   *  stochastic programming */
  SyntaxNode **values; //!< list of arguments 

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
  SyntaxNode();
  SyntaxNode(int opCode, SyntaxNode *val1=NULL, SyntaxNode *val2=NULL, SyntaxNode *val3=NULL);
  SyntaxNode(SyntaxNode &src);
  // Destructor
  virtual ~SyntaxNode();

  /** for nodes that are indexing expressions, get the set that is indexed over
   * FIXME: move this to SyntaxNodeIx, also indexing expressions are more compl. */
  SyntaxNode *getIndexingSet();

  string print();              // recursive printing of expression

  /** for nodes that represent values (ID, INT_VAL, FLOAT_VAL), this returns
      the value of this node as a c_string */
  virtual char *getValue() const { throw exception(); return strdup("(fail)"); }

  void dump(ostream &fout); //!< diagnostic printing

  string printDummyVar();      // node is a dummy var -> remove (..) 

  /** return comma separated list of arguments for IDREF nodes */
  string getArgumentList() const;  //< return comma separated list of arguments 

  /** find all IDREF's below current node and print to screen */
  void findIDREF();           

  /** find all IDREFs below current node */
  virtual void findIDREF(list<ModelComp*> &lmc);

  /** find all IDREF nodes below current node */
  virtual void findIDREF(list<SyntaxNode*> *lnd);

  /** find all nodes of opCode oc below current node */
  virtual void findOpCode(int oc, list<SyntaxNode*> *lnd);

  /** find ModelComp (if it exitst) refered to by this SyntaxNode:
   *  If the expression given by this SyntaxNode is an immediate reference to
   *  a ModelComp then return that. Otherwise return NULL
   */
  ModelComp *findModelComp();

  SyntaxNode &merge(const SyntaxNode &src);

  virtual int nchild() const { return nval; }

  class Iterator {
     private:
      SyntaxNode **ptr;

     public:
      Iterator(SyntaxNode **p) { ptr=p; }
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

      SyntaxNode* operator*() const {
         return *ptr;
      }

      SyntaxNode** operator&() const {
         return ptr;
      }
  };

  virtual Iterator begin() const { return Iterator(values); }
  virtual Iterator end() const { return Iterator(values+nval); }
  
  /** creates a deep_copy of the nodes: SyntaxNodes pointed to are recreated 
   *  as well. 
   *  Non-SyntaxNode objects pointed to are not recreated, here just pointers
   *  are copied (->ref in the case of an SyntaxNodeIDREF object)
   *  The int/double entries pointed to by INT_VAL/FLOAT_VAL SyntaxNodes *are*
   *  recreated
   */
  virtual SyntaxNode *deep_copy();

  /** creates a copy of the node, reusing the pointer in the current node */
  virtual SyntaxNode *clone();
  //  virtual void foo();

  virtual ostream& put(ostream&s) const;
  virtual SyntaxNode *push_back(SyntaxNode *newitem);
  virtual SyntaxNode *push_front(SyntaxNode *newitem);
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
class SyntaxNodeIx : public SyntaxNode {
 private:
  SyntaxNodeIx(const int opCode) :
    SyntaxNode(opCode) {}
 public:
  SyntaxNode *qualifier;    //!< the stuff to the right of ':' (if present)
  int ncomp;            //!< number of 'dummy IN set'-type expressions
  SyntaxNode **sets;        //!< list of the set expressions
  ModelComp **sets_mc;  //!< list of ModelComp for the indexing sets
  SyntaxNode **dummyVarExpr;//!< list of the dummyVarExpressions
  // private:            
  int done_split; //!< indicates that extra fields have been set: qualifier/ncomp/sets/dummyVarExpr
  // --------------------------- methods ----------------------------------
  SyntaxNodeIx();  //!< default constructor: sets everything to 0/NULL
  SyntaxNodeIx(SyntaxNode *on); //!< initialise values from an SyntaxNode

  //! Finds if the indexing expression defines a given dummy variable 
  SyntaxNode *hasDummyVar(const char *const name); 

  //! returns list of all dummy variables defined by this index'g expression 
  list<SyntaxNode *> getListDummyVars(); 

  //! set up the ->sets, ->dummyVarExpr, ->ncomp, ->qualifier components 
  void splitExpression();   
  
  //! copies node and all its subnodes into new datastructures 
  SyntaxNodeIx *deep_copy();    

  void printDiagnostic(ostream &fout);   //!< diagnostic print of class variables
};

class ValueNodeBase {
  public:
   virtual double getFloatVal() const { throw exception(); return 0.0; }
};


/* ----------------------------------------------------------------------------
IDNode
---------------------------------------------------------------------------- */
/** \brief A node on the tree representing a user identifier [ie variable name]
 */
class IDNode : public SyntaxNode, virtual ValueNodeBase {
  public:
   const string name;
   long stochparent;
  
  public:
   IDNode(const char *const new_name, long stochparent=0);
   IDNode(const string name, long stochparent=0);
   char *getValue() const { return strdup(name.c_str()); }
   void findIDREF(list<ModelComp*> &lmc) { return; }
   void findIDREF(list<SyntaxNode*> *lnd) { return; }
   // We never search for ID:
   void findOpCode(int oc, list<SyntaxNode*> *lnd) { return; }
   ostream& put(ostream&s) const { 
      return s << name;
   }
   SyntaxNode *deep_copy() { 
      return new IDNode(name, stochparent);
   }
   SyntaxNode *clone() { return deep_copy(); }
};


/* ----------------------------------------------------------------------------
SyntaxNodeIDREF 
---------------------------------------------------------------------------- */
/** \brief A node on the tree representing a reference to a ModelComp. 
 *
 * IDREF is an SyntaxNode that represents a reference to a ModelComponent
 *
 */
class SyntaxNodeIDREF : public SyntaxNode {
 public:
  ModelComp *ref; //!< pointer to the ModelComp referred to by this node
  /* stochrecourse was for the same purpose as stochparent, just that the
     recourse level was given as an SyntaxNode (i.e. expression to be 
     eveluated by AMPL rather than an explicit  INT_VAL */
  //?SyntaxNode *stochrecourse; //!< resourse level in stoch programming
  /** This field is only meaningful if the node represents a component
   *  in a stochastic program. In that case stochparent gives the recourse
   *  level of the component. This is the first argument in expressions
   *  such as xh(-1,i) which refers to xh[i] in the parent stage.
   */
  int stochparent; //!< levels above this one for which the reference is
  // ---------------------------- methods -----------------------------
  // constructor
  SyntaxNodeIDREF(ModelComp *r=NULL);
  SyntaxNodeIDREF(int opCode, ModelComp *r=NULL);
  
  //! creates a shallow copy: points to the same components as the original
  SyntaxNodeIDREF *clone();
  
  //! creates a copy using all new datastrutures (does not duplicate ->ref)
  SyntaxNodeIDREF *deep_copy();

  ostream& put(ostream&s) const;
};

/** @class ValueNode
 * represents a value.
 */
template<class T> class ValueNode : public SyntaxNode, virtual ValueNodeBase {
  public:
   const T value;

  public:
   ValueNode(const T new_value) :
      SyntaxNode(-99), value(new_value) {}
   double getFloatVal() const { return value; }
   char *getValue() const ;
   void findIDREF(list<ModelComp*> &lmc) { return; }
   void findIDREF(list<SyntaxNode*> *lnd) { return; }
   // We never search for INT_VAL or FLOAT_VAL:
   void findOpCode(int oc, list<SyntaxNode*> *lnd) { return; }
   ostream& put(ostream&s) const { return s << this->value; }
   SyntaxNode *deep_copy() { return new ValueNode<T>(value); }
   SyntaxNode *clone() { return deep_copy(); }
};
template<class T> char *ValueNode<T>::getValue() const {
   ostringstream ost;
   ost << value;
   return strdup(ost.str().c_str());
}

/** @class ListNode
 * represents a comma seperated list of SyntaxNodes
 */
class ListNode: public SyntaxNode {
  public:
   typedef list<SyntaxNode *>::const_iterator iterator;

  private:
   list<SyntaxNode *> list;

  public:
   ListNode(SyntaxNode *val1=NULL, SyntaxNode *val2=NULL);
   iterator begin() { return list.begin(); }
   iterator end() { return list.end(); }
   ostream& put(ostream&s) const;
   ListNode *deep_copy();
   ListNode *clone();
   SyntaxNode *push_front(SyntaxNode *node) { 
      list.push_front(node); return this; 
   }
   SyntaxNode *push_back(SyntaxNode *node) { 
      list.push_back(node); return this; 
   }
   int nchild() { return list.size(); }
};

class OpNode : public SyntaxNode {
  public:
   SyntaxNode *operand[3];

  public:
   OpNode(int opCode, SyntaxNode *op1, SyntaxNode *op2=NULL, 
      SyntaxNode *op3=NULL);
   ostream& put(ostream& s) const;
   OpNode *deep_copy();
   OpNode *clone();
   void findIDREF(list<ModelComp*> &lmc) { 
      for(int i=0; i<nval; ++i) operand[i]->findIDREF(lmc);
   }
   void findIDREF(list<SyntaxNode*> *lnd) { 
      for(int i=0; i<nval; ++i) operand[i]->findIDREF(lnd);
   }
   void findOpCode(int oc, list<SyntaxNode*> *lnd) {
      for(int i=0; i<nval; ++i) operand[i]->findOpCode(oc, lnd);
   }
};

SyntaxNode *addItemToListOrCreate(int oc, SyntaxNode *list, SyntaxNode *newitem);
char *print_SyntaxNodesymb(SyntaxNode *node);

ostream& operator<<(ostream& s, const SyntaxNode &node);
ostream& operator<<(ostream& s, const SyntaxNode *node);

// Routines taken from ampl.h
SyntaxNode *findKeywordinTree(SyntaxNode *root, int oc);
SyntaxNode* find_var_ref_in_context(AmplModel *context, SyntaxNode *ref);
SyntaxNodeIDREF* find_var_ref_in_context_(AmplModel *context, IDNode *ref);
SyntaxNode* find_var_ref_in_indexing(const char *const name);

#endif
