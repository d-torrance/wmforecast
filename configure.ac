AC_INIT([wmforecast], [0.5], [dtorrance@monmouthcollege.edu])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADER([config.h])   
AC_SUBST(VERSION)
AC_PROG_CC                      
PKG_CHECK_MODULES([libxml2],[libxml-2.0])
PKG_CHECK_MODULES([libcurl],[libcurl])
PKG_CHECK_MODULES([X11],[x11])
AC_CHECK_LIB(WINGs,WMCreateWindow)
AC_CONFIG_FILES([Makefile icons/Makefile])
AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)
AC_OUTPUT
