#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "backend.h"
#include "ampl.h"
#include "nodes.h"
#include "ampl.tab.hpp"
#include "sml-oops.h"

void print_model(AmplModel *model);
void process_model(AmplModel *model);
void add_to_index_stack(opNode *ix);
void rem_from_index_stack();

void write_ampl_for_submodel(FILE *fout, AmplModel *root, 
			     AmplModel *submodel);
void write_columnfile_for_submodel(FILE *fout, AmplModel *submodel);

/* this struct stores an indexing expression in an easy to modify form:
   the add_index below will be rendered as
   val(dummyVar) in val(set) 
     or just
   val(set)
     if dummyVar is NULL
*/
//typedef struct add_index_st {
//  opNode *dummyVar;     /* an opNode representing the dummy variable expr */
//  opNode *set;          /* an opNode representing the set */
//} add_index;


/* Stack of indexing expressions that are applicable to all variables that
   are printed:

   In the original AMPL-file variables are subscripted (and set are
   indexed) *relative* to the current location in the model tree.
   
   In the AMPL-subfiles a global naming of variables is used,
   therefore indexing sets and dummy variables of the block
   definitions leading to the current node in the model tree have to
   be added to all variables.

   This is done by the addIndex stack: as the sub file writer traverses through
   the tree the block-indexing expressions are added to the tree           */

// FIXME: this stack could be implemented as its own class:
//        these two would become static class variables 
// FIXME: this is fairly dumb at the moment: it cannot deal with 
//         mulitple dimenaions {i in SET1,j in SET2} 
//         SET valued expressions: {i in SET1 cross SET2} 
//         or conditions:    {(i,j) in SET1:i<j}
int n_addIndex;           /* number and list of indexing expressions */
add_index *l_addIndex[5];  /* to add to all statements */


/* ---------------------------------------------------------------------------
do_stuff
---------------------------------------------------------------------------- */
/* This is the entry function once the model has been parsed. It is called from
   main in ampl.y
   This includes calls to the various phases of the backend
*/
void do_stuff(AmplModel *model)
{
  print_model(model);
  process_model(model);
}


void
print_model(AmplModel *model)
{
  model_comp *entry;
  AmplModel *submod;
  opNode::use_global_names=0;
  printf("-----------------------------------------\n");
  printf("Model: %s\n",model->name);
  printf("n_sets: %d\n",model->n_sets);
  entry = model->first;
  while(entry!=NULL){
    if (entry->type==TSET){
      printf("    %s\n",entry->id);
      printf("       %s\n",print_opNode(entry->indexing));
      printf("       %s\n",print_opNode(entry->attributes));
    }
    entry = entry->next;
  }
  printf("n_cons: %d\n",model->n_cons);
  entry = model->first;
  while(entry!=NULL){
    if (entry->type==TCON){
      printf("    %s\n",entry->id);
      printf("       %s\n",print_opNode(entry->indexing));
      printf("       %s\n",print_opNode(entry->attributes));
    }
    entry = entry->next;
  }
  printf("n_vars: %d\n",model->n_vars);
  entry = model->first;
  while(entry!=NULL){
    if (entry->type==TVAR){
      printf("    %s\n",entry->id);
      printf("       %s\n",print_opNode(entry->indexing));
      printf("       %s\n",print_opNode(entry->attributes));
    }
    entry = entry->next;
  }
  printf("n_params: %d\n",model->n_params);
  entry = model->first;
  while(entry!=NULL){
    if (entry->type==TPARAM){
      printf("    %s\n",entry->id);
      printf("       %s\n",print_opNode(entry->indexing));
      printf("       %s\n",print_opNode(entry->attributes));
    }
    entry = entry->next;
  }
  
  printf("submodels: %d\n",model->n_submodels);
  entry = model->first;
  while(entry!=NULL){
    if (entry->type==TMODEL){
      submod = (AmplModel*)entry->other;
      print_model(submod);
    }
    entry = entry->next;
  }
  

}



