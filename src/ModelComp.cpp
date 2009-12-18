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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "ModelComp.h"
#include "backend.h"
#include "GlobalVariables.h" //for GlobalVariables class
#include "AmplModel.h"

static bool prtAnaDep = false;

int ModelComp::tt_count=0;  // initialise static class member

const string ModelComp::nameTypes[] = {
   "variable","constraint","parameter",
   "set", "objective min","objective max", 
   "submodel"};
const string ModelComp::compTypes[] = {
   "var","subject to","param",
   "set", "minimize","maximize","block"};

extern void modified_write(ostream &fout, ModelComp *comp);

/* This should be an IDREF (or IDREFM) node that needs to be converted
   into its global name 
   
   The node is a pointer to a ModelComp structure. Need to work out
   which (if any) blocks it belongs to and pre-pend the name of any block to
   the global name

   Also need to work out which dummy variables need to be put on the
   argument list

   IN: ModelComp *node          : the model comp of which the name should
                                   be obtained
       int        witharg        : 1 if argument list should be printed 
       SyntaxNode     *opn           : the IDREF node that should be named
       AmplModel *current_model : the model in which this is referenced

   arguments opn, current_model are only needed if the argument list should
   be printed as well

   opn is the IDREF(M) node of the object that should be named. All the
   subscripts that are used for this in the model description are
   part of the 'opn' node. To get the complete global argument list, these
   subscripts need to be prefixed by the ones corresponding to the
   model from which this object is referenced
*/

list<ModelComp*> ModelComp::global_list;

/** Construct a model component given its name, id, indexing and attribute
 *  sections.
 *  Also analyses dependencies in indexing and attribute and set the 
 *  dependencies list
 *  @param id          name of the component
 *  @param type        type of the component
 *  @param indexing    root node of the indexing expression. 
 *                     IDs should have been replaced by IDREFs 
 *  @param attrib      root node of the attribute expression.
 *                     IDs should have been replaced by IDREFs 
 */
ModelComp::ModelComp(const string& id, compType type,
                       SyntaxNode *indexing, SyntaxNode *attrib)
{
  value = NULL;
  this->tag = false;
  this->id = id;
  this->type = type;
  this->indexing = dynamic_cast<SyntaxNodeIx*>(indexing);
  if (indexing) (this->indexing)->splitExpression();
  this->attributes = attrib;

  this->count = ModelComp::tt_count++;
  if (GlobalVariables::prtLvl>0) 
    cout << "Defining model component (" << this->count << "): " << id << "\n";

  /* now set up the dependency list for the component */
  setUpDependencies();

  global_list.push_back(this);
}

ModelComp::~ModelComp() {

}

