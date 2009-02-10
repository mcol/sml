#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include "AmplModel.h"
#include "backend.h"
#include "ExpandedModel.h"
#include "AmplsolverCalls.h"
#include "nodes.h"
#include "sml.tab.h"

list<changeitem*> AmplModel::changes; //initialize to empty list
AmplModel *AmplModel::root = NULL; //initialize root to NULL

extern void modified_write(ostream &fout, ModelComp *comp);
//extern int n_addIndex;
//extern add_index *l_addIndex[];

// Utility function
bool is_int(const char *tok); // 'const' here means that tok is not modified

/* ---------------------------------------------------------------------------
AmplModel::AmplModel()
---------------------------------------------------------------------------- */
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
  //first(NULL),
  //last(NULL),
  parent(NULL),
  ix(NULL) {}

AmplModel::AmplModel(const char *orig_name, AmplModel *par)
{
   AmplModel();

   name = strdup(orig_name);
   parent = par;

   setGlobalName();
}

AmplModel::~AmplModel()
{
   // FIXME:
   //free(name); // Allocated by strdup in constructor and needs freeing
}



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
AmplModel::setGlobalNameRecursive()
---------------------------------------------------------------------------- */
void 
AmplModel::setGlobalNameRecursive()
{
  ModelComp *mc;
  setGlobalName();
  
  // loop through all model components
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    mc = *p;
    if (mc->type==TMODEL){
      AmplModel *am = (AmplModel*)mc->other;
      am->setGlobalNameRecursive();
    }
    //mc = mc->next;
  }
}

/**
 *  Write all tagged model components in this model and submodels to a file.
 *
 *  @param fout:
 *         Name of the file to which to write. If not indicated, the writing
 *         will be directed to the standard output.
 */
void
AmplModel::writeTaggedComponents(ostream &fout)
{
  
  SyntaxNode::default_model = this;
  SyntaxNodeIx *ix = node->indexing;

  list <add_index*>*li = new list<add_index*>;
  if (ix){

    if (!ix->done_split) {
      cerr << "All SyntaxNodeIx should have the expression split done!\n";
      exit(1);
    }
    
    if (ix->ncomp>1){
      cerr << "More than 1 indexing expression is not supported yet\n";
      cerr << "Expression: " << ix->print() << "\n";
      exit(1);
    }
    // FIXME: need to check for number of expressions and place them all
    // onto the stack
    
    //Place the indexing expression of the current model onto the addIndex stack 
    // first make sure that there is space
    //if (l_addIndex[n_addIndex]==NULL){
    //  l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
    //}
    add_index *ai = (add_index*)calloc(1, sizeof(add_index));
    ai->dummyVar = ix->dummyVarExpr[0];
    ai->set = ix->sets[0];
    //    l_addIndex[n_addIndex]->dummyVar = ix->dummyVarExpr[0];
    //    l_addIndex[n_addIndex]->set = ix->sets[0];
    li->push_back(ai);
    //n_addIndex++;
  }
  l_addIndex.push_back(li);

  // loop through all model components
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    ModelComp *c = *p;
    //if (c->tag) printf("%s\n",c->id);
    if (c->tag) modified_write(fout, c);
    if (c->type==TMODEL) {
      AmplModel *am = (AmplModel *)c->other;
      am->writeTaggedComponents(fout);
    }
  }
  //  if (ix) n_addIndex--;
  l_addIndex.pop_back();

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
  
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    ModelComp *mc = *p;
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
        // this is conceptually the same as done in breaking down the SyntaxNodeIx
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
            if (is_int(tok)){
              globname += string(tok)+",";
            }else{
              globname += "'"+string(tok)+"',";
            }
          }
          globname.replace(globname.size()-1,1,"");// delete last element
          token--;
          free(token);
        }else{
          if (is_int((*p).c_str())){
            globname += (*p);
          }else{
            globname += "'"+(*p)+"'";
          }
        }
        //printf("->%s",(*p).c_str());
      }
      //printf("\n");
      //cout << globname+'\n';
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
      if (sinstanceStub.length() > 0)
        nameSetFile += "_" + sinstanceStub;
      
      // get cardinality of this node in the ExpandedModel
      ifstream fcrd((nameSetFile+".crd").c_str());
      if (!fcrd) {
        cout << "Cannot open file: "+ nameSetFile+".crd\n";
        exit(1);
      }

      fcrd >> card;

      // if this node is indeed repeated over an indexing set
      if (card>0){
        ifstream fset((nameSetFile+".set").c_str());
        if (!fset) {
          cout << "Cannot open file: "+ nameSetFile+".set\n";
          exit(1);
        }

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
      }else{
        // if this node is not repeated over an indexing set
        string subModelName = smodelname+"_"+string(mc->id);
        string subModelInst;
        cout << subModelName+":"+sinstanceStub+'\n';
        AmplModel *subampl = (AmplModel*)mc->other;
        ExpandedModel *subem = subampl->createExpandedModel(subModelName, sinstanceStub);
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
  if (ix){ 
    printf("AM: indexing: %s\n", ix->print().c_str());
  }else{
    printf("AM: indexing: NULL\n");
  }
  printf("AM: Nb submodels  : %d\n", n_submodels);
  printf("AM: Nb sets       : %d\n", n_sets);
  printf("AM: Nb parameters : %d\n", n_params);
  printf("AM: Nb objectives : %d\n", n_objs);
  printf("AM: Nb variables  : %d\n", n_vars);
  printf("AM: Nb constraints: %d\n", n_cons);
  printf("AM: Nb objectives: %d\n", n_submodels);
  printf("AM: Entities declared:\n");
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    printf("AM:   ");
    (*p)->printBrief();
  }

  if (n_submodels>0)
    printf("AM: now list the submodels:\n");
  
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    ModelComp *mc = *p;
    if (mc->type == TMODEL){
      AmplModel *am = (AmplModel*)mc->other;
      am->print();
    }
  }
}
/* ---------------------------------------------------------------------------
AmplModel::dump(char *filename)
---------------------------------------------------------------------------- */
void 
AmplModel::dump(char *filename)
{
  ofstream fout(filename);
  dump(fout);
}