/* ---------------------------------------------------------------------------
process_model
--------------------------------------------------------------------------- */
/* This routine will output something useful
    1) generate list of all the (sub)models defined
    2) output and AMPL file declaring each one of the submodels
   Other tasks
    3) generate the algebra tree
    4) generate the ampl script to generate a *.nl file for every
       algebra-tree node
    5) generate the corresponding structure files that partition the 
       *.nl files by columns
*/
int count_models_(AmplModel *model);
void fill_model_list_(AmplModel *model, AmplModel **list, int *pos);

void
process_model(AmplModel *model) /* should be called with model==root */
{
  int i, j, k, n_models, n_local_model, n;
  AmplModel **model_list;
  AmplModel *local_model_list[10]; /* up to 10 levels of models */
  FILE *fout;
  FILE *fscript;
  char buffer[200];   /* this is the name of the model file */
  
  printf("-------------- start of process_model ----------------------\n");

  /* 1) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> generate list of all models */

  /* recursively count all the models defined in the model tree */
  n_models = count_models_(model);
  printf("Found %d models\n",n_models);
  model_list = (AmplModel**)calloc(n_models, sizeof(AmplModel*));
  n_models=0;
  model->level=0; /* root is on level 0, fill_model_list also sets levels */
  fill_model_list_(model, model_list, &n_models);
  /* model_list[0-(n_models-1)] is now a list of all (sub)models defined
     in the ampl file */

  printf("These are the models on the list:\n");
  for(i=0;i<n_models;i++){
    AmplModel *thism = model_list[i];
    printf("%d: %s (level=%d)\n",i, thism->name, thism->level);
  }
  
  /* 2) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> output all submodel files */
  /* and */
  /* 3) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> generate the script file */
  fscript = fopen("script.scr","w");
  
  /* loop over every single model defined */
  for(i=0;i<n_models;i++){
    AmplModel *this_model;
    AmplModel **anc_list;
    /* need to construct the name of the model file:
       => this is a concatenated list of the models down the path
          (and 'root' for the top level model)
    */
    this_model = model_list[i];
    if (this_model->parent==NULL){
      /* this is root */
      sprintf(buffer, "%s", "root");
    }else{
      /* find name if model file by concatenating the names of all ancestors */
      char *bufcopy;
      AmplModel *tmp_model;
      tmp_model = this_model;
      sprintf(buffer, "%s", tmp_model->name);
      while (tmp_model->parent){
	tmp_model = tmp_model->parent;
	bufcopy = strdup(buffer);
	sprintf(buffer, "%s_%s",tmp_model->name, bufcopy);
	free(bufcopy);
      }
    }
    strcat(buffer, ".mod");
    /* write the script file */
    fprintf(fscript, "\nreset;\noption auxfiles rc;\noption presolve 0;\n");
    fprintf(fscript, "model %s;\ndata %s;\n",buffer, "global.dat");
    /* FIXME: need to know the name of the global data file 
           this should be an argument to the parser that is passed
           through to the backend                                           */
    
    /* the script file needs to look something like

    model submod.mod;
    data allthedata.dat;
    
    for {i in ARCS}{	
      let ARCS_SUB := {i};

      print card(COMM) > "nameofsub" &i&".crd"
      for {j in COMM}{
        let COMM_SUB := {j};
        fix all;
        unfix all;
        write("nameofsub"& i &"_"& j);
      }
    }
    */
    
    /* create a new model list that lists all models in this branch of the tree
       (i.e. all ancestors of the current model up to root */
    /* root will be entry 0, current model at entry model->level */
    anc_list = (AmplModel**)calloc(this_model->level+1, sizeof(AmplModel*));
    {
      AmplModel *tmp_model = this_model;
      anc_list[tmp_model->level] = tmp_model;
      while(tmp_model->parent){
	tmp_model = tmp_model->parent;
	anc_list[tmp_model->level] = tmp_model;
      }
    }

    /* go up the model_list and write a 'for{...}'/'let ...' combination
       for each model on the list */

    /* change extension of current model name to ".crd" */
    n = strlen(buffer);buffer[n-4] = 0;//strcat(buffer, ".crd");

    for(k=1;k<=this_model->level;k++){
      AmplModel *tmp_model = anc_list[k];
      model_comp *node = tmp_model->node; /* the node corresponding to model */
      opNode *ix = node->indexing; /* the indexing expression */
      opNode *set; /* the set that is indexed over */
      opNode *dummyVar; /* the dummy var of the indexing expression */

      /* Need to analyse the indexing expression to get the set that is
	 indexed over 
	 (I guess this should be done by an "indexing" object) */
      
      /* remove outside braces from indexing expression */
      if (ix->opCode==LBRACE) ix = (opNode*)ix->values[0];
      /* assumes that the next level is the 'IN' keyword (if present) */
      if (ix->opCode==IN){
	dummyVar = (opNode*)ix->values[0];
	set = (opNode*)ix->values[1];
      }else{
	dummyVar = NULL;
	set = ix;
      }

      // FIXME: how to deal with multidimensional dummy variables?
      //        need to work out the dimenension of a set?
      //        => for now simply use the original dummy variable
      //        => REQUIRE a dummy variable in block indexing
      if (!dummyVar){
	printf("Indexing expressions for block's need dummy variables!\n");
	printf("   %s",(node->indexing)->print());
	exit(1);
      }
      //for(j=1;j<k;j++) fprintf(fscript,"  "); /* prettyprinting */
      //fprintf(fscript, "for {i%d in %s }{\n", k, print_opNode(set));
      //for(j=1;j<=k;j++) fprintf(fscript,"  ");
      //fprintf(fscript, "let %s_SUB := {i%d};\n", print_opNode(set), k);
      for(j=1;j<k;j++) fprintf(fscript,"  "); /* prettyprinting */
      fprintf(fscript, "for {%s in %s }{\n", dummyVar->print(), print_opNode(set));
      for(j=1;j<=k;j++) fprintf(fscript,"  ");
      fprintf(fscript, "let %s_SUB := {%s};\n", print_opNode(set), dummyVar->print());
    }

    /* FIXME: still need to take the "print card()" statement from the
       bit below and add it here. 
       Unclear what that really needs to do:
        - Write out all indexing sets? Just their cardinality? 
	Or the whoroole set? Whole set might be nice so that we can refer
	  to submodels to their "proper" AMPL-name. 
	- These indexing sets might need to be subscripted as well:
	  COMM might be different for each incarnation of root_MCNF
        - Should take the chance and get AMPL to write out all the information
	  we need here
    */

    /* If the current model (i) has children, then write out the size
       of its childrens indexing epression to disk */
    if (this_model->n_submodels>0){
      model_comp *mc = this_model->first;
      while(mc->next && mc->type!=TMODEL) mc = mc->next;
      if (mc->type==TMODEL){
	/* found a submodel */
	AmplModel *submodel = (AmplModel*)mc->other;
	opNode *ix = mc->indexing;
	opNode *set;

	set = ix->getIndexingSet();
	
	// the name of the *.crd file is the global name of the submodel
	// with all the current values of loop variables up to
	// this level attached
	
	// so buffer here should be name of current model
	int len = strlen(buffer);
	int os;
	char *p = buffer+len;
	char *p1;
	os = sprintf(p, "_%s", submodel->name);
	p1 = p+os;
	// for all levels from root up to here add the value of 
	// indexing variables
	

	os = sprintf(p1, "\"");
	p1 += os;
	for(k=1;k<=this_model->level;k++){
	  // get all the indexing variables and add them together (joined by &)
	  opNodeIx *ixn = (anc_list[k]->node)->indexing;
	  list<char *>* dvl = ixn->getListDummyVars();
	  char buffer2[50], *p=buffer2; // assume that this hides the global p
	  int os2;
	  for(list<char *>::iterator q=dvl->begin();q!=dvl->end();q++){
	    os2 = sprintf(p, "%s&",*q);
	    p+=os2;
	  }
	  p--;p[0]=0; // delete the last '&'
	  

	  os = sprintf(p1, "&\"_\"&%s", buffer2);
	  p1+=os;
	}
	sprintf(p1, "&\".crd\"");
	printf("name of file is: %s\n",buffer);

	// Need to write something like
 	//print card(indexing expression) > exact_model_name.&1.&2.txt
	// or whatever it takes to get ampl to produce filenames
	// of the form stub_1.2.crd
	// where 1, 2 are  the values of the iteration indices in the 
	// scriptfile
	fprintf(fscript, "print card(%s) > (\"%s);\n", print_opNode(set), buffer);
	sprintf(p1, "&\".set\"");
	fprintf(fscript, "display %s > (\"%s);\n", print_opNode(set), buffer);


	*p=0; //delete the extension again	       
      }
    }
    

    /* write the main part of the submodel generation part in the scripts */
    for(j=1;j<k;j++) fprintf(fscript,"  ");fprintf(fscript, "fix all;\n");
    for(j=1;j<k;j++) fprintf(fscript,"  ");fprintf(fscript, "unfix all;\n");
    
    /* take the .mod suffix away from buffer */
    //n = strlen(buffer);buffer[n-4] = 0;
    for(j=1;j<k;j++) fprintf(fscript,"  "); /* prettyprinting */
    fprintf(fscript, "write (\"b%s\"",buffer);
    for(k=1;k<=this_model->level;k++) {
      // get all the indexing variables and add them together (joined by &)
      opNodeIx *ixn = (anc_list[k]->node)->indexing;
      list<char *>* dvl = ixn->getListDummyVars();
      char buffer2[50], *p=buffer2; // assume that this hides the global p
      int os2;
      for(list<char *>::iterator q=dvl->begin();q!=dvl->end();q++){
	os2 = sprintf(p, "%s&",*q);
	p+=os2;
      }
      p--;p[0]=0; // delete the last '&'
      
      fprintf(fscript, "&\"_\"&%s",buffer2);
    }
    fprintf(fscript, ");\n");
    
    /* and close all the brackets (prettyprinting) */
    for(k=this_model->level;k>0;k--){
      for(j=1;j<k;j++) fprintf(fscript,"  ");
      fprintf(fscript, "}\n");
      //rem_from_index_stack();
    }
    
    /* write the submodel file */
    //n = strlen(buffer);buffer[n-4] = 0;
    strcat(buffer, ".mod");
    fout = fopen(buffer, "w");
    printf("Write to model file: %s\n", buffer);
    write_ampl_for_submodel(fout, model_list[n_models-1], model_list[i]);
    fclose(fout);

    /* FIXME: this looks redundant */
    /* and also write the column file for this submodel */
    n = strlen(buffer);buffer[n-4] = 0;strcat(buffer, ".acl");
    fout = fopen(buffer, "w");
    write_columnfile_for_submodel(fout, model_list[i]);
    fclose(fout);
    free(anc_list);
  }

  fclose(fscript);

  /* FIXME: while we are at it, the AMPL scripts should write out the
     cardinality of the involved indexing sets and write these
     somewhere in a form that can be retrieved by whatever routine
     generates the algebra tree. Possible ideas:
     - write a file with
        [name of block] [number of entries]
       lines that can be scanned. These lines could be generated fairly easily
       in the above script. No problem with lines generated multiple times
     - have additional scripts just for the purpose of getting at the 
       cardinality numbers
  */


  /* 4) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> build algebra tree */
  
  model->print();

  ExpandedModel *em = new ExpandedModel(model);
  em->print();

  SML_OOPS_driver(em);
  
}

