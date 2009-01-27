#ifndef SYMTAB_H
#define SYMTAB_H

#include<string>
#include<list>
using namespace std;

class SymbolTable {
public:
   enum symb_type {ST_NONE, ST_PARAM, ST_VAR, ST_CONS, ST_OBJ, ST_SET};
   typedef pair<string, symb_type> Entry;

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
