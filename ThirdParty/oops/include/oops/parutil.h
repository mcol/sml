/* This file is part of OOPS.
 *
 * OOPS is (c) 2003-2009 Jacek Gondzio and Andreas Grothey, 
 *                       University of Edinburgh
 *
 * OOPS is distributed in a restricted form in the hope that it will be a useful
 * example of what can be done with SML, however it is NOT released under a free
 * software lisence.
 *
 * You may only redistribute this version of OOPS with a version of SML. You
 * may not link OOPS with code which is not part of SML lisenced under the
 * LGPL v3.
 *
 * You may NOT modify, disassemble, or otherwise reverse engineer OOPS.
 *
 * OOPS is distributed WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
/**
 *  @file parutil.h
 *
 *  Utilities for parallel processing.
 */

#ifndef PARUTIL_H
#define PARUTIL_H


int ParPrintf(const char *format, ...);

/** Initialise MPI and set the global variables used for parallelism */
int InitLippPar(const int argc, char *argv[]);

/** Exit immediately from OOPS if something really wrong has happened */
void LeaveOOPS(void);


#ifdef WITH_MPI

/* include mpi.h with the path provided by the macro MPI_PATH : */
#ifndef  MPI_PATH
#error   "MPI_PATH is not defined"
#else
#include MPI_PATH
#endif

#define MAX_PROC_NAME 200

/* Global Variables: */
extern int     myid_lipp;
extern int     numprocs_lipp;
extern char    processor_name_lipp[MPI_MAX_PROCESSOR_NAME];

#define EVERYBODY_PAR   -99
#define ROOT_PROC_PAR   0
#define SAMEID_PAR(id)  (id == myid_lipp)
#define FILTER_PAR(id)  (SAMEID_PAR(id) || id == EVERYBODY_PAR)

#define MYID_PAR        (myid_lipp)

#define NB_PROCS_PAR    (numprocs_lipp)
#define PROCNAME_PAR    (processor_name)
#define IS_ROOT_PAR     (SAMEID_PAR(ROOT_PROC_PAR))

#define MAX_PROC    1600


#define PARALLEL_CODE(code_fragment)    code_fragment 
#define SERIAL_CODE(code_fragment)


#else /* WITH_MPI not defined */


#define PARALLEL_CODE(code_fragment)
#define SERIAL_CODE(code_fragment)    code_fragment 
#define IS_ROOT_PAR     (1)

/* Define MYID_PAR for unified output */
#define MYID_PAR -1


#endif /* WITH_MPI */

#endif /* PARUTIL_H */
