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
  
  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash;
}


#if 0
// OLD defineSymbol
void defineSymbol(int type, char *id, opNode *domain, AmplModel *model)
{
  char *nametype;
  int hash;
   symb_entry *cur_entry, *this_symbol;
   symb_entry **sht_above;
   AmplModel *mod_above;


   /* create hash table entry for this symbol */
   this_symbol = (symb_entry*)calloc(1,sizeof(symb_entry));
   this_symbol->name = strdup(id);
   this_symbol->next = NULL;



   /* this routine should go through the hash table, make sure that the
      current name is not registered yet and then add it to the hash table */

   hash = hash_function(id)%N_HASH;

   /* loop through all hash tables from here to the top level and
      see if this entry is registered enywhere already */

   mod_above = model;

   while(mod_above!=NULL){
     sht_above = mod_above->symb_hash_table;
     if (sht_above[hash]!=NULL){
     /* loop through all registered hashes and check that they are not
	identical to the current one */
       cur_entry = sht_above[hash];
       while (cur_entry!=NULL){
	 if (strcmp(cur_entry->name,id)==0){
	   printf("Error: symbol %s already defined in model\n",id,
		  mod_above->name);
	   exit(1);
	 }
	 cur_entry = cur_entry->next;
       }
     }
     mod_above = mod_above->parent;
   }

   /* okay: not found this entry anywhere, register it in this hash_table */
   
   cur_entry = model->symb_hash_table[hash];
   if (cur_entry==NULL){
     model->symb_hash_table[hash] = this_symbol;
   }else{
     while (cur_entry->next!=NULL) cur_entry=cur_entry->next;
     cur_entry->next = this_symbol;
   }


   switch(type) {
      case VAR:
        nametype = "var";
	this_symbol->type = ST_VAR; 
         break;
      case SET:
         nametype = "set";
	 this_symbol->type = ST_SET; 
         break;
      case PARAM:
         nametype = "param";
	 this_symbol->type = ST_PARAM; 
         break;
      case SUBJECTTO:
         nametype = "constraint";
	 this_symbol->type = ST_CONS; 
         break;
      case MAXIMIZE:
	 nametype = "obj";
	 this_symbol->type = ST_OBJ; 
         break;
      case MINIMIZE:
         nametype = "obj";
	 this_symbol->type = ST_OBJ; 
         break;
   }
   if (logSymtab) printf("SYMTAB: defined %s of type %s\n", id, nametype);
}
#endif
