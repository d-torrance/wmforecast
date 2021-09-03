/* Copyright (C) 2014-2021 Doug Torrance <dtorrance@piedmont.edu>
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
#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include <libgweather/gweather.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <WINGs/WINGs.h>

#define DEFAULT_TEXT_COLOR "light sea green"
#define DEFAULT_BG_COLOR "black"
#define APPLICATION_ID "org.friedcheese.wmforecast"
#define CONTACT_INFO "dtorrance@piedmont.edu"

#define icondir_warning(tried, current) \
	wwarning("%s is not a valid icon directory; falling back to %s", \
		 tried, current)

#define round(x) (int)(x + 0.5)

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
	long int minutesLeft;
	Preferences *prefs;
	PreferencesWindow *prefsWindow;
	WMFrame *frame;
	WMLabel *icon;
	WMLabel *text;
	WMScreen *screen;
} Dockapp;

typedef struct {
	char *day;
	char *low;
	char *high;
	char *text;
} Forecast;

typedef struct {
	int length;
	Forecast *forecasts;
} ForecastArray;

typedef struct {
	char *temp;
	char *text;
	ForecastArray *forecasts;
	RImage *icon;
	int errorFlag;
	char *errorText;
	char retrieved[20];
	const char *attribution;
	GWeatherTemperatureUnit units;
} Weather;

Forecast *newForecast(void);
ForecastArray *newForecastArray(void);
void appendForecast(ForecastArray *array, Forecast *forecast);
Weather *newWeather(void);
void freeForecast(Forecast *forecast);
void freeForecastArray(ForecastArray *array);
void freeWeather(Weather *weather);
void setError(Weather *weather, const char *errorText);
void setConditions(
	Weather *weather, WMScreen *screen, const char *temp, const char *text,
	const char *code, const char *background, const char *icondir);
void setForecast(Forecast *forecast, const char *day, const char *low,
		 const char *high, const char *text);
void close_window(WMWidget *self, void *data);
WMWindow *WMCreateDockapp(WMScreen *screen, const char *name, int argc,
			  char **argv, Bool windowed);
Dockapp *newDockapp(WMScreen *screen, Preferences *prefs,
		    int argc, char **argv);
char *getBalloonText(Weather *weather, int days);
char *getTemp(GWeatherInfo *info, GWeatherTemperatureUnit unit);
void gather_forecasts(Weather *weather, GSList *gforecasts);
char *strip_tags(const char *to_strip);
void getWeather(GWeatherInfo *info, Dockapp *dockapp);
Bool check_icondir(char *icondir);
GWeatherTemperatureUnit string_to_unit(char *unit_string);
void readPreferences(Preferences *prefs);
Preferences *setPreferences(int argc, char **argv);
void do_glib_loop(void *data);
void restore_default_colors(WMWidget *widget, void *data);

Forecast *newForecast(void)
{
	Forecast *forecast = wmalloc(sizeof(Forecast));
	forecast->day = NULL;
	forecast->low = NULL;
	forecast->high = NULL;
	forecast->text = NULL;
	return forecast;
}

ForecastArray *newForecastArray(void)
{
	ForecastArray *array = wmalloc(sizeof(ForecastArray));
	array->length = 0;
	array->forecasts = NULL;
	return array;
}

void appendForecast(ForecastArray *array, Forecast *forecast)
{
	array->length++;
	array->forecasts = (Forecast *)wrealloc(array->forecasts, sizeof(Forecast)*(array->length));
	array->forecasts[(array->length)-1] = *forecast;
}

Weather *newWeather(void)
{
	Weather *weather = wmalloc(sizeof(Weather));
	weather->temp = NULL;
	weather->text = NULL;
	weather->icon = NULL;
	weather->forecasts = newForecastArray();
	weather->errorFlag = 0;
	weather->errorText = NULL;
	return weather;
}

void freeForecast(Forecast *forecast)
{
	wfree(forecast->day);
	wfree(forecast->low);
	wfree(forecast->high);
	wfree(forecast->text);
}

void freeForecastArray(ForecastArray *array)
{
	int i;
	for (i = 0; i < array->length; i++)
		if (&array->forecasts[i])
			freeForecast(&array->forecasts[i]);
	wfree(array);
}

void freeWeather(Weather *weather)
{
	wfree(weather->temp);
	wfree(weather->text);
	if (weather->forecasts)
		freeForecastArray(weather->forecasts);
	wfree(weather->errorText);
	wfree(weather);
}

void setError(Weather *weather, const char *errorText)
{
	weather->errorFlag = 1;
	if (errorText) {
		weather->errorText = wrealloc(
			weather->errorText, strlen(errorText) + 1);
		strcpy(weather->errorText, errorText);
	} else
		weather->errorText = wstrdup("An error occurred");
}

void setConditions(Weather *weather,
		   WMScreen *screen,
		   const char *temp,
		   const char *text,
		   const char *code,
		   const char *background,
		   const char *icondir
	)
{
	RContext *context;
	char filename[1024];
	time_t currentTime;


	weather->temp = wstrdup(temp);
	weather->text = wstrdup(text);

	context = WMScreenRContext(screen);
	sprintf(filename, "%s/%s.png", icondir, code);

	weather->icon = RLoadImage(context, filename, 0);
	if (!weather->icon) {
		setError(weather, wstrconcat(filename, " not found"));
	} else {
		RColor color;
		color = WMGetRColorFromColor(
			WMCreateNamedColor(screen, background, True));
		RCombineImageWithColor(weather->icon, &color);
	}

	currentTime = time(NULL);
	strftime(weather->retrieved, sizeof weather->retrieved, "%l:%M %p %Z",
		 localtime(&currentTime));
}

void setForecast(Forecast *forecast,
		 const char *day,
		 const char *low,
		 const char *high,
		 const char *text
	)
{
	forecast->day = wrealloc(forecast->day, strlen(day) + 1);
	strcpy(forecast->day, day);
	forecast->low = wrealloc(forecast->low, strlen(low) + 1);
	strcpy(forecast->low, low);
	forecast->high = wrealloc(forecast->high, strlen(high) + 1);
	strcpy(forecast->high, high);
	forecast->text = wrealloc(forecast->text, strlen(text) + 1);
	strcpy(forecast->text, text);
}

void close_window(WMWidget *self, void *data)
{
	(void)data;
	WMDestroyWidget(self);
	exit(0);
}

WMWindow *WMCreateDockapp(WMScreen *screen, const char *name, int argc,
			  char **argv, Bool windowed)
{
	WMWindow *dockapp;
	XWMHints *hints;
	Display *display;
	Window window;

	display = WMScreenDisplay(screen);
	dockapp = WMCreateWindow(screen, name);
	WMRealizeWidget(dockapp);

	window = WMWidgetXID(dockapp);

	hints = XGetWMHints(display, window);
	hints->flags |= WindowGroupHint;
	hints->window_group = window;

	if (!windowed) {
		hints->flags |= IconWindowHint | StateHint;
		hints->icon_window = window;
		hints->initial_state = WithdrawnState;
	}

	XSetWMHints(display, window, hints);
	XFree(hints);

	WMSetWindowCloseAction(dockapp, close_window, NULL);

	XSetCommand(display, window, argv, argc);

	WMResizeWidget(dockapp, 56, 56);
	return dockapp;
}

Dockapp *newDockapp(WMScreen *screen, Preferences *prefs, int argc, char **argv)
{
	Dockapp *dockapp = wmalloc(sizeof(Dockapp));
	WMColor *background;
	WMColor *text;
	WMWindow *window;

	dockapp->screen = screen;
	dockapp->prefs = prefs;
	dockapp->minutesLeft = prefs->interval;
	dockapp->prefsWindowPresent = 0;

	window = WMCreateDockapp(screen, "", argc, argv, prefs->windowed);
	WMSetWindowTitle(window, "wmforecast");

	background = WMCreateNamedColor(screen, prefs->background, True);
	if (!background) {
		background = WMCreateNamedColor(screen, DEFAULT_BG_COLOR, True);
		prefs->background = DEFAULT_BG_COLOR;
	}
	text = WMCreateNamedColor(screen, prefs->text, True);
	if (!text) {
		text = WMCreateNamedColor(screen, DEFAULT_TEXT_COLOR, True);
		prefs->text = DEFAULT_TEXT_COLOR;
	}

	dockapp->frame = WMCreateFrame(window);
	WMSetFrameRelief(dockapp->frame, WRSunken);
	WMResizeWidget(dockapp->frame, 56, 56);
	WMSetWidgetBackgroundColor(dockapp->frame, background);
	WMRealizeWidget(dockapp->frame);

	dockapp->text = WMCreateLabel(dockapp->frame);
	WMSetWidgetBackgroundColor(dockapp->text, background);
	WMSetLabelFont(
		dockapp->text,
		WMCreateFont(
			screen,
			"-Misc-Fixed-Medium-R-SemiCondensed--13-120-75-75-C-60-ISO10646-1"));
	WMSetLabelTextColor(dockapp->text, text);
	WMSetLabelTextAlignment(dockapp->text, WACenter);
	WMResizeWidget(dockapp->text, 52, 14);
	WMMoveWidget(dockapp->text, 2, 40);
	WMRealizeWidget(dockapp->text);

	dockapp->icon = WMCreateLabel(dockapp->frame);
	WMSetWidgetBackgroundColor(dockapp->icon, background);
	WMRealizeWidget(dockapp->icon);
	WMSetLabelImagePosition(dockapp->icon, WIPImageOnly);
	WMResizeWidget(dockapp->icon, 32, 32);
	WMMoveWidget(dockapp->icon, 12, 5);

	WMMapWidget(window);
	WMMapWidget(dockapp->frame);
	WMMapSubwidgets(dockapp->frame);

	return dockapp;
}

char *getBalloonText(Weather *weather, int days)
{
	char *text;
	int i;

	text = wstrdup("\n"PACKAGE_STRING);
	text = wstrappend(text, "\n\nRetrieved: ");
	text = wstrappend(text, weather->retrieved);
	text = wstrappend(text, "\n\nCurrent Conditions:\n");
	text = wstrappend(text, weather->text);
	text = wstrappend(text, ", ");
	text = wstrappend(text, weather->temp);

	text = wstrappend(text, "°\n\nForecast");

	if (weather->forecasts->length == 0)
		text = wstrappend(text, " not available.");
	else {
		text = wstrappend(text, ":\n");
		for (i = 0; i < weather->forecasts->length && i < days; i++) {
			text = wstrappend(text,
					  weather->forecasts->forecasts[i].day);
			text = wstrappend(text, " - ");
			text = wstrappend(text,
					  weather->forecasts->forecasts[i].text);
			text = wstrappend(text,
					  ". High: ");
			text = wstrappend(text,
					  weather->forecasts->forecasts[i].high);
			text = wstrappend(text,
					  "° Low: ");
			text = wstrappend(text,
					  weather->forecasts->forecasts[i].low);
			text = wstrappend(text, "°\n");
		}
	}

	text = wstrappend(text, "\n");
	text = wstrappend(text, weather->attribution);
	text = wstrappend(text, "\n\n");
	return text;
}

char *getTemp(GWeatherInfo *info, GWeatherTemperatureUnit unit)
{
	double temp_double;
	char temp[4];

	gweather_info_get_value_temp(info, unit, &temp_double);
	sprintf(temp, "%d", round(temp_double));
	return wstrdup(temp);
}

void gather_forecasts(Weather *weather, GSList *gforecasts)
{
	GDateTime *d;
	int current_weekday, high, low;
	const char *conditions;

	d = g_date_time_new_now_local();
	current_weekday = g_date_time_get_day_of_week(d);
	high = INT_MIN;
	low = INT_MAX;
	conditions = "";

	while (gforecasts) {
		time_t time;
		double temp;
		int weekday;

		gweather_info_get_value_update(gforecasts->data, &time);
		d = g_date_time_new_from_unix_utc(time);
		weekday = g_date_time_get_day_of_week(d);

		/* follow gnome weather's convention of using 2 pm for
		 * conditions */
		if (g_date_time_get_hour(d) >= 14 &&
		    strcmp(conditions, "") == 0) {
			conditions = gweather_info_get_conditions(
				gforecasts->data);
			if (strcmp(conditions, "-") == 0)
				conditions = gweather_info_get_sky(
					gforecasts->data);
		}

		if (weekday != current_weekday) {
			/* don't create forecast if we don't have any info
			 * (e.g., for today if it's almost midnight) */
			if (high != INT_MIN && low != INT_MAX) {
				Forecast *forecast;
				char high_text[12], low_text[12];
				char *day_name;

				forecast = newForecast();

				/* subtract one since we've already advanced to
				 * the next day */
				day_name = g_date_time_format(
					g_date_time_add_days(d, -1), "%a");

				snprintf(high_text, 12, "%d", high);
				snprintf(low_text, 12, "%d", low);

				setForecast(forecast, day_name, low_text,
					    high_text, conditions);
				appendForecast(weather->forecasts, forecast);
			}

			current_weekday = weekday;
			high = INT_MIN;
			low = INT_MAX;
			conditions = "";
		}
		gweather_info_get_value_temp(gforecasts->data,
					     weather->units, &temp);
		if (temp > high)
			high = round(temp);
		if (temp < low)
			low = round(temp);
		gforecasts = gforecasts->next;
	}
}

