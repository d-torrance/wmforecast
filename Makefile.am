SUBDIRS = icons
bin_PROGRAMS = wmforecast
wmforecast_SOURCES = src/wmforecast.c
dist_man_MANS = wmforecast.1

AM_CFLAGS = $(GEOCLUE_CFLAGS) $(GWEATHER_CFLAGS) $(X11_CFLAGS) $(WINGS_CFLAGS) \
	$(GOBJECT_CFLAGS)
AM_CPPFLAGS =  -DDATADIR=\"$(pkgdatadir)\"
LIBS += $(GEOCLUE_LIBS) $(GWEATHER_LIBS) $(X11_LIBS) $(WINGS_LIBS) \
	$(GOBJECT_LIBS)

desktopdir = $(datadir)/applications
dist_desktop_DATA = wmforecast.desktop

metainfodir = $(datadir)/metainfo
dist_metainfo_DATA = org.friedcheese.wmforecast.metainfo.xml

EXTRA_DIST = autogen.sh README.md wmforecast.1.in NEWS.md

CLEANFILES = wmforecast.1

wmforecast.1: wmforecast.1.in $(srcdir)/NEWS.md
	$(eval DATE = $(shell grep "Release date:" $(word 2,$^) | \
		head -1 | cut -d ' ' -f 4 | tr -d '\n'))
	sed -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' -e 's|@DATE[@]|$(DATE)|g' \
		-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' $< > $@

sign: wmforecast-@PACKAGE_VERSION@.tar.gz.asc

%.asc: %
	gpg --detach-sign --armor $<

.PHONY: sign
