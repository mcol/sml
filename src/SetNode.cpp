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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include "SetNode.h"

SimpleSet::SimpleSet(SyntaxNode *bnd1, SyntaxNode *bnd2) :
   SetNode(DOTDOT, bnd1, bnd2),
   lbc_(bnd1),
   ubc_(bnd2),
   interval_(1),
   parsed_(false)
{
   ValueNode<long> *inode;

   if( (inode = dynamic_cast<ValueNode<long> *>(lbc_)) ) {
      lbc_ = NULL;
      lower_bound_ = inode->value;
   }

   if( (inode = dynamic_cast<ValueNode<long> *>(ubc_)) ) {
      ubc_ = NULL;
      upper_bound_ = inode->value;
   }

   parsed_ = !(lbc_ || ubc_);

   if(parsed_) 
      cout << "Parsed Set " << lower_bound_ << ".." << upper_bound_ << endl;
}

// Safely convert an integer to a string
string itos(int val) {
   ostringstream ost;
   ost << val;
   return ost.str();
}

vector<string> SimpleSet::members(AmplModel &context) {
   if(!parsed_) {
      cerr << "Trying to obtain members of set which has not been parsed!\n";
      exit(1);
   }

   vector<string> result;
   for(int i=lower_bound_; i<=upper_bound_; i+=interval_)
      result.push_back(itos(i));

   return result;
}
