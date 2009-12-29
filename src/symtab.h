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
#ifndef SYMTAB_H
#define SYMTAB_H

#include <string>
#include <list>
#include "ModelComp.h"

class SymbolTable {
public:
   enum symb_type {ST_NONE, ST_PARAM, ST_VAR, ST_CONS, ST_OBJ, ST_SET};
   class Entry {
     public:
      const std::string id;
      const symb_type type;
      ModelComp *mc;

     public:
      Entry(const std::string new_id, const symb_type new_type,
            ModelComp *new_mc) :
         id(new_id), type(new_type), mc(new_mc) {}
   };

private:

   /** Number of available hash codes */
   static const int n_hash = 100;
   static const bool logSymtab = false; // Enable debug logging?
   std::list<Entry> table_[n_hash];

public:
   SymbolTable() {};
   void copy(SymbolTable &src) {
      for(int i=0; i<n_hash; ++i) {
        for(std::list<Entry>::const_iterator j = src.table_[i].begin();
            j != src.table_[i].end(); ++j) {
            table_[i].push_back(*j);
         }
      }
   }
   bool defineSymbol(symb_type, char *id, ModelComp *mc);
   Entry* findSymbol(const std::string& id);
   std::list<Entry> getListByType(const symb_type type) const;

private:
   unsigned long hash_function(const char *str) const;
};

#endif
