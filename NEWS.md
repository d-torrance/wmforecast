version 1.6
-----------
* Release date: 2021-03-13
* Add support for older libgweather versions (>= 3.11.3).  Note that with the
  recent yr.no -> met.no change, using a version < 40 will only display current
  conditions and no forecast data.
* Add release dates to previous NEWS.md entries.
* Stop generating and shipping ChangeLog.

version 1.5
-----------
* Release date: 2021-03-06
* Add support for libgweather 40; needed for fetching forecast data after
  yr.no -> met.no change.
* Bug fixes.

version 1.4
-----------
* Release date: 2020-06-06
* If there is no current weather data but there are upcoming
  forecasts, then use the data from the first forecast.
* New button to restore default colors in the preferences window.
* Bug fixes.

version 1.3
-----------
* Release date: 2020-05-14
* git2cl is no longer a dependency if building from git

version 1.2
-----------
* Release date: 2020-05-12
* Bug fix.
  - You can now build wmforecast from a directory other than the source
    directory without needing git2cl to regenerate ChangeLog.

version 1.1
-----------
* Release date: 2020-05-04
* Bug fix.

version 1.0
-----------
* Release date: 2020-04-29
* Due to changes in the Yahoo! weather API, we have switched to using
  [libgweather](https://wiki.gnome.org/Projects/LibGWeather).
  This brings with it several interface changes:
  - Instead of using WOEID or ZIP Code to determine your location, we
    now use latitude and longitude.  You may (optionally) use
    [GeoClue](https://gitlab.freedesktop.org/geoclue/geoclue/-/wikis/home)
    to find this information in the preferences GUI by clicking the
    "Find Coords" button.
  - Since libgweather uses icons that follow the
    [XDG Icon Naming Specification](
     https://specifications.freedesktop.org/icon-naming-spec),
    we have renamed our icons accordingly (and deleted several which
	would never appear as they did not correspond with any of the names
	in the specification).  You may also use any other icon themes which
	follow this specification using the `--icondir` command-line
	option, the `icondir` option in the configuration file, or by
	clicking the "Icon directory" button in the preferences GUI.
* You may now run wmforecast in windowed mode (i.e., not as a dockapp)
  using the `--windowed` command-line option.

version 0.11
------------
* Release date: 2016-04-01
* New Yahoo! API gives us bad xml pretty frequently, which was causing
  frequent segfaults.  We display an error instead.
* If we get an error, we try again in 1 minute.
* If we manually refresh the display, the countdown to the next automatic
  refresh is reset to the default interval.  (Previously, if it was set to
  refresh every 60 minutes and we manually refreshed it after 10 minutes,
  it would refresh again after 50 minutes.  Now it will refresh again after
  60 minutes.)

version 0.10
------------
* Release date: 2016-03-26
* Update to work with new Yahoo! Weather API.

version 0.9
-----------
* Release date: 2016-01-17
* New feature!
  - You may change the background or text colors either using the command line
    (-b or --background for background, -t or --text for text), or by using the
    preferences GUI.
* The configuration file has been moved to the GNUstep Defaults directory
  (likely ~/GNUstep/Defaults/wmforecast -- note this is the same directory as
  the Window Maker configuration files).  You may also store system-wide
  configuration in, e.g., /etc/GNUstep/Defaults/wmforecast.  This move takes
  advantage of several built-in WINGs functions.

version 0.8
-----------
* Release date: 2014-10-21
* Update license information in svg icon.
* New homepage on github.

version 0.7
-----------
* Release date: 2014-07-17
* Update copyright information for icons.

version 0.6
-----------
* Release date: 2014-06-03
* Bug fixes

version 0.5
-----------
* Release date: 2014-05-11
* Now appears, with icon, in most desktop environment menus
* Bug fix

version 0.4
-----------
* Release date: 2014-05-07
* New features!
  - Right click the icon to edit preferences in a GUI
  - Preferences may also be manually configured in XDG_CONFIG_DIR/wmforecast/
    wmforecastrc, or HOME/.config/wmforecast/wmforecastrc if XDG_CONFIG_DIR is
    undefined
* More bug fixes

version 0.3
-----------
* Release date: 2014-04-21
* New features!
  - Double click icon to refresh data
  - Use -i or --interval option to set how frequently data is automatically
    refreshed
* Error messages displayed, e.g., when internet down or icon files aren't found
* Lots of bug fixes and under-the-hood changes

version 0.2
-----------
* Added manpage
* Bug fixes

version 0.1
-----------
* Initial release
