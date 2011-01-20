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

#include "StochModelComp.h"
#include "misc.h"
#include "nodes.h"
#include "sml.tab.h"
#include <cstdlib>
#include <iostream>

using namespace std;

static bool prtSM = false;

StochModelComp::StochModelComp(const string& id_):
  ModelComp(id_) {}

StochModelComp::StochModelComp(const string& id_, compType type_,
                               SyntaxNode *indexing_, SyntaxNode *attrib,
                               StochModel *stoch):
  ModelComp(id_, type_, indexing_, attrib),
  stochmodel(stoch) {}

/** Transcribe a StochModelComp in a StochModel into a ModelComp.
 *
 *  This function takes a StochModelComp as read in by the parser and
 *  transcribes it into a corresponding ModelComp of the current
 *  FlatModel. It does this by:
 *   - Scanning for all IDREF references to entities defined in the StochModel
 *     and replacing this by references to entities in the FlatModel
 *     (i.e the pointer to a StochModelComp is replaced by a pointer to
 *      the corresponding ModelComp).
 *     This also deals with references to StochModel entities in a different
 *     stage (i.e. through xh(-1;...))
 *   - Objective components have a term for the node probability added
 *   - replacing special StochModel constructs (i.e. Exp(...)) by their
 *     corresponding constructs in the FlatModel
 *  @param[in] current_model
 *             The current AmplModel that all references should be resolved to.
 *  @param[in] nodedummy
 *             The name of the dummy index for the node set.
 *  @param[in] stagedummy
 *             The name of the dummy index for the stage set.
 *  @param[in] level
 *             The level of this AmplModel within the StochModel (root is 0).
 *
 *  @pre SyntaxNode::stage and SyntaxNode::node need to be set.
 *
 *  @note The gloabl variables SyntaxNode::stage and SyntaxNode::node are used
 *  to replace all NODE and STAGE nodes in the attribute list.
 */
