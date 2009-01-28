#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include "backend.h"
#include "ampl.h"
#include "GlobalVariables.h"
#include "nodes.h"
#include "sml.tab.h"
#include "sml-oops.h"
//#include <list>

static bool prt_modwrite = false;
//produces: "Modified write (wealth), level=2, l_addIndex=2"

void print_model(AmplModel *model);
void process_model(AmplModel *model);
void add_to_index_stack(opNode *ix);
void rem_from_index_stack();

void write_ampl_for_submodel(ostream &fout, AmplModel *root, 
           AmplModel *submodel);
void write_columnfile_for_submodel(ostream &fout, AmplModel *submodel);

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
//         mulitple dimensions {i in SET1,j in SET2} 
//         SET valued expressions: {i in SET1 cross SET2} 
//         or conditions:    {(i,j) in SET1:i<j}
//int n_addIndex;           /* number and list of indexing expressions */
vector <list <add_index *>* > l_addIndex;  /* to add to all statements */


/* ---------------------------------------------------------------------------
do_stuff
---------------------------------------------------------------------------- */
/* This is the entry function once the model has been parsed. It is called from
   main in ampl.y
   This includes calls to the various phases of the backend
*/
void do_stuff(AmplModel *model)
{
  model->addDummyObjective();
  //print_model(model);
  model->dump("logModel.dat");
  //exit(1);
  process_model(model);
}

static void
print_entry(const model_comp *entry) {
  cout << "    " << entry->id << "\n";
  cout << "       " << *(entry->indexing) << "\n";
  cout << "       " << *(entry->attributes) << "\n";
}

