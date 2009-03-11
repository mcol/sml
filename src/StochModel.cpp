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
#include <typeinfo>
#include <fstream>
#include <sstream>
#include "GlobalVariables.h"
#include "StochModel.h"
#include "StochModelComp.h"
#include "sml.tab.h"
#include "SetNode.h"

static bool logSM = false;

/* ---------------------------------------------------------------------------
StochModel::StochModel()
---------------------------------------------------------------------------- */
/** Constructor */
StochModel::StochModel(SetNode *onStages, SyntaxNode *onNodes, SyntaxNode *onAncs, 
                       SyntaxNode *onProb, AmplModel *prnt) :
  AmplModel(),
  stageset(onStages),
  nodeset(onNodes), 
  anc(onAncs),
  prob(onProb)
{
  /* Do this later? -> parent not set up yet*/
  /* No: do this here, should set up a nested list of normal AMPL models */
  parent = prnt;
  expandStages();
}



/* ---------------------------------------------------------------------------
expandSet()
---------------------------------------------------------------------------- */
vector<string> expandSet(SetNode *set) {

  /* analyze all dependencies of this expression */
  ModelComp::untagAll();
  
  list <ModelComp*> dep;
  set->findIDREF(dep);
  for(list<ModelComp*>::iterator q=dep.begin(); q!=dep.end(); ++q){
    if (logSM) cout << "dep: " << (*q)->id << endl;
    (*q)->tagDependencies();
  }

  /* Also tag all global set and parameter definitions */
  /** \bug this is just so that the global data file can be read, eventually
     this should be removed */
  {
    list<ModelComp*> &comps = AmplModel::root->comps;
    
    for(list<ModelComp*>::iterator p=comps.begin(); p!=comps.end(); ++p){
      if ((*p)->type==TSET || (*p)->type==TPARAM) (*p)->tagDependencies();
    }
  }

  ofstream out("tmp.mod");
  ModelComp::modifiedWriteAllTagged(out);
  out << "set settemp = " << set << ";\n";
  out.close();
  
  out.open("tmp.scr");
  out << "reset;\n";
  out << "model tmp.mod;\n";
  out << "data ../" << GlobalVariables::datafilename << ";\n";
  out << "display settemp > (\"tmp.out\");\n";
  out.close();
  
  {
    string command = GlobalVariables::amplcommand;
    command += " tmp.scr";
    cout << "Executing `" << command << "`\n";
    int errc = system(command.c_str());
    if (errc!=0){
      cerr << "ERROR: Call to AMPL returns errc=" << errc << endl;
      exit(1);
    }
  }

  ifstream in("tmp.out");
  if (!in){
    cerr << "ERROR: File 'tmp.out' produced by AMPL does not exist. "
       "AMPL processing failed?\n";
    exit(1);
  }
  //in.getline(buffer, 500);
  //if (logSM) cout <<"Set " << set << " members: " << buffer << "\n";

  // parse the set members
  vector<string> member_list;
  {
    string read;
    getline(in, read, ';');

    string::size_type p2 = read.find(":=") + 2;
    string::size_type p = read.find_first_not_of(' ', p2);
    
    while(p!=read.npos){
      p2 = read.find_first_of(' ', p);
      member_list.push_back(read.substr(p,p2-p));
      p = read.find_first_not_of(' ', p2);
    }
  }
  in.close();

  return member_list;
}

/* ---------------------------------------------------------------------------
StochModel::expandStages()
---------------------------------------------------------------------------- */
/** expand the set used for the STAGES part in the sblock ... using (...)
 *  expression. 
 *  An AMPL model file and correspoding script file is created that
 *  when executes writes the components of the set to disk. This routine
 *  also reads in that file and stores the set members in the list stagenames
 */ 

void
StochModel::expandStages()
{
   stagenames = expandSet(stageset);
}

