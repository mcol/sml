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
#ifndef NODES_H
#define NODES_H

#include <sstream>
#include <list>
#include <vector>

class ModelComp;
class AmplModel;


/** @class SyntaxNode
 *  This is a node in the operator tree.
 *
 *  All AMPL/SML expressions are broken down into a tree of operators. 
 *  The nodes of this tree are objects of type SyntaxNode.
 *
 *  Note that values[] is a list of untyped pointers. Normally these
 *  would point to further SyntaxNode structure (children of the current
 *  node). For an ID it is simply a pointer to the string containing
 *  the name.
 *
 *  There are a few special meanings of the values array depending on the
 *  type of node (i.e. the value of opCode).
 *  @bug This should probably be implemented by deriving subclasses, however
 *       an attempt for ID nodes resulted in problems with dynamic_casts
 *
 *  @bug A comma separated list is currently represented by a SyntaxNode with
 *       opCode==COMMA and values[0/1] pointing to the first and last member
 *       in a linked list of SyntaxNodes. This linked list is implemented using
 *       _indexNode objects (which is a SyntaxNode plus a pointer to next). 
 *       This is done to facilitate adding members to the list without 
 *       knowing the dimension beforehand. Better implemented by replacing
 *       the SyntaxNode::values array with a C++ list.
 */
class SyntaxNode {

 public:
  virtual int nchild() const { return nval; }
  class iterator {
     private:
      SyntaxNode **ptr;
     public:
      iterator(SyntaxNode **p) { ptr=p; }
      ~iterator() {}
      iterator& operator=(const iterator &other) {
         ptr = other.ptr;
         return (*this);
      }
      bool operator==(const iterator &other) { return (other.ptr == ptr); }
      bool operator!=(const iterator &other) { return (other.ptr != ptr); }
      iterator& operator++() {
         ptr++;
         return(*this);
      }
      iterator operator++(int) {
         iterator tmp = *this;
         ++(*this);
         return tmp;
      }
      SyntaxNode* operator*() const { return *ptr; }
  };

  iterator begin() const { return iterator(values); }
  iterator end() const { return iterator(values+nval); }

  /** Clear the child list */
  virtual void clear() { nval = 0; delete[] values; values = NULL; }

  /** ID CODE of this node (a list can be found in ampl.tab.h) */
  int opCode;

 protected:

  /** Number of arguments */
  int nval;

  /** List of arguments.
   *
   *  For opCode==ID, there is usually one argument, this is a (char*) to the
   *  name of the entity. If there are two arguments the second argument is
   *  an (int*) stating the ancestor information as set by ancestor(1).ID in
   *  stochastic programming */
  SyntaxNode **values;

 public: 
  
  /* FIXME: not sure if these two should be part of the class. They are 
     global variables that affect the print() method */
  /** if use_global_names is set then the print() method will print out
   *  model component names as global names                               */
  static int use_global_names; 
  static AmplModel *default_model;

 public:
  // ------------------------ methods -----------------------------------

  /** Constructor */
  SyntaxNode(int opCode, SyntaxNode *val1=NULL, SyntaxNode *val2=NULL, SyntaxNode *val3=NULL);

  /** Copy constructor */
  SyntaxNode(const SyntaxNode &src);

  /** Destructor */
  virtual ~SyntaxNode();

  /** Recursive printing of expression */
  std::string print();

  /** for nodes that represent values (ID, INT_VAL, FLOAT_VAL), this returns
      the value of this node as a c_string */
  virtual std::string getValue() const { throw std::exception(); return "(fail)"; }

  /** Diagnostic printing */
  void dump(std::ostream &fout);

  // node is a dummy var -> remove (..)
  std::string printDummyVar();

  /** Return comma separated list of arguments for IDREF nodes */
  std::string getArgumentList() const;

  /** Find all IDREF's below current node and print to screen */
  void findIDREF() const;

  /** Find all IDREFs below current node */
  virtual void findIDREF(std::list<ModelComp*>& lmc) const;

  /** Find all IDREF nodes below current node */
  virtual void findIDREF(std::list<SyntaxNode*> *lnd);

  /** Find all nodes of opCode oc below current node */
  virtual void findOpCode(int oc, std::list<SyntaxNode*> *lnd);

  /** Find the ModelComp (if it exists) refered to by this SyntaxNode.
   *
   *  If the expression given by this SyntaxNode is an immediate reference to
   *  a ModelComp then return that, otherwise return NULL.
   */
  ModelComp *findModelComp();

  SyntaxNode &merge(const SyntaxNode &src);

  /** Creates a deep_copy of the nodes: SyntaxNodes pointed to are recreated
   *  as well. 
   *
   *  Non-SyntaxNode objects pointed to are not recreated, here just pointers
   *  are copied (->ref in the case of a SyntaxNodeIDREF object).
   *  The int/double entries pointed to by INT_VAL/FLOAT_VAL SyntaxNodes *are*
   *  recreated.
   */
  virtual SyntaxNode *deep_copy();

