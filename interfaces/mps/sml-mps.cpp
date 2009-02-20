/* This is an MPS driver for the Structured Modelling Language (SML) */

#include <map>
#include <iostream>
#include "sml-mps.h"

using namespace std;

struct mps_entry {
   string name;  // Row name
   double entry; // Value of entry
};

class NameHasher {
  private:
   map<string, string> var_to_hash;
   map<string, string> hash_to_var;
  public:
   string getHash(string input) {
      if(var_to_hash.find(input)!=var_to_hash.end()) return var_to_hash[input];
      // Otherwise doesn't have a hash value yet.
      string hash = hashFn(input);
      for(;;) {
         if(hash_to_var.find(hash)==hash_to_var.end()) {
            hash_to_var[hash] = input;
            var_to_hash[input] = hash;
            return hash;
         }
         hash[0] = '_';
         hash[1]++;
      }
   }
  private:
   static string hashFn(string input) { // Just use final 7 characters
      string hash = input.substr(input.size()-7, 7);
      for(string::size_type i = 0; i < hash.size(); ++i)
         if(hash[i] == ' ') hash[i] = '_';
      return hash;
   }
};

void SML_MPS_driver(ModelInterface *root) {
   map<string,string> rows; // Row names and equalities
   map<string,list<mps_entry> > cols; // columns, stored by name.
   map<string,double> rhs;
   NameHasher row_hash;
   NameHasher col_hash;

   for(ModelInterface::child_iterator i=root->cbegin(); i!=root->cend(); ++i) {
      cout << "On Model " << (*i)->getName() << " " << (*i)->getNLocalVars() <<
         "x" << (*i)->getNLocalCons() << endl;
      for(list<string>::const_iterator j=(*i)->getLocalVarNames().begin(); 
            j!=(*i)->getLocalVarNames().end(); ++j) {
         cout << "  " << *j << " - " << col_hash.getHash(*j) << endl;
      }
      // We observe that we will have a possible intersection with any of our
      // descendants, but nothing else.
      cout << "  Descendant Intersections:" << endl;
      for(ModelInterface::child_iterator j=(*i)->cbegin(); j!=(*i)->cend(); ++j) {
         cout << "    " << (*j)->getName() << " : " << 
            (*i)->getNzJacobianOfIntersection(*j) << endl;
      }
      cout << "  Ancestoral Intersections:" << endl;
      for(ModelInterface::ancestor_iterator j=(*i)->abegin(); j!=(*i)->aend(); ++j) {
         cout << "    " << (*j)->getName() << " : " << 
            (*i)->getNzJacobianOfIntersection(*j) << endl;
      }
   }
}

void writeMps(ModelInterface *root, ostream &out) {
}
