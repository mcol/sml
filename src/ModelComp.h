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


using namespace std;
/* ------------------------------------------------------------------------ */
/* ModelComp describes a model component such as 
   var/subject to/param/set/minimize 
   
   every such component is stored as
   - id : the name of the component 
   - indexing: the indexing expression stored as a tree 
   - attributes: a list of attributes 
                 (this includes the actual constraint definition for 
		  "subject to" components)
*/
class AmplModel;
/** 
 * @class ModelComp
 * @brief Object to represent a component of an AMPL/SML model/block.
 *
 * The ModelComp object represents a component of an SML model/block.
 * It usually represents one line of AMPL/SML which is a definition of a
 * variable/parameter/set/constraint/objective/block.
 * A model component is broken down into the following parts:
 * - \<TYPE\> \<name\>\<indexing\>_opt \<attributes\>_opt
 */
class ModelComp{
 private:
 public:
  static const string nameTypes[];
  static const string compTypes[];

  compType type;   //!< the type of the component
  char *id;        //!< the name of the component

  /** this is a tree of specifications, this includes
   * :=, within, default, >=                                               */
  SyntaxNode *attributes;   
			 
  SyntaxNodeIx *indexing; //!< indexing expression 
  
  /** List of all entities that this model component depends on:
   *  Basically a list of all model components used in the definition of
   *  this component                                                  */
  list<ModelComp*> dependencies;     //!< list of dependecies:

  AmplModel *model;    //!< The model this belongs to 

  /** A pointer to an AmplModel structure for components of type MODEL
   *  @attention Better implemented as a subclass of ModelComp  */
  void *other;      

  int count;           //!< instance number of ModelComp

  /** Components can be tagged by the tagDependencies method which sets
   *  this tag for this components and everything that it depends on 
   *  (i.e. everything listed in the dependencies list).                 */
  bool tag;            //!< true if part of the current 'needed' set

  /** for sets and parameters this points to an object that gives the
   *  values and further specific information (Set for sets)
   */
  CompDescr *value;   //!< value (for sets and parameters)
  static list<ModelComp*> global_list; //!< global list of all defined comps
  static int tt_count;    //!< number of ModelComps defined
  // ------------------------- METHODS ----------------------------------
  // constructor
  ModelComp(char *id, compType type, SyntaxNode *indexing, SyntaxNode *attrib);
  ModelComp();  //< constructor that sets everything to default values
  virtual ~ModelComp() {}

  /** set up an existing model comp to specified values   */
  void setTo(char *id, compType type, SyntaxNodeIx *indexing, SyntaxNode *attrib); 

  /** set up list of dependencies for this component */
  void setUpDependencies();
  void dump(ostream &fout);   //!< detailed debugging output
  void print();   //!< prints elements of the class
  void printBrief();   //!< prints one liner
  void tagDependencies();  //!< tag this components and all its dependencies
                           // recuresively

  /** recalculate dependency list and re-resolve IDREF nodes */
  void reassignDependencies();

  /** set the tag to false for all models: using global_list */
  static void untagAll();  

  /** recursively set the tag to false for all models */
  static void untagAll(AmplModel *start);  

  /** recursively write name of all tagged components */
  static void writeAllTagged(AmplModel *start);

  /** write definition of all tagged components to file, using global_list */
  static void modifiedWriteAllTagged(ostream &fout); 

  void moveUp(int level);  //< move this model comp up in the model tree 
  virtual ModelComp *clone();     //< duplicate the object: shallow copy
  ModelComp *deep_copy(); //< duplicate the object: deep copy
};

char *
getGlobalName(ModelComp *node, const SyntaxNode *opn, 
	      AmplModel *current_model, int witharg);
char *
getGlobalNameNew(ModelComp *node, const SyntaxNode *opn, 
	      AmplModel *current_model, int witharg);
#endif
