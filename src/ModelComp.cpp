#include <iostream>
#include <fstream>
#include "model_comp.h"
#include "Set.h"
#include "backend.h"
#include "GlobalVariables.h" //for GlobalVariables class

static bool prtAnaDep = false;

int model_comp::tt_count=0;  // initialise static class member

extern void modified_write(ostream &fout, model_comp *comp);

/* This should be an IDREF (or IDREFM) node that needs to be converted
   into its global name 
   
   The node is a pointer to a model_comp structure. Need to work out
   which (if any) blocks it belongs to and pre-pend the name of any block to
   the global name

   Also need to work out which dummy variables need to be put on the
   argument list

   IN: model_comp *node          : the model comp of which the name should
                                   be obtained
       int        witharg        : 1 if argument list should be printed 
       opNode     *opn           : the IDREF node that should be named
       AmplModel *current_model : the model in which this is referenced

   arguments opn, current_model are only needed if the argument list should
   be printed as well

   opn is the IDREF(M) node of the object that should be named. All the
   subscripts that are used for this in the model description are
   part of the 'opn' node. To get the complete global argument list, these
   subscripts need to be prefixed by the ones corresponding to the
   model from which this object is referenced
*/

list<model_comp*> model_comp::global_list;

/* --------------------------------------------------------------------------
model_comp::model_comp(char* is, compType type, opNode *ix, opNode *attr)
---------------------------------------------------------------------------- */
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
model_comp::model_comp(char *id, compType type, 
                       opNode *indexing, opNode *attrib)
{
  //model_comp *newmc = (model_comp*)calloc(1,sizeof(model_comp));
  value = NULL;
  this->tag = false;
  this->id = strdup(id);
  this->type = type;
  this->indexing = dynamic_cast<opNodeIx*>(indexing);
  if (indexing) (this->indexing)->splitExpression();
  //  if (indexing){
  //  this->indexing = new opNodeIx(indexing);
  //}else{
  //  this->indexing = NULL;
  //}
  //delete(indexing);
  this->attributes = attrib;

  this->count = model_comp::tt_count++;
  if (GlobalVariables::prtLvl>0) 
    printf("Defining model component (%4d): %s\n",this->count, id);

  setUpDependencies();

  /* now set up the dependency list for the component */
  //  printf("  dependencies in indexing: \n");
  //  if (indexing) {
  //    list<model_comp*> lmc;
  //    indexing->findIDREF(&lmc);
  //    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
  //      // see if element already on dep list
  //      bool found=false;
  //      for (list<model_comp*>::iterator q=dependencies.begin();
  //           q!=dependencies.end();q++)
  //        if (*p==*q) found = true;
  //      if (!found)
  //        dependencies.push_back(*p);
  //    }
  //  }
  //  if (attrib){
  //    printf("  dependencies in attributes: %s\n", attrib->print());
  //    //attrib->findIDREF();
  //    list<model_comp*> lmc;
  //    attributes->findIDREF(&lmc);
  //    // lmc should be a list of model components
  //    // how do I iterate through it?
  //    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
  //      // see if element already on dep list
  //      bool found=false;
  //      for (list<model_comp*>::iterator q=dependencies.begin();
  //           q!=dependencies.end();q++)
  //        if (*p==*q) found = true;
  //     if (!found)
  //        dependencies.push_back(*p);
  //    }
  //  }
  //  printf("--------------------------------\n");
  //  for( list<model_comp*>::iterator p=dependencies.begin(); 
  //       p!=dependencies.end(); ++p)
  //    printf("%s\n",(*p)->id);

  global_list.push_back(this);
}

