#ifndef MODELCOMP_H
#define MODELCOMP_H

enum {NOARG=0,WITHARG=1};
typedef enum {TVAR, TCON, TPARAM, TSET, TOBJ, TMODEL, TNOTYPE} compType;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include "nodes.h"
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

static char *nameTypes[6] = {"variable","constraint","parameter",
			    "set", "objective","submodel"};
static char *compTypes[6] = {"var","subject to","param",
			    "set", "minimize","block"};
/** 
 * @class model_comp
 * @brief Object to represent a component of an AMPL/SML model/block.
 *
 * The model_comp object represents a component of an SML model/block.
 * It usually represents one line of AMPL/SML which is a definition of a
 * variable/parameter/set/constraint/objective/block.
 * A model component is broken down into the following parts:
 * - <TYPE> <name><indexing>_opt <attributes>_opt
 */
class model_comp{
 public:
  compType type;   //!< the type of the component
  char *id;        //!< the name of the component
  int ismin;       //!< for an objective ismin==1 ->minimize, ==0 ->maximize 

  /** this is a tree of specifications, this includes
   * :=, within, default, >=                                               */
  opNode *attributes;   
			 
  opNodeIx *indexing; //!< indexing expression 

  /** model_comp is set up as a double linked list 
   *  @attention This should be implemented in the AmplModel as a 
   *             list<model_comp>, rather than the linked list here */
  model_comp *next;    //!< next component in a double linked list 
  model_comp *prev;    //!< previous component in a double linked list 
  
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

  static list<model_comp*> global_list; //!< global list of all defined comps
  // ------------------------- METHODS ----------------------------------
  // constructor
  model_comp(char *id, compType type, opNode *indexing, opNode *attrib);
  model_comp();  //< constructor that sets everything to default values
  void print();   //< prints elements of the class
  void printBrief();   //< prints one liner
  void tagDependencies();  // tag this components and all its dependencies
                           // recuresively
  static void untagAll();  //< set the tag to false for all models
  static void writeAllTagged();//< write name of all tagged components
  model_comp *clone();     //< duplicate the object
};

char *
getGlobalName(model_comp *node, opNode *opn, 
	      AmplModel *current_model, int witharg);
#endif
