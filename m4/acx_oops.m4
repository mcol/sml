# SYNOPSIS
#
#   ACX_OOPS([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   This macro searches for the OOPS library in the user specified
#   location. The user may specify the location either by defining the
#   environment variable OOPSHOME or by using the --with-oops option to
#   configure. If the environment variable is defined it has precedent over
#   everything else. If no location was specified then the macro fails.
#   Upon sucessful completion the variables OOPS_LIB and OOPS_INCLUDE are set.
#
#   ACTION-IF-FOUND is a list of shell commands to run if a OOPS library is
#   found, and ACTION-IF-NOT-FOUND is a list of commands to run it if it is
#   not found. If ACTION-IF-FOUND is not specified, the default action will
#   define HAVE_OOPS. If ACTION-IF-NOT-FOUND is not specified then an error
#   will be generated halting configure.
#
# LAST MODIFICATION
#
#   2009-02-25
#
# COPYLEFT
#
#   Copyright (c) 2009 Jonathan Hogg <J.Hogg@ed.ac.uk>
#   based loosely on immdx_lib_oops.m4 by
#   Copyright (c) 2008 Ben Bergen <ben@cs.fau.de>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([ACX_OOPS], [
	AC_REQUIRE([AC_PROG_CC])
	#
	# User hints...
	#
	AC_ARG_VAR([OOPSHOME], [OOPS library location])
	AC_ARG_WITH([oops],
		[AC_HELP_STRING([--with-oops],
		[user defined path to OOPS library])],
		[
			if test -n "$OOPSHOME" ; then
				with_oops=$OOPSHOME
			elif test "$withval" != no ; then
				with_oops=$withval
			fi
		],
		[
			if test -n "$OOPSHOME" ; then
				with_oops=$OOPSHOME
			else
            abssrc=`cd $srcdir; pwd`
            with_oops="$abssrc/ThirdParty/oops"
			fi
		])
	#
	# locate OOPS library
	#
		if test -n "$with_oops" ; then
			old_CFLAGS=$CFLAGS
			old_LDFLAGS=$LDFLAGS
			CFLAGS="-I$with_oops/include"
			LDFLAGS="-L$with_oops/lib"

         AC_LANG_PUSH(C)

			AC_CHECK_LIB(oops, OOPSSetup,	[oops_lib=yes], [oops_lib=no])
			AC_CHECK_HEADER(oops/OopsInterface.h, [oops_h=yes],
				[oops_h=no], [/* check */])

         AC_LANG_POP(C)

			CFLAGS=$old_CFLAGS
			LDFLAGS=$old_LDFLAGS

			AC_MSG_CHECKING(OOPS in $with_oops)
			if test "$oops_lib" = "yes" -a "$oops_h" = "yes" ; then
            OOPS_INCLUDE="-I$with_oops/include"
            OOPS_LIB_DIR="-L$with_oops/lib"
				OOPS_LIB="$OOPS_LIB_DIR -loops"
				AC_MSG_RESULT(ok)
			else
				AC_MSG_RESULT(failed)
			fi
		fi
      #
      #
      #
      # Now check for complex linking
      if test "$HAVE_BLAS" = "yes" -a "$HAVE_LAPACK" = "yes"; then
         old_LIBS=$LIBS
         LIBS="$LIBS $OOPS_LIB $METIS_LIB $LAPACK_LIBS $BLAS_LIBS -lm"
         AC_LANG_PUSH(C++)
         AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([[#include <stdio.h>
                              FILE *globlog=NULL;
                              extern "C" char hopdm();]],
                             [[hopdm();]])],
            [oops_full_link="yes"], [oops_full_link="no"])
         AC_LANG_POP(C++)
         LIBS=$old_LIBS 

         AC_MSG_CHECKING(OOPS linking requirements)
         if test "$oops_full_link" = "yes"; then
            AC_MSG_RESULT(ok)
         else
            AC_MSG_RESULT([failed - check metis?])
         fi
      else
         AC_MSG_CHECKING(OOPS linking requirements)
         AC_MSG_RESULT(Requires BLAS and LAPACK)
         oops_full_link="no"
      fi
         
      if test "$oops_full_link" = "no"; then
         OOPS_LIB=""
         OOPS_INCLUDE=""
      else
         AC_SUBST(OOPS_INCLUDE, [$OOPS_INCLUDE])
         AC_SUBST(OOPS_LIB, [$OOPS_LIB])
      fi

		#
		#
		#
		if test x = x"$OOPS_LIB" ; then
			ifelse([$2],,[AC_MSG_ERROR(Failed to find valid OOPS library)],[$2])
			:
		else
			ifelse([$1],,[AC_DEFINE(HAVE_OOPS,1,[Define if you have OOPS library])],[$1])
			:
		fi
	])dnl ACX_OOPS
