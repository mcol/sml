/* This is an MPS driver for the Structured Modelling Language (SML) */

#include <map>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include "sml-mps.h"

using namespace std;

class NameHasher {
  private:
   map<string, string> var_to_hash;
   map<string, int> var_to_idx;
   vector<string> idx_to_hash;
   const char prepend_;
   int next;
  public:
   NameHasher(char prepend) :
      prepend_(prepend), next(1) {}
   void insertValue(string input, string hash, int idx) {
      var_to_hash[input] = hash;
      var_to_idx[input] = next;
      idx_to_hash.push_back(hash);
   }
   string getHash(string input) {
      if(var_to_hash.find(input)!=var_to_hash.end()) return var_to_hash[input];
      // Otherwise doesn't have a hash value yet.
      ostringstream oss;
      oss << prepend_ << next;
      insertValue(input, oss.str(), next++);
      return oss.str();
   }
   int get_idx(string input) {
      if(var_to_hash.find(input)==var_to_hash.end()) return -1;
      return var_to_idx[input];
   }
   string get_hash(int idx) {
      return idx_to_hash[idx];
   }
};

class sparse_col {
  public:
   vector<int> row;
   vector<double> val;

   void add_entry(int r, double v) {
      row.push_back(r);
      val.push_back(v);
   }
   int size() const {
      return row.size();
   }
};

class mps_bound {
  public:
   string type;
   string name;
   double value;

   mps_bound(char type_[3], string name_, double value_) :
      type(type_), name(name_), value(value_) {}
};

void writeMps(ostream &out, string name, map<string,char> rows,
   map<string,double> rhs, map<string,double> ranges, list<mps_bound> bnds,
   map<string,sparse_col> cols, NameHasher row_hash, NameHasher col_hash);

/*
 * Each model block looks like the following:
 * A_1         C_1
 *     A_2     C_2
 *         A_3 C_3
 * B_1 B_2 B_3  D
 *
 * model->getJacobianOfIntersection(model) returns  D
 * model->getJacobianOfIntersection(A_i)   returns B_i
 * A_i->getJacobianOfIntersection(model)   returns C_i
 */
void SML_MPS_driver(ModelInterface *root, string filename) {
   map<string,char> rows; // Row names and equalities
   map<string,double> rhs;
   map<string,sparse_col> cols;
   map<string,double> ranges;
   list<mps_bound> bnds;
   NameHasher row_hash('r');
   NameHasher col_hash('c');

   const double inf = numeric_limits<double>::infinity();
   const string obj_name = "obj";

   // Initialise objective
   rows[obj_name] = 'N';
   row_hash.insertValue(obj_name, obj_name, 0);

   for(ModelInterface::child_iterator i=root->cbegin(); i!=root->cend(); ++i) {
      ModelInterface *im = *i;
      cout << "On Model " << im->getName() << " " << im->getNLocalVars() <<
         "x" << im->getNLocalCons() << endl;

      // Name columns, add objective entries and bounds
      {
         double obj[im->getNLocalVars()];
         double lwr_bnds[(*i)->getNLocalVars()];
         double upr_bnds[(*i)->getNLocalVars()];
         im->getObjGradient(obj);
         im->getColLowBounds(lwr_bnds);
         im->getColUpBounds(upr_bnds);
         double *op=obj;
         double *lb = lwr_bnds;
         double *ub = upr_bnds;
         for(list<string>::const_iterator j=im->getLocalVarNames().begin(); 
               j!=im->getLocalVarNames().end(); ++j,++op,++lb,++ub) {
            cout << "  v " << *j << " - " << col_hash.getHash(*j) << endl;
            sparse_col &col = cols[col_hash.getHash(*j)];
            col.add_entry(0, *op);
            if(*lb == *ub) { // Fixed
               bnds.push_back(mps_bound("FX", col_hash.getHash(*j), *lb));
            } else if(*lb==-inf && *ub==inf) {
               bnds.push_back(mps_bound("FR", col_hash.getHash(*j), 0.0));
            } else {
               if(*lb!=0.0)
                  bnds.push_back(mps_bound("LO", col_hash.getHash(*j), *lb));
               if(*ub!=inf)
                  bnds.push_back(mps_bound("UP", col_hash.getHash(*j), *ub));
            }
         }
      }

      // Name rows and identify constraint types
      {
         double lwr_bnds[(*i)->getNLocalCons()];
         double upr_bnds[(*i)->getNLocalCons()];
         (*i)->getRowLowBounds(lwr_bnds);
         (*i)->getRowUpBounds(upr_bnds);
         double *lb = lwr_bnds;
         double *ub = upr_bnds;
         for(list<string>::const_iterator j=(*i)->getLocalConNames().begin(); 
               j!=(*i)->getLocalConNames().end(); ++j,++lb,++ub) {
            cout << "  c " << *j << " - " << row_hash.getHash(*j) << endl;
            if(*lb==-inf && *ub==inf) {
               rows[row_hash.getHash(*j)] = 'N';
            } else if(*lb==-inf) {
               rows[row_hash.getHash(*j)] = 'L';
               if(*ub!=0) rhs[row_hash.getHash(*j)] = *ub;
            } else if(*ub== inf) {
               rows[row_hash.getHash(*j)] = 'G';
               if(*lb!=0) rhs[row_hash.getHash(*j)] = *lb;
            } else if(*lb==*ub) {
               rows[row_hash.getHash(*j)] = 'E';
               if(*lb!=0) rhs[row_hash.getHash(*j)] = *lb;
            } else {
               rows[row_hash.getHash(*j)] = 'L';
               if(*ub!=0) rhs[row_hash.getHash(*j)] = *ub;
               ranges[row_hash.getHash(*j)] = *ub-*lb;
            }
         }
      }

      // We observe that we will have a possible intersection with any of our
      // descendants, or ancestors. Note that the child_iterator also
      // covers this node as its final entry.
      cout << "  Descendant Intersections:" << endl;
      for(ModelInterface::child_iterator j=(*i)->cbegin(); j!=(*i)->cend(); ++j) {
         ModelInterface *jm = *j;
         if(im->getNzJacobianOfIntersection(jm)==0) continue;
         cout << "    " << jm->getName() << " : " << 
            im->getNzJacobianOfIntersection(jm) << endl;
         int nz = im->getNzJacobianOfIntersection(jm);
         int colbeg[jm->getNLocalVars()+1];
         int collen[jm->getNLocalVars()];
         int rownbs[nz];
         double elts[nz];
         im->getJacobianOfIntersection(jm, colbeg, collen, rownbs, elts);
         int p = 0;
         string first_con = *(im->getLocalConNames().begin());
         int offset = row_hash.get_idx(first_con);
         for(list<string>::const_iterator j=jm->getLocalVarNames().begin(); 
               j!=jm->getLocalVarNames().end(); ++j,++p) {
            sparse_col &col = cols[col_hash.getHash(*j)];
            for(int k=colbeg[p]; k<colbeg[p]+collen[p]; ++k)
               col.add_entry(rownbs[k]+offset, elts[k]);
         }
      }

      cout << "  Ancestoral Intersections:" << endl;
      for(ModelInterface::ancestor_iterator j=im->abegin(); j!=im->aend(); ++j) {
         ModelInterface *jm = *j;
         if(im->getNzJacobianOfIntersection(jm)==0) continue;
         cout << "    " << (*j)->getName() << " : " << 
            im->getNzJacobianOfIntersection(jm) << endl;
         int nz = im->getNzJacobianOfIntersection(jm);
         int colbeg[jm->getNLocalVars()+1];
         int collen[jm->getNLocalVars()];
         int rownbs[nz];
         double elts[nz];
         im->getJacobianOfIntersection(jm, colbeg, collen, rownbs, elts);
         int p = 0;
         string first_con = *(im->getLocalConNames().begin());
         int offset = row_hash.get_idx(first_con);
         for(list<string>::const_iterator j=jm->getLocalVarNames().begin(); 
               j!=jm->getLocalVarNames().end(); ++j,++p) {
            sparse_col &col = cols[col_hash.getHash(*j)];
            for(int k=colbeg[p]; k<colbeg[p]+collen[p]; ++k)
               col.add_entry(rownbs[k]+offset, elts[k]);
         }
      }
   }

   string newfilename = "../" + filename;
   ofstream fout(newfilename.c_str());
   writeMps(fout, "Test", rows, rhs, ranges, bnds, cols, row_hash, col_hash);
   fout.close();
}

