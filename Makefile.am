SUBDIRS = icons
bin_PROGRAMS = wmforecast
wmforecast_SOURCES = wmforecast.c
dist_man_MANS = wmforecast.1

AM_CFLAGS = $(GEOCLUE_CFLAGS) $(GWEATHER_CFLAGS) $(X11_CFLAGS) $(WINGS_CFLAGS)
AM_CPPFLAGS =  -DDATADIR=\"$(pkgdatadir)\"
LIBS += $(GEOCLUE_LIBS) $(GWEATHER_LIBS) $(X11_LIBS) $(WINGS_LIBS)
AM_LDFLAGS = -Wl,--as-needed

desktopdir = $(datadir)/applications
dist_desktop_DATA = wmforecast.desktop

EXTRA_DIST = autogen.sh README.md