int count_models_(AmplModel *model)
{
  int i, count = 0;
  model_comp* comp;
  if (model->n_submodels>0){
    comp = model->first;
    for(i=0;i<model->n_total;i++){
      if (comp->type==TMODEL)
	count += count_models_((AmplModel*)comp->other);
      comp = comp->next;
    }
  }
  count += 1; /* also count this model */

  return count;
}

/* ---------------------------------------------------------------------------
fill_model_list 
---------------------------------------------------------------------------- */
void
fill_model_list_(AmplModel *model, AmplModel **list, int *pos)
{
  /* fill_model_list:
     recursively creates a depth first list of all the models in the
     model tree 
     Also sets the model->level attribute of all models in the tree
     ASSUMES: that the ->level of the initial model is already set

     IN: 
       AmplModel *model:          The start node (usually root)
       AmplModel **list:          The list in which models are added
       int *pos:                   Position at which to enter the next model
     OUT: 
       list
       model->level
                                                                            */
  int i;
  model_comp *comp;

  /* this works by first adding the current model to the list and then
     all its submodels. 
     It used to be that the submodels are added first, however then the
     level-calculation does not work */

  list[*pos] = model;
  if (model->parent) model->level = model->parent->level+1;
  (*pos)++;

  if (model->n_submodels>0){
    comp = model->first;
    for(i=0;i<model->n_total;i++){
      if (comp->type==TMODEL)
	fill_model_list_((AmplModel*)comp->other, list, pos);
      comp = comp->next;
    }
  }
  
}