/* --------------------------------------------------------------------------
model_comp::setUpDependencies()
---------------------------------------------------------------------------- */
void
model_comp::setUpDependencies()
{
  dependencies.clear();
  /* now set up the dependency list for this component */
  if (prtAnaDep){
    printf("Analyse dependencies for %s\n",id);
    printf(" dependencies in indexing: \n");
  }
  if (indexing) {
    // get list of IDREF nodes in 'indexing'
    list<model_comp*> lmc;
    indexing->findIDREF(&lmc);
    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<model_comp*>::iterator q=dependencies.begin();
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
    list<model_comp*> lmc;
    attributes->findIDREF(&lmc);
    // lmc should be a list of model components
    // how do I iterate through it?
    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<model_comp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (prtAnaDep){
    for( list<model_comp*>::iterator p=dependencies.begin(); 
         p!=dependencies.end(); ++p)
      printf("  %s\n",(*p)->id);
    printf("--------------------------------\n");
  }
}

/* --------------------------------------------------------------------------
model_comp::model_comp()
---------------------------------------------------------------------------- */
/** Default constructor: just sets all fields to -1/NULL/false               */
model_comp::model_comp()
{
  type = TNOTYPE;
  id = NULL;
  indexing = NULL;
  attributes = NULL;
  //next = NULL;
  //prev = NULL;
  model = NULL;
  other = NULL;
  count = -1;
  tag = false;
  value = NULL;
}
/* --------------------------------------------------------------------------
model_comp::setTo()
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
model_comp::setTo(char *id, compType type, 
                       opNodeIx *indexing, opNode *attrib)
{
  static int tt_count=0;
  //model_comp *newmc = (model_comp*)calloc(1,sizeof(model_comp));
  this->tag = false;
  this->id = strdup(id);
  this->type = type;
  this->indexing = indexing;
  if (indexing) (this->indexing)->splitExpression();
  //  if (indexing){
  //  this->indexing = new opNodeIx(indexing);
  //}else{
  //  this->indexing = NULL;
  //}
  //delete(indexing);
  this->attributes = attrib;

  /* now set up the dependency list for the component */
  //printf("Defining model component (%4d): %s\n",tt_count, id);
  this->count = tt_count++;
  if (prtAnaDep) printf(" dependencies in indexing: \n");
  if (indexing) {
    list<model_comp*> lmc;
    indexing->findIDREF(&lmc);
    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<model_comp*>::iterator q=dependencies.begin();
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
    list<model_comp*> lmc;
    attrib->findIDREF(&lmc);
    // lmc should be a list of model components
    // how do I iterate through it?
    for( list<model_comp*>::iterator p=lmc.begin(); p!=lmc.end(); ++p){
      // see if element already on dep list
      bool found=false;
      for (list<model_comp*>::iterator q=dependencies.begin();
           q!=dependencies.end();q++)
        if (*p==*q) found = true;
      if (!found)
        dependencies.push_back(*p);
    }
  }
  if (prtAnaDep){
    for( list<model_comp*>::iterator p=dependencies.begin(); 
         p!=dependencies.end(); ++p)
      printf("  %s\n",(*p)->id);
    //return newmc;
    printf("--------------------------------\n");
  }

  global_list.push_back(this);
}

/* ---------------------------------------------------------------------------
model_comp::untagAll()
---------------------------------------------------------------------------- */
/** Set tag=false for all model components */
void 
model_comp::untagAll()
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    (*p)->tag = false;
  }
}
/* ---------------------------------------------------------------------------
model_comp::untagAll(AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively set tag=false for all model components 
 *  @param start The AmplModel where to start the recursion
 */