/* ---------------------------------------------------------------------------
StochModel::expandStagesOfComp()
---------------------------------------------------------------------------- */
/** expand the sets used in 'stages' qualifiers for all model components of 
 *  this model.
 *  An AMPL model file and corresponding script file is created that
 *  when executes writes the components of the set to disk. This routine
 *  also reads in that file and stores the set members in the list stagenames
 *
 *  This is a StochModel method rather than a StochModelComp method in 
 *  order to gather all expansions into a single call to AMPL
 */ 

void
StochModel::expandStagesOfComp()
{
  char buffer[500];
  list <ModelComp*> dep;
  int cnt;

  /* analyze all dependencies of this expression */
  ModelComp::untagAll(AmplModel::root);
  //ModelComp::untagAll();
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    StochModelComp *smc = dynamic_cast<StochModelComp*>(*p);
    if (smc->stageset){
      smc->stageset->findIDREF(dep);
      for(list<ModelComp*>::iterator q=dep.begin();q!=dep.end();q++){
        if (logSM) printf("dep: %s\n",(*q)->id);
        (*q)->tagDependencies();
      }
    }
  }
  /* Also tag all global set and parameter definitions */
  /** \bug this is just so that the global data file can be read, eventually
     this should be removed */
  {
    AmplModel *root = this;
    while (root->parent) root = root->parent;
    
    for(list<ModelComp*>::iterator p = root->comps.begin();p!=root->comps.end();p++){
      ModelComp *q = *p;
      if (q->type==TSET || q->type==TPARAM) q->tagDependencies();
    }

    
  }

  
  ofstream out("tmp.mod");
  ModelComp::modifiedWriteAllTagged(out);
  cnt=0;
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    StochModelComp *smc=dynamic_cast<StochModelComp*>(*p);
    if (smc->stageset){
      out << "set settemp" << cnt << " = " << smc->stageset <<
         ";\n";
      cnt++;
    }
  }
  out.close();
  
  out.open("tmp.scr");
  out << "reset;\n";
  out << "model tmp.mod;\n";
  out << "data ../" << GlobalVariables::datafilename << ";\n";
  cnt=0;
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    StochModelComp *smc=dynamic_cast<StochModelComp*>(*p);
    if (smc->stageset){
      out << "display settemp" << cnt << " > (\"tmp" << cnt << ".out\");\n";
      cnt++;
    }
  }
  out.close();
  {
    if(strlen(GlobalVariables::amplcommand)+9>500) {
       // Avoid buffer overflow
       cerr << "buffer too short to accomodate amplcommand length.\n";
       exit(1);
    }
    strcpy(buffer, GlobalVariables::amplcommand);
    strcat(buffer, " tmp.scr");
    printf("Executing `%s`\n", buffer);
    int errc = system(buffer);
    if (errc!=0){
      printf("ERROR: Call to AMPL returns errc=%d\n",errc);
      exit(1);
    }
  }

  cnt=0;
  for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
    StochModelComp *smc=dynamic_cast<StochModelComp*>(*p);
    if (smc->stageset){
      char buf[20];
      sprintf(buf, "tmp%d.out",cnt);
      ifstream in(buf);
      if (!in){
             cerr << "ERROR: File '" << buf << "' produces by AMPL does not exist. "
           "AMPL processing failed?\n";
        exit(1);
      }
      in.getline(buffer, 500);
      in.close();
      cout << "Set " << smc->stageset << " members: " << buffer << "\n";

      // parse the set members
      smc->stagenames = new vector<string>;
      {
        char *p, *p2;
        p = strstr(buffer, ":=");
        p+=2;
        
        p2 = strtok(p, " ;");
        while(p2!=NULL){
          if (logSM) printf("Member: %s\n",p2);
          smc->stagenames->push_back(p2);
          p2 = strtok(NULL, " ;\n");
        }
      }
      cnt++;
    }
  }

  //exit(1);


}


