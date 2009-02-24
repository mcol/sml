AC_INIT([SML],[0.1],[J.Hogg@ed.ac.uk])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/sml.tab.ypp])
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_INSTALL
ACX_AMPL( [], AC_ERROR([Ampl Solver Library Required]) )
AC_SUBST(AMPL_INCLUDE)
AC_CONFIG_FILES([Makefile src/Makefile interfaces/Makefile interfaces/oops/Makefile interfaces/mps/Makefile])
AC_OUTPUT
