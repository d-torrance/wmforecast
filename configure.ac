AC_INIT([wmforecast], [0.11], [dtorrance@piedmont.edu], [wmforecast],
	[http://wmforecast.friedcheese.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADER([config.h])
AC_PROG_CC
PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([libgweather], [gweather-3.0])
AC_ARG_WITH([geoclue],
	    [AS_HELP_STRING([--with-geoclue],
		   [build with geoclue (provides location information)])],
	    [AS_IF([test "x$with_geoclue" == "xyes"],
		   [with_geoclue=check])],
	    [with_geoclue=check])
AS_IF([test "x$with_geoclue" == "xcheck"],
      [PKG_CHECK_MODULES([geoclue], [libgeoclue-2.0],
			 [AC_DEFINE([HAVE_GEOCLUE], [1],
			 [Define if pkg-config finds geoclue.])], [:])])
dnl Eventually, use pkg-config for WINGs.  However, pkg-config files were broken
dnl until Window Maker 0.95.6, which is not available in older distributions,
dnl e.g., Ubuntu Precise and Trusty.
AC_CHECK_LIB(WINGs,WMCreateWindow)
AC_SUBST(DATE, "esyscmd(head -n 1 ChangeLog | cut -d' ' -f1 | tr -d '\n')")
AC_CONFIG_FILES([Makefile icons/Makefile wmforecast.1])
AC_OUTPUT