/* ---------------------------------------------------------------------------
StochModel::expandToFlatModel()
---------------------------------------------------------------------------- */
/* Processing of StochModel:
   for the (normal) AmplModels processing proceeds in three steps:
   - write submodel AMPL files
   - write script
   - create ExpandedModel tree
   How would these steps work for the StochModel?

   a) convert StochModel into a tree of AmplModel (one for each stage).
      Need to know:
      - number of stages (Stages set would need to be expanded)
      
      Can we write down what the nested AmplModel tree for the ALM problem
      would look like?
      (Note that this model is never build in practice, but all the information
      - in particular the description of the indexing sets - need to be held
      in the AmplModel objects).
      
  This routine works in two passes: 
   1) In the first pass the chain of AmplModel's is build. The StochModelComp
      components are just copied (references are copied), but dependencies
      are not resolved with respect to the new model chain. 
      The chain of AmplModels is build from the leaves up
   2) In the second pass the StochModelComp components are transcribed into
      ModelComp's and their dependencies are resolved with respect to the
      new model chain. This passed is executed from root down to the leaves
*/

AmplModel *
StochModel::expandToFlatModel()
{
  AmplModel *model_above = NULL;
  AmplModel *am;
  StochModelComp **indset;
  int stgcnt;

  printf("----------------------------------------------------------------\n");
  printf(" StochModel::expandToFlatModel: \n");
  printf("----------------------------------------------------------------\n");
  
  /* expand the stages set for all the StochModelComp entities in this model */
  expandStagesOfComp();

  /* Now create a FlatModel (AmplModel) for each element of the overall
     stages set and put in it all entities that either have no stageset
     defined, or whose stageset includes the current stage */

  // we need to have the pointers to the submodel indexing sets all set
  // up from the beginning, so that they can be referred to
  indset = (StochModelComp**)calloc(stagenames.size(),sizeof(StochModelComp*));
  for(unsigned int i=0;i<stagenames.size();i++) {
    indset[i] = new StochModelComp();
    // give it a dummy name just so that debugging routines work
    // @bug this introduces a memory leak
    indset[i]->id = strdup("indset");
  }

  // loop over all stages and create an AmplModel for every stage 
  stgcnt = stagenames.size()-1;
  for(vector<string>::reverse_iterator st=stagenames.rbegin();
      st!=stagenames.rend();  st++,stgcnt--){// loops backwards through list
    printf("Creating ampl model at stage %d: %s\n",stgcnt, (*st).c_str());
    am = new AmplModel();

    // set name and global name for this ampl model
    if (stgcnt==0){
      char buffer[30];
      sprintf(buffer, "%s%s",name, (*st).c_str()); 
      am->name = strdup(buffer);
    }else{
      am->name = strdup((*st).c_str());
    }

    ModelComp *comp;

    // loop over all components of the StochModel 
    for(list<ModelComp*>::iterator p = comps.begin();p!=comps.end();p++){
      StochModelComp *smc=dynamic_cast<StochModelComp*>(*p);

      /** @bug Submodel components within sblock's is not supported yet
          This is not implemented when creating a nested AmplModel tree
          out of the StochModel. 
       */
      if (smc->type==TMODEL){
        cerr << "Not quite sure what to do for submodels within Stochastic "
          "Blocks" << endl;
        exit(1);
      }

      bool inc;
      // check if this component should be included in the current stage
      
      if (smc->stageset){
        inc = false;
        for(vector<string>::iterator p=(smc->stagenames)->begin();
            p!=smc->stagenames->end();p++){
          if (*st==*p) {
            inc = true;
            break;
          }
        }
      }else{
        inc = true;
      }
      
      // if this component should be included in the current stage
      if (inc){
        /* I presume this is all that is needed? */
        /** @bug: NO! need to translate all references to ModelComp (IDREF) 
         * that will currently refer to ModelComps of the StochModel
         * into ModelComps of the appropriate FlatModel on the FlatModel tree.
         * This might be modified by any (-1;...) expressions that refer
         * to parents in the FlatModel tree
         * I guess also need to do something with the Exp(...) expression
         */

        /* In the first pass we just copy the original reference */


        // need to clone so that pointers to ->model, ->next are setup 
        // correctly
        comp = (ModelComp*)smc->clone();
        //comp = smc->transcribeToModelComp(am);
        
        am->addComp(comp);
        // this will change comp->model. If the original pointer to StochModel
        // needs to be retained, that should be stored in a stochmodel
        // entry in StochModelComp?
      }

    } // end loop over model components

    {
    /* FIXME: need to add indexing to this model  */
    /* 
       The indexing expression should look something like 
       "all children of current node":
       
       {i in NODES: A[i]==nd}

       where nd is the current node. I guess we have to build the SyntaxNode
       representation of this expression by hand.

       so this should really read
       set indS1 := {i in NODES: A[i] in indS0_sub}
       and use indS1 as the indexing set

    */
    
      /* so this is a set definition ModelComp with an SyntaxNode tree 
         dscribing the indexing set
       */
      
      // start with the "i in NODES" bit
      SyntaxNode *on1, *on2, *on_iinN, *onai, *on3;
      StochModelComp *smctmp;
      // NODES is a reference to the NODES set that is part of the 
      // smodel description

      if (stgcnt==0){
        /* add this for the zeroth (root) stage to identify the name of
           the root node */
        // set rootset := {this_nd in NODES:Parent[this_nd] == "null"};
        // on_iinn: this_nd in NODES
        on_iinN = new OpNode(IN, new IDNode("this_nd"), nodeset->clone());
        // onai: A[this_nd]  
        onai= new SyntaxNode(LSBRACKET, anc->clone(), 
                       new ListNode(COMMA, new IDNode("this_nd")));
        // on2: A[this_nd]=="null"
        on2 =  new OpNode(EQ, onai, new IDNode("\"null\""));
        // on1: :={this_nd in NODES:Parent[this_nd] == "null"};
        on1 = new OpNode(DEFINED, 
          new SyntaxNode(LBRACE, new SyntaxNode(COLON, on_iinN, on2)));
        // and add this to the model
        smctmp = new StochModelComp("rootset", TSET, NULL, on1);
        smctmp->stochmodel = this;
        am->addComp(smctmp);
      }
      /* EITHER we can set this up as an ID node with name NODES and do a
         search for it by calling find_var_ref_in_context
         BUT: find_var_ref_in_context needs the context set up and that
         wont be the case?
         OR directly set up the IDREF node (for this we have to know where
         the definition statement of this model components (NODES) is
         to be found */
      
      on1 = nodeset->clone(); // does this work?
      // this is going to be the dummy variable i (just an ID node?)
      on2 = new IDNode("this_nd");
      /** @bug 'this_nd' is a reserved variables now */
      on_iinN = new OpNode(IN, on2, on1);
      //printf("Test first part of expression: %s\n",on_iinN->print());
      // set up the 'A[i] in indS0' part now


      // this is the A[i] part: just clone the corresponding anc node and add
      // a comma separated list consisting of only i (IDREF) to it?
      on1 = anc->clone();
      // this is the comma separated list
      /* I think that  dummy variable is just left as a ID */
      on2 = new ListNode(COMMA, new IDNode("this_nd"));
      // this is A[i]
      onai= new SyntaxNode(LSBRACKET, on1, on2);
      // this is the A[i] in indSO
      //printf("Test second part of expression: %s\n",onai->print());

      /* this is a reference to indS0 which is the indexing set of the
         model below (or root if there is none below)
         Trouble is that the model below has not been set up yet... */
      if (stgcnt>0){
        //on3 = new SyntaxNodeIDREF(indset[stgcnt-1]);
        ostringstream dummy_var;
        dummy_var << "ix" << stgcnt-1;
        on3 = new IDNode(dummy_var.str());
        on2 = new OpNode(EQ, onai, on3);
      }else{
        /* No, the first indexing set is not over the nodes that have "root"
           as parent (this would require the root node to be always named 
           "root"), but rather "root" is the set of nodes (should be only one) 
           that have "null/none" as parent. The first indexing set is the nodes
           that have this node as parent:
           
           set rootset := {this_nd in NODES:Parent[this_nd] = "null"};
           set alm0_ind0 := {this_nd in NODES: Parent[this_nd] in rootset};
        */

        //on3 = new SyntaxNode(ID, strdup("\"root\"")); //??? this root is a literal not an ID!
        //on3 = new SyntaxNode(ID, strdup("rootset"));
        on3 = new SyntaxNodeIDREF(smctmp);
        on2 = new OpNode(IN, onai, on3);
      }
      // problem: Since indset[stgct-1] is not set up it, the expression 
      // cannot be printed here
      //printf("Test third part of expression: %s\n",on2->print());

      // and build everything
      on1 = new SyntaxNode(COLON, on_iinN, on2);
      // and add curly brackets to it
      on3 = new SyntaxNode(LBRACE, on1);
      // and the :=
      on1 = new OpNode(DEFINED, on3);
      //printf("Test all of expression: %s\n",on1->print());
      
      // so we've got an SyntaxNode to '{i in NODES:A[i] in indS0}'
      
      /* Add this as a model component defining a set? */
      char buf[20];
      sprintf(buf, "ind%s",(*st).c_str());
      (indset[stgcnt])->setTo(/*name*/ buf, /*type*/TSET, /*ix*/NULL, on1);
      indset[stgcnt]->stochmodel = this;
      am->addComp(indset[stgcnt]);


      if (model_above){
        // create a dummy variable
        ostringstream dummy_var;
        dummy_var << "ix" << stgcnt;

        // add a submodel component that points to the model above 
        // need to create an indexing expression for the model above
        //on1 = new SyntaxNode(ID, strdup(buf)); //indset
        on1 = new SyntaxNodeIDREF(indset[stgcnt]); //indset
        on_iinN = new OpNode(IN, new IDNode(dummy_var.str()), on1); // i in N
        on2 = new SyntaxNode(LBRACE, on_iinN);    // {i in N}
        //printf("Indexing Expression: %s\n",on2->print());

        ModelComp *newmc = new ModelComp(model_above->name, TMODEL, 
                                           new SyntaxNodeIx(on2), NULL);
        //ModelComp *newmc = new ModelComp(strdup(((*st)++).c_str()), TMODEL, 
        //                                   new SyntaxNodeIx(on2), NULL);
        newmc->other = model_above;
        am->addComp(newmc);
        model_above->node = newmc;
        model_above->ix = newmc->indexing;
      }
      
      model_above = am;
    }
  } // end loop over stages

  
  am->parent = parent;
  am->setGlobalNameRecursive();
  am->node = node;
  am->ix = node->indexing;

  printf(" -----------------------------------------------------------\n");
  printf(" StochModel::expandToFlatModel: Finished Pass 1: ");
  if (GlobalVariables::prtLvl>1)
    printf("printing FlatModel tree:");
  printf("\n");
  printf(" -----------------------------------------------------------\n");
  if (GlobalVariables::prtLvl>1) am->print();

  /* =========================== PASS 2 ================================== */
     
  /* recursively work on all the ModelComps in the model tree and 
     convert them to version whose IDREF nodes are resolved with 
     respect to the new AmplModel tree */

  _transcribeComponents(am, 0);
  // and tidy up changes
  AmplModel::applyChanges();
  am->reassignDependencies();

  printf(" -----------------------------------------------------------\n");
  printf(" StochModel::expandToFlatModel: Finished converting:");
  if (GlobalVariables::prtLvl>1)
    printf(" printing FlatModel tree:");
  printf("\n");
  printf(" -----------------------------------------------------------\n");
  if (GlobalVariables::prtLvl>1){
    am->print();
    printf(" -----------------------------------------------------------\n");
  }
  return am;
}


