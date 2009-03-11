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
 *  @file OopsInterface.h
 *
 *  OOPS interface to the outside world.
 *
 *  @author Andreas Grothey
 *
 *  This file should contain all the code that is needed to call OOPS
 *  from the outside.
 *
 *  @todo The idea is that eventually only this file needs to be #include'd
 *  and that the source of this file can be made available.
 */

#ifndef OOPSINTERFACE_H
#define OOPSINTERFACE_H

#include "hopdm.h"
#include "CallBack.h"
#include "MatrixSparseSimple.h"
#include "BlockDenseAlg.h"
#include "DblBordDiagSimpleAlg.h"
#include "BlockDiagSimpleAlg.h"
#include "WriteMps.h"

/** Perform all the necessary steps to set up the OOPS internal structures. */
Algebra *OOPSSetup(Algebra *A, Algebra *Q);

#endif /* OOPSINTERFACE_H */