/* ---------------------------------------------------------------------------
write_ampl_for_submodel
--------------------------------------------------------------------------- */
/* This is task 2: writing the ampl model for a given submodel
   
   It does the following tasks:
    - (at the moment): all set/param/var definitions above the given 
      submodel are copied verbatim
      (This could be done by using a dependency graph)
    - The declaration of the current subblock:
      - indexing 'i in ARCS' changed to 'i in ARCS_SUB'
        this indexing is added as an extra (first) paremeter to all
	entities declared within this block
    - all names of var/subject to/param/set entities declared within a block
      are changed to a global name by pre-pending the name of the block to
      the entity name (to avoid local/global naming problems)
      
    - sister blocks and subblocks of the current block:
      var/set/param declarations are copied, constraints not

*/
void write_ampl_for_submodel_(FILE *fount, int thislevel, int sublevel, 
			      AmplModel **list, AmplModel *submodel);
void modified_write(FILE *fout, model_comp *comp);

void
write_ampl_for_submodel(FILE *fout, AmplModel *root, AmplModel *submodel)
{
  AmplModel *list[5];  /* assume models are not nested further than
			   5 levels */
  int i, level;
  
  opNode::use_global_names = 1;
  n_addIndex = 0; // clear addIndex stack

  printf("================================================================\n");
  printf("     ampl model for part: %s\n",submodel->name);
  printf("================================================================\n");
  

  /* need list of models at different levels from here up to root */
  {
    AmplModel *tmp;
    
    list[0] = submodel;
    tmp = submodel;
    level = 0;
    while(tmp->parent){
      tmp = tmp->parent;
      level++;
      list[level] = tmp;
    }
  }
  printf("-> this model is on level %d\n", level);
  printf("   Levels from top are: \n");
  for(i=0;i<=level;i++){
    printf("%d: %s\n",i, list[i]->name);
  }
  
  /* mark all model components that are needed by the current model */
  model_comp::untagAll();
  /* and loop through all model_components and mark it and its 
     dependencies as needed */
  
  for(model_comp *thism=submodel->first;thism!=NULL;thism=thism->next){
    thism->tagDependencies();
    //printf("processing %s\n", thism->id);
    //printf("-------> tagged now\n");
    //model_comp::writeAllTagged();
  }

  /* now start reporting the modified model recursively */
  
  write_ampl_for_submodel_(fout, level, 0, list, submodel);

  
  /* - indexing 'i in ARCS' changed to 'i in ARCS_SUB'
    
  This needs further clarification: The indexing expression can have several
  forms, which are changed accordingly
   - {ARCS} : just a name, no 'in' statement
      => simply replaced by ARCS_SUB 
   - {i in ARCS}: 
      => replaced by 'i in ARCS_SUB'
   - {ARCS diff i}: more complex setdefinition
      => set MCNF_IDX = ARCS diff i;
         all entities  indexed by {MCNF_IDX_SUB}
   - {i in ARCS diff i}: more complex setdefinition
      => set MCNF_IDX = ARCS diff i;
         all entities  indexed by {i in MCNF_IDX_SUB}

   it is possibly sufficient to start implementing only the first two,
   keeping in mind that the extension might be wanted later
 
  */

  


} 


