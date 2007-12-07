#include "model_comp.h"
#include "backend.h"

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
  static int tt_count=0;
  //model_comp *newmc = (model_comp*)calloc(1,sizeof(model_comp));
  this->tag = false;
  this->id = strdup(id);
  this->type = type;
  this->indexing = (opNodeIx*)indexing;
  if (indexing) (this->indexing)->splitExpression();
  //  if (indexing){
  //  this->indexing = new opNodeIx(indexing);
  //}else{
  //  this->indexing = NULL;
  //}
  //delete(indexing);
  this->attributes = attrib;

  /* now set up the dependency list for the component */
  printf("Defining model component (%4d): %s\n",tt_count, id);
  this->count = tt_count++;
  printf("  dependencies in indexing: \n");
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
    char *tmp = attrib->print();
    printf("  dependencies in attributes: %s\n", tmp);
    free(tmp);
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
  printf("--------------------------------\n");
  for( list<model_comp*>::iterator p=dependencies.begin(); 
       p!=dependencies.end(); ++p)
    printf("%s\n",(*p)->id);
  //return newmc;

  global_list.push_back(this);
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
  next = NULL;
  prev = NULL;
  model = NULL;
  other = NULL;
  count = -1;
  tag = false;
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
model_comp::print()
---------------------------------------------------------------------------- */
/** Print a detailed diagnostic description of this model component
 *  with the values of all its fields                                        */
void
model_comp::print()
{
  printf("------------------------------------------------------------\n");
  printf("model_comp: %s\n",id);
  printf("  type: %s\n",nameTypes[type]);
  printf("   (ismin: %d)\n",ismin);
  printf("  attributes: %s\n",print_opNode(attributes));
  printf("  indexing: %s\n", print_opNode(indexing));
  if (indexing) indexing->printDiagnostic();
  printf("  next: %s\n",(next==NULL)?"NULL":next->id);
  printf("  next: %s\n",(prev==NULL)?"NULL":prev->id);
  printf("  dependencies: %d:\n",dependencies.size());
  printf("      ");
  for(list<model_comp*>::iterator p = dependencies.begin();
      p!=dependencies.end();p++)
    printf("%s ",(*p)->id);
  printf("  model: %s\n",model->name);
  printf("  count: %d\n",count);
  printf("  tag: %s\n",tag?"true":"false");
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
model_comp::clone()
---------------------------------------------------------------------------- */
model_comp *
model_comp::clone()
{
  model_comp *newm = new model_comp();

  newm->type = type;
  newm->id = id;
  newm->ismin = ismin;
  newm->attributes = attributes;
  newm->indexing = indexing;
  newm->next = next;
  newm->prev = prev;
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
/* find the global name of the model component pointed to by node:
   IN: 
     model_comp *node:           The model component in question 
     opNode *opn:
     AmplModel *current_model:  The block for which this is written:
                   indexing is given in the original AMPL model wrt a 
                   given node in the model tree. 
        FIXME: what happens if the component referenced in the definition is
               not in the same model_tree node as the component to be defined?
               In the original ampl file this is correct, since the indexing
               will be given relative to the current_model. 
               However the local indexing is lost(?) in the node representation
               => I don't think so, it is still encoded in the rest of the
                  opNode structure
     int witharg: 

   DEP:
     n_addIndex/l_addIndex:      These are set up outside. It is assumed that
                                 all indexing expressions of blocks below
                                 the current_model are on this stack
     FIXME: this should be part of the AmplModel class

*/

char *
getGlobalName(model_comp *node, opNode *opn, AmplModel *current_model, 
	      int witharg)
{
  //model_comp *node = opn->values[0];  /* this is the model_component */
  AmplModel *model_of_comp = node->model;/* this is the model it belongs to */
  AmplModel *tmp;
  char namebuffer[200];
  char arglistbuffer[200];
  int n_index = 0;
  int i;

  /* need to get list of model names and argument list to prefix */
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

  /* ok, first need to find the common ancestor of default_model and
     model_of_comp 
     => build the two stacks of models and see where they diverge */
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
    
    /* for every level above 0 there should be a dummy variable on the stack 
       => go up the path and add dummy varables to arglist */
    for(i=0;i<clvl;i++){
      /* FIXME: here the dv cannot be printed by opNode.print() if it is 
	 a list like (i,j)! */
      if (n_index==0){
	opNode *dv = l_addIndex[i]->dummyVar;
	if (dv) {
	  char *tmp1 = dv->printDummyVar();
	  sprintf(arglistbuffer, "%s", tmp1);
	  free(tmp1);
	  //printf("add dummy variable: %s\n",print_opNode(dv));
	  n_index++;
	}
      }else{
	/* need to put subscript before the current list */
	opNode *dv = l_addIndex[i]->dummyVar;
	if (dv){
	  char *tmp1 = dv->printDummyVar();
	  strcat(arglistbuffer,",");
	  strcat(arglistbuffer, tmp1);
	  free(tmp1);
	  //printf("add dummy variable: %s\n",print_opNode(dv));
	  n_index++;
	}
      }
    }
  }
  

  /* work on the argument list */
  /* opn->nval is the number of arguments (at this level),
     the arguments follow in positions values[1] - values[nval]
     
     We also need to prefix this with the dummy arguments corresponding
     to the blocks that this entity is in */

  if (opn->nval+n_index>0){
    for(i=0;i<opn->nval;i++){
      if (n_index==0){
	sprintf(arglistbuffer, "%s", print_opNode((opNode*)opn->values[i+1]));
	n_index++;
      }else{
	char *tmpc = strdup(arglistbuffer);
	sprintf(arglistbuffer, "%s,%s",tmpc, print_opNode((opNode*)opn->values[i+1]));
	free(tmpc);
	n_index++;
      }
    }
    
    strcat(namebuffer, "[");
    strcat(namebuffer, arglistbuffer);
    strcat(namebuffer, "]");

  }
  return strdup(namebuffer);
}
