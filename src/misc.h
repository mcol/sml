#ifndef MISC_H
#define MISC_H

#include <sstream> // for to_string() below

/* ---------------------------------------------------------------------------
string to_string()
---------------------------------------------------------------------------- */
/* bit below from 
http://notfaq.wordpress.com/2006/08/30/c-convert-int-to-string/
to convert any numeric type into a string 
(surprised C++ cannot do this out-of-the-box)                                */

template <class T>
inline std::string to_string (const T& t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}

#endif
