AC_INIT([wmforecast], [1.9.0],
	[https://github.com/d-torrance/wmforecast/issues],
	[wmforecast], [http://wmforecast.friedcheese.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign -Wno-portability subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([GWEATHER], [gweather4], [
    PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
    HAVE_GWEATHER_VERSION=4000000], [
    PKG_CHECK_MODULES([GWEATHER], [gweather-3.0 >= 3.11.3])
    SAVE=$CPPFLAGS
    CPPFLAGS="$SAVE `pkg-config gweather-3.0 --cflags` \
	-Werror=implicit-function-declaration"
    AC_MSG_CHECKING([for gweather-3.0 >= 3.27.4])
    AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([
		#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
		#include <libgweather/gweather.h>
	],
	[
		gweather_info_new(NULL);
	])],
	[AC_MSG_RESULT([yes])
	 HAVE_GWEATHER_VERSION=3027004],
	AC_MSG_RESULT([no]))
    AC_MSG_CHECKING([for gweather-3.0 >= 40])
    AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([
		#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
		#include <libgweather/gweather.h>
	],
	[
		GWeatherInfo *info;
		gweather_info_set_application_id(info, "application id");
		gweather_info_set_contact_info(info, "contact info");
	])],
	[AC_MSG_RESULT([yes])
	 HAVE_GWEATHER_VERSION=3040000],
	AC_MSG_RESULT([no]))
    CPPFLAGS=$SAVE])
AC_DEFINE_UNQUOTED([HAVE_GWEATHER_VERSION], [$HAVE_GWEATHER_VERSION],
	  [Available version of libgweather])
AC_ARG_WITH([geoclue],
	    [AS_HELP_STRING([--with-geoclue],
		   [build with geoclue (provides location information)])],
	    [AS_IF([test "x$with_geoclue" == "xyes"],
		   [with_geoclue=check])],
	    [with_geoclue=check])
AS_IF([test "x$with_geoclue" == "xcheck"],
      [PKG_CHECK_MODULES([GEOCLUE], [libgeoclue-2.0],
			 [AC_DEFINE([HAVE_GEOCLUE], [1],
			 [Define if pkg-config finds geoclue.])], [:])])
PKG_CHECK_MODULES([WINGS], [WINGs])
AC_CONFIG_FILES([Makefile icons/Makefile])
AC_OUTPUT
