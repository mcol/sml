#ifndef SML_H
#define SML_H

#include "ModelInterface.h"
#include <string>

std::string sml_version();

ModelInterface* sml_generate(const std::string modelfilename, const std::string datafilename, const bool debug);

#endif
