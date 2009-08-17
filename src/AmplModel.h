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
#ifndef AMPLMODEL_H
#define AMPLMODEL_H

#include "ModelComp.h"
#include "nodes.h"
#include "symtab.h"
#include "ExpandedModel.h"
#include <string>

enum {CHANGE_NOACT=0,CHANGE_REM=1,CHANGE_ADD=2};

/** @class changeitem
 *  Simple struct that stores a queued change to the model tree.
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
 *  This class describes a model (block) in the flat model tree.
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
  
  /** Hash table of entries in this model. The symb_entry encodes name and
   *  type of the model component. 
   *
   *  @attention This does not seem to be ever used to lookup model components
   *  by name.
   *
   *  @attention Should have a global hash table of *all* defined model
   *  components. Could be used in find_var_ref_in_context which does the job
   *  of finding the ModelComponent object reference for components refered to
   *  in expressions.
   *             => Need a way to only look for a match in the current part
   *                of the model tree.
   */
  SymbolTable symbol_table;

  /** Name of the block defining this (sub)model */
  char *name;          

  /** Name with ancestors name prepended (excluding root) */
  string global_name;  

  int n_vars;      //!< number of variable declarations 
  int n_cons;      //!< number of constraint declarations 
  int n_params;    //!< number of parameter declarations 
  int n_sets;      //!< number of set declarations 
  int n_objs;      //!< number of objective declarations 
  int n_submodels; //!< number of submodel/block declarations
  int n_total;     //!< total number of declarations
  int level;       //!< level of this model on the flat model tree (root=0)

  /** The ModelComp node corresponding to this model (defined if this is not
   *  root) */
  ModelComp *node; 

  /** The list of components of this model */
  list<ModelComp*> comps;

  /** The parent if this is a submodel of another model */
  AmplModel *parent;

  /** Indexing expression.
   *
   *  All models except root might have an indexing expression:
   *  block name{i in SET}.
   */
  SyntaxNodeIx *ix;
    
  /** List of changes that should be applied to the models */
  static list<changeitem*> changes;

  /** The root model of the AmplModel tree */
  static AmplModel *root;

  // -------------------------- methods ----------------------------------
  /** Constructor */
  AmplModel();
  AmplModel(const char *orig_name, AmplModel *par=NULL);
  
  /** Destructor */
  virtual ~AmplModel();

  /** Set the global name by concatenating ancestor names */
  void setGlobalName();      

  /** Set the global name recursively for this and all submodels */
  void setGlobalNameRecursive();      

  /** Recursively write out all tagged model components in this model and 
      submodels to file */
  void writeTaggedComponents(ostream &fout=cout);  
                                
  /** Recursively creates an ExpandedModel tree from the flat AmplModel */
  ExpandedModel* createExpandedModel(string smodelname, string sinstanceStub);

  /** Add a dummy objective that uses (sums up) all variables in the model */
  void addDummyObjective();

  /** Add a model component to the model */
  virtual void addComp(ModelComp *comp);

  /** Remove a model component from the model */
  void removeComp(ModelComp *comp);

  /** Recursively recalculate dependency list and re-resolve IDREF nodes */
  void reassignDependencies();

  void print();    //< prints debugging output recursively
  void check();    //< checks instance for consistency

  /** Recursive detailed debugging output */
  void dump(const char *filename);

  /** Recursive detailed debugging output */
  void dump(ostream &fout);

  static void applyChanges(); //< apply the model changes stored in Q

  SymbolTable::Entry *findComponent(string id);
  list<SymbolTable::Entry> getObjList() const;

  virtual SyntaxNodeIDREF* find_var_ref_in_context(IDNode *ref);
};

#endif
