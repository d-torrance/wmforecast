SUBDIRS = icons
bin_PROGRAMS = wmforecast
wmforecast_SOURCES = wmforecast.c
man_MANS = wmforecast.1

wmforecast.1: wmforecast.c config.h
	$(HELP2MAN) --output=$@ -N -n "a weather dockapp for Window Maker using the Yahoo Weather API" ./wmforecast

AM_CFLAGS = $(libxml2_CFLAGS) $(libcurl_CFLAGS) $(X11_CFLAGS)
AM_CPPFLAGS =  -DDATADIR=\"$(pkgdatadir)\"
LIBS += $(libxml2_LIBS) $(libcurl_LIBS) $(X11_LIBS) -lWINGs -lwraster -lWUtil

EXTRA_DIST = $(man_MANS) 
