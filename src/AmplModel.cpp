#include "ampl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include "backend.h"
#include "ExpandedModel.h"
#include "AmplsolverCalls.h"

extern void modified_write(FILE *fout, model_comp *comp);

extern int n_addIndex;
extern add_index *l_addIndex[];
void
addCompToModel(AmplModel *model, model_comp *comp)
{
  AmplModel *subm;
  model_comp *lastinmodel = model->last;
  switch(comp->type)
    {
    case TVAR:
      model->n_vars++;
      break;
    case TCON:
      model->n_cons++;
      break;
    case TSET:
      model->n_sets++;
      break;
    case TPARAM:
      model->n_params++;
      break;
    case TOBJ:
      model->n_objs++;
      break;
    case TMODEL:
      model->n_submodels++;
      subm = (AmplModel*)comp->other;
      subm->parent = model;
      break;
    }
  model->n_total++;
  comp->model = model;
  comp->next = NULL;
  comp->prev = lastinmodel;
  
  if (model->first==NULL){
    model->first=comp;
    model->last = comp;
  }else{
    model->last = comp;
    lastinmodel->next = comp;
  }
}

/** Constructor */
AmplModel::AmplModel() :
  n_vars(0),
  n_cons(0),
  n_params(0),
  n_sets(0),
  n_objs(0),
  n_submodels(0),
  n_total(0),
  level(0),
  first(NULL),
  last(NULL),
  parent(NULL),
  ix(NULL) {}

/* ---------------------------------------------------------------------------
AmplModel::setGlobalName()
---------------------------------------------------------------------------- */
void 
AmplModel::setGlobalName()
{
  if (parent==NULL){
    global_name = "";
  }else if (strcmp(parent->name,"root")==0){
    global_name = string(name);
  }else{
    global_name = parent->global_name + "_" + string(name);
  }
  //cout << "\n\ndefined model: " + global_name + "\n\n"; 
}

/* ---------------------------------------------------------------------------
AmplModel::writeTaggedComponents()
---------------------------------------------------------------------------- */
void AmplModel::writeTaggedComponents(){writeTaggedComponents(stdout);}

void
AmplModel::writeTaggedComponents(FILE *fout)
{

  ix = node->indexing;

  if (!ix->done_split) {
    printf("All opNodeIx should have the expression split done!\n");
    exit(1);
  }
  
  if (ix->ncomp>1){
    printf("More than 1 indexing expression is not supported yet\n");
    printf("Expression: %s\n",ix->print());
  }
  // FIXME: need to check for number of expressions and place them all
  // onto the stack

  //Place the indexing expression of the current model onto the addIndex stack 
  // first make sure that there is space
  if (l_addIndex[n_addIndex]==NULL){
    l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
  }

  l_addIndex[n_addIndex]->dummyVar = ix->dummyVarExpr[0];
  l_addIndex[n_addIndex]->set = ix->sets[0];
  n_addIndex++;
  
  // loop through all model components
  for (model_comp *c = first;c!=NULL;c=c->next){
    //if (c->tag) printf("%s\n",c->id);
    if (c->tag) modified_write(fout, c);
    if (c->type==TMODEL) {
      AmplModel *am = (AmplModel *)c->other;
      am->writeTaggedComponents();
    }
  }
  n_addIndex--;

}

/* ---------------------------------------------------------------------------
AmplModel::createExpandedModel
---------------------------------------------------------------------------- */
char *crush(const char *inst);
list<string> *getListOfInstances(string set);

/** This method is given a location in the Expanded tree and the correspoding
 *  node of the AmplModel tree and creates recursively the ExpandedModel 
 *  instances corresponding to it.
 *  The location in the ExpandedTree to be created is encoded in two 
 *  strings. One gives the location in the flat AmplModel tree, the other
 *  gives the instance of the node. 
 *
 *  @param smodelname       Name of model in flat model tree 
 *                          (this is identical to this->global_name?)
 *  @param sinstanceStub    The instance that should be created.
 *                          Concatenating of the instances up to here:
 *       INS1_INS2... where INSn is the value of the indexing variable at
 *       the n-th level.
 *  
 *  The expanded tree is created by concatenating of strings and reading
 *  the instance names of the nodes in the next level from the corresponding
 *  *.set file.
 *
 *  @attention It seems that the first parameter (name of flat model tree 
 *             node is redundant, since this could be obtained from the  
 *             current AmplModel instance.
 */
