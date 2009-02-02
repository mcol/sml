#include "SetElement.h"
#include <string.h>
#include <string>

using namespace std;

SetElement::SetElement(int n, char **new_val) :
   n(n), val(NULL)
{
  if(n==0) return;
  assert(new_val);

  val = new string[n];
  for(int i=0; i<n; i++)
     val[i] = new_val[i];
}

SetElement::SetElement(int n, IDNode **new_val) :
   n(n), val(NULL)
{
  if(n==0) return;
  assert(new_val);

  val = new string[n];
  for(int i=0; i<n; i++)
     val[i] = new_val[i]->name;
}

SetElement::~SetElement() {
  // FIXME
  //if(val) delete [] val;
}


bool SetElement::operator()(const SetElement el1, const SetElement el2) const 
{
  assert(el1.n==el2.n);
  for(int i=0;i<el1.n;i++){
    int cmp = strcmp(el1.val[i].c_str(),el2.val[i].c_str());
    if (cmp<0) return true;
    if (cmp>0) return false;
  }
  return false;
}

char* SetElement::toCharA()
{
  string str(val[0]);
  for(int i=1;i<n;i++){
    str += ",";
    str += val[i];
  }
  return strdup(str.c_str());
}

