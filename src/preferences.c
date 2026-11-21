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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GEOCLUE
#include <geoclue.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wmforecast.h"

#define COPYRIGHT_YEARS "2014-2023"

#define icondir_warning(tried, current) \
	wwarning("%s is not a valid icon directory; falling back to %s", \
		 tried, current)

static Bool check_icondir(char *icondir)
{
	int i, good;
	const char icon_names[10][30] = {
		"/dialog-error.png",
		"/weather-clear-night.png",
		"/weather-clear.png",
		"/weather-few-clouds-night.png",
		"/weather-few-clouds.png",
		"/weather-fog.png",
		"/weather-overcast.png",
		"/weather-showers.png",
		"/weather-snow.png",
		"/weather-storm.png"
	};

	good = True;

	for (i = 0; i < 10; i++) {
		char *filename;

		filename = wstrconcat(icondir, icon_names[i]);
		if (access(filename, F_OK) != 0) {
			good = False;
			break;
		}
	}

	return good;
}

static GWeatherTemperatureUnit string_to_unit(char *unit_string)
{
	if (strcmp(unit_string, "c") == 0)
		return GWEATHER_TEMP_UNIT_CENTIGRADE;

	else /* default to fahrenheit */
		return GWEATHER_TEMP_UNIT_FAHRENHEIT;
}

static void readPreferences(Preferences *prefs)
{
	if (prefs->defaults) {
		char *value;
		value = WMGetUDStringForKey(prefs->defaults, "units");
		if (value)
			prefs->units = string_to_unit(value);
		value = WMGetUDStringForKey(prefs->defaults, "interval");
		if (value)
			prefs->interval = strtol(value, NULL, 10);
		value = WMGetUDStringForKey(prefs->defaults, "background");
		if (value)
			prefs->background = value;
		value = WMGetUDStringForKey(prefs->defaults, "text");
		if (value)
			prefs->text = value;
		value = WMGetUDStringForKey(prefs->defaults, "latitude");
		if (value)
			prefs->latitude = atof(value);
		value = WMGetUDStringForKey(prefs->defaults, "longitude");
		if (value)
			prefs->longitude = atof(value);
		value = WMGetUDStringForKey(prefs->defaults, "icondir");
		if (value) {
			if (check_icondir(value))
				prefs->icondir = value;
			else
				icondir_warning(value, prefs->icondir);
		}
	}
}