ExpandedModel*
AmplModel::createExpandedModel(string smodelname, string sinstanceStub)
{
  AmplModel *ampl=this;
  /* the route down the AmplModel tree is encoded in modelname:
         root_MOD1_MOD2 
     the instance is encoded in instancestub 
         INS1_INS2
     both together root_MOD1_MOD2_INS1_INS2.nl gives the name of the *.nl file

     for the submodel MOD3 of this node, the file
         root_MOD1_MOD2_MOD3_INS1_INS2.crd 
     gives the cardinality of this submodel. The submodel instances can be 
     obtained by either looking at the possible filename completions in the
     directory, or by looking at the set
         root_MOD1_MOD2_MOD3_INS1_INS2.set
  */ 
 
  /* - need to build this up by recursively going through the new tree
       => Can not use a routine that returns a list of ExpandedModel nodes

     - need to encode the current position on the ExpandedModel tree, this
       is given by
       + The approprite AmplModel file
       + The path up to here 
          (this is either a filename stub, encoding the AmplModel tree place 
           and the instance name at each level OR
           the AmplModel and a list of instance numbers, the names could then
           be obtained from the SETs (in the *.set files))
     - The trouble with doing anything clever/elegant is that the only 
       information on cardinality/names for the ExpandedModel tree is
       in the files on the disk (either in the filenames or in the content)

     - if we use the filename as 
  */
  if (strcmp(name,"root")==0){
    ExpandedModel::pathToNodeStack.clear();
  }


  ExpandedModel *em = new ExpandedModel();
  
  // copy information on the constraints/variables?
  
  em->model_file = smodelname;
  if (sinstanceStub!="") em->model_file += "_"+sinstanceStub;
  em->nlfile = new NlFile(em->model_file);
  // Open this with AMPL interface and check which variables are
  // part of this model? I guess we need to do this
  // No, just need to look at the *.col file (produced by the 'write b' 
  // command) and run a check on that.
  // What do we need to compare against? Is the stub of the variables enough?
  // Can we just run a regex match (or even simpler a match of the start of the
  // variable name? Or do we need to go into checking the correct indices.
  // => Try this on paper. root_MCNF includes Flow variables do we include
  //    all or just a subset (I guess the Flow variables are not actually local
  //    this however becomes an issue for the OOPS view
  // => write down all nodes in the tree. Vars/Constraints in each *.nl file
  //    and which ones belong to which node on both the Benders and OOPS trees

  // I assume that *all* constraints declared in the *.nl file are part of 
  // the node
  
  for(model_comp *mc = ampl->first;mc!=ampl->last;mc=mc->next){
    if (mc->type == TVAR){
      // need to translate this into the global name of the variable, tgether
      // with any applicable indexing expression
      string globname(getGlobalName(mc, NULL, NULL, NOARG));
      // and add all applicable indexing to it
      // if this is on the branch (N1,N0)->C0 then the variable indexing will
      // start with
      // ['N1','NO','CO']
      // In order to recreate this we will need to somewhere store the actual
      // path of set elements that was taken to get here
      // => Can be done by a gobal stack associated with ExpandedModel
      //    (similar to addIndex, but this time a C++ implementation
      //printf("Stack of path instances is: \n");
      for(list<string>::iterator p = ExpandedModel::pathToNodeStack.begin();
	  p!=ExpandedModel::pathToNodeStack.end();p++){
	// separator is '[' for first item, ',' thereafter
	if (p==ExpandedModel::pathToNodeStack.begin()){
	  globname += '[';
	}else{
	  globname += ',';
	}
	// the expression is (N1,N2) or N1 
	// if a simple expression (no '(') just put it to the end of 
	// globname quoted
	// otherwise, need to break this down into tokens
	// this is conceptually the same as done in breaking down the opNodeIx
	// indexing nodes for the blocks. However this time it is actual
	// instances of the dummyVariables rather than the dummy variables 
	// themselves. The expressions come from AMPL output so they have not
	// been parsed by the lexer/yacc. Need to do te parsing by hand
	if ((*p).at(0)=='('){
	  char *token = strdup((*p).c_str());
	  // remove first and last character
	  int len = strlen(token);
	  assert(token[len-1]==')');
	  token[len-1]=0;
	  token++;
	  for(char*str=token; ;str=NULL){
	    char *tok = strtok(str, ",");
	    if (tok==NULL) break;
	    globname += "'"+string(tok)+"',";
	  }
	  globname.replace(globname.size()-1,1,"");// delete last element
	  token--;
	  free(token);
	}else{
	  globname += "'"+(*p)+"'";
	}
	//printf("->%s",(*p).c_str());
      }
      //printf("\n");
      cout << globname+'\n';
      em->localVarDef.push_back(globname);

    }
    if (mc->type == TMODEL){
      int card;
      string setElements;
      // okay this is a submodel declaration:
      // - work out the cardinality
      // - work out the set and the names of the subproblem instancestub
      // - create the ExpandedModel nodes by calling this routine
      
      string nameSetFile = smodelname+"_"+string(mc->id);
      if (strlen(sinstanceStub.c_str())>0) nameSetFile += "_"+sinstanceStub;
      
      // get cardinality of this node in the ExpandedModel
      ifstream fcrd((nameSetFile+".crd").c_str());
      if (!fcrd) {
	cout << "Cannot open file: "+ nameSetFile+".crd\n";
	exit(1);
      }

      ifstream fset((nameSetFile+".set").c_str());
      if (!fcrd) {
	cout << "Cannot open file: "+ nameSetFile+".set\n";
	exit(1);
      }

      fcrd >> card;
      getline(fset, setElements); // FIXME: can this be more than one line?

      list<string>* li = getListOfInstances(setElements);
      for(list<string>::iterator p=li->begin();p!=li->end();p++){
	string subModelName = smodelname+"_"+string(mc->id);
	string subModelInst;
	if (strlen(sinstanceStub.c_str())>0) subModelInst = sinstanceStub+"_";
	subModelInst += crush((*p).c_str());
	cout << subModelName+":"+subModelInst+'\n';
	AmplModel *subampl = (AmplModel*)mc->other;
	ExpandedModel::pathToNodeStack.push_back(*p);
	ExpandedModel *subem = subampl->createExpandedModel(subModelName, subModelInst);
	ExpandedModel::pathToNodeStack.pop_back();
	subem->parent = em;
	(em->children).push_back(subem);
      }
      

    }
  }
  
  return em;
}