ModelComp *
StochModelComp::transcribeToModelComp(AmplModel *current_model,
                                      const string &nodedummy,
                                      const string &stagedummy,
                                      const int level)
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
     (3a) replace them with the values in SyntaxNode::stage and SyntaxNode::node
     (4)  find all EXP nodes in tbe attribute section
     (4a) replace them by path probabilities
     //(4)  if this is an OBJ component, then add probabilities to it
  */
  ModelComp *newmc;
  list<SyntaxNode*> idrefnodes;
  StochModel *thissm = this->stochmodel;
  if (thissm==NULL){
    cerr << "ERROR: SMC.transcribeToModelComp: this->stochmodel not set.\n";
    exit(1);
  }
  if (prtSM)
    cout << "Call transcribe for " << id << " level " << level << "\n";

  // ---------- (1) make deep_copy of StochModelComp ------------------------

  newmc = deep_copy(); // clone the current node
  
  // ---------- (2) find list of all IDREF nodes in indexing/attributes ------

  // find all IDREF nodes that are referenced in the attributes section
  if (newmc->indexing) (newmc->indexing)->findIDREF(&idrefnodes);
  if (newmc->attributes) (newmc->attributes)->findIDREF(&idrefnodes);

  // ---------- (2a) resolve IDREF nodes w.r.t AmplModel tree ----------------

  // loop through all IDREFs that are in dependency list
  for (list<SyntaxNode*>::iterator p = idrefnodes.begin();
       p != idrefnodes.end(); p++) {
    // check if this is a reference within the current StochModel
    // (*p) is a SyntaxNodeIDREF
    SyntaxNodeIDREF *onr = dynamic_cast<SyntaxNodeIDREF*>(*p);
    if (onr==NULL){
      cerr << "SyntaxNode should be SyntaxNodeIDREF but dynamic cast fails" << endl;
      exit(1);
    }
    ModelComp *mc = onr->ref;
    if (mc->model==thissm){
      // ok, component refered to belongs to StochModel
      // => change change the ->ref of this SyntaxNodeIDREF to point to
      //    a model comp in the ModelComp model

      // set the correct model w.r.t which this should be resolved
      // (deal with ancestor(1).x notation)
      model = current_model;
      for (int lvl = onr->getStochParent(); lvl > 0; lvl--) {
        model = model->parent;
        if (model==NULL){
          cerr << "ERROR: Ancestor " << onr->getStochParent()
               << " doesn't exist.\n";
          exit(1);
        }
      }
      
      // search for this entity in the current model
      bool fnd = false;
      for(list<ModelComp*>::iterator q = model->comps.begin();
          q != model->comps.end(); q++) {
        ModelComp *amc = *q;
        // all we can do is judge by name
        if (mc->id == amc->id) {
          onr->ref = amc;
          fnd = true;
          break;
        }
      }
      if (!fnd){
        cerr << "ERROR: no entity named '" << mc->id
             << "' found in current model.\n";
        exit(1);
      }
    }

  }// end loop over IDREF nodes

  // ---------- (3) find STAGE/NODE nodes in attributes ----------------------
  
  // find all STAGE & NODE nodes
  idrefnodes.clear();
  if (newmc->attributes){
    newmc->attributes->findOpCode(ID, &idrefnodes);
  }

  // ---------- (3a) and replace them by text -------------------------------

  for (list<SyntaxNode*>::iterator p = idrefnodes.begin();
       p != idrefnodes.end(); p++) {
    IDNode *node = static_cast<IDNode *>(*p);
    if (node->id() == stagedummy)
      node->setName(StageNodeNode::stage);
    else if (node->id() == nodedummy)
      node->setName(StageNodeNode::node);
  }

  // ---------- (4) add probablities to Exp components ---------------

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
   */

  // find all STAGE & NODE nodes
  idrefnodes.clear();
  if (newmc->attributes){
    newmc->attributes->findOpCode(EXPECTATION, &idrefnodes);
  }

  // -------- (4a) and replace them by path probabilities --------------------
  
  // first create the tree of path probabilities for this node in the
  // scenario tree

  AmplModel *thisam = current_model;

  for (list<SyntaxNode*>::iterator p = idrefnodes.begin();
       p != idrefnodes.end(); p++) {
    // use "(*p)->..." to access the EXP-SyntaxNode
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

    SyntaxNode *child = (*p)->front();

    if (child->getOpCode() != COMMA || child->nchild() == 1) {

      // surround the argument of Exp(...) with brackets for the multiplication
      // by the probability to apply correctly to all terms
      SyntaxNode *up = new SyntaxNode(LBRACKET, child);

      // one argument version of Exp within an objective function
      if (type==TMIN || type==TMAX){

        // multiply by the conditional probability terms (one per stage)
        for (int i=level;i>0;i--){
          
          // find the dummy variable expression
          SyntaxNodeIx *cnix = thisam->node->indexing;
          list<SyntaxNode *> dv = cnix->getListDummyVars();
          //dv->front() is a string giving name of dummy var
          thisam = thisam->parent;
        
          // thissm->prob is an ID SyntaxNode giving path probabilities
          
          // create the *CP[ix0] term
          SyntaxNodeIDREF *opn_prob = dynamic_cast<SyntaxNodeIDREF*>(thissm->getProbs());
          if (opn_prob==NULL){
            cerr << "ERROR: Probabilities parameter in stochastic block must "
                    "be given as IDREF.\n";
            exit(1);
          }
          SyntaxNodeIDREF *oncp = new SyntaxNodeIDREF(opn_prob->ref, 
             new IDNode((dv.front())->print()));
          up = new OpNode('*', oncp, up);
        }
        // up is now a pointer into the expression, this should
        // replace the EXP node?
        (*p)->clear();
        (*p)->push_back(up);
      }

      // one argument version of Exp used in constraint
      else {
        // => this constraint should be moved to the top level; 
        //    there it will encompass all nodes that are in the current stage
        //    this should become
        //     subject to ExpCons: 
        //       mu = sum{ix0 in almS0_indS0, ix1 in almS0_S1_indS1[ix0]}
        //            CP[ix0]*CP[ix1]*(sum{i in ASSETS}xh[ix0, ix1, i])
        list<SyntaxNode*> listofsum; // expressions in the sum{..}

        for (int i=level;i>0;i--){
          
          // find the dummy variable expression
          SyntaxNodeIx *cnix = thisam->node->indexing;
          list<SyntaxNode *> dv = cnix->getListDummyVars();
          //dv->front() is a string giving name of dummy var
        
          // thissm->prob is an ID SyntaxNode giving path probabilities
          
          // create the *CP[ix0] term
          SyntaxNodeIDREF *opn_prob = dynamic_cast<SyntaxNodeIDREF*>(thissm->getProbs());
          if (opn_prob==NULL){
            cerr << "ERROR: Probabilities parameter in stochastic block must "
                    "be given as IDREF.\n";
            exit(1);
          }
          SyntaxNodeIDREF *oncp = new SyntaxNodeIDREF(opn_prob->ref,
            new IDNode(dv.front()->print()));
          up = new OpNode('*', oncp, up);

          // put together the sum expression 
          // => build the ix0 in almS0_indS0 (which is the indexing expression
          //    used for this model)

          // cnix might contain a '{' => strip it if present
          SyntaxNode *cnixon = cnix;
          if (cnixon->getOpCode() == LBRACE)
            cnixon = cnixon->front();
          listofsum.push_front(cnixon->deep_copy());
          
          thisam = thisam->parent;
        }
        // up is now a pointer into the expression, this should
        // replace the EXP node?

        // create the sum expression: first build comma separated list
        if (listofsum.size()==0){
          cerr << "ERROR: Expectation indexing expression *must* be present.\n";
          exit(1);
        }
        SyntaxNode *cslon = new ListNode(COMMA);
        for(list<SyntaxNode*>::iterator q = listofsum.begin();q!=listofsum.end();
            q++){
          cslon->push_back(*q);
        }
        // and put braces around it
        cslon = new SyntaxNode(LBRACE, cslon);
        
        // now build the sum
        //cout << "Expression to be summed: " << up->print() << "\n";
        //cout << "This is the set of the sum: " << cslon->print() << "\n";
        cslon = new SyntaxNode(SUM, cslon, up);
        //cout << "This is the sum: " << cslon->print() << "\n";

        (*p)->clear();
        (*p)->push_back(cslon);
        
        //FIXME: need to somehow move this model comp to the root
        // model of the stoch prog.
        //print();
        //exit(1);
        // actually queues this to be moved up
        newmc->id = newmc->id + "_up" + to_string(level);
        newmc->moveUp(level);
      }
    }
    if (child->getOpCode() == COMMA && child->nchild() > 1) {
      // this is the two argument version of Exp(..., ...)
      // the second argument is the stage in which the expression should
      // be averaged
      // NEED TO KNOW:
      // - current stage
      // - stage of where averaging should take place
      cerr << "ERROR: Two argument version of Exp() not supported yet.\n";
      exit(1);
    }
  }

  return newmc;
}

/* ---------------------------------------------------------------------------
StochModelComp::clone()
---------------------------------------------------------------------------- */
StochModelComp*
StochModelComp::clone() const {

  // can we call clone for the ModelComp? 
  //  => I guess no, since this would create a ModelComp object and not a
  //     StochModelComp

  StochModelComp *newsmc = new StochModelComp(id);

  newsmc->type = type;
  newsmc->attributes = attributes;
  newsmc->indexing = indexing;
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

void
StochModelComp::addStageName(const string& name) {
  stagenames.push_back(name);
}