Preferences *setPreferences(int argc, char **argv)
{
	Preferences *prefs = wmalloc(sizeof(Preferences));

	/* set defaults */
	prefs->units = GWEATHER_TEMP_UNIT_FAHRENHEIT;
	/* default location is nyc */
	prefs->latitude = 40.7128;
	prefs->longitude = -74.0060;
	prefs->interval = 60;
	prefs->background = DEFAULT_BG_COLOR;
	prefs->text = DEFAULT_TEXT_COLOR;
	prefs->icondir = DATADIR;
	prefs->geoclue = True;
	prefs->windowed = False;
	prefs->days = 7;
	prefs->defaults = WMGetStandardUserDefaults();
	readPreferences(prefs);

	/* command line */
	while (1) {
		int c;

		static struct option long_options[] = {
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{"units", required_argument, 0, 'u'},
			{"interval", required_argument, 0, 'i'},
			{"background", required_argument, 0, 'b'},
			{"text", required_argument, 0, 't'},
			{"latitude", required_argument, 0, 'p'},
			{"longitude", required_argument, 0, 'l'},
			{"icondir", required_argument, 0, 'I'},
			{"no-geoclue", no_argument, 0, 'n'},
			{"windowed", no_argument, 0, 'w'},
			{"days", required_argument, 0, 'd'},
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "vhu:i:b:t:p:l:I:nwd:",
				 long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (long_options[option_index].flag != 0)
				break;
			printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case 'v':
			printf("%s\n"
			       "Copyright © " COPYRIGHT_YEARS " Doug Torrance\n"
			       "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
			       "This is free software: you are free to change and redistribute it.\n"
			       "There is NO WARRANTY, to the extent permitted by law.\n"
			       , PACKAGE_STRING);
			exit(0);

		case 'u':
			if ((strcmp(optarg, "f") != 0) && (strcmp(optarg, "c") != 0)) {
				printf("units must be 'f' or 'c'\n");
				exit(0);
			}
			prefs->units = string_to_unit(optarg);
			break;

		case 'i':
			prefs->interval = strtol(optarg, NULL, 10);
			break;

		case 'b':
			prefs->background = optarg;
			break;

		case 't':
			prefs->text = optarg;
			break;

		case 'p':
			prefs->latitude = atof(optarg);
			break;

		case 'l':
			prefs->longitude = atof(optarg);
			break;

		case 'I':
			if (check_icondir(optarg))
				prefs->icondir = optarg;
			else
				icondir_warning(optarg, prefs->icondir);
			break;

		case 'n':
			prefs->geoclue = False;
			break;

		case 'w':
			prefs->windowed = True;
			break;

		case 'd':
			prefs->days = atoi(optarg);
			break;

		case '?':
		case 'h':
			printf("A weather dockapp for Window Maker using libgweather\n"
			       "Usage: wmforecast [OPTIONS]\n"
			       "Options:\n"
			       "    -v, --version            print the version number\n"
			       "    -h, --help               print this help screen\n"
			       "    -i, --interval <min>     number of minutes between refreshes (default 60)\n"
			       "    -u, --units <c|f>        whether to use Celsius or Fahrenheit (default f)\n"
			       "    -b, --background <color> set background color\n"
			       "    -t, --text <color>       set text color\n"
			       "    -p, --latitude <coord>   set latitude\n"
			       "    -l, --longitude <coord>  set longitude\n"
			       "    -I, --icondir <dir>      set icon directory\n"
			       "                             (default "DATADIR")\n"
			       "    -n, --no-geoclue         disable geoclue\n"
			       "    -w, --windowed           run in windowed mode\n"
			       "    -d, --days               number of days to show in forecast (default 7)\n"
			       "Report bugs to: %s\n"
			       "wmforecast home page: %s\n",
			       PACKAGE_BUGREPORT, PACKAGE_URL
				);
			exit(0);

		default:

			abort();
		}
	}

	return prefs;
}

static void closePreferences(WMWidget *widget, void *data)
{
	Dockapp *d = (Dockapp *)data;
	(void)widget;
	WMDestroyWidget(d->prefsWindow->window);
	d->prefsWindowPresent = 0;
}

static void savePreferences(WMWidget *widget, void *data)
{
	Dockapp *d = (Dockapp *)data;
	(void)widget;
	if (WMGetButtonSelected(d->prefsWindow->celsius))
		WMSetUDStringForKey(d->prefs->defaults, "c", "units");
	if (WMGetButtonSelected(d->prefsWindow->fahrenheit))
		WMSetUDStringForKey(d->prefs->defaults, "f", "units");
	WMSetUDStringForKey(d->prefs->defaults,
			    WMGetTextFieldText(d->prefsWindow->interval),
			    "interval");
	WMSetUDStringForKey(d->prefs->defaults,
			    WMGetColorRGBDescription(
				    WMGetColorWellColor(
					    d->prefsWindow->background)),
			    "background");
	WMSetUDStringForKey(d->prefs->defaults,
			    WMGetColorRGBDescription(
				    WMGetColorWellColor(
					    d->prefsWindow->text)),
			    "text");
	WMSetUDStringForKey(d->prefs->defaults,
			    WMGetTextFieldText(d->prefsWindow->latitude),
			    "latitude");
	WMSetUDStringForKey(d->prefs->defaults,
			    WMGetTextFieldText(d->prefsWindow->longitude),
			    "longitude");
	if (check_icondir(d->prefsWindow->icondir))
		WMSetUDStringForKey(d->prefs->defaults, d->prefsWindow->icondir,
				"icondir");
	else /* it shouldn't be possible to get here, but just in case */
		icondir_warning(d->prefsWindow->icondir, d->prefs->icondir);

	WMSaveUserDefaults(d->prefs->defaults);

	readPreferences(d->prefs);
	updateDockapp(d);

}