/* ---------------------------------------------------------------------------
getListOfInstances
--------------------------------------------------------------------------- */
/* Helper routine: read the set defining statement from the given string
   extract all the set elements, crush them if mutidimensional and
   return them in a list   */
list<string> *
getListOfInstances(string set)
{
  list<string> *li = new list<string>;

  // I do this in c style

  char *buffer = strdup(set.c_str());
  //printf("Set definition is %s\n",buffer);
  
  buffer = strstr(buffer, ":=");
  if (buffer==NULL){
    printf("Set definition must contain a ':='!\n  %s\n",set.c_str());
    exit(1);
  }
  buffer += 3; // step past the ':= '
  // final char should be ';'
  int len = strlen(buffer);
  assert(buffer[len-1]==';');
  buffer[len-1]=0; // delete it

  // now the rest of the string is tokenized into the set elements
  for(char *str1=buffer; ;str1=NULL){
    char *svptr1;
    char *token = strtok_r(str1, " ", &svptr1);
    if (token==NULL) break;
    // token now points to a set element
    if (token[0]=='('){
      // this is a multidimensional element
      //printf("Multidimensional token found: %s\n",token);
      // remove front and back
      li->push_back(string(token));
    }else{
      li->push_back(string(token));
    }
  }

  return li;
}


/* ---------------------------------------------------------------------------
crush
---------------------------------------------------------------------------- */
/* helper routine: crushes a set element (N0,N1) into N0N1 */
char *crush(const char *inst)
{
  if (inst[0]!='(') return strdup(inst);
  char *token = strdup(inst);
  int len = strlen(token);
  assert(token[len-1]==')');
  token[len-1]=0;
  token++;
  // now tokenize again by ',' and crush all parts together
  string crush;
  for(char*str2=token; ;str2=NULL){
    char *svptr2;
    char *tok2 = strtok_r(str2, ",", &svptr2);
    if (tok2==NULL) break;
    crush += string(tok2);
  }
  //printf("Crushed mutidim token is: %s\n",crush.c_str());
  
  token--;
  free(token);
  return strdup(crush.c_str());
}

/* ---------------------------------------------------------------------------
AmplModel::print
---------------------------------------------------------------------------- */
void 
AmplModel::print()
{

  printf("AM: ------------------------------------------------------------\n");
  printf("AM: This is AmplModel: %s\n",name);
  printf("AM: global name: %s\n",global_name.c_str());
  printf("AM: level: %d\n",level);
  printf("AM: parent: %s\n",(parent)?parent->name:"NULL");
  printf("AM: indexing: %s\n",ix->print());
  printf("AM: Nb submodels  : %d\n", n_submodels);
  printf("AM: Nb sets       : %d\n", n_sets);
  printf("AM: Nb parameters : %d\n", n_params);
  printf("AM: Nb objectives : %d\n", n_objs);
  printf("AM: Nb variables  : %d\n", n_vars);
  printf("AM: Nb constraints: %d\n", n_cons);
  printf("AM: Nb objectives: %d\n", n_submodels);
  printf("AM: Entities declared:\n");
  for(model_comp *mc = first;mc;mc=mc->next){
    mc->printBrief();
  }

  if (n_submodels>0)
    printf("AM: now list the submodels:\n");
  
  for(model_comp *mc = first;mc;mc=mc->next){
    if (mc->type == TMODEL){
      AmplModel *am = (AmplModel*)mc->other;
      am->print();
    }
  }
}
