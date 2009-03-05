#include <iostream>
#include <sys/stat.h>
#include "sml.h"
#include "sml-mps.h"

using namespace std;
string progname = "smlmps";

void writeHelp(ostream &out, string progname) {
   out << "Syntax:" << endl;
   out << "   " << progname << 
      " [-d] [--help] modelfile datafile mpsfile" << endl;
   out << endl;
   out << "Option summary:" << endl;
   out << " -d            Enables debug information when reading model "
      "file." << endl;
   out << " --help        Displays this help information." << endl;
   out << " modelfile     File containing SML model." << endl;
   out << " datafile      File containing SML data." << endl;
   out << " mpsfile       Filename of MPS file to write." << endl;
}

void analyseOptions(int argc, char **argv, string &modelfilename, 
      string &datafilename, string &mpsfilename, bool &debug) {
   int found = 0;

   for (int i=1;i<argc;i++){
      if(strcmp(argv[i], "-d")==0){
         debug = true;
      }
      else if(strcmp(argv[i], "--help")==0) {
         writeHelp(cout, progname);
         exit(0);
      }
      else {
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

   if(modelfilename=="" || datafilename=="" || mpsfilename=="") {
      cerr << "ERROR: all of modelfile, datafile and mpsfile " 
         "must be supplied." << endl << endl;
      writeHelp(cerr, progname);
      exit(1);
   }
}

/* ----------------------------------------------------------------------------
main
---------------------------------------------------------------------------- */
int main(int argc, char **argv) {
   string modelfilename = "";
   string datafilename = "";
   string mpsfilename = "";
   bool debug = false;

   cout << "SML MPS generator, SML version " << sml_version() << endl;
   cout << "(c) Andreas Grothey and Jonathan Hogg, "
      "University of Edinburgh 2009" << endl << endl;

   analyseOptions(argc, argv, modelfilename, datafilename, mpsfilename, debug);

   if(debug) 
      cout << "======================================================" << endl;

   ExpandedModelInterface *em = sml_generate(modelfilename, datafilename, debug);

   SML_MPS_driver(em, mpsfilename);

   return 0;
}