#ifdef HAVE_GEOCLUE
static void foundCoords(GObject *source_object, GAsyncResult *res,
			gpointer user_data)
{
	Dockapp *d = (Dockapp *)user_data;
	GClueSimple *simple;
	GError *error;
	GClueLocation *location;

	(void)source_object;
	error = NULL;
	simple = gclue_simple_new_finish(res, &error);
	location = NULL;

	if (simple)
		location = gclue_simple_get_location(simple);

	if (location) {
		char latitude[10], longitude[10];

		sprintf(latitude, "%.4f",
			gclue_location_get_latitude(location));
		sprintf(longitude, "%.4f",
			gclue_location_get_longitude(location));
		WMSetTextFieldText(d->prefsWindow->latitude, latitude);
		WMSetTextFieldText(d->prefsWindow->longitude, longitude);
		WMSetButtonText(d->prefsWindow->find_coords, "Find Coords");
	} else {
		WMSetButtonText(d->prefsWindow->find_coords,
				"Error. Try again?");
		if (error) {
			WMSetBalloonTextForView(
				error->message,
				WMWidgetView(d->prefsWindow->find_coords));
			g_error_free(error);
		}
	}
}

static void findCoords(WMWidget *widget, void *data)
{
	Dockapp *d = (Dockapp *)data;
	(void)widget;
	WMSetButtonText(d->prefsWindow->find_coords, "Finding...");
	WMSetBalloonTextForView(
		NULL,
		WMWidgetView(d->prefsWindow->find_coords));

	gclue_simple_new("wmforecast", GCLUE_ACCURACY_LEVEL_CITY, NULL,
			 foundCoords, d);
}
#endif

static void restore_default_colors(WMWidget *widget, void *data)
{
	Dockapp *d = (Dockapp *)data;

	(void)widget;

	WMSetColorWellColor(d->prefsWindow->background,
			    WMCreateNamedColor(d->screen, DEFAULT_BG_COLOR,
					       True));
	WMSetColorWellColor(d->prefsWindow->text,
			    WMCreateNamedColor(d->screen, DEFAULT_TEXT_COLOR,
					       True));
}

static void icon_chooser(WMWidget *widget, void *data)
{

	Dockapp *d = (Dockapp *)data;
	char *icondir;

	(void)widget;
	if (!WMRunModalFilePanelForDirectory(
		    d->prefsWindow->icon_chooser, NULL, d->prefsWindow->icondir,
		    "Icon directory", NULL))
		return;

	icondir = WMGetFilePanelFileName(d->prefsWindow->icon_chooser);
	if (check_icondir(icondir))
		d->prefsWindow->icondir = icondir;
	else
		WMRunAlertPanel(
			d->screen, d->prefsWindow->window,
			"Invalid icon directory",
			"You must select a directory containing "
			"dialog-error.png and weather-*.png.",
			NULL, "Close", NULL);
}

