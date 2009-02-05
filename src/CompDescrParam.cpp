#include "CompDescrParam.h"
#include "GlobalVariables.h"
#include "ModelComp.h"
#include "nodes.h"
#include <assert.h>
#include "data.tab.h"
#include "misc.h"


/* ---------------------------------------------------------------------------
CompDescrParam::CompDescrParam()
---------------------------------------------------------------------------- */
CompDescrParam::CompDescrParam():
  nix(-1),
  indices(NULL),
  n(-1),
  is_symbolic(false),
  values(NULL),
  symvalues(NULL)
{}

/* ---------------------------------------------------------------------------
CompDescrParam::CompDescrParam(ModelComp *mc, SyntaxNode *desc)
---------------------------------------------------------------------------- */
/** Parses the parameter description given in a data file
 *  This routine constructs a CompDescrParam (the actual parameter values)
 *  from a tree of SyntaxNodes originating from the data file
 *  @param mc   Reference to the Component in the model file
 *               (so we can get indexing sets and dimension)
 *  @param desc  The SyntaxNode tree giving the parameter value description as
 *               expressed in the data file
 */
CompDescrParam::CompDescrParam(ModelComp *mc, SyntaxNode *desc):
  nix(-1),
  indices(NULL),
  n(-1),
  nread(-1),
  is_symbolic(false),
  values(NULL),
  symvalues(NULL)
{

  /* 
     1) Match the parameter with a description in the model file and work
        out the dimension of the parameter (scalar?, vector?, array?) and
        how many parameter values to expect
     2) Parse the SyntaxNode-tree and fill in the corresponding entries of
        this object
  */


  /* ------------- 1) Work out dimension of parameter --------------------- */

  SyntaxNode *paramspec;

  /* First work out the dimension and cardinality of this parameter: 
     for this analyse the indexing expression given in the corresponding
     model component */
  SyntaxNodeIx *ix = mc->indexing;

 /* this should just be a comma separated list of sets */
  /* SyntaxNodeIx has components: 
     - int ncomp
     - SyntaxNode **sets
     - SyntaxNode *qualifier (if any ":" expression is given) */

  if (ix){
    /* if an indexing set is given, then the parameter is a vector (matrix) */
    /* work out the dimension and cardinality of the parameter */
    n = 1;
    nsets = ix->ncomp; // the number of indexing sets given 
    indices = (Set **)malloc(ix->ncomp*sizeof(Set *));
    nix =0;
    for(int i=0;i<ix->ncomp;i++){
      indices[i] = dynamic_cast<Set*>((ix->sets_mc[i])->value);
      if (indices[i]==NULL){
        printf("Value of parameter %s given, before indexing set: %s is defined\n",
               mc->id, (ix->sets_mc[i])->id);
        /* Of course it is possible to define the membership of the sets at 
           the same time as the parameters 
           Although a particular syntax needs to be used. Should probably have
           a separate routine for that case
        */
        exit(1);
      }
      n *= indices[i]->size(); // number of parameters is cartesian product of sets 
      nix += indices[i]->dim;
    }
  }else{ /* no indexing expression => scalar parameter */
    nix = 0;
    nsets = 0;
    n = 1;
  }
  nread = 0;

  /* --------- 2) parse the actual parameter description ----------------- */

  /* Parameter declarations can look as follows
   - data_paramdef: PARAM param_name paramdefault_opt DEFINED paramspec_list
                    PARAM param_name paramdefault_opt value_table_list  

   - data_paramdef_alt: 
        PARAM paramdefault_opt COLON param_name_list DEFINED value_list
        PARAM paramdefault_opt COLON set_name COLON param_name_list 
            DEFINED value_list

     + paramspec: param_template_opt value value_list
                  param_template_opt value_table_list

     + value_table: (TR) COLON col_label_list DEFINED value_list
  
   NOTE: The second version is not supported yet, and will probably be
         implemented by a different function(?)

                      TOKPARAMSPECLIST
                    /                  \
   (TOKPARAMTEMPLATE)                      TOKVALUETABLELIST
            |                                      |
   ' ' (placeholder for value_list)          TOKVALUETABLE
            |                                      |
          values                 ' '(col_label_list), ' '(value_list)
                                           |                   |
                                         col_labels           values

   In any case, the tree should start with a TOKPARAMSPECLIST token

     OK, we are probably ready to analyse the actual parameter description 


   - paramspec_list: ' ' separated list of 'paramspec'

   - paramspec:  param_template_opt value value_list
     param_template_opt value_table_list
   the last two are not decided yet how they are represented on the tree
   I guess we should invent some clear tokens
  */

  if (is_symbolic){
    symvalues = new string[n]; 
  }else{
    values = new double[n];
  }

  // the paramdefinition is a list of PARAMSPEC's
  assert(desc->opCode==TOKPARAMSPECLIST);

  // the value list at the heart of the parameter specification is a list 
  // of the form
  // object ... object entry
  // with nobj objects. The objects specify for which component of the
  // indexing set(s) the value is given. 
  // Some components of the indexing set(s) can be given by templates
  // in which case nobj is reduced
  int nobj = nix;
  for(SyntaxNode::Iterator i=desc->begin(); i!=desc->end(); ++i){
    SyntaxNode *templ = NULL;
    paramspec = *i;

    // either of which can have a param_template
    if (paramspec->opCode==TOKPARAMTEMPLATE){
      SyntaxNode::Iterator psi = paramspec->begin();
      templ = (SyntaxNode*)*psi;
      paramspec = (SyntaxNode*)*(++psi);
    }

    // now this can be either a "value list" or a "value table list"
    if (paramspec->opCode==TOKVALUETABLELIST){
      processValueTableList(paramspec, ix);
    }else{
      // this is a value list, i.e. a ' '-separated list of values
      // need to know the dimension of the parameter
      assert(paramspec->opCode==' ');
      int nval = paramspec->nchild()/(nobj+1);
      if (paramspec->nchild()%(nobj+1)!=0){
        cerr << "Paramdef requires " << nobj << " position specifiers." << endl;
        cerr << "Length of value list is " << paramspec->nchild() << 
          " which is not divisible by " << nobj+1 << "." << endl;
        exit(1);
      }
      // loop through all parameter values that are given
      for(int j=0;j<nval;j++){
        // need to think how these are stored
        int pos_in_paramspec = j*(nobj+1);
        
        // get the "nobj" identifiers
        int pos_in_array = 0;
        for(int k=0;k<nobj;k++){
          pos_in_array*= indices[k]->size();
          SyntaxNode *onid = (SyntaxNode *)(paramspec->values[pos_in_paramspec+k]);
          char *obj = onid->getValue();
          // FIXME: if we want to allow multidimensional indexing sets we
          //        need to gather indices[k]->dim entries together and 
          //        convert them into a string[]. This is then passed to
          //        findPos
          pos_in_array += indices[k]->findPos(SetElement(1,&obj));
        }          
        SyntaxNode *on = (SyntaxNode *)paramspec->values[pos_in_paramspec+nobj];
        assert(on->opCode==ID||on->opCode==-99);
        if (is_symbolic){
          if (on->opCode==ID){
            symvalues[pos_in_array] = string(((IDNode *)on)->name);
            //symvalues[pos_in_array] = string((char*)on->values[0]);
          }else{
            cerr << "symbolic but not ID? help!" << endl;
            throw exception();
            symvalues[pos_in_array] = to_string(on->values[0]);
          }            
        }else{ // not symbolic => numeric
          values[pos_in_array] = ((ValueNodeBase*)on)->getFloatVal();
        }
        nread++;
      }// end loop over j
    }

  }
  if (GlobalVariables::prtLvl>0){
    printf("Paramdef finished processing: %s\n",mc->id);
    printf("  Read %d values, need %d\n",nread, n);
  }
}