  /** Creates a copy of the node, reusing the pointer in the current node */
  virtual SyntaxNode *clone();

  virtual std::ostream& put(std::ostream& s) const;
  virtual SyntaxNode *push_back(SyntaxNode *newitem);
  virtual SyntaxNode *push_front(SyntaxNode *newitem);
};

/** @class StageNodeNode
 */
class StageNodeNode : public SyntaxNode {

 private:
  std::string value_;

 public:

  /** Current replacement string for the 'node' keyword */
  static std::string node;

  /** Current replacement string for the 'stage' keyword */
  static std::string stage;

  /** Constructor */
  StageNodeNode(int opCode_, const std::string& value = "") :
     SyntaxNode(opCode_), value_(value) {}

  std::ostream& put(std::ostream& s) const;
  SyntaxNode *clone() { return new StageNodeNode(opCode, value_); }
  SyntaxNode *deep_copy() { return clone(); }

  void setValue(const std::string& value) {
    value_ = value;
  }

};

/** @class SyntaxNodeIx
 *  A node on the operator tree representing an indexing expression.
 *
 *  This is node on the operator tree that represents an indexing expression.
 *  A general indexing expression can be of the form:
 *
 *  {(i,j) in ARCS, k in SET: i>k} 
 * 
 *  which will be broken down into a list of 'dummy IN set' expressions plus
 *  an optional qualifier (the condition to the right of COLON).
 */
class SyntaxNodeIx : public SyntaxNode {

 private:

  SyntaxNodeIx(const int opCode_) :
    SyntaxNode(opCode_), qualifier(NULL), ncomp(0), sets(NULL), sets_mc(NULL),
    dummyVarExpr(NULL), done_split(0) {};

 public:

  SyntaxNode *qualifier;    //!< the stuff to the right of ':' (if present)
  int ncomp;            //!< number of 'dummy IN set'-type expressions
  SyntaxNode **sets;        //!< list of the set expressions
  ModelComp **sets_mc;  //!< list of ModelComp for the indexing sets
  SyntaxNode **dummyVarExpr;//!< list of the dummyVarExpressions
  // private:            
  int done_split; //!< indicates that extra fields have been set: qualifier/ncomp/sets/dummyVarExpr
  // --------------------------- methods ----------------------------------

  //! Constructor
  SyntaxNodeIx(SyntaxNode *on);

  //! Finds if the indexing expression defines a given dummy variable 
  SyntaxNode *hasDummyVar(const std::string& name);

  //! returns list of all dummy variables defined by this index'g expression 
  std::list<SyntaxNode *> getListDummyVars();

  //! set up the ->sets, ->dummyVarExpr, ->ncomp, ->qualifier components 
  void splitExpression();   
  
  //! copies node and all its subnodes into new datastructures 
  SyntaxNodeIx *deep_copy();    

  //!< diagnostic print of class variables
  void printDiagnostic(std::ostream &fout);

  /** for nodes that are indexing expressions, get the set that is indexed over
   */
  SyntaxNode *getIndexingSet();
};

class ValueNodeBase {
  public:
   virtual ~ValueNodeBase() {}
   virtual double getFloatVal() const = 0;
};


/* ----------------------------------------------------------------------------
IDNode
---------------------------------------------------------------------------- */
/** @class IDNode
 *  A node on the tree representing a user identifier (ie variable name).
 */
class IDNode : public SyntaxNode {

 private:
   std::string name;
   long stochparent;
  
  public:
   IDNode(const std::string& name, long stochparent=0);
   std::string getValue() const { return name; }
   void findIDREF(std::list<ModelComp*> &lmc) { return; }
   void findIDREF(std::list<SyntaxNode*> *lnd) { return; }
   // We never search for ID:
   void findOpCode(int oc, std::list<SyntaxNode*> *lnd);
   std::ostream& put(std::ostream& s) const {
      return s << name;
   }
   SyntaxNode *deep_copy() { 
      return new IDNode(name, stochparent);
   }

   void setName(const std::string& id) {
     name = id;
   }

   std::string id() const {
     return name;
   }

   void setStochParent(long parent) {
     stochparent = parent;
   }

   long getStochParent() const {
     return stochparent;
   }

};


/* ----------------------------------------------------------------------------
SyntaxNodeIDREF 
---------------------------------------------------------------------------- */
/** @class SyntaxNodeIDREF
 *  A node on the tree representing a reference to a ModelComp.
 *
 *  IDREF is a SyntaxNode that represents a reference to a ModelComponent.
 */
class SyntaxNodeIDREF : public SyntaxNode {

 public:

  /** Pointer to the ModelComp referred to by this node */
  ModelComp *ref;

  /* stochrecourse was for the same purpose as stochparent, just that the
     recourse level was given as a SyntaxNode (i.e. expression to be
     eveluated by AMPL rather than an explicit  INT_VAL) */
  //?SyntaxNode *stochrecourse; //!< resourse level in stoch programming

  /** Levels above this one for which the reference is.
   *
   *  This field is only meaningful if the node represents a component
   *  in a stochastic program. In that case stochparent gives the recourse
   *  level of the component. This is the first argument in expressions
   *  such as xh(-1,i) which refers to xh[i] in the parent stage.
   */
  int stochparent;

  // ---------------------------- methods -----------------------------
  // constructor
  SyntaxNodeIDREF(ModelComp *r=NULL, SyntaxNode *val1=NULL);
  SyntaxNodeIDREF(int opCode, ModelComp *r=NULL);
  
  /** Creates a shallow copy: points to the same components as the original */
  SyntaxNodeIDREF *clone();
  
  /** Creates a copy using all new datastructures (does not duplicate ref) */
  SyntaxNodeIDREF *deep_copy();

  std::ostream& put(std::ostream& s) const;
};

/** @class ValueNode
 *  Represents a value.
 */
template<class T> class ValueNode : public SyntaxNode, virtual ValueNodeBase {
  public:
   const T value;

  public:
   ValueNode(const T new_value) :
      SyntaxNode(-99), value(new_value) {}
   double getFloatVal() const { return value; }
   std::string getValue() const;
   void findIDREF(std::list<ModelComp*> &lmc) { return; }
   void findIDREF(std::list<SyntaxNode*> *lnd) { return; }
   // We never search for INT_VAL or FLOAT_VAL:
   void findOpCode(int oc, std::list<SyntaxNode*> *lnd) { return; }
   std::ostream& put(std::ostream&s) const { return s << this->value; }
   SyntaxNode *deep_copy() { return new ValueNode<T>(value); }
   SyntaxNode *clone() { return deep_copy(); }
};
template<class T> std::string ValueNode<T>::getValue() const {
   std::ostringstream ost;
   ost << value;
   return ost.str();
}

/** @class ListNode
 *  Represents a comma separated list of SyntaxNodes.
 */
class ListNode: public SyntaxNode {

  public:
   typedef std::vector<SyntaxNode*>::const_iterator literator;

  private:
   std::vector<SyntaxNode *> list_;

  public:
   ListNode(int opCode=',', SyntaxNode *val1=NULL, SyntaxNode *val2=NULL);
   literator lbegin() const { return list_.begin(); }
   literator lend() const { return list_.end(); }
   std::ostream& put(std::ostream& s) const;
   ListNode *deep_copy();
   ListNode *clone();
   ListNode *push_front(SyntaxNode *node) { 
      SyntaxNode::push_front(node);
      list_.insert(list_.begin(), node); 
      return this; 
   }
   ListNode *push_back(SyntaxNode *node) { 
      SyntaxNode::push_back(node);
      list_.push_back(node); 
      return this; 
   }
   int nchild() const { return list_.size(); }
   SyntaxNode *operator[](int i) const { return list_[i]; }
   void clear() { 
      SyntaxNode::clear();
      list_.clear();
   }
};

/** @class OpNode
 *  Represents an operator.
 */
class OpNode : public SyntaxNode {
  public:
   SyntaxNode *left;
   SyntaxNode *right;

  public:
   OpNode(int opCode, SyntaxNode *op1, SyntaxNode *op2=NULL);
   std::ostream& put(std::ostream& s) const;
   OpNode *deep_copy();
   OpNode *clone();
   void findIDREF(std::list<ModelComp*>& lmc) {
      if(left) left->findIDREF(lmc);
      if(right) right->findIDREF(lmc);
   }
   void findIDREF(std::list<SyntaxNode*> *lnd) {
      if(left) left->findIDREF(lnd);
      if(right) right->findIDREF(lnd);
   }
   void findOpCode(int oc, std::list<SyntaxNode*> *lnd) {
      if(left) left->findOpCode(oc, lnd);
      if(right) right->findOpCode(oc, lnd);
   }
};

ListNode *addItemToListOrCreate(int oc, ListNode *list, SyntaxNode *newitem);
std::string print_SyntaxNodesymb(SyntaxNode *node);

std::ostream& operator<<(std::ostream& s, const SyntaxNode &node);
std::ostream& operator<<(std::ostream& s, const SyntaxNode *node);

// Routines taken from ampl.h
SyntaxNode *findKeywordinTree(SyntaxNode *root, int oc);
SyntaxNode* find_var_ref_in_context(AmplModel *context, SyntaxNode *ref);
SyntaxNode* find_var_ref_in_indexing(const std::string& name);

#endif