void editPreferences(void *data)
{
	char intervalPtr[50];
	Dockapp *d = (Dockapp *)data;

	d->prefsWindowPresent = 1;

	d->prefsWindow = wmalloc(sizeof(PreferencesWindow));

	d->prefsWindow->screen = d->screen;
	d->prefsWindow->prefs = d->prefs;


	d->prefsWindow->window = WMCreateWindow(d->prefsWindow->screen, "wmforecast");
	WMSetWindowTitle(d->prefsWindow->window, "wmforecast");
	WMSetWindowCloseAction(d->prefsWindow->window, closePreferences, d);
	WMResizeWidget(d->prefsWindow->window, 424, 180);
	WMRealizeWidget(d->prefsWindow->window);
	WMMapWidget(d->prefsWindow->window);

	d->prefsWindow->units = WMCreateFrame(d->prefsWindow->window);
	WMSetFrameTitle(d->prefsWindow->units, "Units");
	WMResizeWidget(d->prefsWindow->units, 112, 80);
	WMMoveWidget(d->prefsWindow->units, 10, 10);
	WMRealizeWidget(d->prefsWindow->units);
	WMMapWidget(d->prefsWindow->units);

	d->prefsWindow->celsius = WMCreateButton(d->prefsWindow->units, WBTRadio);
	WMSetButtonText(d->prefsWindow->celsius, "Celsius");
	WMMoveWidget(d->prefsWindow->celsius, 10, 20);
	if (d->prefsWindow->prefs->units == GWEATHER_TEMP_UNIT_CENTIGRADE)
		WMSetButtonSelected(d->prefsWindow->celsius, 1);
	WMRealizeWidget(d->prefsWindow->celsius);
	WMMapWidget(d->prefsWindow->celsius);

	d->prefsWindow->fahrenheit = WMCreateButton(d->prefsWindow->units, WBTRadio);
	WMSetButtonText(d->prefsWindow->fahrenheit, "Fahrenheit");
	WMMoveWidget(d->prefsWindow->fahrenheit, 10, 44);
	if (d->prefsWindow->prefs->units == GWEATHER_TEMP_UNIT_FAHRENHEIT)
		WMSetButtonSelected(d->prefsWindow->fahrenheit, 1);
	WMRealizeWidget(d->prefsWindow->fahrenheit);
	WMMapWidget(d->prefsWindow->fahrenheit);

	WMGroupButtons(d->prefsWindow->celsius, d->prefsWindow->fahrenheit);

	d->prefsWindow->locationFrame = WMCreateFrame(d->prefsWindow->window);
	WMSetFrameTitle(d->prefsWindow->locationFrame, "Location");
	WMResizeWidget(d->prefsWindow->locationFrame, 160, 80);
	WMMoveWidget(d->prefsWindow->locationFrame, 132, 10);
	WMRealizeWidget(d->prefsWindow->locationFrame);
	WMMapWidget(d->prefsWindow->locationFrame);

	d->prefsWindow->latitudeLabel = WMCreateLabel(
		d->prefsWindow->locationFrame);
	WMSetLabelText(d->prefsWindow->latitudeLabel, "Latitude:");
	WMMoveWidget(d->prefsWindow->latitudeLabel, 10, 18);
	WMRealizeWidget(d->prefsWindow->latitudeLabel);
	WMMapWidget(d->prefsWindow->latitudeLabel);

	d->prefsWindow->latitude = WMCreateTextField(
		d->prefsWindow->locationFrame);
	sprintf(intervalPtr, "%.4f", d->prefsWindow->prefs->latitude);
	WMSetTextFieldText(d->prefsWindow->latitude, intervalPtr);
	WMResizeWidget(d->prefsWindow->latitude, 65, 18);
	WMMoveWidget(d->prefsWindow->latitude, 80, 17);
	WMRealizeWidget(d->prefsWindow->latitude);
	WMMapWidget(d->prefsWindow->latitude);

	d->prefsWindow->longitudeLabel = WMCreateLabel(
		d->prefsWindow->locationFrame);
	WMSetLabelText(d->prefsWindow->longitudeLabel, "Longitude:");
	WMMoveWidget(d->prefsWindow->longitudeLabel, 10, 37);
	WMResizeWidget(d->prefsWindow->longitudeLabel, 65, 14);
	WMRealizeWidget(d->prefsWindow->longitudeLabel);
	WMMapWidget(d->prefsWindow->longitudeLabel);

	d->prefsWindow->longitude = WMCreateTextField(
		d->prefsWindow->locationFrame);
	sprintf(intervalPtr, "%.4f", d->prefsWindow->prefs->longitude);
	WMSetTextFieldText(d->prefsWindow->longitude, intervalPtr);
	WMResizeWidget(d->prefsWindow->longitude, 65, 18);
	WMMoveWidget(d->prefsWindow->longitude, 80, 36);
	WMRealizeWidget(d->prefsWindow->longitude);
	WMMapWidget(d->prefsWindow->longitude);

	d->prefsWindow->find_coords = WMCreateButton(
		d->prefsWindow->locationFrame, WBTMomentaryPush);
	WMSetButtonText(d->prefsWindow->find_coords, "Find Coords");
#ifdef HAVE_GEOCLUE
	WMSetButtonAction(d->prefsWindow->find_coords, findCoords, d);
	WMSetButtonEnabled(d->prefsWindow->find_coords,
			   d->prefs->geoclue);
#else
	WMSetButtonEnabled(d->prefsWindow->find_coords, False);
#endif
	WMResizeWidget(d->prefsWindow->find_coords, 120, 18);
	WMMoveWidget(d->prefsWindow->find_coords, 20, 57);
	WMRealizeWidget(d->prefsWindow->find_coords);
	WMMapWidget(d->prefsWindow->find_coords);


	d->prefsWindow->intervalFrame = WMCreateFrame(d->prefsWindow->window);
	WMSetFrameTitle(d->prefsWindow->intervalFrame, "Refresh interval");
	WMResizeWidget(d->prefsWindow->intervalFrame, 112, 80);
	WMMoveWidget(d->prefsWindow->intervalFrame, 302, 10);
	WMRealizeWidget(d->prefsWindow->intervalFrame);
	WMMapWidget(d->prefsWindow->intervalFrame);

	d->prefsWindow->interval = WMCreateTextField(d->prefsWindow->intervalFrame);
	sprintf(intervalPtr, "%lu", d->prefsWindow->prefs->interval);
	WMSetTextFieldText(d->prefsWindow->interval, intervalPtr);
	WMResizeWidget(d->prefsWindow->interval, 30, 20);
	WMMoveWidget(d->prefsWindow->interval, 15, 33);
	WMRealizeWidget(d->prefsWindow->interval);
	WMMapWidget(d->prefsWindow->interval);

	d->prefsWindow->minutes = WMCreateLabel(d->prefsWindow->intervalFrame);
	WMSetLabelText(d->prefsWindow->minutes, "minutes");
	WMMoveWidget(d->prefsWindow->minutes, 45, 35);
	WMRealizeWidget(d->prefsWindow->minutes);
	WMMapWidget(d->prefsWindow->minutes);

	d->prefsWindow->colors = WMCreateFrame(d->prefsWindow->window);
	WMSetFrameTitle(d->prefsWindow->colors, "Colors");
	WMResizeWidget(d->prefsWindow->colors, 265, 80);
	WMMoveWidget(d->prefsWindow->colors, 10, 92);
	WMRealizeWidget(d->prefsWindow->colors);
	WMMapWidget(d->prefsWindow->colors);

	d->prefsWindow->backgroundLabel = WMCreateLabel(d->prefsWindow->colors);
	WMSetLabelText(d->prefsWindow->backgroundLabel, "Background");
	WMSetLabelTextAlignment(d->prefsWindow->backgroundLabel, WACenter);
	WMResizeWidget(d->prefsWindow->backgroundLabel, 70, 20);
	WMMoveWidget(d->prefsWindow->backgroundLabel, 10, 20);
	WMRealizeWidget(d->prefsWindow->backgroundLabel);
	WMMapWidget(d->prefsWindow->backgroundLabel);

	d->prefsWindow->background = WMCreateColorWell(d->prefsWindow->colors);
	WMSetColorWellColor(d->prefsWindow->background, WMCreateNamedColor(d->screen, d->prefs->background, True));
	WMMoveWidget(d->prefsWindow->background, 88, 16);
	WMRealizeWidget(d->prefsWindow->background);
	WMMapWidget(d->prefsWindow->background);

	d->prefsWindow->textLabel = WMCreateLabel(d->prefsWindow->colors);
	WMSetLabelText(d->prefsWindow->textLabel, "Text");
	WMSetLabelTextAlignment(d->prefsWindow->textLabel, WACenter);
	WMResizeWidget(d->prefsWindow->textLabel, 26, 20);
	WMMoveWidget(d->prefsWindow->textLabel, 156, 20);
	WMRealizeWidget(d->prefsWindow->textLabel);
	WMMapWidget(d->prefsWindow->textLabel);

	d->prefsWindow->text = WMCreateColorWell(d->prefsWindow->colors);
	WMSetColorWellColor(d->prefsWindow->text, WMCreateNamedColor(d->screen, d->prefs->text, True));
	WMMoveWidget(d->prefsWindow->text, 190, 16);
	WMRealizeWidget(d->prefsWindow->text);
	WMMapWidget(d->prefsWindow->text);

	d->prefsWindow->restore_defaults = WMCreateButton(
		d->prefsWindow->colors, WBTMomentaryPush);
	WMSetButtonText(d->prefsWindow->restore_defaults, "Restore defaults");
	WMSetButtonAction(d->prefsWindow->restore_defaults,
			  restore_default_colors, d);
	WMResizeWidget(d->prefsWindow->restore_defaults, 125, 20);
	WMMoveWidget(d->prefsWindow->restore_defaults, 70, 55);
	WMRealizeWidget(d->prefsWindow->restore_defaults);
	WMMapWidget(d->prefsWindow->restore_defaults);

	d->prefsWindow->open_icon_chooser = WMCreateButton(
		d->prefsWindow->window, WBTMomentaryPush);
	WMSetButtonText(d->prefsWindow->open_icon_chooser, "Icon directory");
	WMSetButtonAction(d->prefsWindow->open_icon_chooser, icon_chooser, d);
	WMResizeWidget(d->prefsWindow->open_icon_chooser, 128, 20);
	WMMoveWidget(d->prefsWindow->open_icon_chooser, 285, 110);
	WMRealizeWidget(d->prefsWindow->open_icon_chooser);
	WMMapWidget(d->prefsWindow->open_icon_chooser);
	WMSetBalloonTextForView(
		"Select a directory containing dialog-error.png and "
		"weather-*.png, ideally 48x48 pixels.",
		WMWidgetView(d->prefsWindow->open_icon_chooser));

	d->prefsWindow->icon_chooser = WMGetOpenPanel(d->screen);
	WMSetFilePanelCanChooseDirectories(d->prefsWindow->icon_chooser, True);
	WMSetFilePanelCanChooseFiles(d->prefsWindow->icon_chooser, False);
	d->prefsWindow->icondir = wstrdup(d->prefs->icondir);

	d->prefsWindow->save = WMCreateButton(d->prefsWindow->window, WBTMomentaryPush);
	WMSetButtonText(d->prefsWindow->save, "Save");
	WMSetButtonAction(d->prefsWindow->save, savePreferences, d);
	WMResizeWidget(d->prefsWindow->save, 63, 20);
	WMMoveWidget(d->prefsWindow->save, 285, 137);
	WMRealizeWidget(d->prefsWindow->save);
	WMMapWidget(d->prefsWindow->save);

	d->prefsWindow->close = WMCreateButton(d->prefsWindow->window, WBTMomentaryPush);
	WMSetButtonText(d->prefsWindow->close, "Close");
	WMSetButtonAction(d->prefsWindow->close, closePreferences, d);
	WMResizeWidget(d->prefsWindow->close, 63, 20);
	WMMoveWidget(d->prefsWindow->close, 350, 137);
	WMRealizeWidget(d->prefsWindow->close);
	WMMapWidget(d->prefsWindow->close);

	WMSetTextFieldNextTextField(d->prefsWindow->latitude,
				    d->prefsWindow->longitude);
	WMSetTextFieldNextTextField(d->prefsWindow->longitude,
				    d->prefsWindow->interval);
	WMSetTextFieldNextTextField(d->prefsWindow->interval,
				    d->prefsWindow->latitude);
}