/* --------------------------------------------------------------------------
ModelComp::setUpDependencies()
---------------------------------------------------------------------------- */
void
ModelComp::setUpDependencies()
{
  dependencies.clear();
  /* now set up the dependency list for this component */
  if (prtAnaDep){
    cout << "Analyse dependencies for " << id << "\n"
         << " dependencies in indexing: \n";
  }
  if (indexing) {
    // get list of IDREF nodes in 'indexing'
    list<ModelComp*> lmc;
    indexing->findIDREF(lmc);
    for( list<ModelComp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<ModelComp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (attributes){
    if (prtAnaDep)
       cout << " dependencies in attributes: " << *attributes << "\n";
    //attrib->findIDREF();
    list<ModelComp*> lmc;
    attributes->findIDREF(lmc);
    // lmc should be a list of model components
    // how do I iterate through it?
    for( list<ModelComp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<ModelComp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (prtAnaDep){
    for( list<ModelComp*>::iterator p=dependencies.begin(); 
         p!=dependencies.end(); ++p)
      cout << "  " << (*p)->id << "\n";
    printf("--------------------------------\n");
  }
}

/* --------------------------------------------------------------------------
ModelComp::ModelComp()
---------------------------------------------------------------------------- */
/** Default constructor: just sets all fields to -1/NULL/false               */
ModelComp::ModelComp() :
  type(TNOTYPE),
  id(""),
  attributes(NULL),
  indexing(NULL),
  model(NULL),
  other(NULL),
  count(-1),
  tag(false),
  value(NULL) { }

/* --------------------------------------------------------------------------
ModelComp::setTo()
---------------------------------------------------------------------------- */
/** Set a model component to a given name, id, indexing and attribute
 *  sections.
 *  Also analyses dependencies in indexing and attribute and set the 
 *  dependencies list
 *  @param id          name of the component
 *  @param type        type of the component
 *  @param indexing    root node of the indexing expression. 
 *                     IDs should have been replaced by IDREFs 
 *  @param attrib      root node of the attribute expression.
 *                     IDs should have been replaced by IDREFs 
 */
void
ModelComp::setTo(char *id, compType type, 
                       SyntaxNodeIx *indexing, SyntaxNode *attrib)
{
  static int tt_count=0;
  this->tag = false;
  this->id = id;
  this->type = type;
  this->indexing = indexing;
  if (indexing) (this->indexing)->splitExpression();
  this->attributes = attrib;

  /* now set up the dependency list for the component */
  //printf("Defining model component (%4d): %s\n",tt_count, id);
  this->count = tt_count++;
  if (prtAnaDep) printf(" dependencies in indexing: \n");
  if (indexing) {
    list<ModelComp*> lmc;
    indexing->findIDREF(lmc);
    for( list<ModelComp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<ModelComp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (attrib){
    if (prtAnaDep) 
      printf(" dependencies in attributes: %s\n", attrib->print().c_str());
    //attrib->findIDREF();
    list<ModelComp*> lmc;
    attrib->findIDREF(lmc);
    // lmc should be a list of model components
    for( list<ModelComp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<ModelComp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (prtAnaDep){
    for( list<ModelComp*>::iterator p=dependencies.begin(); 
         p!=dependencies.end(); ++p)
      cout << "  " << (*p)->id << "\n";
    printf("--------------------------------\n");
  }

  global_list.push_back(this);
}

/* ---------------------------------------------------------------------------
ModelComp::untagAll()
---------------------------------------------------------------------------- */
/** Set tag=false for all model components */
void 
ModelComp::untagAll()
{
  // iterate through the global list
  for (list<ModelComp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    (*p)->tag = false;
  }
}
/* ---------------------------------------------------------------------------
ModelComp::untagAll(AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively set tag=false for all model components.
 *
 *  @param start
 *         The AmplModel where to start the recursion.
 */
void 
ModelComp::untagAll(AmplModel *start)
{
  // iterate through the local list
  for (list<ModelComp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    (*p)->tag = false;
    if ((*p)->type==TMODEL){
      ModelComp::untagAll((AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
ModelComp::writeAllTagged(AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively write out a list of all model components that have the tag set.
 *
 *  @param start
 *         The AmplModel where to start the recursion.
 */
void 
ModelComp::writeAllTagged(AmplModel *start)
{
  // iterate through the global list
  for (list<ModelComp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    if ((*p)->tag) {
      cout << start->name << "::" << (*p)->id << "\n";
    }
    if ((*p)->type==TMODEL){
      ModelComp::writeAllTagged((AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
ModelComp::modifiedWriteAllTagged()
---------------------------------------------------------------------------- */
/** Recursively write name of all tagged components.
 *
 *  Write out a list of all model components that have the tag set: write
 *  every component how it would appear in the global model file.
 *
 *  @bug
 *  modified_write should be called within the model writing process:
 *  it depends on addIndex/l_addIndex, i.e. some indexing expressions (and
 *  subbmodel names) should be added to entity names depending on where in
 *  the model it is called.
 */
void 
ModelComp::modifiedWriteAllTagged(ostream &fout)
{
  // iterate through the global list
  for (list<ModelComp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    if ((*p)->tag) {
      modified_write(fout, *p);
    }
  }
}

/* ---------------------------------------------------------------------------
ModelComp::print()
---------------------------------------------------------------------------- */
/** Print a detailed diagnostic description of this model component
 *  with the values of all its fields                                        */
void
ModelComp::print()
{
  cout << "------------------------------------------------------------\n";
  cout << "ModelComp: " << id << "\n";
  cout << "  type: " << ModelComp::nameTypes[type] << "\n";
  cout << "  attributes: " << *attributes << "\n";
  cout << "  indexing: " << *indexing << "\n";
  if (indexing) indexing->printDiagnostic(cout);
  cout << "  dependencies: " << dependencies.size() << ":\n";
  cout << "      ";
  for(list<ModelComp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++)
    cout << (*p)->model->name << "::" << (*p)->id << " ";
  cout << "  model: " << model->name<< "\n";
  cout << "  count: " << count << "\n";
  cout << "  tag: " << tag << "\n";
}

/* ---------------------------------------------------------------------------
ModelComp::dump(ostream &fout)
---------------------------------------------------------------------------- */
/** Print a detailed diagnostic description of this model component
 *  with the values of all its fields                                        */
void
ModelComp::dump(ostream &fout)
{
  fout << "MCDP  --------------------------------------------------------"
     "----\n";
  fout << "MCDP ModelComp: " << id << " ("<< (void *) this << ")\n";
  fout << "MCDP  type: " << ModelComp::nameTypes[type] << "\n";
  fout << "MCDP  attributes: " << attributes << "\n";
  if (attributes) attributes->dump(fout);
  fout << "MCDP  indexing: " << indexing << "\n";
  if (indexing) indexing->printDiagnostic(fout);
  if (indexing) indexing->dump(fout);
  fout << "MCDP  dependencies: " << dependencies.size() << ":\n";
  fout << "      ";
  for(list<ModelComp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++)
    fout << (*p)->model->name << "::" << (*p)->id << " ";
  fout << "\nMCDP  model: " << model->name << "\n";
  fout << "MCDP  count: " << count << "\n";
  fout << "MCDP  tag: " << tag << "\n";
  if (value) {
    fout << "MCDP  value: " << value->toString() << "\n";
  }
}

/* ---------------------------------------------------------------------------
ModelComp::printBrief()
---------------------------------------------------------------------------- */
/** Print a one line description of the object: type and name */
void
ModelComp::printBrief()
{
  cout << ModelComp::nameTypes[type] << " " << id << endl;
}
/* ---------------------------------------------------------------------------
ModelComp::tagDependencies()
---------------------------------------------------------------------------- */
/** Tag this components and all its dependencies recursively.
 *
 *  Recursively set tag=true for this model component and all components that
 *  it depends on (i.e. everything listed in its dependency list.
 */
void
ModelComp::tagDependencies()
{
  this->tag = true;
  for(list<ModelComp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++){
    (*p)->tagDependencies();
  }
}

/* ---------------------------------------------------------------------------
ModelComp::deep_copy()
---------------------------------------------------------------------------- */
/** Create a deep-copy of the ModelComp object.
 *
 *  The tree of attributes and indexing expressions is recreated using 
 *  entirely new objects.
 */
ModelComp *
ModelComp::deep_copy()
{
  ModelComp *newm = new ModelComp();

  newm->type = type;
  newm->id = id;
  if (attributes) newm->attributes = attributes->deep_copy();
  if (indexing) newm->indexing = indexing->deep_copy();
  newm->dependencies = dependencies;
  newm->model = model;
  newm->other = other;
  newm->count = count;
  newm->tag = tag;

  return newm;
}
/* ---------------------------------------------------------------------------
ModelComp::clone()
---------------------------------------------------------------------------- */
/** Create a shallow copy of the object: only the top level object is 
 *  copied, pointers below are reused 
 */
ModelComp *
ModelComp::clone()
{
  ModelComp *newm = new ModelComp();

  newm->type = type;
  newm->id = id;
  newm->attributes = attributes;
  newm->indexing = indexing;
  newm->dependencies = dependencies;
  newm->model = model;
  newm->other = other;
  newm->count = count;
  newm->tag = tag;

  return newm;
}

/* ---------------------------------------------------------------------------
getGlobalName
---------------------------------------------------------------------------- */
/** Find the global name of the model component pointed to by 'node':
 * - Generate the global name of a model component by pre-pending the names of 
 *   models on the model-tree up to the model of this component to the name
 *    'Flow' becomes 'MCNF_Net_Flow'  
 *
 * - If 'witharg' is set, then the argument list is also generated:
 *   The argument list is composed of 
 *    + dummy variables of indexing expressions of block up to model_of_comp
 *    + original arguments of the component (as given in the SML file)
 *   The appropriate argument list depends on both the model of the component
 *   and in which model this instance of referal of the ModelComp is 
 *   (the current_model)
 *   Basically we need to identify the common ancestor model of the 
 *   current_model and the model_of_comp. The arguments originating from 
 *   block indexing expressions between here and the model_of_comp are already 
 *   included in the argument list of the ModelComp. Anything below needs to
 *   be prepended to the argument list
 *
 *  @param[in] node
 *             The model component in question.
 *  @param[in] opn
 *             The node (IDREF) of the model component (needed for the (local)
 *             argument list).
 *  @param[in] current_model
 *             The block for which this is written: indexing is given in the
 *             original SML model wrt a given node in the model tree.
 *      FIXME: what happens if the component referenced in the definition is
 *             not in the same model_tree node as the component to be defined?
 *             In the original ampl file this is correct, since the indexing
 *             will be given relative to the current_model. 
 *             However the local indexing is lost(?) in the node representation
 *             => I don't think so, it is still encoded in the rest of the
 *                SyntaxNode structure
 *  @param[in] witharg
 *             WITHARG: if the argument list should be processed,
 *             NOARG:   only the global name,
 *             ONLYARG: only the argument list.
 *
 *  @pre l_addIndex needs to be set. It is assumed that this is a stack of
 *       indexing expressions from the root at least to the common ancestor
 *        node (likely set up to the current_model).
 */
string
getGlobalName(ModelComp *node, const SyntaxNode *opn, AmplModel *current_model, 
              int witharg)
{
  AmplModel *model_of_comp = node->model;/* this is the model it belongs to */
  AmplModel *tmp;
  string globalName;
  string arglist;
  int n_index = 0;
  int i;

  globalName = "";
  /* need to get list of model names and argument list to prefix */
  if (witharg==NOARG||witharg==WITHARG){
    globalName = node->id;
    tmp = model_of_comp;
    while (tmp->name != "root") {
      /* work on name */
      globalName = tmp->name + ("_" + globalName);
      
      if (tmp->parent==NULL) {
        cerr << "has no parent >" << tmp->name << "<\n";
        exit(1);
      }
      tmp = tmp->parent;
    }
  }
  
  if (witharg==NOARG)
    return globalName;

  /* FIXME: still need to add the argument list, every level down from root
            should have put a indexing expression on the l_addIndex list
            use the dummyVar parts from this
            then add the indexing that comes with this node 

      The argument list is still a bit tricky. 
      - We are currently in model 'current_model' 
        (this is the block in the original ampl file that we are
        currently processing.  That is in the original ampl-file all
        references to objects are given with respect to this model)
      - the object whose name we are currently printing lives in model
        'model_of_comp'
      => we need to go down to the first common ancestor of these two models.
         all indices from there down (in direction of the leaves) are 
         included with the model-component, all indices from there
         up (in direction of root) need to be taken of the 
         addIndex stack. Not quite clear where to start taking 
         bits of the addIndex stack, probably go all the way down to root and 
         then back up
  */

  // ---- find the common ancestor of current_model and model of component ---
  {
    AmplModel *path1[5], *path2[5];
    int n_path1, n_path2;
    int clvl;

    n_path1=1;n_path2=1;
    tmp = current_model;
    path1[0] = tmp;
    while(tmp->parent) {
      tmp = tmp->parent;
      path1[n_path1] = tmp;
      n_path1++;
    }
    tmp = model_of_comp;
    path2[0] = tmp;
    while(tmp->parent) {
      tmp = tmp->parent;
      path2[n_path2] = tmp;
      n_path2++;
    }
    /* okay the two paths are build, let's print them */
    //printf("Path to current_model: %s:\n",current_model->name);
    //for(i=0;i<n_path1;i++)  printf("%d %s\n",i, path1[n_path1-1-i]->name);
    //printf("Path to model_of_comp: %s:\n",model_of_comp->name);
    //for(i=0;i<n_path2;i++)  printf("%d %s\n",i, path2[n_path2-1-i]->name);
    
    /* now go and find the last common node: 
       path[n_path-1] should be root in both cases */
    clvl = 0;
    while (clvl<n_path1-1 && clvl<n_path2-1 && path1[n_path1-2-clvl]==path2[n_path2-2-clvl]) clvl++;
    /* okay common ancestor should be path[n_path-1-i] */
    tmp = path1[n_path1-1-clvl];
    //printf("Common ancestor is %s\n",tmp->name);
    //printf("which is on level %d (0 is root)\n",clvl);

    // => tmp is now set to the common ancestor of current_model and model of 
    //        comp

    
    /* for every level above 0 there should be a dummy variable on the stack 
       => go up the path and add dummy variables to arglist */
    for(i=0;i<clvl;i++){
      /* FIXME: here the dv cannot be printed by SyntaxNode.print() if it is 
         a list like (i,j)! */
      list <add_index *> *li = l_addIndex.at(i);
      for(list<add_index*>::iterator p = li->begin();p!=li->end();p++){
        if (n_index==0){
          //SyntaxNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
          SyntaxNode *dv = (*p)->dummyVar;
          if (dv) {
            arglist = dv->printDummyVar();
            //printf("add dummy variable: %s\n",print_SyntaxNode(dv));
            n_index++;
          }
        }else{
          /* need to put subscript before the current list */
          //SyntaxNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
          SyntaxNode *dv = (*p)->dummyVar;
          if (dv){
            arglist += ",";
            arglist += dv->printDummyVar();
            //printf("add dummy variable: %s\n",print_SyntaxNode(dv));
            n_index++;
          }
        }
      }
    }
  }

  /* work on the argument list */
  /* opn->nval is the number of arguments (at this level),
     the arguments follow in positions values[1] - values[nval]
     
     We also need to prefix this with the dummy arguments corresponding
     to the blocks that this entity is in */

  /* concatenate argument list from dummy variables on l_addIndex stack
     with those that belong to the component:
     - n_index   come from the l_addIndex stack
     - opn->nval come from the component
     if both are present we need to put a comma in beween 
  */

  if (opn->nchild()+n_index>0){
    if (n_index==0){
      arglist = opn->getArgumentList();
    }else{
      if (opn->nchild()==0){
        // arglistbuffer stays as it is
      }else{
        arglist += ",";
        arglist += opn->getArgumentList();
      }
    }
    
    globalName += "[" + arglist + "]";
  }

  return globalName;
}

/* FIXME: this is a stub for getGlobalNameNew, a version of getGlobalName
 that does not need the addIndex stack, but derives the added indexing
 from the indexing of the submodels in the tree up to this component
 This should be used in the rendering of expectation constraints

 I don't think this is going to work though: components might be refered to
 from below the model in which they are defined in which case some of the
 indexing is already part of the SyntaxNode structure (and different from just 
 using the submodel indexing)

 Could get around this by making indexing dependend on the
  - model to which the comp belongs 
    (to be able to follow down submodels to get indexing expressions)
  - model from which the component was refered to 
    (indexing expressions between these two models are already part of
    the SyntaxNode structure)
 Indeed this is done in the old getGlobalName method.

 Latest thought (01/04/08:11:50) is that this *might* indeed work
*/

/* ---------------------------------------------------------------------------
getGlobalNameNew(ModelComp *node, SyntaxNode *opn, AmplModel *current_model, 
              int witharg)
---------------------------------------------------------------------------- */
/** New version of getGlobalName that does *not* use the addIndex stack
 *  but creates the modified argument list by looking at the indexing
 *  expressions of the submodel tree leading to this ModelComp
 *
 *  @param[in] node
 *             The model component in question.
 *  @param[in] opn
 *             The node (IDREF) of the model component (needed for the (local)
 *             argument list).
 *  @param[in] current_model
 *             The block for which this is written: indexing is given in the
 *             original SML model wrt a given node in the model tree.
 *  @param[in] witharg
 *             WITHARG: if the argument list should be processed,
 *             NOARG:   only the global name,
 *             ONLYARG: only the argument list.
 *
 *  Find the global name of the model component pointed to by 'node':
 * - Generate the global name of a model component by pre-pending the names of 
 *   models on the model-tree up to the model of this component to the name
 *    'Flow' becomes 'MCNF_Net_Flow'  
 *
 * - If 'witharg' is set, then the argument list is also generated:
 *   The argument list is composed of 
 *    + dummy variables of indexing expressions of block up to model_of_comp
 *    + original arguments of the component (as given in the SML file)
 *   The appropriate argument list depends on both the model of the component
 *   and in which model this instance of referal of the ModelComp is 
 *   (the current_model)
 *   Basically we need to identify the common ancestor model of the 
 *   current_model and the model_of_comp. The arguments originating from 
 *   block indexing expressions between here and the model_of_comp are already 
 *   included in the argument list of the ModelComp. Anything below needs to
 *   be prepended to the argument list
 */
string
getGlobalNameNew(ModelComp *node, const SyntaxNode *opn, AmplModel *current_model, 
              int witharg)
{
  AmplModel *model_of_comp = node->model;/* this is the model it belongs to */
  AmplModel *tmp;
  string arglist;
  string globalName;
  int n_index = 0;
  int i;

  globalName = "";
  /* need to get list of model names and argument list to prefix */
  if (witharg==NOARG||witharg==WITHARG){
    globalName = node->id;
    tmp = model_of_comp;
    while (tmp->name != "root") {
      /* work on name */
      globalName = tmp->name + ("_" + globalName);
      
      if (tmp->parent==NULL) {
        cerr << "has no parent >" << tmp->name << "<\n";
        exit(1);
      }
      tmp = tmp->parent;
    }
  }
  
  if (witharg==NOARG)
    return globalName;

  /* FIXME: still need to add the argument list, every level down from root
            should have put a indexing expression on the l_addIndex list
            use the dummyVar parts from this
            then add the indexing that comes with this node 

      The argument list is still a bit tricky. 
      - We are currently in model 'current_model' 
        (this is the block in the original ampl file that we are
        currently processing.  That is in the original ampl-file all
        references to objects are given with respect to this model)
      - the object whose name we are currently printing lives in model
        'model_of_comp'
      => we need to go down to the first common ancestor of these two models.
         all indices from there down (in direction of the leaves) are 
         included with the model-component, all indices from there
         up (in direction of root) need to be taken of the 
         addIndex stack. Not quite clear where to start taking 
         bits of the addIndex stack, probably go all the way down to root and 
         then back up
  */

  // ---- find the common ancestor of current_model and model of component ---
  {
    AmplModel *path1[5], *path2[5];
    int n_path1, n_path2;
    int clvl;

    n_path1=1;n_path2=1;
    tmp = current_model;
    path1[0] = tmp;
    while(tmp->parent) {
      tmp = tmp->parent;
      path1[n_path1] = tmp;
      n_path1++;
    }
    tmp = model_of_comp;
    path2[0] = tmp;
    while(tmp->parent) {
      tmp = tmp->parent;
      path2[n_path2] = tmp;
      n_path2++;
    }
    /* okay the two paths are build, let's print them */
    //printf("Path to current_model: %s:\n",current_model->name);
    //for(i=0;i<n_path1;i++)  printf("%d %s\n",i, path1[n_path1-1-i]->name);
    //printf("Path to model_of_comp: %s:\n",model_of_comp->name);
    //for(i=0;i<n_path2;i++)  printf("%d %s\n",i, path2[n_path2-1-i]->name);
    
    /* now go and find the last common node: 
       path[n_path-1] should be root in both cases */
    clvl = 0;
    while (clvl<n_path1-1 && clvl<n_path2-1 && path1[n_path1-2-clvl]==path2[n_path2-2-clvl]) clvl++;
    /* okay common ancestor should be path[n_path-1-i] */
    tmp = path1[n_path1-1-clvl];
    //printf("Common ancestor is %s\n",tmp->name);
    //printf("which is on level %d (0 is root)\n",clvl);

    // => tmp is now set to the common ancestor of current_model and model of 
    //        comp

    
    /* for every level above 0 there should be a dummy variable on the stack 
       => go up the path and add dummy variables to arglist */
    for(i=1;i<=clvl;i++){ //start from 1: 0 is the root which has no indexing
      /* FIXME: here the dv cannot be printed by SyntaxNode.print() if it is 
         a list like (i,j)! */
      ModelComp *node_model = path1[n_path1-1-i]->node;
      SyntaxNodeIx *indexing_model = node_model->indexing;
      
      if (indexing_model){
        for(int p=0;p<indexing_model->ncomp;p++){
          if (n_index==0){
            //SyntaxNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
            SyntaxNode *dv = indexing_model->dummyVarExpr[p];
            if (dv) {
              arglist = dv->printDummyVar();
              //printf("add dummy variable: %s\n",print_SyntaxNode(dv));
              n_index++;
            }
          }else{
            /* need to put subscript before the current list */
            //SyntaxNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
            SyntaxNode *dv = indexing_model->dummyVarExpr[p];
            if (dv){
              arglist += ",";
              arglist += dv->printDummyVar();
              //printf("add dummy variable: %s\n",print_SyntaxNode(dv));
              n_index++;
            }
          }
        }
      }
    }
  }
  

  /* work on the argument list */
  /* opn->nval is the number of arguments (at this level),
     the arguments follow in positions values[1] - values[nval]
     
     We also need to prefix this with the dummy arguments corresponding
     to the blocks that this entity is in */

  /* concatenate argument list from dummy variables on l_addIndex stack
     with those that belong to the component:
     - n_index   come from the l_addIndex stack
     - opn->nval come from the component
     if both are present we need to put a comma in beween 
  */

  if (opn->nchild()+n_index>0){
    if (n_index==0){
      arglist = opn->getArgumentList();
    }else{
      if (opn->nchild()==0){
        // arglistbuffer stays as it is
      }else{
        arglist += ",";
        arglist += opn->getArgumentList();
      }
    }
    
    globalName += "[" + arglist + "]";
  }

  return globalName;
}

/* --------------------------------------------------------------------------
ModelComp::moveUp(int level)
---------------------------------------------------------------------------- */
/** Queue the ModelComp to be moved up by 'level' levels in the model tree:
 *  Just removing the component from the current model and adding it to a
 *  parent is dangerous, since ModelComp::moveUp is typically called from
 *  within a (nested) loop over all ModelComps (->comps) in the AmplModels
 *  removing/adding items to list<ModelComp*> comps while there is an
 *  iterator running over it will invalidate that iterator.
 *  => hence the request to move is scheduled to be executed by
 *     AmplModel::applyChanges() after the loop over all components
 *
 *  This method will also re-write the component for the new model
 *  I.e. all IDREFs to components below the new model will have their
 *  local indexing expression expanded
 */
void
ModelComp::moveUp(int level){
  AmplModel *current = model;
  int i, posm;
  
  // -------------------- Expand local indexing expression -----------------
  /* This ModelComp is written for the 'current' model and is now re-asigned
     to a different model. In order for that to work the indexing expressions
     in all IDREFs in its attribute/indexing section have got to be
     rewritten

     Indexing expressions applicable to a IDREF are divided into local and 
     block indexing. 'local' is directly associated with the IDREF (as
     arguments in ->values[]. 'block' originate from the indexing expressions
     of the blocks up to the current model in the model tree. Both indexing 
     expression together need to combine to get the correct global indexing

     When moving a ModelComp up in the tree, we therefore need to do the
     following to have correct global indexing:
       - all IDREFs to ModelComp's in models below the new model 
         (current.parent(level)) need to have the block indexing expressions
         between their own model and current.parent(level) added
         to their local indexing
  */

  // get list of models from current model to root
  vector<AmplModel*> mlist;

  for(AmplModel *tmp=current;tmp->parent!=NULL;tmp=tmp->parent){
    mlist.push_back(tmp);
  }

  list<SyntaxNode*> *idrefnodes = new list<SyntaxNode*>;

  // get list of all IDREF nodes in dependencies
  if (indexing) indexing->findIDREF(idrefnodes); 
  if (attributes) attributes->findIDREF(idrefnodes);

  // loop over all IDREF nodes
  for(list<SyntaxNode*>::iterator p = idrefnodes->begin();
      p!=idrefnodes->end();p++){
    SyntaxNodeIDREF *onidr = dynamic_cast<SyntaxNodeIDREF*>(*p);
    ModelComp *mc = onidr->ref;
    AmplModel *model = mc->model;
    
    // need to check if this model is below the new assigned model
    bool found = false;
    for(posm=0;posm<level;posm++){
      if (mlist[posm]==model){
        found = true;
        break;
      }
    }
    if (found){
      // this is a model between the old and new model for ModelComp *this
      // posm gives the position: 0 is the old model, level is the new
      // one
      // => need to add indexing expressions between posm and level-1
      // starting with level-1
      for(i=posm; i<level; ++i){
        SyntaxNodeIx *mix = mlist[i]->ix;
        if (mix->ncomp!=1){
          cerr << "ModelComp::moveUp() does not support intermediate models "
            "with !=1 dummy Var" << endl;
          exit(1);
        }
        onidr->push_front(mix->dummyVarExpr[0]);
        /* indexing dummy var of mlist[level-1-i]*/
      }
    }
  }

  // and queue this item to be moved up by AmplModel::applyChanges 
  changeitem *rem = (changeitem*)calloc(1, sizeof(changeitem));
  changeitem *add = (changeitem*)calloc(1, sizeof(changeitem));
  rem->comp = this;
  rem->model = model;
  rem->action = CHANGE_REM;
  AmplModel::changes.push_back(rem); // Q for removal
  for(i=0;i<level;i++){
    model = model->parent;
  }
  add->comp = this;
  add->model = model;
  add->action = CHANGE_ADD;
  AmplModel::changes.push_back(add);
}

/* --------------------------------------------------------------------------
ModelComp::reassignDependencies()
---------------------------------------------------------------------------- */
/** Recalculate dependency list and re-resolve IDREF nodes.
 *
 *  In the process of building the AmplModel tree from the StochModelTree
 *  some of the IDREF dependency nodes still point to the StochModelComp
 *  nodes from the StochModel tree (or the intermediate tree).
 *
 *  This routine makes sure that IDREF nodes are resolved with respect to the 
 *  correct ModelComp and rebuilds the dependency lists.
 */
void
ModelComp::reassignDependencies()
{
  list<SyntaxNode*> *idrefnodes = new list<SyntaxNode*>;
  list<ModelComp*> newdep;

  if (indexing) indexing->findIDREF(idrefnodes);
  if (attributes) attributes->findIDREF(idrefnodes);

  for(list<SyntaxNode*>::iterator p = idrefnodes->begin();
      p!=idrefnodes->end();p++){
    SyntaxNodeIDREF *onidr = dynamic_cast<SyntaxNodeIDREF*>(*p);
    ModelComp *mc = onidr->ref;
    AmplModel *model = mc->model;
    
    //check that this ModelComp belongs to this model
    bool found = false;
    for(list<ModelComp*>::iterator q = model->comps.begin();
        q!=model->comps.end();q++){
      if ((*q)->id == mc->id) {
        found = true;
        if ((*q)!=mc){
          if (GlobalVariables::prtLvl>1)
            cout << "Model component " << mc->id << " referenced in "
                 << this->id << " is reassigned.\n";
          found = true;
          onidr->ref = (*q);
        }
        newdep.push_back(*q);
      }
    }
    if (!found){
      cerr << "ERROR: Model component " << mc->id << " referenced in "
           << this->id << " not found.\n";
      exit(1);
    }
  }
  dependencies = newdep;
  
  delete(idrefnodes);

}
