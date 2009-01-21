#include "StochModelComp.h"
#include "StochModel.h"
#include "ampl.tab.h"

static bool prtSM = false;

StochModelComp::StochModelComp():
  model_comp()
{}

StochModelComp::StochModelComp(char *id, compType type, 
			       opNode *indexing, opNode *attrib):
  model_comp(id, type, indexing, attrib)
{}


/* ---------------------------------------------------------------------------
StochModelComp::transcribeToModelComp()
---------------------------------------------------------------------------- */
/** transcribeToModelComp():
 *  This function takes a StochModelComp as read in by the parses and
 *  transcribes it into a corresponding ModelComp of the current
 *  FlatModel. It does this by
 *   - Scanning for all IDREF references to entities defined in the StochModel
 *     and replacing this by references to entities in the FlatModel
 *     (i.e the pointer to a StochModelComp is replaced by a pointer to
 *      the corresponding model_comp)
 *     This also deals with references to StochModel entities in a diffent
 *     stage (i.e. through xh(-1;...))
 *   - Objective components have a term for the node probability added
 *   - replacing special StochModel constructs (i.e. Exp(...) by their
 *     corresponding constructs in the FlatModel
 *  @param[in] current_model The current AmplModel that all references should
 *         be resolved to
 *  @param[in] level The level of this AmplModel within the StochModel
                     (root is 0).
 *  @remarks the gloabl variables opNode::stage and opNode::node are used to 
 *           replace all NODE and STAGE nodes in the attribute list
 *  @pre opNode::stage and opNode::node need to be set.
 */

