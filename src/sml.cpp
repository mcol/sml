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
#include <iostream>
#include <sys/stat.h>
#include "sml.h"
#include "GlobalVariables.h"
#include "AmplModel.h"
#include "backend.h"

using namespace std;

/* global variables */
char *GlobalVariables::modelfilename = NULL;
char *GlobalVariables::datafilename = NULL;
char *GlobalVariables::amplcommand = "ampl";

bool GlobalVariables::logParseModel = false;
int GlobalVariables::prtLvl = 0;

extern int yydebug;
void parse_data(AmplModel*, char*);
void parse_model(char *);

string sml_version() {
   return "0.7";
}

void createSubdirTmpIfNotExist(void)
{
   int fl_is_dir;
   bool fl_exists;
   struct stat sbuf;
  
   fl_exists = !(stat("tmp", &sbuf) == -1);
   fl_is_dir = S_ISDIR(sbuf.st_mode);

   if (fl_exists && !fl_is_dir){
      cerr << "'tmp/' exists but is no directory!\n";
      cerr << "Cannot continue\n";
      exit(1);
   }
   if (!fl_exists){
      int err;
      err = mkdir("tmp", S_IRWXU);
      if (err){
         cerr << "Failed to create temporary directory 'tmp/'\n";
         cerr << "Cannot continue\n";
         exit(1);
      }
   }
}

/* ----------------------------------------------------------------------------
main
---------------------------------------------------------------------------- */
ExpandedModelInterface* sml_generate(const string modelfilename, 
      const string datafilename, const bool debug) {
   int errcode;
   GlobalVariables::modelfilename = strdup(modelfilename.c_str());
   GlobalVariables::datafilename = strdup(datafilename.c_str());
   yydebug = debug ? 1 : 0;

   /* make sure dir '/tmp' for temporary files exists */
   createSubdirTmpIfNotExist();
   errcode = chdir("tmp");
   if (errcode){
      cerr << "Could not change working directory to 'tmp/'\n";
      cerr << "Cannot continue\n";
      exit(1);
   }

   // change working directory back to original for the parsing of files
   errcode = chdir("..");
   if (errcode){
      cerr << "Could not change working directory to '../'\n";
      cerr << "Cannot continue\n";
      exit(1);
   }
   
   parse_model(GlobalVariables::modelfilename);
   parse_data(AmplModel::root, GlobalVariables::datafilename);

   // change working directory back to tmp/ for processing model
   errcode = chdir("tmp");
   if (errcode){
      cerr << "Could not change working directory to 'tmp/'\n";
      cerr << "Cannot continue\n";
      exit(1);
   }

   AmplModel::root->addDummyObjective();
   AmplModel::root->dump("logModel.dat");

   /* Write out and run ampl on ExpandedModels */
   process_model(AmplModel::root);

   /* Call Solver */

   cout << "------------- Generate ExpandedModel tree ------------ \n";
   ExpandedModel *em = AmplModel::root->createExpandedModel("root", "");
   em->print();

   return (ExpandedModelInterface*) em;
}

