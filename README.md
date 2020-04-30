wmforecast
==========
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0) [![Build Status](https://travis-ci.org/d-torrance/wmforecast.svg?branch=master)](https://travis-ci.org/d-torrance/wmforecast)

<http://wmforecast.friedcheese.org>

wmforecast is a weather dockapp for Window Maker using
[libgweather](https://wiki.gnome.org/Projects/LibGWeather)

The icons were designed by MerlinTheRed and are available at
<http://merlinthered.deviantart.com/art/plain-weather-icons-157162192>

Download
--------

To download, either clone the git repository:

    git clone https://github.com/d-torrance/wmforecast

or download a tarball from the
[releases page](https://github.com/d-torrance/wmforecast/releases).

wmforecast binary packages are also available in the Debian and Ubuntu archives,
but possibly older versions.  To obtain the latest version in Ubuntu, you may
use the PPA:

    sudo add-apt-repository ppa:profzoom/dockapps
    sudo apt-get update
    sudo apt-get install wmforecast

Installation
------------

To compile wmforecast from source, you will need

* [libgweather](https://wiki.gnome.org/Projects/LibGWeather)
* [WINGs](http://windowmaker.org/)
* (optional) [GeoClue](https://gitlab.freedesktop.org/geoclue/geoclue/-/wikis/home)

If building from git (not necessary if using a tarball):

    ./autogen.sh

Then the usual

    ./configure
    make
    sudo make install

Usage
-----

    wmforecast [OPTIONS]
    Options:
    -v, --version            print the version number
    -h, --help               print this help screen
    -i, --interval <min>     number of minutes between refreshes (default 60)
    -u, --units <c|f>        whether to use Celsius or Fahrenheit (default f)
    -b, --background <color> set background color
    -t, --text <color>       set text color
    -p, --latitude <coord>   set latitude
    -l, --longitude <coord>  set longitude
    -I, --icondir <dir>      set icon directory
                             (default /usr/local/share/wmforecast)
    -n, --no-geoclue         disable geoclue
    -w, --windowed           run in windowed mode
    -d, --days               number of days to show in forecast (default 7)

Bugs
----

Please report bugs and feature requests at the
[issues page](https://github.com/d-torrance/wmforecast/issues).

Copyright
---------

### wmforecast
2014-2016 Doug Torrance  
<dtorrance@piedmont.edu>  
GNU General Public License v3+

### icons
2010 MerlinTheRed  
<http://merlinthered.deviantart.com/>  
Creative Commons Attribution-ShareAlike 3.0 License
