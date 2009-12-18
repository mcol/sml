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

/** @class ModelComp
 *  Object to represent a component of an AMPL/SML model/block.
 *
 * The ModelComp object represents a component of an SML model/block.
 * It usually represents one line of AMPL/SML which is a definition of a
 * variable/parameter/set/constraint/objective/block.
 * A model component is broken down into the following parts:
 * - <typo\> <name\> <indexing\>_opt <attributes\>_opt
 */
class ModelComp{
 private:
 public:
  static const string nameTypes[];
  static const string compTypes[];

  /** Type of the component */
  compType type;

  /** Name of the component */
  string id;

  /** this is a tree of specifications, this includes
   * :=, within, default, >=                                               */
  SyntaxNode *attributes;   
			 
  /**  Indexing expression */
  SyntaxNodeIx *indexing;
  
  /** List of all entities that this model component depends on:
   *  Basically a list of all model components used in the definition of
   *  this component                                                  */
  list<ModelComp*> dependencies;     //!< list of dependencies:

  AmplModel *model;    //!< The model this belongs to 

  /** A pointer to an AmplModel structure for components of type MODEL
   *  @attention Better implemented as a subclass of ModelComp. */
  void *other;      

  /** Instance number of ModelComp */
  int count;

  /** Components can be tagged by the tagDependencies method which sets
   *  this tag for this components and everything that it depends on 
   *  (i.e. everything listed in the dependencies list).                 */
  bool tag;            //!< true if part of the current 'needed' set

  /** for sets and parameters this points to an object that gives the
   *  values and further specific information (Set for sets)
   */
  CompDescr *value;   //!< value (for sets and parameters)

  /** Global list of all defined ModelComps */
  static list<ModelComp*> global_list;

  /** Number of ModelComps defined */
  static int tt_count;

  // ------------------------- METHODS ----------------------------------
  /** Constructor */
  ModelComp(const string& id, compType type,
            SyntaxNode *indexing, SyntaxNode *attrib);
  ModelComp();  //< constructor that sets everything to default values

  /** Destructor */
  virtual ~ModelComp();

  /** Set up an existing model comp to specified values */
  void setTo(char *id, compType type, SyntaxNodeIx *indexing, SyntaxNode *attrib); 

  /** Set up list of dependencies for this component */
  void setUpDependencies();
  void dump(ostream &fout);   //!< detailed debugging output
  void print();   //!< prints elements of the class
  void printBrief();   //!< prints one liner

  /** Tag this components and all its dependencies recursively */
  void tagDependencies();

  /** Recalculate dependency list and re-resolve IDREF nodes */
  void reassignDependencies();

  /** Set the tag to false for all models: using global_list */
  static void untagAll();  

  /** Recursively set the tag to false for all models */
  static void untagAll(AmplModel *start);  

  /** Recursively write name of all tagged components */
  static void writeAllTagged(AmplModel *start);

  /** Write definition of all tagged components to file, using global_list */
  static void modifiedWriteAllTagged(ostream &fout); 

  void moveUp(int level);  //< move this model comp up in the model tree 
  virtual ModelComp *clone();     //< duplicate the object: shallow copy
  ModelComp *deep_copy(); //< duplicate the object: deep copy
};

string
getGlobalName(ModelComp *node, const SyntaxNode *opn, 
	      AmplModel *current_model, int witharg);
string
getGlobalNameNew(ModelComp *node, const SyntaxNode *opn, 
	      AmplModel *current_model, int witharg);
#endif
