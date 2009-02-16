#include <iostream>
#include <sys/stat.h>
#include "GlobalVariables.h"
#include "AmplModel.h"
#include "backend.h"

using namespace std;

extern int yydebug;
void parse_data(AmplModel*, char*);
void parse_model(char *);

void analyseOptions(int argc, char **argv){
   int found = 0;
   for (int i=1;i<argc;i++){
      if (strcmp(argv[i], "-d")==0){
         yydebug = 1;
      }else{
         if (found==0){
            // first proper argument is the model file to read
            GlobalVariables::modelfilename = argv[i];
            found++;
         }else{
            // next one is data file
            GlobalVariables::datafilename = argv[i];
         }
      }
   }
   // now echo the options
   if (yydebug==1)
      cout << "OPTIONS: debug mode\n";
   if (GlobalVariables::modelfilename){
      cout << "OPTIONS: model file: " << GlobalVariables::modelfilename << "\n";
   }else{
      cout << "OPTIONS: read model from stdout\n";
   }
   if (GlobalVariables::datafilename){
      cout << "OPTIONS: data file: " << GlobalVariables::datafilename << "\n";
   }else{
      cout << "OPTIONS: read data from 'global.dat'\n";
   }
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
int main(int argc, char **argv) {
   int errcode;
   GlobalVariables::modelfilename = NULL;
   GlobalVariables::datafilename = "global.dat";

   analyseOptions(argc, argv);

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

   process_model(AmplModel::root);

   return 0;
}

