#ifndef SYMTAB_H
#define SYMTAB_H

#include<string>
#include<list>
using namespace std;

class SymbolTable {
public:
   enum symb_type {ST_NONE, ST_PARAM, ST_VAR, ST_CONS, ST_OBJ, ST_SET};
   class Entry {
     public:
      const string id;
      const symb_type type;

     public:
      Entry(const string new_id, const symb_type new_type) :
         id(new_id), type(new_type) {}
   };

private:
   static const int n_hash = 100; // Number of available has codes
   static const bool logSymtab = false; // Enable debug logging?
   list<Entry> table_[n_hash];

public:
   bool defineSymbol(symb_type, char *id);

private:
   unsigned long hash_function(char *str);
};

#endif