/* ---------------------------------------------------------------------------
write_ampl_for_submodel_
--------------------------------------------------------------------------- */
/* This routine does part fo the work in writing out an AMPL submodel file
   for the model given by 'submodel'.
   It is passed the list of ancestors of submodel (in 'list') and works
   down this list recursively. For an ancestor of the current model
   it writes out everything except the constraint definitions and submodel 
   declarations. For the current model it writes out everything.
   Global names are used throughout.
 
   This routine recursively works on the model given by list[level] and
   writes the model definition out.
   
   It ignores all submodel definitions that are not on the path to the 
   'submodel' (i.e. which are not listed on the 'list').
   
   It does special treatment for the model in question (the 'submodel')
   
   IN: 
     FILE *fout             the file where the model definition should
                            be written to
     int thislevel          level of the current node (root=0)
     int sublevel           NOT USED!
     AmplModel[] *list     list[0] is current model, list[thislevel] is root
     AmplModel *submodel   the current model again
   OUT: none (output on data file)


   It keeps track of the stack of block indexing expressions, by putting them
   onto the l_addIndex stack

*/
void
write_ampl_for_submodel_(FILE *fout, int thislevel, int sublevel, 
			 AmplModel **list, AmplModel *submodel)
{
  AmplModel *thism = list[thislevel];
  int i, j;
  model_comp *comp;
  
  opNode::default_model = thism;
  comp = thism->first;

  // loop through all entities in this model 
  for(i=0;i<thism->n_total;i++)
    {
      if (comp->type!=TMODEL){
	// if it is not a model declaration, simpy write the declaration out
	if (comp->type!=TCON || thism==submodel)
	  modified_write(fout, comp);
      }else{
	/* this is a model declaration */
	/* check that it needs to be followed */
	if (thism!=submodel && list[thislevel-1]==comp->other){
	  opNode *ix;
	  /* ok, looks like this needs to be followed */
	  /* add the indexing expression */
	  
	  // initialise the new entry in l_addIndex stack if not already done
	  if (l_addIndex[n_addIndex]==NULL){
	    l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
	  }
	
	  // and place the indexing expression of this BLOCK onto the stack
	  ix = comp->indexing;
	  // the stack stores the dummy variable and the SET separately, 
	  // so take them to bits here
	  if (ix->opCode==LBRACE) ix = (opNode*)ix->values[0]; // rem {..}
	  if (ix->opCode==IN){
	    l_addIndex[n_addIndex]->dummyVar = (opNode*)ix->values[0];
	    l_addIndex[n_addIndex]->set = (opNode*)ix->values[1];
	  }else{ // no dummy variable, just a set
	    l_addIndex[n_addIndex]->dummyVar = NULL;
	    l_addIndex[n_addIndex]->set = ix;
	  }
	  
	  /* okay we have placed the set description on the stack
	     but really this should be modified:
	     'i in ARCS' should read 'i in ARCS_SUB'

	     ought to 'write set ARCS_SUB within ARCS'; as well
	  */
	  {
	    opNode *setn = l_addIndex[n_addIndex]->set;
	    opNode *newn = new opNode();
	    model_comp *tmp;
	    char *newname; 
	    fprintf(fout, "set %s_SUB within %s;\n", 
		   print_opNode(setn), print_opNode(setn));
	    /* and now modify the set declaration */
	    if (setn->opCode!=IDREF){
	      printf("At the moment can index blocks only with simple sets\n");
	      exit(1);
	    }
	    // FIXME: rewrite this code by
	    //  - using the opNodeIDREF subclass
	    //  - using the opNodeIDREF::clone() method
	    /* newn is the new node, first copy the old one */
	    newn->opCode = IDREF;
	    newn->nval = setn->nval;
	    newn->values = (void **)calloc(setn->nval+1, sizeof(void *));
	    for(j=0;j<setn->nval;j++) newn->values[j+1] = setn->values[j+1];
	    // clone the model_comp that is referred to
	    newn->values[0] = (model_comp *)calloc(1,sizeof(model_comp));
	    memcpy(newn->values[0], setn->values[0], sizeof(model_comp));
	
	    /* and finally set the new name (add _SUB) to the name */
	    tmp = (model_comp*)newn->values[0];
	    newname = (char *)calloc(strlen(tmp->id)+5, sizeof(char));
	    strcpy(newname, tmp->id);
	    strcat(newname, "_SUB");
	    tmp->id = newname;
	    /* and put this on the stack */
	    l_addIndex[n_addIndex]->set = newn;
	  }	     
	  n_addIndex++;
	  write_ampl_for_submodel_(fout, thislevel-1, sublevel, list, 
				   submodel);
	  opNode::default_model = thism;
	  n_addIndex--;
	} /* end of (model on the current list branch) */
	else if (thislevel==0) {
	  // this is the current model: 
	  // processing a definition of a child block of the current block
	  // => write out everything that is tagged
	  AmplModel *childm = (AmplModel *)comp->other;
	  
	  //printf("\n\n-----------------------------------------------\n");
	  //printf("  current model: %s\n",submodel->name);
	  //printf("  these are the components needed from children:\n");
	  childm->writeTaggedComponents(fout);
	  //printf("-----------------------------------------------\n\n");

	  // For normal model components we simply call modified_write
	  // on these components
	  // => what does modified_write depend on?
	}

      } /* end else (end of the TMODEL branch) */

      comp = comp->next;
    }
  
}

