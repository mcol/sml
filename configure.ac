AC_INIT([SML],[0.7],[sml-support@maths.ed.ac.uk])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/sml.tab.ypp])
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_INSTALL

# amplsolver is required
ACX_AMPL( [], AC_ERROR([Ampl Solver Library Required]) )

# OOPS interface requires OOPS and MeTiS
IMMDX_LIB_METIS( [HAVE_METIS=yes], [] )
ACX_OOPS( [HAVE_OOPS=yes], [] )
if test "$HAVE_METIS" = "yes" -a "$HAVE_OOPS" = "yes" ; then
  AC_SUBST(OOPS_INTERFACE, [oops])
fi

AC_ARG_WITH(
   [mps],
   [AC_HELP_STRING([--with-mps],
   [enable SML-MPS interface])
   ],
   [
      if test "$withval" == yes ; then
         AC_SUBST(MPS_INTERFACE, [mps])
      fi
   ],
   [
      AC_SUBST(MPS_INTERFACE, [mps])
   ])


AC_CONFIG_FILES([Makefile src/Makefile interfaces/Makefile
                 interfaces/oops/Makefile interfaces/mps/Makefile])
AC_OUTPUT

echo "==========================================="
echo "Summary:"
echo "Interfaces to build: $MPS_INTERFACE $OOPS_INTERFACE"
