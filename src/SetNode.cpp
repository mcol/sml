#include <iostream>
#include <sstream>
#include "SetNode.h"

SimpleSet::SimpleSet(opNode *bnd1, opNode *bnd2) :
   SetNode(DOTDOT, bnd1, bnd2),
   interval_(1),
   parsed_(false)
{
   if(bnd1->opCode!=INT_VAL) return;
   lower_bound_ = *(int*)*(bnd1->begin());

   if(bnd2->opCode!=INT_VAL) return;
   upper_bound_ = *(int*)*(bnd2->begin());

   parsed_ = true;
   cout << "Parsed Set " << lower_bound_ << ".." << upper_bound_ << endl;
}

// Safely convert an integer to a string
string itos(int val) {
   ostringstream ost;
   ost << val;
   return ost.str();
}

vector<string> SimpleSet::members() {
   if(!parsed_) {
      cerr << "Trying to obtain members of set which has not been parsed!\n";
      exit(1);
   }

   vector<string> result;
   for(int i=lower_bound_; i<=upper_bound_; i+=interval_)
      result.push_back(itos(i));

   return result;
}