void
print_model(AmplModel *model)
{
  model_comp *entry;
  AmplModel *submod;
  opNode::use_global_names=0;
  cout << "-------------------------- backend::print_model ----------------"
     "----------\n";
  cout << "Model: " << model->name << "\n";
  cout << "n_sets: " << model->n_sets << "\n";
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    if (entry->type==TSET){
      print_entry(entry);
    }
    //entry = entry->next;
  }
  cout << "n_cons: " << model->n_cons << "\n";
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    //  entry = model->first;
    //while(entry!=NULL){
    if (entry->type==TCON){
      print_entry(entry);
    }
    //entry = entry->next;
  }
  cout << "n_vars: " << model->n_vars << "\n";
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    //entry = model->first;
    //while(entry!=NULL){
    if (entry->type==TVAR){
      print_entry(entry);
    }
    //entry = entry->next;
  }
  cout << "n_params: " << model->n_params << "\n";
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    // entry = model->first;
    //while(entry!=NULL){
    if (entry->type==TPARAM){
      print_entry(entry);
    }
    //entry = entry->next;
  }

  cout << "n_obj: "<< model->n_objs;
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    //  entry = model->first;
    //while(entry!=NULL){
    if (entry->type==TMIN||entry->type==TMAX){
      cout << "    " << entry->id << "\n";
      cout << "       " << entry->indexing << "\n";
      cout << "       " << entry->attributes << "\n";
    }
    //entry = entry->next;
  }
  
  cout << "submodels: " << model->n_submodels << "\n";
  for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
    entry = *p;
    //entry = model->first;
    //while(entry!=NULL){
    if (entry->type==TMODEL){
      cout << "    " << entry->id << "\n";
      cout << "       " << entry->indexing << "\n";
      cout << "       " << entry->attributes << "\n";
      submod = (AmplModel*)entry->other;
      print_model(submod);
    }
    //entry = entry->next;
  }
  
  cout << "---END-------------------- backend::print_model -------------"
     "-------------\n";

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
  int i, j, k, n_models, n;
  AmplModel **model_list;
  
  cout << "-------------- start of process_model ----------------------\n";

  /* 1) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> generate list of all models */

  /* recursively count all the models defined in the model tree */
  n_models = count_models_(model);
  cout << "Found " << n_models << " models\n";
  model_list = (AmplModel**)calloc(n_models, sizeof(AmplModel*));
  n_models=0;
  model->level=0; /* root is on level 0, fill_model_list also sets levels */
  fill_model_list_(model, model_list, &n_models);
  /* model_list[0-(n_models-1)] is now a list of all (sub)models defined
     in the ampl file */

  //printf("These are the models on the list:\n");
  for(i=0;i<n_models;i++){
    AmplModel *thism = model_list[i];
    cout << i << ": " << thism->name << " (level=" << thism->level << ")\n";
  }
  
  cout << "----------- generate submodel files --------------\n";

  /* 2) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> output all submodel files */
  /* and */
  /* 3) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> generate the script file */
  ofstream fscript("script.scr");
  string filename;   /* this is the name of the model file */
  
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
      filename = "root";
    }else{
      /* find name if model file by concatenating the names of all ancestors */
      AmplModel *tmp_model;
      tmp_model = this_model;
      filename = tmp_model->name;
      while (tmp_model->parent){
        tmp_model = tmp_model->parent;
        filename = tmp_model->name + ("_" + filename);
      }
    }
    filename = filename + ".mod";


    /* ==================== write the script file ===================== */

    fscript << "\nreset;\noption auxfiles rc;\noption presolve 0;\n";
    fscript << "model " << filename << ";\ndata ../" <<
       GlobalVariables::datafilename << ";\n";
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
    filename.erase(filename.size()-4);
    
    l_addIndex.clear();

    for(k=1;k<=this_model->level;k++){
      AmplModel *tmp_model = anc_list[k];
      model_comp *node = tmp_model->node; /* the node corresponding to model */
      opNode *ix = node->indexing; /* the indexing expression */
      opNode *set; /* the set that is indexed over */
      opNode *dummyVar; /* the dummy var of the indexing expression */

      // need to set opNode::default_model for component printing routines
      opNode::default_model = tmp_model; 
      
      /* Need to analyse the indexing expression to get the set that is
        indexed over 
        (I guess this should be done by an "indexing" object) */
      
      /* remove outside braces from indexing expression */
      list <add_index*>* li = new list<add_index*>();
      if (ix){
        add_index *ai = (add_index*)calloc(1, sizeof(add_index));
        li->push_back(ai);
        if (ix->opCode==LBRACE) ix = (opNode*)*(ix->begin());
        /* assumes that the next level is the 'IN' keyword (if present) */
        if (ix->opCode==IN){
          opNode::Iterator ixi = ix->begin();
          dummyVar = (opNode*)*ixi;
          set = (opNode*)*(++ixi);
        }else{
          dummyVar = NULL;
          set = ix;
        }
        ai->dummyVar = dummyVar;
        ai->set = set;
      }else{
        set = NULL;
      }

      // FIXME: how to deal with multidimensional dummy variables?
      //        need to work out the dimenension of a set?
      //        => for now simply use the original dummy variable
      //        => REQUIRE a dummy variable in block indexing
      if (!dummyVar){
        cerr << "Indexing expressions for block's need dummy variables!\n";
        cerr << "   " << node->indexing;
        exit(1);
      }
      //for(j=1;j<k;j++) fprintf(fscript,"  "); /* prettyprinting */
      //fprintf(fscript, "for {i%d in %s }{\n", k, print_opNode(set));
      //for(j=1;j<=k;j++) fprintf(fscript,"  ");
      //fprintf(fscript, "let %s_SUB := {i%d};\n", print_opNode(set), k);
      if (set){
        opNodeIDREF *set_ref = dynamic_cast<opNodeIDREF*>(set);
        if (set_ref==NULL){
          cerr << "ERROR: set reference must be opNodeIDREF\n";
          exit(1);
        }
        for(j=1;j<k;j++) fscript << "  "; /* prettyprinting */
        fscript << "for {" << dummyVar << " in " << set << " }\n";
        for(j=1;j<k;j++) fscript << "  "; /* prettyprinting */
        fscript << "{\n";
        // the "reset data" command is needed to avoid the invalid subscript
        // error
        for(j=1;j<=k;j++) fscript << "  ";
        fscript << "reset data " << getGlobalName(set_ref->ref, set,
            opNode::default_model, NOARG) << "_SUB;\n";
        for(j=1;j<=k;j++) fscript << "  ";
        fscript << "let " << 
          getGlobalName(set_ref->ref, set, opNode::default_model, NOARG) <<
          "_SUB" <<
          getGlobalName(set_ref->ref, set, opNode::default_model, ONLYARG) <<
          " := {" << dummyVar << "};\n";
      }else{
        for(j=1;j<k;j++) fscript << "  "; /* prettyprinting */
        fscript << "{\n";
      }
      l_addIndex.push_back(li);
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
      for(list<model_comp*>::iterator p = this_model->comps.begin();
          p!=this_model->comps.end();p++){
        if ((*p)->type!=TMODEL) continue;

        model_comp *mc = *p;

        /* found a submodel */
        AmplModel *submodel = (AmplModel*)mc->other;
        opNode *ix = mc->indexing;
        opNode *set;
    
        set = ix->getIndexingSet(); // set is NULL if ix is NULL
  
        // the name of the *.crd file is the global name of the submodel
        // with all the current values of loop variables up to
        // this level attached
        
        // so buffer here should be name of current model
        string rootfilename = filename;
        filename += "_";
        filename += submodel->name;
        // for all levels from root up to here add the value of 
        // indexing variables
        

        filename+= "\"";
        for(k=1;k<=this_model->level;k++){
          // get all the indexing variables and add them together (joined by &)
          opNodeIx *ixn = (anc_list[k]->node)->indexing;
          if (ixn){
            list<opNode *>* dvl = ixn->getListDummyVars();
            string innerp = "";
            for(list<opNode *>::iterator q=dvl->begin();q!=dvl->end();q++){
              innerp += (*q)->print() + "&";
            }
            innerp.erase(innerp.size()-1); // delete the last '&'
            
            filename += "&\"_\"&" + innerp;
          }
        }
        filename += "&\".crd\"";
        //printf("name of file is: %s\n",buffer);

        // Need to write something like
        //  print card(indexing expression) > exact_model_name.&1.&2.txt
        // or whatever it takes to get ampl to produce filenames
        // of the form stub_1.2.crd
        // where 1, 2 are  the values of the iteration indices in the 
        // scriptfile
        if (set){
          for(j=1;j<=this_model->level;j++) fscript << "  "; 
          fscript << "print card(" << set << ") > (\"" << filename << ");\n";
          filename.replace(filename.find("&\".crd\""), 7, "&\".set\"");
          for(j=1;j<=this_model->level;j++) fscript << "  "; 
          fscript << "display " << set << " > (\"" << filename << ");\n";
        }else{
          fscript << "print \"0\" > (\"" << filename << ");\n";
        }

        filename = rootfilename; //delete the extension again
      }
    }
    

    /* write the main part of the submodel generation part in the scripts */
    for(j=1;j<k;j++) fscript << "  "; fscript << "fix all;\n";
    for(j=1;j<k;j++) fscript << "  "; fscript << "unfix all;\n";
    
    /* take the .mod suffix away from buffer */
    //n = strlen(buffer);buffer[n-4] = 0;
    for(j=1;j<k;j++) fscript << "  "; /* prettyprinting */
    fscript << "write (\"b" << filename << "\"";
    for(k=1;k<=this_model->level;k++) {
      // get all the indexing variables and add them together (joined by &)
      opNodeIx *ixn = (anc_list[k]->node)->indexing;
      if (ixn){
        list<opNode *>* dvl = ixn->getListDummyVars();
        string textrep = "";
        for(list<opNode *>::iterator q=dvl->begin();q!=dvl->end();q++){
          textrep += (*q)->print() + "&";
        }
        textrep.erase(textrep.size()-1); // delete the last '&'
  
        fscript << "&\"_\"&" << textrep;
      }
    }
    fscript << ");\n";
    
    /* and close all the brackets (prettyprinting) */
    for(k=this_model->level;k>0;k--){
      for(j=1;j<k;j++) fscript << "  ";
      fscript << "}\n";
      //rem_from_index_stack();
      l_addIndex.pop_back();
    }
    
    /* write the submodel file */
    //n = strlen(buffer);buffer[n-4] = 0;
    filename += ".mod";
    ofstream fout(filename.c_str());
    cout << "Write to model file: " << filename << "\n";
    write_ampl_for_submodel(fout, model_list[n_models-1], model_list[i]);
    fout.close();

    /* FIXME: this looks redundant */
    /* and also write the column file for this submodel */
    filename.replace(filename.find(".mod"), 4, ".acl");
    fout.open(filename.c_str());
    write_columnfile_for_submodel(fout, model_list[i]);
    fout.close();
    free(anc_list);
  }

  fscript.close();

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

  /* 3b) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> call ampl to process script */

  cout << "call AMPL to process script file: ";

  {
    // call ampl to process script and analyse the output

    FILE *ain = NULL;
    char buffer[256];
    int n_nocsobj=0; // number of model with "No constraints or objectives."
    int n_novar=0; // number of model with "No variables declared."
    int n_other=0; // other ampl error

    ain = popen("ampl script.scr 2>&1", "r"); // "2>&1" sends stderr to stdout
    while(!feof(ain)){
      char *p;
      p = fgets(buffer, 256, ain);
      if (p){
        cout << buffer;
        if (strncmp(buffer, "No constraint",13)==0){
          n_nocsobj++;
        } else if (strncmp(buffer, "No variables",12)==0){
          n_novar++;
        } else if (strncmp(buffer, "ILOG AMPL",9)==0){
          // Do nothing, version and liscence string
          // eg ILOG AMPL 10.000, licensed to "university-edinburgh".
        } else if (strncmp(buffer, "AMPL Version",12)==0){
          // Do nothing version string
          // eg AMPL Version 20051214 (Linux 2.6.9-5.ELsmp)
        }else{
          n_other++;
        }
      }
    }
    if (n_nocsobj+n_other>0){
      printf("\nAMPL: ampl returned output\n");
      cout << "AMPL: Model without constraints and objectives: " <<
        n_nocsobj << "\n";
      cout << "AMPL: Model without variables                 : " <<
        n_novar << "\n";
      cout << "AMPL: Other errors                            : " <<
        n_other << "\n";
      if (n_other>0) exit(1);
    }
  }
  cout << "done\n";

  /* 4) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> build algebra tree */
  
  //model->print();
  
  cout << "------------- Generate ExpandedModel tree ------------ \n";
  ExpandedModel *em = new ExpandedModel(model);
  em->print();
  cout << "=============================================================== \n";
  cout << "----------------- Call OOPS generator ---------------- \n";

  SML_OOPS_driver(em);
  
}

