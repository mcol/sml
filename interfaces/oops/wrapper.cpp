#include <iostream>
#include <sys/stat.h>
#include "sml.h"
#include "sml-oops.h"

using namespace std;

void analyseOptions(int argc, char **argv, string &modelfilename, 
      string &datafilename, bool &debug) {
   int found = 0;
   for (int i=1;i<argc;i++){
      if (strcmp(argv[i], "-d")==0){
         debug = true;
      }else{
         if (found==0){
            // first proper argument is the model file to read
            modelfilename = argv[i];
            found++;
         }else{
            // next one is data file
            datafilename = argv[i];
         }
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
   bool debug = false;

   analyseOptions(argc, argv, modelfilename, datafilename, debug);

   cout << "=============================================================== \n";
   cout << "----------------- Call OOPS generator ---------------- \n";

   ModelInterface *em = sml_generate(modelfilename, datafilename, debug);

   SML_OOPS_driver(em);

   return 0;
}
