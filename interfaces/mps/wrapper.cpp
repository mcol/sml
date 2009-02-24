#include <iostream>
#include <sys/stat.h>
#include "sml.h"
#include "sml-mps.h"

using namespace std;

void analyseOptions(int argc, char **argv, string &modelfilename, 
      string &datafilename, string &mpsfilename, bool &debug) {
   int found = 0;
   for (int i=1;i<argc;i++){
      if (strcmp(argv[i], "-d")==0){
         debug = true;
      }else{
         switch(found) {
         case 0:
            // first proper argument is the model file to read
            modelfilename = argv[i];
            break;
         case 1:
            // next one is data file
            datafilename = argv[i];
            break;
         default:
            mpsfilename = argv[i];
            break;
         }
         found++;
      }
   }
   if (modelfilename != ""){
      cout << "OPTIONS: model file: " << modelfilename << "\n";
   }else{
      cout << "OPTIONS: read model from stdout\n";
   }
}

/* ----------------------------------------------------------------------------
main
---------------------------------------------------------------------------- */
int main(int argc, char **argv) {
   int errcode;
   string modelfilename = "";
   string datafilename = "global.dat";
   string mpsfilename = "problem.mps";
   bool debug = false;

   analyseOptions(argc, argv, modelfilename, datafilename, mpsfilename, debug);

   cout << "=============================================================== \n";
   cout << "----------------- Call OOPS generator ---------------- \n";

   ModelInterface *em = sml_generate(modelfilename, datafilename, debug);

   SML_MPS_driver(em, mpsfilename);

   return 0;
}
