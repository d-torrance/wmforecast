SUBDIRS = icons
bin_PROGRAMS = wmforecast
wmforecast_SOURCES = wmforecast.c
dist_man_MANS = wmforecast.1

AM_CFLAGS = $(geoclue_CFLAGS) $(libgweather_CFLAGS) $(X11_CFLAGS)
AM_CPPFLAGS =  -DDATADIR=\"$(pkgdatadir)\"
LIBS += $(geoclue_LIBS) $(libgweather_LIBS) $(X11_LIBS) -lWINGs -lwraster -lWUtil
AM_LDFLAGS = -Wl,--as-needed

desktopdir = $(datadir)/applications
dist_desktop_DATA = wmforecast.desktop

EXTRA_DIST = autogen.sh README.md
