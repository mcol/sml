#ifndef AMPLMODEL_H
#define AMPLMODEL_H

#include "ModelComp.h"
#include "nodes.h"
#include "symtab.h"
#include "ExpandedModel.h"
#include <string>

enum {CHANGE_NOACT=0,CHANGE_REM=1,CHANGE_ADD=2};

/** @class changeitem
 *  @brief Simple struct that stores a queued change to the model tree
 *
 *  This is needed to treat expectation constraints that in the postprocessing
 *  need to be removed from the model in which they are defined and added
 *  to a different model. This action cannot be done by recursively working
 *  through all models and ModelComps (since removing/adding comps 
 *  invalidates the iterators used in the recursion)
 */
class changeitem{
 public:
  ModelComp *comp;   //< The component to be added or removed
  AmplModel *model;   //< The model to which it should be added/removed
  int action;         //< the action (CHANGE_REM/CHANGE_ADD)
};


/** @class AmplModel
 *  @brief This class describes a model (block) in the flat model tree.
 *
 *  It should really be called FlatModelNode (or something like that).
 *  It keeps track of the components (vars/cons/sets/params/submodels) 
 *  associated with this model. 
 *  Each component is stored in *symbolic* form: i.e. a tree of AMPL
 *  expressions for the body of the component definition and a tree of AMPL
 *  expressions for the indexing expression. It does not know about the
 *  cardinality of each component (it does not expand indexing expressions)
 *  It keeps track of both the number of every type
 *  registered and a linked list of entries describing each of the 
 *  entities in more detail. 
 */
class AmplModel{
 public:
  
  /** hash table of entries in this model. The symb_entry encodes name and
   *  type of the model component. 
   *  @attention this does not seem to be ever used to lookup model comonents
   *             by name.
   *  @attention should have a global hash table of *all* defined model 
   *             components. Could be used in find_var_ref_in_context which
   *             does the job of finding the ModelComponent object reference
   *             for components refered to in expressions.
   *             => Need a way to only look for a match in the current part
   *                of the model tree.
   */
  SymbolTable symbol_table;

  /** name of the block defining this (sub)model */
  char *name;          

  /** name with ancenstors name prepended (excluding root) */
  string global_name;  

  int n_vars;      //!< number of variable declarations 
  int n_cons;      //!< number of constraint declarations 
  int n_params;    //!< number of parameter declarations 
  int n_sets;      //!< number of set declarations 
  int n_objs;      //!< number of objective declarations 
  int n_submodels; //!< number of submodel/block declarations
  int n_total;     //!< total number of declarations
  int level;       //!< level of this model on the flat model tree (root=0)

  /** the ModelComp node correspding to this model 
   * (this is defined if this is not root) */
  ModelComp *node; 
		       

  /** The list of components of this model 
   */
  list<ModelComp*> comps;

  AmplModel *parent; /**< the parent if this is a submodel of another model */
  // all models except root might have an indexing expression:
  // block name{i in SET}:
  // this is taken apart and stored in the next two variables
  SyntaxNodeIx *ix;       ///< indexing expression
    
  /** list of changes that should be applied to the models */
  static list<changeitem*> changes;

  /** the root model of the AmplModel tree */
  static AmplModel *root;

  // -------------------------- methods ----------------------------------
  /** Constructor */
  AmplModel();
  AmplModel(const char *orig_name, AmplModel *par=NULL);
  
  /** Destructor */
  virtual ~AmplModel();

  /** set global name by concatenating ancestor names */
  void setGlobalName();      

  /** set global name recursively for this and all submodels */
  void setGlobalNameRecursive();      
                                //
  /** recursively write all out all tagged model components in this model and 
   * submodels to file          */
  void writeTaggedComponents(ostream &fout=cout);  
                                
  /** Recursively creates an ExpandedModel tree from the flat AmplModel */
  ExpandedModel* createExpandedModel(string smodelname, string sinstanceStub);

  /** add dummy objective that uses (sums up) all variables in the model */
  void addDummyObjective();

  /** add a model component to the model */
  virtual void addComp(ModelComp *comp);

  /** remove a model component from the model */
  void removeComp(ModelComp *comp);

  /** recursively recalculate dependency list and re-resolve IDREF nodes */
  void reassignDependencies();

  void print();    //< prints debugging output recursively
  void check();    //< checks instance for consistency

  /** recursive detailed debugging output */
  void dump(char *filename);

  /** recursive detailed debugging output */
  void dump(ostream &fout);

  static void applyChanges(); //< apply the model changes stored in Q

  SymbolTable::Entry *findComponent(string id);
};



#endif