int count_models_(AmplModel *model)
{
  int count = 0;
  model_comp* comp;
  if (model->n_submodels>0){
    for(list<model_comp*>::iterator p = model->comps.begin();
  p!=model->comps.end();p++){
      comp = *p;
      if (comp->type==TMODEL)
  count += count_models_((AmplModel*)comp->other);
    }
  }
  count += 1; /* also count this model */

  return count;
}

/* ---------------------------------------------------------------------------
fill_model_list 
---------------------------------------------------------------------------- */
void
fill_model_list_(AmplModel *model, AmplModel **listam, int *pos)
{
  /* fill_model_list:
     recursively creates a depth first list of all the models in the
     model tree 
     Also sets the model->level attribute of all models in the tree
     ASSUMES: that the ->level of the initial model is already set

     IN: 
       AmplModel *model:          The start node (usually root)
       AmplModel **listam:          The list in which models are added
       int *pos:                   Position at which to enter the next model
     OUT: 
       listam
       model->level
                                                                            */
  model_comp *comp;

  /* this works by first adding the current model to the list and then
     all its submodels. 
     It used to be that the submodels are added first, however then the
     level-calculation does not work */

  listam[*pos] = model;
  if (model->parent) model->level = model->parent->level+1;
  (*pos)++;

  if (model->n_submodels>0){
    for(list<model_comp*>::iterator p = model->comps.begin();p!=model->comps.end();p++){
      comp = *p;
      if (comp->type==TMODEL)
  fill_model_list_((AmplModel*)comp->other, listam, pos);
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
void write_ampl_for_submodel_(ostream &fout, int thislevel, int sublevel, 
            AmplModel **list, AmplModel *submodel);
void modified_write(ostream &fout, model_comp *comp);

void
write_ampl_for_submodel(ostream &fout, AmplModel *root, AmplModel *submodel)
{
  AmplModel *listam[5];  /* assume models are not nested further than
         5 levels */
  int i, level;
  
  opNode::use_global_names = 1;
  //n_addIndex = 0; // clear addIndex stack
  l_addIndex.clear();

  if (GlobalVariables::prtLvl>1){
    cout << "==============================================================\n";
    cout << "     ampl model for part: " << submodel->name << "\n";
    cout << "==============================================================\n";
  }

  /* need list of models at different levels from here up to root */
  {
    AmplModel *tmp;
    
    listam[0] = submodel;
    tmp = submodel;
    level = 0;
    while(tmp->parent){
      tmp = tmp->parent;
      level++;
      listam[level] = tmp;
    }
  }
  if (GlobalVariables::prtLvl>1){
    cout << "-> this model is on level " <<  level << "\n";
    cout << "   Levels from top are: \n";
    for(i=0;i<=level;i++){
      cout << i << ": " << listam[i]->name << "\n";
    }
  }

  /* mark all model components that are needed by the current model */
  //model_comp::untagAll();
  model_comp::untagAll(AmplModel::root);
  /* and loop through all model_components and mark it and its 
     dependencies as needed */
  
  for(list<model_comp*>::iterator p = submodel->comps.begin();
      p!=submodel->comps.end();p++){
    (*p)->tagDependencies();
  }
  if (GlobalVariables::prtLvl>1){
    cout << "processing " <<  submodel->name << "\n";
    cout << "-------> tagged now\n";
    //model_comp::writeAllTagged();
    model_comp::writeAllTagged(AmplModel::root);
  }

  /* now start reporting the modified model recursively */
  
  write_ampl_for_submodel_(fout, level, 0, listam, submodel);

  
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
/* This routine does part of the work in writing out an AMPL submodel file
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
     AmplModel[] *listam     list[0] is current model, list[thislevel] is root
     AmplModel *submodel   the current model again
   OUT: none (output on data file)


   It keeps track of the stack of block indexing expressions, by putting them
   onto the l_addIndex stack

*/
void
write_ampl_for_submodel_(ostream &fout, int thislevel, int sublevel, 
       AmplModel **listam, AmplModel *submodel)
{
  AmplModel *thism = listam[thislevel];
  int j;
  model_comp *comp;
  
  opNode::default_model = thism;
  // loop through all entities in this model 
  fout << "\n# start of model " << thism->global_name << "\n\n";
  for(list<model_comp*>::iterator p = thism->comps.begin();
      p!=thism->comps.end();p++){
    comp = *p;

    if (comp->type!=TMODEL){
      // if it is not a model declaration, simply write the declaration out
      // write out *all* components of current model and everything except
      // objectives and constraints in submodels
      if (comp->type==TVAR || comp->type==TSET || comp->type==TPARAM 
          || thism==submodel){
        //opNode::default_model = comp->model;
        modified_write(fout, comp);
      }
    }else{ // if (comp->type!=TMODEL)
      /* this is a model declaration */
      /* check that it needs to be followed */
      if (thism!=submodel && listam[thislevel-1]==comp->other){
        opNode *ix;
        list <add_index*>* li = new list<add_index*>();
        /* ok, looks like this needs to be followed */
        /* add the indexing expression */
  
        // initialise the new entry in l_addIndex stack if not already done
        ix = comp->indexing;
        // l.addIndex.push_back(li);
        if (ix){
          add_index *ai = (add_index*)calloc(1, sizeof(add_index));
          //if (l_addIndex[n_addIndex]==NULL){
          //  l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
          //}
          // add new entry on the addIndex stack
          //list<add_index*> li = l_addIndex.last();
          li->push_back(ai);
          
          // and place the indexing expression of this BLOCK onto the stack
          // the stack stores the dummy variable and the SET separately, 
          // so take them to bits here
          if (ix->opCode==LBRACE) ix = (opNode*)*(ix->begin()); // rem {..}
          if (ix->opCode==IN){
            //l_addIndex[n_addIndex]->dummyVar = (opNode*)ix->values[0];
            //l_addIndex[n_addIndex]->set = (opNode*)ix->values[1];
             opNode::Iterator ixi = ix->begin();
            ai->dummyVar = (opNode*)*ixi;
            ai->set = (opNode*)*(++ixi);
          }else{ // no dummy variable, just a set
            //l_addIndex[n_addIndex]->dummyVar = NULL;
            //l_addIndex[n_addIndex]->set = ix;
            ai->dummyVar = NULL;
            ai->set = ix;
          }
          
          /* okay we have placed the set description on the stack
             but really this should be modified:
             'i in ARCS' should read 'i in ARCS_SUB'
             
             ought to 'write set ARCS_SUB within ARCS'; as well
          */
          
          
          /* 14/03/08: what we are trying to do is to create a model_comp
             that represents the expression
             set indset_SUB within indset;   
             and print this with modified_write. Then it should be
             automatically indexed over all subproblem indexing sets:
             set indset_SUB{ix0 in indset0_SUB} within indset[ix0];
             
             
             The model_comp is of type SET, with no indexing expression
          */
          {
            model_comp *newmc = new model_comp();
            opNode *setn = ai->set;
            
            newmc->type = TSET;
            
            // set name of the model_comp
            if (setn->opCode!=IDREF){
              cerr << "At the moment can index blocks only with simple sets\n";
              cerr << "Indexing expression is " << setn->print() << "\n";
              exit(1);
            }
            opNodeIDREF *setnref = dynamic_cast<opNodeIDREF*>(setn);
            if (setnref==NULL){
              cerr << "IDREF node should be of type opNodeIDREF\n";
              exit(1);
            }
            model_comp *setmc = setnref->ref;
            newmc->id = strdup((setmc->id+string("_SUB")).c_str());
            
            // and build "within indset" as attribute tree
            newmc->attributes = new opNode(WITHIN, setn);
            //newmc->model = comp->model;
            newmc->model = setmc->model;
            modified_write(fout, newmc);
          }
          
          if (true)
          {
            //opNode *setn = l_addIndex[n_addIndex]->set;
            opNode *setn = ai->set;
            opNodeIDREF *newn = new opNodeIDREF();
            model_comp *tmp;
            char *newname; 
            //fprintf(fout, "set %s_SUB within %s;\n", 
            //      print_opNode(setn), print_opNode(setn));
            /* and now modify the set declaration */
            if (setn->opCode!=IDREF){
              cerr << "At the moment can index blocks only with simple sets\n";
              cerr << "Indexing expression is " << setn->print() << "\n";
              exit(1);
            }
            // FIXME: rewrite this code by
            //  - using the opNodeIDREF subclass
            //  - using the opNodeIDREF::clone() method
            /* newn is the new node, first copy the old one */
            newn->opCode = IDREF;
            newn->nval = setn->nval;
            newn->values = (void **)calloc(setn->nval+1, sizeof(void *));
            for(j=0;j<setn->nval;j++) newn->values[j] = setn->values[j];
            // clone the model_comp that is referred to
            newn->ref = (model_comp *)calloc(1,sizeof(model_comp));
            memcpy(newn->ref, ((opNodeIDREF*)setn)->ref, sizeof(model_comp));
            // ???but associate this with the current model
            //newn->ref->model = thism;

            /* and finally set the new name (add _SUB) to the name */
            tmp = newn->ref;
            newname = (char *)calloc(strlen(tmp->id)+5, sizeof(char));
            strcpy(newname, tmp->id);
            strcat(newname, "_SUB");
            tmp->id = newname;
            /* and put this on the stack */
            //l_addIndex[n_addIndex]->set = newn;
            ai->set = newn;
          }       
          //n_addIndex++;
        }
        l_addIndex.push_back(li);
        write_ampl_for_submodel_(fout, thislevel-1, sublevel, listam, 
               submodel);
        opNode::default_model = thism;
        //if (ix) n_addIndex--;
        l_addIndex.pop_back();
      } /* end of (model on the current list branch) */
      else if (thislevel==0) {
        // we are in the current model and are 
        // processing a definition of a child block of the current block
        // => write out everything that is tagged
        AmplModel *childm = (AmplModel *)comp->other;
        
        //printf("\n\n-----------------------------------------------\n");
        //printf("  current model: %s\n",submodel->name);
        //printf("  these are the components needed from children:\n");
        childm->writeTaggedComponents(fout);
        opNode::default_model = thism;
        //printf("-----------------------------------------------\n\n");
        
        // For normal model components we simply call modified_write
        // on these components
        // => what does modified_write depend on?
      }
      
    } /* end else (end of the TMODEL branch) */
    
    //comp = comp->next;
  }
  
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
write_columnfile_for_submodel(ostream &fout, AmplModel *submodel)
{
  model_comp *comp;

  for(list<model_comp*>::iterator p = submodel->comps.begin();
      p!=submodel->comps.end();p++){
    comp = *p;
    if (comp->type==TVAR){
      /* print global name here: 
         either just prefix all model names up to here by a loop, or
         use the global name printing routine used elsewhere (but I
         think that also puts indexing expressions on it               */

      /* the NOARG version of getGlobalName should do the trick */
      fout << getGlobalName(comp, NULL, NULL, NOARG) << "\n";
    }
    //comp = comp->next;
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
  add_index *ai = (add_index*)calloc(1, sizeof(add_index));
  //  }
  //  if (l_addIndex[n_addIndex]==NULL){
  //    l_addIndex[n_addIndex] = (add_index*)calloc(1, sizeof(add_index));
  //  }
  //  /* remove outside braces from indexing expression */
  if (ix->opCode==LBRACE) ix = (opNode*)*(ix->begin());
  /* assumes that the next level is the 'IN' keyword (if present) */
  if (ix->opCode==IN){
    //    l_addIndex[n_addIndex]->dummyVar = (opNode*)ix->values[0];
    //    l_addIndex[n_addIndex]->set = (opNode*)ix->values[1];
    opNode::Iterator ixi = ix->begin();
    ai->dummyVar = (opNode*)*ixi;
    ai->set = (opNode*)*(++ixi);
  }else{
    ai->dummyVar = NULL;
    ai->set = ix;
    //    l_addIndex[n_addIndex]->dummyVar = NULL;
    //    l_addIndex[n_addIndex]->set = ix;
  }
  // and put this onto the stack
  list <add_index*> *li = new list<add_index*>;
  li->push_back(ai);
  l_addIndex.push_back(li);
  //  n_addIndex++;
}

void 
rem_from_index_stack(){
  if (l_addIndex.size()==0){
    printf("Attempting to remove item from l_addIndex list when the list is empty\n");
    exit(1);
  }else{
    l_addIndex.pop_back();
  }

  //  if (n_addIndex>0){
  //    n_addIndex--;
  //  }else{
  //    printf("Attempting to remove item from l_addIndex list when the list is emply\n");
  //    exit(1);
  //  }
}

/* ---------------------------------------------------------------------------
modified_write
--------------------------------------------------------------------------- */
/** writes out a component of a model 
 *
 * components can be modified: if this is down into a submodel, then
 *  - all declarations get new indexing expressions appended to it
 *  - all references to entities get new subscripts attached to it.
 *
 * @param[in] fout  The file to write to
 * @param[in] comp  The component definition to write out
 * @pre depends on l_addIndex: currently applicable indexing expresssions
 *   
 *  prints the global definition of the given model_component to the given 
 *  file.
 *
 * - 1) get the global name of the model component
 * - 2) prepend all indexing expressions on the stack to the indexing 
 *      expression of this entity
 * - 3) for all components that are referenced in the definition
 *   - a) use their global name
 *   - b) prepend the dummy variables for all indexing expressions on the stack
 *        to the argument list
 *
 * part 3) is simply done by a call to (comp->attributes)->print() 
 * (opNode::print)
 * (with opNode::use_global_names set to true 
 *   => the argument list version of model_comp::getGlobalName is called)  
 *
 */
void
modified_write(ostream &fout, model_comp *comp)
{
  int i;
  opNode *ixsn;
  int c_addIndex;

  // we should check that the level of the model the component is attached to
  // tallies with the number of expressions on the indexing stack
  
  AmplModel *model = comp->model;
  int level=0;
  while (model->parent!=NULL){
    level++;
    model = model->parent;
  }
  if (prt_modwrite)
    cout << "Modified write (" << comp->id << "), level=" << level << 
      ", l_addIndex=" << l_addIndex.size() << "\n";

  if (comp->type!=TMODEL){
    int first=1;
    /* start statement */
    fout << compTypes[comp->type] << " ";
    
    // find number of indexing expressions on stack
    c_addIndex = 0;
    //for(i=0;i<l_addIndex.size();i++){
    for(i=0;i<level;i++){
      c_addIndex += l_addIndex[i]->size();
      //list <add_index*>* li = l_addIndex.at(i);
      //for(list<add_index*>::iterator p=li->begin();p!=li->end();p++){
      //c_addIndex++;
      //}
    }
    /* write name and indexing expression */
    fout << getGlobalName(comp, NULL, NULL, NOARG) << " ";
    //if (n_addIndex>0 || comp->indexing)
    if (c_addIndex>0 || comp->indexing)
      fout << "{";
    /* write out the additional indexes */
    //for(i=0;i<l_addIndex.size();i++){
    for(i=0;i<level;i++){
      list <add_index*>* li = l_addIndex.at(i);
      for(list<add_index*>::iterator p=li->begin();p!=li->end();p++){
        add_index *ix = *p;
        // for(i=0;i<n_addIndex;i++){
        // add_index *ix = l_addIndex[i];
        if (first) {first = 0;} else {fout << ",";}
        if (ix->dummyVar){
          fout << ix->dummyVar << " in ";
        }
        ixsn = ix->set;
        if (ixsn->opCode==LBRACE) ixsn=(opNode*)*ixsn->begin(); 
        fout << ixsn;
      }
    }
    /* write out the index of this component */
    if (comp->indexing) {
      if (first) {first = 0;} else {fout << ",";}
      ixsn = comp->indexing;
      if (ixsn->opCode==LBRACE) ixsn=(opNode*)*(ixsn->begin()); 
      fout << ixsn;
    }
    //if (n_addIndex>0 || comp->indexing)
    if (c_addIndex>0 || comp->indexing)
      fout << "}";
    
    /* write out special syntax for a particular statement type */
    if (comp->type==TCON||comp->type==TMIN||comp->type==TMAX) 
      fout << ":";
    
    /* write out the rest of the expression */
    fout << comp->attributes << ";\n";
    //fprintf(fout, "%s;\n",print_opNode(comp->attributes));
    //fprintf(fout, "%s;\n", print_opNodesymb(comp->attributes));
  }
}
