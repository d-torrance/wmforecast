SUBDIRS = icons
bin_PROGRAMS = wmforecast
wmforecast_SOURCES = wmforecast.c
dist_man_MANS = wmforecast.1

AM_CFLAGS = $(GEOCLUE_CFLAGS) $(GWEATHER_CFLAGS) $(X11_CFLAGS) $(WINGS_CFLAGS)
AM_CPPFLAGS =  -DDATADIR=\"$(pkgdatadir)\"
LIBS += $(GEOCLUE_LIBS) $(GWEATHER_LIBS) $(X11_LIBS) $(WINGS_LIBS)

desktopdir = $(datadir)/applications
dist_desktop_DATA = wmforecast.desktop

EXTRA_DIST = autogen.sh README.md wmforecast.1.in

CLEANFILES = wmforecast.1

wmforecast.1: wmforecast.1.in
	sed -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' -e 's|@DATE[@]|$(DATE)|g' \
		-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' $< > $@