/* strip html from attribution string */
char *strip_tags(const char *to_strip)
{
	char *stripped, *beginning, *end;

	if (!to_strip)
		return wstrdup("");

	stripped = wstrdup(to_strip);
	while ((beginning = strchr(stripped, '<')) &&
	       (end = strchr(stripped, '>'))) {
		int length;

		length = end - beginning + 1;
		memmove(stripped + length, stripped, beginning - stripped);
		stripped += length;
	}

	return stripped;
}

void getWeather(GWeatherInfo *info, Dockapp *dockapp)
{
	char *temp, *text;
	const char *code;
	WMPixmap *icon;
	Weather *weather;
	GSList *gforecasts;
	gboolean success;
	gdouble dummy;

	weather = newWeather();
	weather->units = dockapp->prefs->units;

	if (!gweather_info_is_valid(info))
		setError(weather,
			 gweather_info_get_weather_summary(info));

	weather->attribution = strip_tags(gweather_info_get_attribution(info));

	gforecasts = gweather_info_get_forecast_list(info);
	if (gforecasts)
		gather_forecasts(weather, gforecasts);

	/* check if we have current conditions */
	success = gweather_info_get_value_temp(info, dockapp->prefs->units,
					       &dummy);
	if (!success) {
		/* if we don't, get the next forecasted conditions */
		gforecasts = gweather_info_get_forecast_list(info);

		if (!gforecasts)
			setError(weather, "Retrieval failed");
		else {
			while (!success) {
				success = gweather_info_get_value_temp(
					gforecasts->data, dockapp->prefs->units,
					&dummy);
				if (success) {
					info = gforecasts->data;
					break;
				}

				gforecasts = gforecasts->next;
				if (!gforecasts) {
					setError(weather, "Retrieval failed");
					break;
				}
			}
		}
	}

	temp = getTemp(info, dockapp->prefs->units);
	text = gweather_info_get_weather_summary(info);
	code = gweather_info_get_icon_name(info);

	setConditions(weather, dockapp->screen, temp, text, code,
		      dockapp->prefs->background, dockapp->prefs->icondir);

	if (weather->errorFlag) {
		RContext *context;

		WMSetLabelText(dockapp->text, "ERROR");

		context = WMScreenRContext(dockapp->screen);
		weather->icon = RLoadImage(
			context, wstrconcat(dockapp->prefs->icondir,
					    "/dialog-error.png"), 0);
		if (weather->icon) {
			RColor color;

			color = WMGetRColorFromColor(
				WMCreateNamedColor(dockapp->screen,
						   dockapp->prefs->background,
						   True));
			RCombineImageWithColor(weather->icon, &color);
			icon = WMCreatePixmapFromRImage(
				dockapp->screen, weather->icon, 0);
			WMSetLabelImage(dockapp->icon, icon);
		}

		WMSetBalloonTextForView(weather->errorText,
					WMWidgetView(dockapp->icon));

		/* try again in 1 minute */
		dockapp->minutesLeft = 1;
	} else {
		WMSetLabelText(dockapp->text, wstrconcat(weather->temp, "°"));

		icon = WMCreatePixmapFromRImage(dockapp->screen,
						weather->icon, 0);
		WMSetLabelImage(dockapp->icon, icon);

		WMSetBalloonTextForView(
			getBalloonText(weather, dockapp->prefs->days),
			WMWidgetView(dockapp->icon));
	}

	WMRedisplayWidget(dockapp->icon);
	WMRedisplayWidget(dockapp->text);

	freeWeather(weather);
}