model_comp *
StochModelComp::transcribeToModelComp(AmplModel *current_model, int level)
{
  /* The routine works as follows:
     (1)  create a deep copy of the StochModelComp
     (2)  find all IDREF nodes in the indexing and attribute section
     (2a) resolve all IDREF nodes with respect to the AmplModel tree,
          rather than the StochModel tree
	  - here xh(-1;i) or xh[i].parent(1) notation is resolved, that 
	    indicates the IDREF should be resolved with respect to AmplModel 
	    nodes higher up in the tree
     (3)  find all STAGE and NODE nodes in the attribute section
     (3a) replace them with the values in opNode::stage and opNode::node
     (4)  find all EXP nodes in tbe attribute section
     (4a) replace them by path probabilities
     //(4)  if this is an OBJ component, then add probablilities to it
  */
  model_comp *newmc;
  list<opNode*> *idrefnodes = new list<opNode*>;
  StochModel *thissm = this->stochmodel;
  if (thissm==NULL){
    printf("SMC.transcribeToModelComp: this->stochmodel not set\n");
    exit(1);
  }
  if (prtSM) printf("Call transcribe for %s level %d\n",id, level);

  // ---------- (1) make deep_copy of StochModelComp ------------------------

  // FIXME: this should be a deep copy
  newmc = deep_copy(); // clone the current node
  //newmc = clone(); // clone the current node
  
  // ---------- (2) find list of all IDREF nodes in indexing/attributes ------

  // find all IDREF nodes that are referenced in the attributes section
  if (newmc->indexing) (newmc->indexing)->findIDREF(idrefnodes);
  if (newmc->attributes) (newmc->attributes)->findIDREF(idrefnodes);

  // ---------- (2a) resolve IDREF nodes w.r.t AmplModel tree ----------------

  // loop through all IDREFs that are in dependency list
  for(list<opNode*>::iterator p=idrefnodes->begin(); p!=idrefnodes->end();p++)
  {
    // check if this is a reference within the current StochModel
    // (*p) is an opNodeIDREF
    opNodeIDREF *onr = dynamic_cast<opNodeIDREF*>(*p);
    if (onr==NULL){
      printf("opNode should be opNodeIDREF but dynamic cast fails\n");
      exit(1);
    }
    model_comp *mc = onr->ref;
    if (mc->model==thissm){
      // ok, component refered to belongs to StochModel
      // => change change the ->ref of this opNodeIDREF to point to
      //    a model comp in the ModelComp model

      // set the correct model w.r.t which this should be resolved
      // (deal with xh(-1;i)/xh[i].parent(i) notation)
      AmplModel *model = current_model;
      for (int lvl=onr->stochparent;lvl>0;lvl--){
	model = model->parent;
	if (model==NULL){
	  printf("Trying to take the %d ancestor in a model that does not have %d ancestors\n",
		 onr->stochparent, onr->stochparent);
	  exit(1);
	}
      }
      
      // search for this entity in the current model
      bool fnd = false;
      for(list<model_comp*>::iterator p = model->comps.begin();
	  p!=model->comps.end();p++){
	model_comp *amc=*p;
	// all we can do is judge by name
	if (strcmp(mc->id, amc->id)==0){
	  onr->ref = amc;
	  fnd = true;
	  break;
	}
      }
      if (!fnd){
	printf("ERROR: no entity named %s found in current model\n",mc->id);
	exit(1);
      }
    }

  }// end loop over IDREF nodes

  // ---------- (3) find STAGE/NODE nodes in attributes ----------------------
  
  // find all STAGE & NODE nodes
  idrefnodes->clear();
  if (newmc->attributes){
    (newmc->attributes)->findOpCode(STAGE,idrefnodes);
    (newmc->attributes)->findOpCode(NODE,idrefnodes);
  }

  // ---------- (3a) and replace them by text -------------------------------

  for(list<opNode*>::iterator p=idrefnodes->begin(); p!=idrefnodes->end();p++)
  {
    (*p)->nval = 1;
    (*p)->values = (void**)calloc(1, sizeof(void*));
    if ((*p)->opCode==STAGE){
      (*p)->values[0] = (void*)strdup(opNode::stage.c_str());
    }else{
      (*p)->values[0] = (void*)strdup(opNode::node.c_str());
    }
    (*p)->opCode = ID;
  }

  // ---------- (4) add probablilities to Exp components ---------------

  /* Exp(..) can be used in two forms in the SML model files:
   *
   * - Exp(xh[i]) in the objective function, such as
   *       maximize FinalWealth stages {last(TIME)}: 
   *                         (1-tc)*Exp(sum{i in ASSETS} xh[i])
   *   in this case the expectation goes over *all* nodes in the stage in
   *   which the objective function was defined. It is transcribed into the
   *   FlatModel file (at the final time stage) as
   *       maximize almS0_S1_S2_FinalWealth 
   *                 {ix0 in almS0_indS0,ix1 in almS0_S1_indS1[ix0]}:
   *                       (1-tc)*CP[ix0]*CP[ix1]*
   *                          (sum {i in ASSETS}almS0_S1_S2_xh[ix0,ix1,i]);
   *   i.e. just the local contribution of this node on the ExpandedModel tree
   *   to the total objective function.
   *
   * - Exp(xh[i], stage) in a constraint, such as
   *      subject to ExpCons stages {first(TIME)}:
   *         (1-tc)*Exp(sum{i in ASSETS}xh[i], last(TIME)) = mu;
   *   in this case it is a constraint *at each node* in the specified time 
   *   stage (in this case the root) that links the nodes in time stage
   *   'last(TIME)' originating from the current node. It is transcribed into
   *   the FlatModel file (at time stage first(TIME)) as:
   *      subject to almS0_ExpCons:
   *         (1-tc)*(sum{ix0 in almS0_indS0, ix1 in almS0_S1_indS1[ix0]}
   *              CP[ix0]*CP[ix1]*(sum{i in ASSETS}almS0_S1_S2_xh[ix0,ix1,i]));
   *
   */


  // find all STAGE & NODE nodes
  idrefnodes->clear();
  if (newmc->attributes){
    (newmc->attributes)->findOpCode(EXPECTATION,idrefnodes);
  }

  // -------- (4a) and replace them by path probabilities --------------------
  
  // first create the tree of path probabilities for this node in the
  // scenario tree

  
  AmplModel *thisam = current_model;

  for(list<opNode*>::iterator p=idrefnodes->begin(); p!=idrefnodes->end();p++)
  {
    // use "(*p)->..." to access the EXP-opNode
    /* need to build a tree of OpNodes that represent
         CP[ix0]*CP[ix1]*
       for this we need to know
       - level within the Stoch Program block
       - list of dummy variable used until here
         (can get this by following down the AmplModels)
       - name of conditional probabilities parameter (CP)
         (can get this from the StochModel)
    */

    // (*p) is the EXPECTATION node, it should have one child

    opNode *child = (opNode*)((*p)->values[0]);

    if (child->opCode!=COMMA||child->nval==1){
      // this is the "one argument" use if Exp within an objective function
      if (type==TMIN || type==TMAX){
	// set "up" to the argument of Exp(...)
	// FIXME: need to put brackets around this?
	opNode *up = (opNode*)((*p)->values[0]);
	for (int i=level;i>0;i--){
	  
	  // find the dummy variable expression
	  opNodeIx *cnix = thisam->node->indexing;
	  list<char*>* dv = cnix->getListDummyVars();
	  //dv->front() is a string giving name of dummy var
	  thisam = thisam->parent;
	
	  // thissm->prob is an ID opNode giving path probabilities
	  
	  // create the *CP[ix0] term
	  opNodeIDREF *opn_prob = dynamic_cast<opNodeIDREF*>(thissm->prob);
	  if (opn_prob==NULL){
	    printf("Probabilities parameter in sblock nust be given as IDREF\n");
	    exit(1);
	  }
	  opNodeIDREF *oncp = new opNodeIDREF(opn_prob->ref);
	  oncp->nval = 1;
	  oncp->values = (void**)calloc(1, sizeof(void*));
	  oncp->values[0] = new opNode(ID, strdup(dv->front()));
	  opNode *onmult = new opNode('*', oncp, up);
	  up = onmult;
	}
	// up/onmult is now a pointer into the expression, this should
	// replace the EXP node?
	(*p)->values[0] = up;
	(*p)->nval = 1;
	(*p)->opCode = 0;

      }else{

	// one argument version of Exp used in constraint
	// => this constraint should be moved to the top level; 
	//    there it will encompass all nodes that are in the current stage
	//    this should become
	//     subject to ExpCons: 
	//       mu = sum{ix0 in almS0_indS0, ix1 in almS0_S1_indS1[ix0]}
	//            CP[ix0]*CP[ix1]*(sum{i in ASSETS}xh[ix0, ix1, i])
	// set "up" to the argument of Exp(...)
	// FIXME: need to put brackets around this?
	list<opNode*> listofsum; // expressions in the sum{..}


	opNode *up = (opNode*)((*p)->values[0]);
	// put brackets around this
	up = new opNode(LBRACKET, up);
	for (int i=level;i>0;i--){
	  
	  // find the dummy variable expression
	  opNodeIx *cnix = thisam->node->indexing;
	  list<char*>* dv = cnix->getListDummyVars();
	  //dv->front() is a string giving name of dummy var
	
	  // thissm->prob is an ID opNode giving path probabilities
	  
	  // create the *CP[ix0] term
	  opNodeIDREF *opn_prob = dynamic_cast<opNodeIDREF*>(thissm->prob);
	  if (opn_prob==NULL){
	    printf("Probabilities parameter in sblock nust be given as IDREF\n");
	    exit(1);
	  }
	  opNodeIDREF *oncp = new opNodeIDREF(opn_prob->ref);
	  oncp->nval = 1;
	  oncp->values = (void**)calloc(1, sizeof(void*));
	  oncp->values[0] = new opNode(ID, strdup(dv->front()));
	  opNode *onmult = new opNode('*', oncp, up);
	  up = onmult;

	  // put together the sum expression 
	  // => build the ix0 in almS0_indS0 (which is the indexing expression
	  //    used for this model)

	  // cnix might contain a '{' => strip it if present
	  opNode *cnixon = cnix;
	  if (cnixon->opCode==LBRACE) cnixon = (opNode*)cnixon->values[0];
	  listofsum.push_front(cnixon->deep_copy());
	  
	  thisam = thisam->parent;
	}
	// up/onmult is now a pointer into the expression, this should
	// replace the EXP node?

	// create the sum expression: first build comma separated list
	opNode *cslon = new opNode();
	cslon->nval = listofsum.size();
	if (cslon->nval==0){
	  printf("Expectation indexing expression *must* be present\n");
	  exit(1);
	}
	cslon->values = (void**)calloc(cslon->nval, sizeof(void*));
	cslon->opCode = COMMA;
	int cnt=0;
	for(list<opNode*>::iterator q = listofsum.begin();q!=listofsum.end();
	    q++){
	  cslon->values[cnt] = (void*)(*q);
	  cnt++;
	}
	// and put braces around it
	cslon = new opNode(LBRACE, cslon);
	

	// now build the sum
	//printf("This is the sum: %s\n",cslon->print());
	cslon = new opNode(SUM, cslon, up);
	//printf("This is the sum: %s\n",cslon->print());

	(*p)->values[0] = cslon;
	(*p)->nval = 1;
	(*p)->opCode = 0;
	
	//FIXME: need to somehow move this model comp to the root
	// model of the stoch prog.
	//print();
	//exit(1);
	// actually queues this to be moved up
	char *id2 = (char *)calloc(strlen(newmc->id)+5, sizeof(char));
	strcpy(id2, newmc->id);
	sprintf(id2+strlen(newmc->id), "_up%d",level);
	free(newmc->id);
	newmc->id = id2;
	newmc->moveUp(level);
	
      }
    }
    if (child->opCode==COMMA&&child->nval>1){
      // this is the two argument version of Exp(..., ...)
      // the second argument is the stage in which the expression should
      // be averaged
      // NEED TO KNOW:
      // - current stage
      // - stage of where averaging should take place
      printf("Two argument version of Exp(...,...) not supported yet\n");
      exit(1);
    }
  }

  

  

  delete(idrefnodes);
  return newmc;
}

/* ---------------------------------------------------------------------------
StochModelComp::clone()
---------------------------------------------------------------------------- */
StochModelComp*
StochModelComp::clone()
{
  // can we call clone for the model_comp? 
  //  => I guess no, since this would create a model_comp object and not a
  //     StochModelComp

  StochModelComp *newsmc = new StochModelComp();

  newsmc->type = type;
  newsmc->id = id;
  //  newsmc->ismin = ismin;
  newsmc->attributes = attributes;
  newsmc->indexing = indexing;
  //  newsmc->next = next;
  //  newsmc->prev = prev;
  newsmc->dependencies = dependencies;
  newsmc->model = model;
  newsmc->stochmodel = stochmodel;
  newsmc->other = other;
  newsmc->count = count;
  newsmc->tag = tag;

  // and clone the additional StochModelComp entries
  
  newsmc->is_deterministic = is_deterministic;
  newsmc->stageset = stageset;
  newsmc->stagenames = stagenames;

  return newsmc;
}
