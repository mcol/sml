#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "sml.h"
#include "sml-oops.h"

using namespace std;
string progname = "smloops";

void writeHelp(ostream &out, string progname) {
   out << "Syntax:" << endl;
   out << "   " << progname << 
      " [OPTIONS] modelfile datafile" << endl;
   out << endl;
   out << "Option summary:" << endl;
   out << " -d                  Enables debug information when reading model "
      "file." << endl;
   out << " --help              Displays this help information and exit." <<
      endl;
   out << " --output=outfile," << endl;
   out << "   -o outfile        Write solution to file outfile." << endl;
   out << " modelfile           File containing SML model." << endl;
   out << " datafile            File containing SML data." << endl;
}

void analyseOptions(int argc, char **argv, string &modelfilename, 
      string &datafilename, string &outfilename, bool &debug) {
   int found = 0;
   for (int i=1;i<argc;i++){
      if (strcmp(argv[i], "-d")==0){
         debug = true;
      }
      else if(strcmp(argv[i], "--help")==0) {
         writeHelp(cout, progname);
         exit(0);
      }
      else if(strcmp(argv[i], "-o")==0) {
         if(i+1==argc) {
            cerr << "-o supplied without filename" << endl;
            exit(1);
         }
         outfilename = argv[++i];
         if(outfilename.at(0)=='-') {
            cerr << "-o supplied without filename" << endl;
            exit(1);
         }
      }
      else if(strncmp(argv[i], "--output=", 9)==0) {
         outfilename = (argv[i]+9);
      }
      else if(*(argv[i]) == '-') {
         cerr << "Unrecognised option '" << argv[i] << "'" << endl;
         exit(1);
      }
      else {
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

   if(modelfilename=="" || datafilename=="") {
      cerr << "ERROR: both modelfile and datafile " 
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
   string outfilename = "";
   bool debug = false;

   analyseOptions(argc, argv, modelfilename, datafilename, outfilename, debug);

   if(debug) {
      cout << "======================================================" << endl;
      cout << "----------------- Call OOPS generator ----------------" << endl;
   }

   ModelInterface *em = sml_generate(modelfilename, datafilename, debug);

   SML_OOPS_driver(em);

   if(outfilename!="") {
      ofstream outfile(("../" + outfilename).c_str());
      em->outputSolution(outfile);
      outfile.close();
      cout << "Solution written to file '" << outfilename << "'" << endl;
   }

   return 0;
}
