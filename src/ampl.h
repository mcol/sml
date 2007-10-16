#ifndef AMPL_H
#define AMPL_H

#include "model_comp.h"
#include "nodes.h"
#include "symtab.h"
#include "ExpandedModel.h"
#include <string>
/* This is an item that is part of an AMPL model. It can take information
   about one Set/Variable/Constraint/Parameter, etc definition */

class AmplModel;
class model_comp;

opNode *findKeywordinTree(opNode *root, int oc);
int countKeywordinTree(opNode *root, int oc);
void findIDinTreeAndAddToList(opNode *node, char  **list, int *nlist);
opNode* find_var_ref_in_context(AmplModel *context, opNode *ref);
opNode* find_var_ref_in_context_(AmplModel *context, opNode *ref);
opNode *find_var_ref_in_indexing(char *name);
void add_indexing(opNodeIx *indexing);
void rem_indexing(opNodeIx *indexing);


/* ------------------------------------------------------------------------ */
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
   *             does the job of finding the model_component object reference
   *             for components refered to in expressions.
   *             => Need a way to only look for a match in the current part
   *                of the model tree.
   */
  symb_entry **symb_hash_table;

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

  /** the model_comp node correspding to this model 
   * (this is defined if this is not root) */
  model_comp *node; 
		       

  /* how do we lift single members of the collection to models
     higher up ? */
  
  // FIXME: this should be implemented using a list
  model_comp *first; /**< pointer to the first model component */
  model_comp *last;  /**< pointer to the last model component */
  //model_comp *vars;
  //model_comp *cons;
  //model_comp *objs;
  //model_comp *sets;
  //model_comp *params;

  AmplModel *parent; /**< the parent if this is a submodel of another model */
  // all models except root might have an indexing expression:
  // block name{i in SET}:
  // this is taken apart and stored in the next two variables
  opNodeIx *ix;       ///< indexing expression

  // -------------------------- methods ----------------------------------
  /** set global name by concatenating ancestor names */
  void setGlobalName();      

  /** recursively write all out all tagged model components in this model and 
   * submodels.          */
  void writeTaggedComponents(); 
                                //
  /** recursively write all out all tagged model components in this model and 
   * submodels to file          */
  void writeTaggedComponents(FILE *fout);  
                                
  /** Recursively creates an ExpandedModel tree from the flat AmplModel */
  ExpandedModel* createExpandedModel(string smodelname, string sinstanceStub);

  void print();    //< prints debugging output recursively
};


/* ----------------- stub for methods in ampl.y ------------------------- */

void begin_model(char *name, opNode *indexing);
void end_model();
void add_set_to_model(char *id, opNode *indx, opNode *attrib);
void add_obj_to_model(int token, char *id, opNode *indx, opNode *attrib);
void addCompToModel(AmplModel *model, model_comp *comp);


#endif