/* --------------------------------------------------------------------------
write_ampl_for_submodel2
--------------------------------------------------------------------------- */
/* write the ampl file for the submodel (block) pointed to by submodel 
   
   IN:
     fout:       The file to write to
     root:       the root node of the model tree
     submodel:   the current submodel

   This routine uses the following rule:
     - loop through all the components in the current model
     - mark those (and all their dependencies) for inclusion
     - all model definition lines up to the current model need to be 
       included
     - write them all out to file in their 'natural' order (i.e. the order
       used in the original model file):
       + all model definition lines are replaced by
         set ARCS_SUB with ARCS   (if line was 'block MCNF{ARCS}'
       + all names are replaced by their global names
         (i.e. subproblem path prepended)
       + the argument list gets the dummy variables of its parent blocks 
         prepended:
          i.e. Flow[l,k] within RouteComm{j in COMM} becomes
               RouteComm_Flow[j,l,k]
       + This also holds for the definition of VARS/SETS
       + all references to sets that index subproblems are replaced by
         the name_SUB
   Implementation:
     - Every submodel has its path (i.e. MCNF_RouteComm) and its additional
       indexing (i.e. 'j in COMM'). These can be stored in the submodel class
     
*/
void
write_ampl_for_submodel2(FILE *fout, AmplModel *root, AmplModel *submodel)
{

}