/*-----------------------------------------------------------------------------
StochModel::_transcribeComponents(AmplModel *current, int level)
-----------------------------------------------------------------------------*/
/** This routine recursively calls StochModelComp::transcribeToModelComp
 *  for all components of this StochModel
 *
 *  It sets SyntaxNode::stage and SyntaxNode::node to the correct values for each
 *  new AmplModel encountered in the recursion
 *
 *  @param current The AmplModel that is currently worked on. I.e. in
 *       the current level of the recursion the routine is working on
 *       AmplModel current within this StochModel.
 *  @param level The recursion level (to work out the correct way to resolve
 *       the 'stage' and 'node' keywords and 'ancestor' references)
 * 
 */
void
StochModel::_transcribeComponents(AmplModel *current, int level)
{
  ModelComp *mc;
  list<SyntaxNode*> dv;
  // need to set stage and node for the current model
  
  /* What should we do here: I think use quotation marks if the set of stages
     is symbolic. otherwise don't use quotation marks */

  if (is_symbolic_stages){
    StageNodeNode::stage = "\""+stagenames.at(level)+"\"";
  } else {
    StageNodeNode::stage = stagenames.at(level);
  }

  if (level==0){
    StageNodeNode::node = "\"root\"";
  }else{
    SyntaxNodeIx *cnix = current->node->indexing;
    dv = cnix->getListDummyVars();
    StageNodeNode::node = (dv.front())->print();
  }

  //list<ModelComp*> newcomps(current->comps.size());
  list<ModelComp*> newcomps;
  // loop through all the entities in this model
  for(list<ModelComp*>::iterator p = current->comps.begin();
      p!=current->comps.end();p++){
    mc = *p;
    if (mc->type==TMODEL){
      _transcribeComponents((AmplModel*)mc->other, level+1);
      newcomps.push_back(mc);
    }else{
      /* The component in question is just a pointer to the original
         StochModelComp: need to resolve this with respect to the current 
         setting */
      StochModelComp *smc;
      ModelComp* mcnew;
      smc = dynamic_cast<StochModelComp*>(mc);
      if (smc){
        mcnew = smc->transcribeToModelComp(current, level);
        newcomps.push_back(mcnew);
      }else{
        // This is not a StochModelComp: this should be an indexing set 
        // definition for a submodel. 
        printf("StochModel::_transcribeComponents: unwritten branch executed\n");
        exit(1);
      }
    }
  }
  current->comps = newcomps;

}