/* ---------------------------------------------------------------------------
AmplModel::dump(ostream &fout)
---------------------------------------------------------------------------- */
void 
AmplModel::dump(ostream &fout)
{
  fout << "DUMP: ----------------------------------------------------------\n";
  fout << "DP: This is AmplModel (" << (void *) this << "): " << name << "\n";
  fout << "DP: global name: " << global_name << "\n";
  fout << "DP: level: " << level << "\n";
  fout << "DP: parent: " << (parent?parent->name:"NULL") << "\n";
  fout << "DP: indexing: " << ix << "\n";
  ix->dump(fout);
  fout << "DP: Nb submodels  : " <<  n_submodels << "\n";
  fout << "DP: Nb sets       : " <<  n_sets << "\n";
  fout << "DP: Nb parameters : " <<  n_params << "\n";
  fout << "DP: Nb objectives : " <<  n_objs << "\n";
  fout << "DP: Nb variables  : " <<  n_vars << "\n";
  fout << "DP: Nb constraints: " <<  n_cons << "\n";
  fout << "DP: Nb objectives: " <<  n_submodels << "\n";
  fout << "DP: List components:";
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    (*p)->dump(fout);
  }

  if (n_submodels>0)
    fout << "DP: now list the submodels:\n";
  
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    ModelComp *mc = *p;
    if (mc->type == TMODEL){
      AmplModel *am = (AmplModel*)mc->other;
      am->dump(fout);
    }
  }
}

/* ---------------------------------------------------------------------------
AmplModel::check()
---------------------------------------------------------------------------- */
/** Check consistency of the instance */
void
AmplModel::check()
{
  if (name==NULL){
    cerr << "AmplModel::check: AmplModel has no name given\n";
    exit(1);
  }
  if (global_name==""){
    cerr << "AmplModel has no global name set\n";
    exit(1);
  }
  if (node==NULL && strcmp(name,"root")!=0){
    cerr << "AmplModel " << name << " is not root but has no ModelComp "
       "node associated\n";
    exit(1);
  }
  
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_vars = " << n_vars << "\n";
    exit(1);
  }
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_cons = " << n_cons << "\n";
    exit(1);
  }
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_params = " << n_params << "\n";
    exit(1);
  }
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_sets = "<< n_sets << "\n";
    exit(1);
  }
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_objs = " << n_objs << "\n";
    exit(1);
  }
  if (n_vars<0){
    cerr << "AmplModel " << name << ": n_submodels = " << n_submodels << "\n";
    exit(1);
  }
  if (n_vars+n_cons+n_params+n_sets+n_submodels+n_objs!=n_total){
    cerr << "AmplModel " << name << ": n_total does not equal sum of comps\n";
    exit(1);
  }
  
  if (parent==NULL && strcmp(name,"root")!=0){
    cerr << "AmplModel " << name << " is not root but has no parent\n";
    exit(1);
  }
  
}


