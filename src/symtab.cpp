#include <iostream>
#include "symtab.h"

/* Note: original symboltable also checked that this variable was not defined
 * in an enclosing scope, rather than just hiding the previously defined
 * variables as we do now. */
bool SymbolTable::defineSymbol(symb_type type, char *id, ModelComp *mc)
{
   /* Calculate hashcode */
   int hash = hash_function(id) % n_hash;

   /* First check id not already defined, return false if it is */
   for(list<Entry>::iterator i=table_[hash].begin(); i!=table_[hash].end(); ++i)
      if((*i).id == id) return false;

   /* Otherwise insert at the start of the list */
   table_[hash].push_back(Entry(id,type,mc));
   if (logSymtab)
      cout << "SYMTAB: defined " << id << " of type " << type << "\n";

   /* Sucessfully defined NEW symbol */
   return true;
}

SymbolTable::Entry* SymbolTable::findSymbol(string id) {
   /* Calculate hashcode */
   int hash = hash_function(id.c_str()) % n_hash;

   list<Entry>::iterator i;
   for(i=table_[hash].begin(); i!=table_[hash].end(); ++i)
      if((*i).id == id) break;

   if(i==table_[hash].end()) return NULL;

   return &(*i);
}

/* ----------------------------------------------------------------------------
hash function:
---------------------------------------------------------------------------- */
/* this is djb2 (k=33) of dan bernstein taken from 
   http://www.cse.yorku.ca/~oz/hash.html
*/

unsigned long SymbolTable::hash_function(const char *str)
{
  unsigned long hash = 5381;
  int c;
  
  while ( (c=*str++) )
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash;
}