void StochModel::addComp(ModelComp *comp) {
   AmplModel::addComp(comp);
   ((StochModelComp*) comp)->stochmodel = this;
}

#ifdef OLD
void
StochModel::_transcribeComponents(AmplModel *current, int level)
{
  ModelComp *mc, *prev;
  list<char*>* dv;
  // need to set stage and node for the current model
  SyntaxNode::stage = "\""+stagenames.at(level)+"\"";
  if (level==0){
    SyntaxNode::node = "\"root\"";
  }else{
    SyntaxNodeIx *cnix = current->node->indexing;
    dv = cnix->getListDummyVars();
    SyntaxNode::node = (dv->front());
  }
  // loop through all the entities in this model
  for(mc=current->first;mc!=NULL;mc = mc->next){
    if (mc->type==TMODEL){
      _transcribeComponents((AmplModel*)mc->other, level+1);
      if (mc!=current->first){
        prev->next = mc;
      }
      prev = mc;
    }else{
      /* The component in question is just a pointer to the original
         StochModelComp: need to resolve this with respect to the current 
         setting */
      StochModelComp *smc;
      ModelComp* mcnew;
      smc = dynamic_cast<StochModelComp*>(mc);
      if (smc){
        mcnew = smc->transcribeToModelComp(current, level);
      
        // and place this on the model, instead of mc
        if (mc==current->first) {
          current->first = mcnew;
        }else{
          prev->next = mcnew;
        }
        prev = mcnew;
        mcnew->next = NULL;
      }else{
        // This is not a StochModelComp: this should be an indexing set 
        // definition for a submodel. 
        
      }
    }
  }
}
#endif
