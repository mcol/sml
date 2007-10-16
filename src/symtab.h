#ifndef SYMTAB_H
#define SYMTAB_H

//#include "ampl.h"
#define N_HASH 100

/* global symbol list is a hashed linked list of all symbols that are defined */

enum symb_type {ST_NONE, ST_PARAM, ST_VAR, ST_CONS, ST_OBJ, ST_SET};

class AmplModel;

typedef struct symb_entry_st{
  struct symb_entry_st *next;
  char *name;
  enum symb_type type;
} symb_entry;


void defineSymbol(int type, char *id, opNode *domain, AmplModel *model);

#endif