/* ---------------------------------------------------------------------------
AmplModel::addDummyObjective()
---------------------------------------------------------------------------- */
/** AMPL will remove variables from the model that are not used in any
 *  constraint/objective within the model. In order to prevent this,
 *  we need to add a dummy objective that uses every defined variable
 *  (by simply summing them up)
 *
 *  This routine creates a list of all variable declaration in the 
 *  model and creates a dummy objective function that uses them all
 */
void
AmplModel::addDummyObjective()
{
  ModelComp *newobj;
  vector<SyntaxNode*> list_on_sum;
  SyntaxNode *attr;
  ModelComp *comp;
  int i;

  // get list of all variable declarations in the model:
  /* build up list_on_sum which is a list of all variable definitions in this
     model:
      - for variables without indexing expression a SyntaxNodeIDREF pointing
        to this ModelComp is added
      - for variables with indexing expression an SyntaxNode tree that encodes
          sum {dumN in SET} var[dumN]
        is added
  */

  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    comp = *p;
    if (comp->type==TVAR){
      // if there is no indexing expression, simply add this
      if (comp->indexing){
        SyntaxNodeIx *ix = comp->indexing;
        // Need to create "sum{dummy in set} var[dummy]":
        // 1) create "dummy in set"
        vector<SyntaxNode*> commaseplist;
        SyntaxNode *commasepon;
        for(i=0;i<ix->ncomp;i++){
          if (ix->dummyVarExpr[i]){
            SyntaxNode *newon = new SyntaxNode(IN, ix->dummyVarExpr[i], ix->sets[i]);
            commaseplist.push_back(newon);
          }else{
            // need to make up a dummy variable
            ostringstream ost;
            ost << "dum" << i;
            SyntaxNode *newon = new SyntaxNode(IN, new IDNode(ost.str()), ix->sets[i]);
            commaseplist.push_back(newon);
          }
        } // end for
        // make the commaseplist
        if (ix->ncomp==1){
          commasepon = commaseplist[0];
        }else{
          commasepon = new ListNode(COMMA);
          for(i=0;i<ix->ncomp;i++){
            commasepon->push_back(commaseplist[i]);
          }
        }
        SyntaxNodeIDREF *onref = new SyntaxNodeIDREF(comp);
        for(i=0;i<ix->ncomp;i++){
          // this is the dummy variable of the i-th indexing expression
          SyntaxNode *ondum = (SyntaxNode*)*(commaseplist[i]->begin());
          if (ondum->opCode==LBRACKET) ondum=(SyntaxNode*)*(ondum->begin());
          onref->push_back(ondum);
        }
        // make the sum part
        commasepon = new SyntaxNode(LBRACE, commasepon);
        list_on_sum.push_back(new SyntaxNode(SUM, commasepon, onref));

      }else{ // no indexing expression, simply add this node
        list_on_sum.push_back(new SyntaxNodeIDREF(comp));
      } // end if (indexing)
    }
  }

  // we have now build a list of SyntaxNodes representing the components:
  // build the attribute SyntaxNode as a sum of them all
  if (list_on_sum.size()>0){
    if (list_on_sum.size()==1){
      attr = list_on_sum[0];
    }else{
      attr = new OpNode('+', list_on_sum[0], list_on_sum[1]);
      for(i=2;i<(int) list_on_sum.size();i++){
        attr = new OpNode('+', attr, list_on_sum[i]);
      }
    }
    
    newobj = new ModelComp(strdup("dummy"), TMIN, NULL, attr);
    this->addComp(newobj);
  }

  // and recursively do this for all AmplModels below this one
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    comp = *p;
    if (comp->type==TMODEL){
      // might be that we need to add the indexing expression on the stack 
      ((AmplModel*)(comp->other))->addDummyObjective();
    }
  }
}