void 
model_comp::untagAll(AmplModel *start)
{
  // iterate through the local list
  for (list<model_comp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    (*p)->tag = false;
    if ((*p)->type==TMODEL){
      model_comp::untagAll((AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::writeAllTagged()
---------------------------------------------------------------------------- */
/** Write out a list of all model components that have the tag set           */
void 
model_comp::writeAllTagged()
{

  // iterate through the global list
  for (list<model_comp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    if ((*p)->tag) {
      printf("%s\n",(*p)->id);
    }
  }
}
/* ---------------------------------------------------------------------------
model_comp::writeAllTagged(AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively write out a list of all model components that have the tag set 
 *  @param start The AmplModel where to start the recursion
 */
void 
model_comp::writeAllTagged(AmplModel *start)
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    if ((*p)->tag) {
      printf("%s::%s\n",start->name, (*p)->id);
    }
    if ((*p)->type==TMODEL){
      model_comp::writeAllTagged((AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::writeAllTagged()
---------------------------------------------------------------------------- */
/** Write out a list of all model components that have the tag set:
 Just write a list of names */
void 
model_comp::writeAllTagged(ostream &fout)
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    if ((*p)->tag) {
      fout << (*p)->id << "\n";
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::writeAllTagged(ostream &fout, AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively write out a list of all model components that have the tag set 
 *  @param start The AmplModel where to start the recursion
 *  @param fout File where to write to
 */
void 
model_comp::writeAllTagged(ostream &fout, AmplModel *start)
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    if ((*p)->tag) {
      fout << (*p)->id << "\n";
    }
    if ((*p)->type==TMODEL){
      model_comp::writeAllTagged(fout, (AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::modifiedWriteAllTagged()
---------------------------------------------------------------------------- */
/** Write out a list of all model components that have the tag set:
 write every component how it would appear in the global model file 
\bug modified_write should be called within the model writing process:
  it depends on addIndex/l_addIndex, i.e. some indexing expressions (and
  subbmodel names) should be added to entity names depending on where in the
  model it is called 
*/
void 
model_comp::modifiedWriteAllTagged(ostream &fout)
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=global_list.begin();p!=global_list.end();
       p++){
    if ((*p)->tag) {
      modified_write(fout, *p);
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::modifiedWriteAllTagged(ostream &fout, AmplModel *start)
---------------------------------------------------------------------------- */
/** Recursively write out a list of all model components that have the tag set:
 * write every component how it would appear in the global model file 
 * @bug modified_write should be called within the model writing process:
 *  it depends on addIndex/l_addIndex, i.e. some indexing expressions (and
 *  submodel names) should be added to entity names depending on where in the
 *  model it is called 
 *  @param start The AmplModel where to start the recursion
 *  @param fout File where to write to
 */

void 
model_comp::modifiedWriteAllTagged(ostream &fout, AmplModel *start)
{
  // iterate through the global list
  for (list<model_comp*>::iterator p=start->comps.begin();
       p!=start->comps.end();p++){
    if ((*p)->tag) {
      modified_write(fout, *p);
    }
    if ((*p)->type==TMODEL){
      model_comp::modifiedWriteAllTagged(fout, (AmplModel*)(*p)->other);
    }
  }
}

/* ---------------------------------------------------------------------------
model_comp::getSetMembership()
---------------------------------------------------------------------------- */
/** Write out the members of a set */
list <string>
model_comp::getSetMembership()
{
  char buffer[500];

  if (type!=TSET){
    printf("ERROR: Called getSetMembership() for model_comp not of type SET\n");
    exit(1);
  }
  
  ofstream out("tmp.mod");

  model_comp::untagAll();
  tagDependencies();
  model_comp::writeAllTagged(out);
  out.close();
  
  out.open("tmp.scr");
  out << "reset;\n";
  out << "model tmp.mod;\n";
  out << "data ../" << GlobalVariables::datafilename << ";\n";
  out << "display " << id << " > (\"tmp.out\");\n";
  out.close();
  
  if(strlen(GlobalVariables::amplcommand)+9>500) {
     // Avoid buffer overflow
     cerr << "buffer too short to accomodate amplcommand length.\n";
     exit(1);
  }
  strcpy(buffer, GlobalVariables::amplcommand);
  strcat(buffer, " tmp.scr");
  system(buffer);

  ifstream in("tmp.out");
  string setmem;
  in >> setmem;
  in.close();
  cout << "Set " << id << " members: " << setmem << "\n";
  exit(1);

}

/* ---------------------------------------------------------------------------
model_comp::print()
---------------------------------------------------------------------------- */
/** Print a detailed diagnostic description of this model component
 *  with the values of all its fields                                        */
void
model_comp::print()
{
  char *tmp;
  cout << "------------------------------------------------------------\n";
  cout << "model_comp: " << id << "\n";
  cout << "  type: " << nameTypes[type] << "\n";
  //printf("   (ismin: %d)\n",ismin);
  cout << "  attributes: " << *attributes << "\n";
  cout << "  indexing: " << *indexing << "\n";
  if (indexing) indexing->printDiagnostic(cout);
  //printf("  next: %s\n",(next==NULL)?"NULL":next->id);
  //printf("  prev: %s\n",(prev==NULL)?"NULL":prev->id);
  cout << "  dependencies: " << dependencies.size() << ":\n";
  cout << "      ";
  for(list<model_comp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++)
    cout << (*p)->model->name << "::" << (*p)->id << " ";
  cout << "  model: " << model->name<< "\n";
  cout << "  count: " << count << "\n";
  cout << "  tag: " << tag << "\n";
}

/* ---------------------------------------------------------------------------
model_comp::dump(ostream &fout)
---------------------------------------------------------------------------- */
/** Print a detailed diagnostic description of this model component
 *  with the values of all its fields                                        */
void
model_comp::dump(ostream &fout)
{
  fout << "MCDP  --------------------------------------------------------"
     "----\n";
  fout << "MCDP model_comp: " << id << " ("<< (void *) this << ")\n";
  fout << "MCDP  type: " << nameTypes[type] << "\n";
  //printf("   (ismin: %d)\n",ismin);
  fout << "MCDP  attributes: " << attributes << "\n";
  if (attributes) attributes->dump(fout);
  fout << "MCDP  indexing: " << indexing << "\n";
  if (indexing) indexing->printDiagnostic(fout);
  if (indexing) indexing->dump(fout);
  //printf("  next: %s\n",(next==NULL)?"NULL":next->id);
  //printf("  prev: %s\n",(prev==NULL)?"NULL":prev->id);
  fout << "MCDP  dependencies: " << dependencies.size() << ":\n";
  fout << "      ";
  for(list<model_comp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++)
    fout << (*p)->model->name << "::" << (*p)->id << " ";
  fout << "\nMCDP  model: " << model->name << "\n";
  fout << "MCDP  count: " << count << "\n";
  fout << "MCDP  tag: " << tag << "\n";
  if (value) {
    fout << "MCDP  value: " << value->printToString() << "\n";
  }
}

/* ---------------------------------------------------------------------------
model_comp::printBrief()
---------------------------------------------------------------------------- */
/** Print a one line description of the object: type and name                */
void
model_comp::printBrief()
{
  printf("%s %s\n",nameTypes[type], id);
}
/* ---------------------------------------------------------------------------
model_comp::tagDependencies()
---------------------------------------------------------------------------- */
/** Recursively set tag=true for this model component and all components that  
 *  it depends on (i.e. everything listed in its dependency list           */
void
model_comp::tagDependencies()
{
  this->tag = true;
  for(list<model_comp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++){
    (*p)->tagDependencies();
  }
}

/* ---------------------------------------------------------------------------
model_comp::deep_copy()
---------------------------------------------------------------------------- */
/** Create a deep-copy of the model_comp object: 
 *  The tree of attributes and indexing expressions is recreated using 
 *  entirely new objects.
 */
model_comp *
model_comp::deep_copy()
{
  model_comp *newm = new model_comp();

  newm->type = type;
  newm->id = strdup(id);
  //newm->ismin = ismin;
  if (attributes) newm->attributes = attributes->deep_copy();
  if (indexing) newm->indexing = indexing->deep_copy();
  //newm->next = next;
  //newm->prev = prev;
  newm->dependencies = dependencies;
  newm->model = model;
  newm->other = other;
  newm->count = count;
  newm->tag = tag;

  return newm;
}
/* ---------------------------------------------------------------------------
model_comp::clone()
---------------------------------------------------------------------------- */
/** Create a shallow copy of the object: only the top level object is 
 *  copied, pointers below are reused 
 */
model_comp *
model_comp::clone()
{
  model_comp *newm = new model_comp();

  newm->type = type;
  newm->id = id;
  //newm->ismin = ismin;
  newm->attributes = attributes;
  newm->indexing = indexing;
  //newm->next = next;
  //newm->prev = prev;
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
 *   and in which model this instance of referal of the model_comp is 
 *   (the current_model)
 *   Basically we need to identify the common ancestor model of the 
 *   current_model and the model_of_comp. The arguments originating from 
 *   block indexing expressions between here and the model_of_comp are already 
 *   included in the argument list of the model_comp. Anything below needs to
 *   be prepended to the argument list
 *
 * @param[in] node           The model component in question 
 * @param[in] opn            The node (IDREF) of the model component
 *                           (needed for the (local) argument list)
 * @param[in] current_model:  The block for which this is written:
 *              indexing is given in the original SML model wrt a 
 *              given node in the model tree. 
 *      FIXME: what happens if the component referenced in the definition is
 *             not in the same model_tree node as the component to be defined?
 *             In the original ampl file this is correct, since the indexing
 *             will be given relative to the current_model. 
 *             However the local indexing is lost(?) in the node representation
 *             => I don't think so, it is still encoded in the rest of the
 *                opNode structure
 * @param[in] witharg    WITHARG if the argument list should be processed, 
 *                       NOARG   only the global name
 *                       ONLYARG only the argument list
 *
 * @pre
 *   l_addIndex   Needs to be set. It is assumed that this is a stack of 
 *                indexing expressions from the root at least to the common
 *                ancestor node (likely set up to the current_model)
 *
 */

char *
getGlobalName(model_comp *node, const opNode *opn, AmplModel *current_model, 
              int witharg)
{
  //model_comp *node = opn->values[0];  /* this is the model_component */
  AmplModel *model_of_comp = node->model;/* this is the model it belongs to */
  AmplModel *tmp;
  char namebuffer[200];
  char arglistbuffer[200];
  int n_index = 0;
  //  int p_ix_list = n_addIndex-1;
  int i;

  namebuffer[0]=0;
  /* need to get list of model names and argument list to prefix */
  if (witharg==NOARG||witharg==WITHARG){
    strcpy(namebuffer, node->id);
    tmp = model_of_comp;
    while(strcmp(tmp->name,"root")!=0){
      /* work on name */
      char *tmpc = strdup(namebuffer);
      strcpy(namebuffer, tmp->name);
      strcat(namebuffer, "_");
      strcat(namebuffer, tmpc);
      free(tmpc);
      
      if (tmp->parent==NULL) {
        printf("has no parent >%s<\n",tmp->name);
        exit(1);
      }
      tmp = tmp->parent;
    }
  }
  
  if (witharg==NOARG) return strdup(namebuffer);
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
    /* okay the two paths are build, lets print them */
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
       => go up the path and add dummy varables to arglist */
    for(i=0;i<clvl;i++){
      /* FIXME: here the dv cannot be printed by opNode.print() if it is 
         a list like (i,j)! */
      list <add_index *> *li = l_addIndex.at(i);
      for(list<add_index*>::iterator p = li->begin();p!=li->end();p++){
        if (n_index==0){
          //opNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
          opNode *dv = (*p)->dummyVar;
          if (dv) {
            sprintf(arglistbuffer, "%s", dv->printDummyVar().c_str());
            //printf("add dummy variable: %s\n",print_opNode(dv));
            n_index++;
          }
        }else{
          /* need to put subscript before the current list */
          //opNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
          opNode *dv = (*p)->dummyVar;
          if (dv){
            strcat(arglistbuffer,",");
            strcat(arglistbuffer,dv->printDummyVar().c_str());
            //printf("add dummy variable: %s\n",print_opNode(dv));
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

  if (opn->nval+n_index>0){
    if (n_index==0){
      sprintf(arglistbuffer, "%s", (opn->getArgumentList()).c_str());
    }else{
      if (opn->nval==0){
        // arglistbuffer stays as it is
      }else{
        char *tmpc = strdup(arglistbuffer);
        sprintf(arglistbuffer, "%s,%s", tmpc, (opn->getArgumentList()).c_str());
      }
    }
    //for(i=0;i<opn->nval;i++){
    //  if (n_index==0){
    //sprintf(arglistbuffer, "%s", print_opNode((opNode*)opn->values[i+1]));
    //        n_index++;
    //  }else{
    //        char *tmpc = strdup(arglistbuffer);
    //        sprintf(arglistbuffer, "%s,%s",tmpc, print_opNode((opNode*)opn->values[i+1]));
    //        free(tmpc);
    //n_index++;
    //}
    //}
    
    strcat(namebuffer, "[");
    strcat(namebuffer, arglistbuffer);
    strcat(namebuffer, "]");

  }
  return strdup(namebuffer);
}

/* FIXME: this is a stub for getGlobalNameNew, a version of getGlobalName
 that does not need the addIndex stack, but derives the added indexing
 from the indexing of the submodels in the tree up to this component
 This should be used in the rendering of expectation constraints

 I don't think this is going to work though: components might be refered to
 from below the model in which they are defined in which case some of the
 indexing is already part of the opNode structure (and different from just 
 using the submodel indexing)

 Could get around this by making indexing dependend on the
  - model to which the comp belongs 
    (to be able to follow down submodels to get indexing expressions)
  - model from which the component was refered to 
    (indexing expressions between these two models are already part of
    the opNode structure)
 Indeed this is done in the old getGlobalName method.

 Latest thought (01/04/08:11:50) is that this *might* indeed work
*/

/* ---------------------------------------------------------------------------
getGlobalNameNew(model_comp *node, opNode *opn, AmplModel *current_model, 
              int witharg)
---------------------------------------------------------------------------- */
/** New version of getGlobalName that does *not* use the addIndex stack
 *  but creates the modified argument list by looking at the indexing
 *  expressions of the submodel tree leading to this model_comp
 *
 * @param[in] node           The model component in question 
 * @param[in] opn            The node (IDREF) of the model component
 *                           (needed for the (local) argument list)
 * @param[in] current_model:  The block for which this is written:
 *              indexing is given in the original SML model wrt a 
 *              given node in the model tree. 
 * @param[in] witharg    WITHARG if the argument list should be processed, 
 *                       NOARG   only the global name
 *                       ONLYARG only the argument list
 *
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
 *   and in which model this instance of referal of the model_comp is 
 *   (the current_model)
 *   Basically we need to identify the common ancestor model of the 
 *   current_model and the model_of_comp. The arguments originating from 
 *   block indexing expressions between here and the model_of_comp are already 
 *   included in the argument list of the model_comp. Anything below needs to
 *   be prepended to the argument list
 *
 */

char *
getGlobalNameNew(model_comp *node, const opNode *opn, AmplModel *current_model, 
              int witharg)
{
  //model_comp *node = opn->values[0];  /* this is the model_component */
  AmplModel *model_of_comp = node->model;/* this is the model it belongs to */
  AmplModel *tmp;
  char namebuffer[200];
  char arglistbuffer[200];
  int n_index = 0;
  //  int p_ix_list = n_addIndex-1;
  int i;

  namebuffer[0]=0;
  /* need to get list of model names and argument list to prefix */
  if (witharg==NOARG||witharg==WITHARG){
    strcpy(namebuffer, node->id);
    tmp = model_of_comp;
    while(strcmp(tmp->name,"root")!=0){
      /* work on name */
      char *tmpc = strdup(namebuffer);
      strcpy(namebuffer, tmp->name);
      strcat(namebuffer, "_");
      strcat(namebuffer, tmpc);
      free(tmpc);
      
      if (tmp->parent==NULL) {
        printf("has no parent >%s<\n",tmp->name);
        exit(1);
      }
      tmp = tmp->parent;
    }
  }
  
  if (witharg==NOARG) return strdup(namebuffer);
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
    /* okay the two paths are build, lets print them */
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
       => go up the path and add dummy varables to arglist */
    for(i=1;i<=clvl;i++){ //start from 1: 0 is the root which has no indexing
      /* FIXME: here the dv cannot be printed by opNode.print() if it is 
         a list like (i,j)! */
      model_comp *node_model = path1[n_path1-1-i]->node;
      opNodeIx *indexing_model = node_model->indexing;
      
      if (indexing_model){
        for(int p=0;p<indexing_model->ncomp;p++){
          if (n_index==0){
            //opNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
            opNode *dv = indexing_model->dummyVarExpr[p];
            if (dv) {
              sprintf(arglistbuffer, "%s", dv->printDummyVar().c_str());
              //printf("add dummy variable: %s\n",print_opNode(dv));
              n_index++;
            }
          }else{
            /* need to put subscript before the current list */
            //opNode *dv = (l_addIndex[i])?l_addIndex[i]->dummyVar:NULL;
            opNode *dv = indexing_model->dummyVarExpr[p];
            if (dv){
              strcat(arglistbuffer,",");
              strcat(arglistbuffer,dv->printDummyVar().c_str());
              //printf("add dummy variable: %s\n",print_opNode(dv));
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

  if (opn->nval+n_index>0){
    if (n_index==0){
      sprintf(arglistbuffer, "%s", (opn->getArgumentList()).c_str());
    }else{
      if (opn->nval==0){
        // arglistbuffer stays as it is
      }else{
        char *tmpc = strdup(arglistbuffer);
        sprintf(arglistbuffer, "%s,%s", tmpc, (opn->getArgumentList()).c_str());
      }
    }
    //for(i=0;i<opn->nval;i++){
    //  if (n_index==0){
    //sprintf(arglistbuffer, "%s", print_opNode((opNode*)opn->values[i+1]));
    //        n_index++;
    //  }else{
    //        char *tmpc = strdup(arglistbuffer);
    //        sprintf(arglistbuffer, "%s,%s",tmpc, print_opNode((opNode*)opn->values[i+1]));
    //        free(tmpc);
    //n_index++;
    //}
    //}
    
    strcat(namebuffer, "[");
    strcat(namebuffer, arglistbuffer);
    strcat(namebuffer, "]");

  }
  return strdup(namebuffer);
}

/* --------------------------------------------------------------------------
model_comp::moveUp(int level)
---------------------------------------------------------------------------- */
/** Queue the model_comp to be moved up by 'level' levels in the model tree:
 *  Just removing the component from the current model and adding it to a
 *  parent is dangerous, since model_comp::moveUp is typically called from
 *  within a (nested) loop over all model_comps (->comps) in the AmplModels
 *  removing/adding items to list<model_comp*> comps while there is an
 *  iterator running over it will invalidate that iterator.
 *  => hence the request to move is scheduled to be executed by
 *     AmplModel::applyChanges() after the loop over all components
 *
 *  This method will also re-write the component for the new model
 *  I.e. all IDREFs to components below the new model will have their
 *  local indexing expression expanded
 */
void
model_comp::moveUp(int level){
  AmplModel *current = model;
  int i, posm;
  
  // -------------------- Expand local indexing expression -----------------
  /* This model_comp is written for the 'current' model and is now re-asigned
     to a different model. In order for that to work the indexing expressions
     in all IDREFs in its attribute/indexing section have got to be
     rewritten

     Indexing expressions applicable to a IDREF are divided into local and 
     block indexing. 'local' is directly associated with the IDREF (as
     arguments in ->values[]. 'block' originate from the indexing expressions
     of the blocks up to the current model in the model tree. Both indexing 
     expression together need to combine to get the correct global indexing

     When moving a model_comp up in the tree, we therefor need to do the 
     following to have correct global indexing:
       - all IDREFs to model_comp's in models below the new model 
         (current.parent(level)) need to have the block indexing expressions
         between their own model and current.parent(level) added
         to their local indexing
  */

  // get list of models from current model to root
  vector<AmplModel*> mlist;

  for(AmplModel *tmp=current;tmp->parent!=NULL;tmp=tmp->parent){
    mlist.push_back(tmp);
  }
  

  list<opNode*> *idrefnodes = new list<opNode*>;

  // get list of all IDREF nodes in dependencies
  if (indexing) indexing->findIDREF(idrefnodes); 
  if (attributes) attributes->findIDREF(idrefnodes);

  // loop over all IDREF nodes
  for(list<opNode*>::iterator p = idrefnodes->begin();
      p!=idrefnodes->end();p++){
    opNodeIDREF *onidr = dynamic_cast<opNodeIDREF*>(*p);
    model_comp *mc = onidr->ref;
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
      // this is a model between the old and new model for model_comp *this
      // posm gives the position: 0 is the old model, level is the new
      // one
      // => need to add indexing expressions between posm and level-1
      // starting with level-1
      int shift = level-posm;
      void **newval = (void**)calloc(onidr->nval+shift, sizeof(void*));
      for(i=0;i<onidr->nval;i++) newval[i+shift] = onidr->values[i];
      for(i=0;i<shift;i++){
        opNodeIx *mix = mlist[level-1-i]->ix;
        if (mix->ncomp!=1){
          printf("model_comp::moveUp() does not support intermediate models with !=1 dummy Var\n");
          exit(1);
        }
        newval[i] = mix->dummyVarExpr[0];
        /* indexing dummy var of mlist[level-1-i]*/
      }
      onidr->nval+=shift;
      free(onidr->values);
      onidr->values = newval;
    }
  }

  // and queue this item to be moved up by AmplModel::applyChanges 
  changeitem *rem = (changeitem*)calloc(1, sizeof(changeitem));
  changeitem *add = (changeitem*)calloc(1, sizeof(changeitem));
  rem->comp = this;
  rem->model = model;
  rem->action = CHANGE_REM;
  AmplModel::changes.push_back(rem); // Q for removal
  //model->removeComp(this);
  for(i=0;i<level;i++){
    model = model->parent;
  }
  add->comp = this;
  add->model = model;
  add->action = CHANGE_ADD;
  AmplModel::changes.push_back(add);
}

/* --------------------------------------------------------------------------
model_comp::reassignDependencies()
---------------------------------------------------------------------------- */
/** In the process of building the AmplModel tree from the StochModelTree
 *  some of the IDREF dependency nodes still point to the StochModelComp
 *  nodes from the StochModel tree (or the intermediate tree)
 *
 *  This routine makes sure that IDREF nodes are resolved with respect to the 
 *  correct ModelComp and rebuilds the dependency lists.
 */

void
model_comp::reassignDependencies()
{
  list<opNode*> *idrefnodes = new list<opNode*>;
  list<model_comp*> newdep;

  if (indexing) indexing->findIDREF(idrefnodes);
  if (attributes) attributes->findIDREF(idrefnodes);

  for(list<opNode*>::iterator p = idrefnodes->begin();
      p!=idrefnodes->end();p++){
    opNodeIDREF *onidr = dynamic_cast<opNodeIDREF*>(*p);
    model_comp *mc = onidr->ref;
    AmplModel *model = mc->model;
    
    //check that this model_comp belongs to this model
    bool found = false;
    for(list<model_comp*>::iterator q = model->comps.begin();
        q!=model->comps.end();q++){
      if (strcmp((*q)->id, mc->id)==0){
        found = true;
        if ((*q)!=mc){
          if (GlobalVariables::prtLvl>1)
            printf("Model comp %s referenced in %s is reassigned\n",mc->id,
                 this->id);
          found = true;
          onidr->ref = (*q);
        }
        newdep.push_back(*q);
      }
    }
    if (!found){
      printf("Model comp %s referenced in %s is not found\n",mc->id,
             this->id);
      exit(1);
    }
  }
  dependencies = newdep;
  
  delete(idrefnodes);

}

void
model_comp::foo(){}
