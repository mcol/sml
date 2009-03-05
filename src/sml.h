#ifndef SML_H
#define SML_H

#include "ExpandedModelInterface.h"
#include <string>

std::string sml_version();

ExpandedModelInterface* sml_generate(const std::string modelfilename, const std::string datafilename, const bool debug);

#endif