static void updateDockapp(void *data)
{
	Dockapp *dockapp = (Dockapp *)data;
	WMColor *background;
	WMColor *text;
	WMScreen *screen = dockapp->screen;
	Preferences *prefs = dockapp->prefs;
	GWeatherLocation *world, *loc;
	GWeatherInfo *info;

	background = WMCreateNamedColor(screen, prefs->background, True);
	text = WMCreateNamedColor(screen, prefs->text, True);

	WMSetLabelText(dockapp->text, "loading");
	WMSetWidgetBackgroundColor(dockapp->text, background);
	WMSetLabelTextColor(dockapp->text, text);
	WMRedisplayWidget(dockapp->text);

	WMSetWidgetBackgroundColor(dockapp->frame, background);
	WMSetWidgetBackgroundColor(dockapp->icon, background);

	world = gweather_location_get_world();
	loc = gweather_location_find_nearest_city(
		world, prefs->latitude, prefs->longitude);
#ifdef HAVE_GWEATHER_3_27_4
	info = gweather_info_new(NULL);
#else
	info = gweather_info_new(NULL, GWEATHER_FORECAST_LIST);
#endif
#ifdef HAVE_GWEATHER_40
	gweather_info_set_application_id(info, APPLICATION_ID);
	gweather_info_set_contact_info(info, CONTACT_INFO);
#endif
	gweather_info_set_location(info, loc);
	gweather_info_set_enabled_providers(info, GWEATHER_PROVIDER_ALL);
	g_signal_connect(
		G_OBJECT(info), "updated", G_CALLBACK(getWeather), dockapp);
	gweather_info_update(info);
}

