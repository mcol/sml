#ifndef AMPL_H
#define AMPL_H

#include "model_comp.h"
#include "nodes.h"
#include "symtab.h"
#include "ExpandedModel.h"
#include "AmplModel.h"
#include <string>
/* This is an item that is part of an AMPL model. It can take information
   about one Set/Variable/Constraint/Parameter, etc definition */

//class AmplModel;
class model_comp;

opNode *findKeywordinTree(opNode *root, int oc);
opNode* find_var_ref_in_context(AmplModel *context, opNode *ref);
opNodeIDREF* find_var_ref_in_context_(AmplModel *context, IDNode *ref);
opNode* find_var_ref_in_indexing(const char *const name);
void add_indexing(opNodeIx *indexing);
void rem_indexing(opNodeIx *indexing);


/* ----------------- stub for methods in ampl.y ------------------------- */

void begin_model(char *name, opNode *indexing);
void end_model();
void add_set_to_model(char *id, opNode *indx, opNode *attrib);
void add_obj_to_model(int token, char *id, opNode *indx, opNode *attrib);


#endif