/* --------------------------------------------------------------------------
write_columnfile_for_submodel
--------------------------------------------------------------------------- */
/* this routine just goes through the subproblem definition and prints out the
   global name of all variables defined in this subproblem
   
   Not sure at the moment what to do about indexing 
   probably just leave the stub of the variable name here and add the 
   index later (indices would have to be given as numbers, here we
   do not know the size of the set indexed over

*/

void
write_columnfile_for_submodel(FILE *fout, AmplModel *submodel)
{
  model_comp *comp;
  int i;

  comp=submodel->first;
  for(i=0;i<submodel->n_total;i++){
    if (comp->type==TVAR){
      /* print global name here: 
	 either just prefix all model names up to here by a loop, or
	 use the global name printing routine used elsewhere (but I
	 think that also puts indexing expressions on it               */

      /* the NOARG version of getGlobalName should do the trick */
      fprintf(fout, "%s\n", getGlobalName(comp, NULL, NULL, NOARG));
    }
    comp = comp->next;
  }


}

/* --------------------------------------------------------------------------
add_to_index_stack
--------------------------------------------------------------------------- */
/**
 * This routine takes an indexing expression given by its opNode, splits
 * it into the dummy variable/set expression (by the IN keyword)
 * and adds both items to the l_addIndex list
 *
 * \param ix The indexing expression to add to the stack
 * \return  modifies l_addIndex and n_addIndex
 * \deprecated Is never used. Adding expressions on the stack is done in 
 *      write_ampl_for_submodel_() directly
 */
