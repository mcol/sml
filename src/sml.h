#ifndef SML_H
#define SML_H

#include "ModelInterface.h"

ModelInterface* sml_generate(const std::string modelfilename, const std::string datafilename, const bool debug);

#endif