Bool check_icondir(char *icondir)
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

GWeatherTemperatureUnit string_to_unit(char *unit_string)
{
	if (strcmp(unit_string, "c") == 0)
		return GWEATHER_TEMP_UNIT_CENTIGRADE;

	else /* default to fahrenheit */
		return GWEATHER_TEMP_UNIT_FAHRENHEIT;
}

void readPreferences(Preferences *prefs)
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
			       "Copyright © 2014-2021 Doug Torrance\n"
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
			werror("%s\n", error->message);
			g_error_free(error);
		}
	}
}

static void findCoords(WMWidget *widget, void *data)
{
	Dockapp *d = (Dockapp *)data;
	(void)widget;
	WMSetButtonText(d->prefsWindow->find_coords, "Finding...");

	gclue_simple_new("wmforecast", GCLUE_ACCURACY_LEVEL_CITY, NULL,
			 foundCoords, d);
}
#endif

void restore_default_colors(WMWidget *widget, void *data)
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

static void editPreferences(void *data)
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
}

static void refresh(XEvent *event, void *data)
{
	Dockapp *d = (Dockapp *)data;
	if (WMIsDoubleClick(event) && event->xbutton.button == Button1) {
		d->minutesLeft = d->prefs->interval;
		updateDockapp(d);
	}
	if (event->xbutton.button == Button3 && !d->prefsWindowPresent)
		editPreferences(d);
}