/* ---------------------------------------------------------------------------
CompDescrParam::printToString
---------------------------------------------------------------------------- */
string
CompDescrParam::printToString()
{
  string str = "";
  
  if (nix==0) str += "scalar";
  for(int i=0;i<nix;i++){
    if (i>0) str+="x";
    Set *st = indices[i];
    str += to_string(st->dim);
  }

  str+=": ";
  for(int i=0;i<n;i++){
    if (i>0) str += " ";
    if (is_symbolic){
      str += symvalues[i];
    }else{
      str += to_string(values[i]);
    }
  }
  return str;
}

/* ---------------------------------------------------------------------------
CompDescrParam::processValueTableList
---------------------------------------------------------------------------- */
/** processes a part of a SyntaxNode-tree representing a value_table_list 
 *  @param node  The SyntaxNode of type TOKVALUETABLELIST that describes the
 *               values
 *  @param ix    The indexing expression of the ModelComp that represents the
 *               parameter (to get ix->sets_mc).
 */
void
CompDescrParam::processValueTableList(SyntaxNode *node, SyntaxNodeIx *ix){
  // this is a value_table_list
  
  /* An SyntaxNode of type TOKVALUETABLELIST can have the following structure:
     - TOKVALUETABLELIST has ->nval children of type TOKVALUETABLE
     - TOKVALUETABLE has 2 children:
       + col_label_list
       + value list
     - col_label_list has opCode=' ' and is a list of objects 
     - value list has opCode=' ' and a list of objects
     - object has opCode=ID and a pointer to the object in question

                 TOKVALUETABLELIST
                          |
                   TOKVALUETABLE
                          |
      ' '(col_label_list), ' '(value_list)
              |                   |
            col_labels           values 
   
     In the simplest case TOKVALUETABLELIST gives values for a 2 dim
     parameter array. The col_label_list gives the indices for the
     first indexing set (i.e. should have a matching no of arguments: n1)
     The value_list consists of rows giving the index for the second
     indexing set and the values (i.e. should have (n1+1)*n2 arguments
  */
  
  /* In the simplest case the col_labels refer to the indexing set in pos 0
     and the row_labels to the indexing set in pos 1. For 'TR' these two
     are interchanged. When using templates these might be different
     values:  */
  
  int ixcolset = 1;
  int ixrowset = 0;
  
  /* Generalisations of this are:
     - A value table could have row/col indices reversed 
     ("TR" in SML file, how is this marked?:
     + TOKVALUETABLETR?
     + a third (dummy) argument to TOKVALUETABLE?
     - 
  */
  

  // loop through all value_table's
  for(SyntaxNode::Iterator i=node->begin(); i!=node->end(); ++i){
    SyntaxNode *on_valtab = (SyntaxNode*)*i;
    assert(on_valtab->opCode==TOKVALUETABLE);
    
    // get the dimensions of the value_table_list
    assert(on_valtab->nchild()==2);
    SyntaxNode::Iterator ovti = on_valtab->begin();
    SyntaxNode *on_collabel = (SyntaxNode*)*ovti;
    SyntaxNode *on_values = (SyntaxNode*)*(++ovti);
    
    int ncol = on_collabel->nchild();
    int nval = on_values->nchild();
    if (nval%(ncol+1)!=0){
      printf("Error in processing value_table:\n");
      printf("Number of values given (%d) is not divisable by",nval);
      printf(" (col_labels+1) (=%d)\n",ncol+1);
      exit(1);
    }
    // dimensions seem to tally, now get a list of row and column
    // labels and look up their position in the appropriate indexing sets
    int nrow = nval/(ncol+1);
    int *rowpos = new int[nrow];
    int *colpos = new int[ncol];
    // process the col_label list first
    
    int jj=0;
    for(SyntaxNode::Iterator j=on_collabel->begin();j!=on_collabel->end();++j,++jj){
      SyntaxNode *cl = (SyntaxNode*)*j;
      assert(cl->opCode==ID || cl->opCode==LBRACKET);
      // need to get reference to the first indexing set and 
      // ask it for the position of this label
      
      // make the label into a SetElement
      if (indices[ixcolset]->dim>1){
        if (cl->opCode!=LBRACKET){
          printf("col_label for multidimensional set '%s' must be a bracketed '(..,..)' expression\n", ix->sets_mc[ixcolset]->id);
          exit(1);
        }
        if (cl->nchild()!=indices[ixcolset]->dim){
          cerr << "Number of entries in bracketed expression used as "
             "col_label (" << cl->nchild() << ")" << endl;
          cerr << "does not match dimension (" << indices[ixcolset]->dim <<
             ") of indexing set '" << ix->sets_mc[ixcolset]->id << "'" << endl;
          exit(1);
        }
        cl = *(cl->begin());
        assert(cl->opCode==COMMA);
        colpos[jj] = indices[ixcolset]->findPos(SetElement(1,
          (IDNode **)cl->values));
      }else{
        // this is a set of dim 1
        colpos[jj] = indices[ixcolset]->findPos(SetElement(1,(IDNode **) &cl));
      }
      
    }
    
    // also do the same loop for row_labels
    for(int j=0;j<nrow;j++){
      int pos = j*(ncol+1); // get position of next row label
      SyntaxNode *rl = (SyntaxNode*)on_values->values[pos];
      assert(rl->opCode==ID || rl->opCode==LBRACKET);
      // need to get reference to the first indexing set and 
      // ask it for the position of this label
      
      // make the label into a SetElement
      if (indices[ixrowset]->dim>1){
        if (rl->opCode!=LBRACKET){
          printf("row_label for multidimensional set '%s' must be a bracketed '(..,..)' expression\n", ix->sets_mc[ixrowset]->id);
          exit(1);
        }
        if (rl->nchild()!=indices[ixrowset]->dim){
          cerr << "Number of entries in bracketed expression used as "
            "row_label (" << rl->nchild() << ")" << endl;
          cerr << "does not match dimension (" << indices[ixrowset]->dim <<
            ") of indexing set '" << ix->sets_mc[ixrowset]->id << "'" << endl;
          exit(1);
        }
        rl = (SyntaxNode*)rl->values[0];
        assert(rl->opCode==COMMA);
        rowpos[j] = indices[ixrowset]->findPos(SetElement(1, (IDNode**)rl->values));
      }else{
        // this is a set of dim 1
        rowpos[j] = indices[ixrowset]->findPos(SetElement(1, (IDNode**)&rl));
      }
    }
    // OK, we now have the lists colpos/rowpos that tell us where the
    // elements go in values/symvalues
    for (int j=0;j<ncol;j++){
      for (int k=0;k<nrow;k++){
        int poslist = (j+1)+k*(ncol+1);
        int posparam = colpos[j]+rowpos[k]*indices[ixcolset]->size();
        SyntaxNode *entry = (SyntaxNode*)on_values->values[poslist];
        if (is_symbolic){
          assert(entry->opCode==ID);
          symvalues[posparam] = string(((IDNode *)entry)->name);
        }else{
          values[posparam] = ((ValueNodeBase *) entry)->getFloatVal();
        }
        nread++;
      }
    }
  }


}
