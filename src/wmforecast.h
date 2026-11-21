/* Copyright (C) 2014-2023 Doug Torrance <dtorrance@piedmont.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef WMFORECAST_H
#define WMFORECAST_H

#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include <libgweather/gweather.h>
#include <WINGs/WINGs.h>

#define DEFAULT_TEXT_COLOR "light sea green"
#define DEFAULT_BG_COLOR "black"

typedef struct {
	Bool geoclue;
	GWeatherTemperatureUnit units;
	double latitude;
	double longitude;
	long int interval;
	const char *background;
	const char *text;
	const char *icondir;
	Bool windowed;
	int days;
	WMUserDefaults *defaults;
} Preferences;

typedef struct {
	Preferences *prefs;
	WMButton *celsius;
	WMButton *close;
	WMButton *fahrenheit;
	WMButton *save;
	WMButton *find_coords;
	WMButton *open_icon_chooser;
	WMButton *restore_defaults;
	WMColorWell *background;
	WMColorWell *text;
	WMFrame *intervalFrame;
	WMFrame *locationFrame;
	WMFrame *units;
	WMFrame *colors;
	WMLabel *minutes;
	WMLabel *backgroundLabel;
	WMLabel *textLabel;
	WMLabel *latitudeLabel;
	WMLabel *longitudeLabel;
	WMScreen *screen;
	WMTextField *interval;
	WMTextField *latitude;
	WMTextField *longitude;
	WMOpenPanel *icon_chooser;
	char *icondir;
	WMWindow *window;
} PreferencesWindow;

typedef struct {
	int prefsWindowPresent;
	int showForecast;
	long int minutesLeft;
	Preferences *prefs;
	PreferencesWindow *prefsWindow;
	WMFrame *frame;
	WMLabel *icon;
	WMLabel *text;
	WMScreen *screen;
} Dockapp;

/* dockapp.c */
Dockapp *newDockapp(WMScreen *screen, Preferences *prefs,
		    int argc, char **argv);
void refresh(XEvent *event, void *data);
void timerHandler(void *data);

/* weather.c */
void updateDockapp(void *data);

/* preferences.c */
Preferences *setPreferences(int argc, char **argv);
void editPreferences(void *data);

#endif