/* --------------------------------------------------------------------------
AmplModel::removeComp()
---------------------------------------------------------------------------- */
void
AmplModel::removeComp(ModelComp *comp)
{
  // FIXME: check of comp is indeed on the list
  
  bool found = false;
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    if (strcmp((*p)->id,comp->id)==0){
      comps.erase(p); // this invalidates the iterator => break from loop
      found = true;
      break;
    }
  }
  if (!found){
    printf("Attempting to remove component %s from model %s:\n",
           comp->id, name);
    printf("Component is not in model!\n");
    exit(1);
  }
  n_total--;
  switch(comp->type){
    case TVAR: 
      n_vars--;
      break;
    case TCON: 
      n_cons--;
      break;
    case TPARAM: 
      n_params--;
      break;
    case TSET: 
      n_sets--;
      break;
    case TMIN: 
    case TMAX: 
      n_objs--;
      break;
    case TMODEL: 
      n_submodels--;
      break;
    default:
      cerr << "removeComp: type " << comp->type << "unrecognised\n";
      exit(1);
  }
}
/* --------------------------------------------------------------------------
AmplModel::removeComp()
---------------------------------------------------------------------------- */
void
AmplModel::addComp(ModelComp *comp)
{
  AmplModel *subm;
  //ModelComp *lastinmodel = model->last;
  switch(comp->type)
  {
    case TVAR:
      n_vars++;
      break;
    case TCON:
      n_cons++;
      break;
    case TSET:
      n_sets++;
      break;
    case TPARAM:
      n_params++;
      break;
    case TMIN:
      n_objs++;
      break;
    case TMAX:
      n_objs++;
      break;
    case TMODEL:
      n_submodels++;
      subm = (AmplModel*)comp->other;
      subm->parent = this;
      break;
    default:
      cerr << "addComp: type " << comp->type << "unrecognised\n";
      exit(1);
    }
  n_total++;
  comp->model = this;
  //comp->next = NULL;
  //comp->prev = lastinmodel;
  comps.push_back(comp);

  comp->setUpDependencies();
  //if (model->first==NULL){
  //  model->first=comp;
  //  model->last = comp;
  //}else{
  //  model->last = comp;
  //  lastinmodel->next = comp;
  // }
  

}

/* --------------------------------------------------------------------------
AmplModel::applyChanges()
---------------------------------------------------------------------------- */
//static   // for some weird reason 'static' is not allowed here 
void
AmplModel::applyChanges()
{
  for(list<changeitem*>::iterator p=changes.begin();p!=changes.end();p++){
    changeitem *ch = (*p);
    if (ch->action==CHANGE_REM){
      (ch->model)->removeComp(ch->comp);
    }
    if (ch->action==CHANGE_ADD){
      (ch->model)->addComp(ch->comp);
    }
  }
  changes.clear();
}

/* --------------------------------------------------------------------------
AmplModel::reassignDependencies()
---------------------------------------------------------------------------- */
/** In the process of building the AmplModel tree from the StochModelTree
 *  some of the IDREF dependency nodes still point to the StochModelComp
 *  nodes from the StochModel tree (or the intermediate tree)
 *
 *  This routine goes through all components and makes sure that IDREF
 *  nodes are resolved with respet to the correct ModelComp and that
 *  the dependency lists are in order.
 *  Recursively follows down submodels.
 */
void
AmplModel::reassignDependencies()
{
  for(list<ModelComp*>::iterator p=comps.begin();p!=comps.end();p++){
    if ((*p)->type==TMODEL){
      AmplModel *submodel = (AmplModel*)((*p)->other);
      submodel->reassignDependencies();
    }else{
      (*p)->reassignDependencies();
    }
  }
}

/* --------------------------------------------------------------------------
AmplModel::findComponent(string id)
---------------------------------------------------------------------------- */
/** Finds a component with name id in correct scoping order.
 * That is to say it will first search this model's SymbolTable, and if it
 * cannot find the component it will recurse to its parental node and so on
 * up to the root.
 */
SymbolTable::Entry *AmplModel::findComponent(string id) {
   SymbolTable::Entry *ent = symbol_table.findSymbol(id);
   if(!ent && parent) parent->findComponent(id);
   return ent;
}

/* ---------------------------------------------------------------------------
bool is_int(char *tok)
---------------------------------------------------------------------------- */
/** Utility function that checks is a cstring represents a natural number
 *  (i.e [0-9]*) or not
 */
bool 
is_int(const char *tok){
  int pos = 0;

  if (tok[0]==0) return false;
  while(tok[pos]!=0){
    if (tok[pos]<'0' || tok[pos]>'9') return false;
    pos++;
  }

  return true;
}