void
add_to_index_stack(opNode *ix){
  if (l_addIndex[n_addIndex]==NULL){
    l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
  }
  /* remove outside braces from indexing expression */
  if (ix->opCode==LBRACE) ix = (opNode*)ix->values[0];
  /* assumes that the next level is the 'IN' keyword (if present) */
  if (ix->opCode==IN){
    l_addIndex[n_addIndex]->dummyVar = (opNode*)ix->values[0];
    l_addIndex[n_addIndex]->set = (opNode*)ix->values[1];
  }else{
    l_addIndex[n_addIndex]->dummyVar = NULL;
    l_addIndex[n_addIndex]->set = ix;
  }
  n_addIndex++;
}

void 
rem_from_index_stack(){
  if (n_addIndex>0){
    n_addIndex--;
  }else{
    printf("Attempting to remove item from l_addIndex list when the list is emply\n");
    exit(1);
  }
}

/* ---------------------------------------------------------------------------
modified_write
--------------------------------------------------------------------------- */
/* writes out a component of a model 
   
  components can be modified: if this is down into a submodel, then
   - all declarations get new indexing expressions appended to it
   - all references to entities get new subscripts attached to it.
  IN: 
    FILE *fout:          The file to write to
    model_comp *comp:    The component definition to write out
  DEP: 
    n_addIndex/l_addIndex: currently applicable indexing expresssions
    
  EFF: 
    prints the global definition of the given model_component to the given 
    file.

  1) get the global name of the model component
  2) prepend all indexing expressions on the stack to the indexing expression
     of this entity
  3) for all components that are referenced in the definition
    3a) use their global name
    3b) prepend the dummy variables for all indexing expressions on the stack
        to the argument list

  part 3) is simply done by a call to (comp->attributes)->print();
  (with opNode::use_global_names set to true 
    => the argument list version of getGlobalName is called)  

*/
void
modified_write(FILE *fout, model_comp *comp)
{
  int i;
  opNode *ixsn;

  if (comp->type!=TMODEL){
    int first=1;
    /* start statement */
    fprintf(fout, "%s ",compTypes[comp->type]);
    
    /* write name and indexing expression */
    fprintf(fout, "%s ",getGlobalName(comp, NULL, NULL, NOARG));
    if (n_addIndex>0 || comp->indexing)
      fprintf(fout, "{");
    /* write out the additional indexes */
    for(i=0;i<n_addIndex;i++){
      add_index *ix = l_addIndex[i];
      if (first){first = 0;}else{fprintf(fout, ",");}
      if (ix->dummyVar){
	fprintf(fout, "%s in ",print_opNode(ix->dummyVar));
      }
      ixsn = ix->set;
      if (ixsn->opCode==LBRACE) ixsn=(opNode*)ixsn->values[0]; 
      fprintf(fout, "%s",print_opNode(ixsn));
    }
    /* write out the index of this component */
    if (comp->indexing) {
      if (first){first = 0;}else{fprintf(fout, ",");}
      ixsn = comp->indexing;
      if (ixsn->opCode==LBRACE) ixsn=(opNode*)ixsn->values[0]; 
      fprintf(fout, "%s", print_opNode(ixsn));
    }
    if (n_addIndex>0 || comp->indexing)
      fprintf(fout, "}");
    
    /* write out special syntax for a particular statement type */
    if (comp->type==TCON||comp->type==TOBJ) fprintf(fout, ":");
    
    /* write out the rest of the expression */
    fprintf(fout, "%s;\n",(comp->attributes)->print());
    //fprintf(fout, "%s;\n",print_opNode(comp->attributes));
    //fprintf(fout, "%s;\n", print_opNodesymb(comp->attributes));
  }
}
