#include "SetElement.h"
#include <string.h>
#include <string>

using namespace std;

SetElement::SetElement()
  :n(0),val(NULL)
{}

SetElement::SetElement(int n, char **val)
{
  this->n=n;
  this->val=val;
}


bool SetElement::operator()(const SetElement el1, const SetElement el2) const 
{
  assert(el1.n==el2.n);
  for(int i=0;i<el1.n;i++){
    int cmp = strcmp(el1.val[i],el2.val[i]);
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
    str += string(val[i]);
  }
  return strdup(str.c_str());
}

