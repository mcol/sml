#ifndef SYMTAB_H
#define SYMTAB_H

#include<string>
#include<list>
#include "model_comp.h"
using namespace std;

class SymbolTable {
public:
   enum symb_type {ST_NONE, ST_PARAM, ST_VAR, ST_CONS, ST_OBJ, ST_SET};
   class Entry {
     public:
      const string id;
      const symb_type type;
      model_comp *mc;

     public:
      Entry(const string new_id, const symb_type new_type, model_comp *new_mc) :
         id(new_id), type(new_type), mc(new_mc) {}
   };

private:
   static const int n_hash = 100; // Number of available has codes
   static const bool logSymtab = false; // Enable debug logging?
   list<Entry> table_[n_hash];

public:
   bool defineSymbol(symb_type, char *id, model_comp *mc);

private:
   unsigned long hash_function(char *str);
};

#endif