string mps_float(double d) {
   ostringstream oss;
   oss << setw(11) << d;
   if(oss.str().find('e') != oss.str().npos) {
      ostringstream oss2;
      if(d<1) { // Very small
         oss2 << fixed << setprecision(10) << d;
      } else { // Very big
         oss2 << fixed << setprecision(0) << d;
      }
      return oss2.str();
   }
   if(oss.str().find('.') == oss.str().npos) oss << '.';
   return oss.str();
}

void writeMps(ostream &out, string name, map<string,char> rows,
      map<string,double> rhs, map<string,double> ranges, list<mps_bound> bnds,
      map<string,sparse_col> cols, NameHasher row_hash, NameHasher col_hash) {
   out << "NAME          " << name << endl;
   
   out << "ROWS" << endl;
   for(map<string,char>::iterator i=rows.begin(); i!=rows.end(); ++i) {
      out << " " << i->second << "  " << i->first << endl;
   }

   out << "COLUMNS" << endl;
   for(map<string,sparse_col>::iterator i=cols.begin(); i!=cols.end(); ++i) {
      sparse_col &col = i->second;
      bool rfirst = true;
      for(int j=0; j<col.size(); ++j) {
         if(rfirst) out << "    " << setw(8) << i->first << "  ";
         else out << "   ";
         out << setw(8) << row_hash.get_hash(col.row[j]) << "  ";
         out << setw(12) << mps_float(col.val[j]);
         if(!rfirst) out << endl;
         rfirst = !rfirst;
      }
      if(!rfirst) out << endl;
   }

   out << "RHS" << endl;
   for(map<string,double>::iterator i=rhs.begin(); i!=rhs.end(); ++i) {
      out << "    rhs1      " << setw(8) << i->first << "  ";
      out << setw(12) << mps_float(i->second) << endl;
   }

   out << "RANGES" << endl;
   for(map<string,double>::iterator i=ranges.begin(); i!=ranges.end(); ++i) {
      out << "    range1    " << setw(8) << i->first << "  ";
      out << setw(12) << mps_float(i->second) << endl;
   }

   out << "BOUNDS" << endl;
   int brow = 0;
   for(list<mps_bound>::iterator i=bnds.begin(); i!=bnds.end(); ++i, ++brow) {
      ostringstream oss;
      oss << "b" << brow;
      out << " " << setw(2) << i->type << " ";
      out << setw(8) << oss.str() << "  ";
      out << setw(8) << i->name << "  ";
      out << setw(12) << mps_float(i->value) << endl;
   }

   out << "ENDATA" << endl;
}