static void timerHandler(void *data)
{
	Dockapp *d = (Dockapp *)data;

	d->minutesLeft--;
	if (d->minutesLeft == 0) {
		d->minutesLeft = d->prefs->interval;
		updateDockapp(data);
	}
}

void do_glib_loop(void *data)
{
	(void)data;
	g_main_context_iteration(NULL, 0);
}


int main(int argc, char **argv)
{
	Display *display;
	Dockapp *dockapp;
	Preferences *prefs;
	WMScreen *screen;

	WMInitializeApplication("wmforecast", &argc, argv);

	prefs = setPreferences(argc, argv);

	display = XOpenDisplay("");

	if (!display) {
		werror("could not connect to the X server");
		exit(EXIT_FAILURE);
	}

	screen = WMCreateScreen(display, DefaultScreen(display));
	dockapp = newDockapp(screen, prefs, argc, argv);

	WMCreateEventHandler(WMWidgetView(dockapp->icon), ButtonPressMask,
			     refresh, dockapp);

	updateDockapp(dockapp);
	WMAddPersistentTimerHandler(60*1000, /* one minute */
				    timerHandler, dockapp);

	WMAddPersistentTimerHandler(100, do_glib_loop, NULL);

	WMScreenMainLoop(screen);

	return 0;
}
