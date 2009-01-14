#ifndef MODELCOMP_H
#define MODELCOMP_H

enum {NOARG=0,WITHARG=1,ONLYARG=2};
typedef enum {TVAR, TCON, TPARAM, TSET, TMIN, TMAX, TMODEL, TNOTYPE} compType;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include "nodes.h"
#include "CompDescr.h"
#include "ampl.h"


using namespace std;
/* ------------------------------------------------------------------------ */
/* model_comp describes a model component such as 
   var/subject to/param/set/minimize 
   
   every such component is stored as
   - id : the name of the component 
   - indexing: the indexing expression stored as a tree 
   - attributes: a list of attributes 
                 (this includes the actual constraint definition for 
		  "subject to" components)
*/
class AmplModel;

static char *nameTypes[7] = {"variable","constraint","parameter",
			     "set", "objective min","objective max", 
			     "submodel"};
static char *compTypes[7] = {"var","subject to","param",
			     "set", "minimize","maximize","block"};
/** 
 * @class model_comp
 * @brief Object to represent a component of an AMPL/SML model/block.
 *
 * The model_comp object represents a component of an SML model/block.
 * It usually represents one line of AMPL/SML which is a definition of a
 * variable/parameter/set/constraint/objective/block.
 * A model component is broken down into the following parts:
 * - \<TYPE\> \<name\>\<indexing\>_opt \<attributes\>_opt
 */
class model_comp{
 public:
  compType type;   //!< the type of the component
  char *id;        //!< the name of the component
  //int ismin;       //!< for an objective ismin==1 ->minimize, ==0 ->maximize 

  /** this is a tree of specifications, this includes
   * :=, within, default, >=                                               */
  opNode *attributes;   
			 
  opNodeIx *indexing; //!< indexing expression 

  ///** model_comp is set up as a double linked list 
  // *  @attention This should be implemented in the AmplModel as a 
  // *             list<model_comp>, rather than the linked list here */
  //model_comp *next;    //!< next component in a double linked list 
  //model_comp *prev;    //!< previous component in a double linked list 
  
  /** List of all entities that this model component depends on:
   *  Basically a list of all model components used in the definition of
   *  this component                                                  */
  list<model_comp*> dependencies;     //!< list of dependecies:

  AmplModel *model;    //!< The model this belongs to 

  /** A pointer to an AmplModel structure for components of type MODEL
   *  @attention Better implemented as a subclass of model_comp  */
  void *other;      

  int count;           //!< instance number of model_comp

  /** Components can be tagged by the tagDependencies method which sets
   *  this tag for this components and everything that it depends on 
   *  (i.e. everything listed in the dependencies list).                 */
  bool tag;            //!< true if part of the current 'needed' set

  /** for sets and parameters this points to an object that gives the
   *  values and further specific information (Set for sets)
   */
  CompDescr *value;   //!< value (for sets and parameters)
  static list<model_comp*> global_list; //!< global list of all defined comps
  static int tt_count;    //!< number of model_comps defined
  // ------------------------- METHODS ----------------------------------
  // constructor
  model_comp(char *id, compType type, opNode *indexing, opNode *attrib);
  model_comp();  //< constructor that sets everything to default values

  /** set up an existing model comp to specified values   */
  void setTo(char *id, compType type, opNodeIx *indexing, opNode *attrib); 

  /** set up list of dependencies for this component */
  void setUpDependencies();
  void dump(FILE *fout);   //!< detailed debugging output
  void print();   //!< prints elements of the class
  void printBrief();   //!< prints one liner
  void tagDependencies();  //!< tag this components and all its dependencies
                           // recuresively
  list<string> getSetMembership(); //!< get a list of members of the set

  /** recalculate dependency list and re-resolve IDREF nodes */
  void reassignDependencies();

  /** set the tag to false for all models: using global_list */
  static void untagAll();  

  /** recursively set the tag to false for all models */
  static void untagAll(AmplModel *start);  

  /** write name of all tagged components: using global_list */
  static void writeAllTagged();

  /** recursively write name of all tagged components */
  static void writeAllTagged(AmplModel *start);

  /** write name of all tagged components to file: using global_list */
  static void writeAllTagged(FILE *fout); 

  /** recursively write name of all tagged components to file */
  static void writeAllTagged(FILE *fout, AmplModel *start); 

  /** write definition of all tagged components to file, using global_list */
  static void modifiedWriteAllTagged(FILE *fout); 

  /** recursively write definition of all tagged components to file */
  static void modifiedWriteAllTagged(FILE *fout, AmplModel *start); 

  void moveUp(int level);  //< move this model comp up in the model tree 
  virtual model_comp *clone();     //< duplicate the object: shallow copy
  model_comp *deep_copy(); //< duplicate the object: deep copy

  virtual void foo();

};

/** @class ModelCompSet
 *  @brief The class describes a model component of type set 
 * 
 *  The class describes properties of a model component particular to sets
 */
class ModelCompSet: public model_comp {
 public:
  bool is_symbolic; //!< indicates whether or not this set is symbolic
};

char *
getGlobalName(model_comp *node, opNode *opn, 
	      AmplModel *current_model, int witharg);
char *
getGlobalNameNew(model_comp *node, opNode *opn, 
	      AmplModel *current_model, int witharg);
#endif
