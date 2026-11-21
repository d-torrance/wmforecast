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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "wmforecast.h"

#define APPLICATION_ID "org.friedcheese.wmforecast"
#define CONTACT_INFO "dtorrance@piedmont.edu"

#define round(x) (int)(x + 0.5)

static char *getConditionsText(GWeatherInfo *info)
{
	char text[1024];

	snprintf(text, 1024, "\n"
		 "City:\t\t\t\t%s\n"
		 "Last update:\t\t%s\n"
		 "Conditions:\t\t%s\n"
		 "Sky:\t\t\t\t%s\n"
		 "Temperature:\t\t%s\n"
		 "Feels like:\t\t\t%s\n"
		 "Dew point:\t\t%s\n"
		 "Relative humidity:\t%s\n"
		 "Wind:\t\t\t%s\n"
		 "Pressure:\t\t\t%s\n"
		 "Visibility:\t\t\t%s\n"
		 "Sunrise:\t\t\t%s\n"
		 "Sunset:\t\t\t%s\n\n",
		 gweather_info_get_location_name(info),
		 gweather_info_get_update(info),
		 gweather_info_get_conditions(info),
		 gweather_info_get_sky(info),
		 gweather_info_get_temp(info),
		 gweather_info_get_apparent(info),
		 gweather_info_get_dew(info),
		 gweather_info_get_humidity(info),
		 gweather_info_get_wind(info),
		 gweather_info_get_pressure(info),
		 gweather_info_get_visibility(info),
		 gweather_info_get_sunrise(info),
		 gweather_info_get_sunset(info));

	return g_strdup(text);
}

static char *getTemp(GWeatherInfo *info, GWeatherTemperatureUnit unit)
{
	double temp;

	gweather_info_get_value_temp(info, unit, &temp);
	return g_strdup_printf("%d", round(temp));
}

/* strip html from attribution string */
static char *strip_tags(const char *to_strip)
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

/* append a line for each day's forecast to lines, up to the given number of
 * days, and return the total number of days forecasted */
static int getForecasts(GString *lines, GSList *gforecasts,
			GWeatherTemperatureUnit units, int days)
{
	GDateTime *d;
	int current_weekday, high, low, count;
	char *conditions;

	d = g_date_time_new_now_local();
	current_weekday = g_date_time_get_day_of_week(d);
	g_date_time_unref(d);
	high = INT_MIN;
	low = INT_MAX;
	conditions = NULL;
	count = 0;

	for (; gforecasts; gforecasts = gforecasts->next) {
		time_t time;
		double temp;
		int weekday;

		if (!gweather_info_get_value_update(gforecasts->data, &time))
			continue;

		d = g_date_time_new_from_unix_utc(time);
		if (!d)
			continue;

		weekday = g_date_time_get_day_of_week(d);

		/* follow gnome weather's convention of using 2 pm for
		 * conditions */
		if (g_date_time_get_hour(d) >= 14 && !conditions) {
			conditions = gweather_info_get_conditions(
				gforecasts->data);
			if (strcmp(conditions, "-") == 0) {
				g_free(conditions);
				conditions = gweather_info_get_sky(
					gforecasts->data);
			}
		}

		if (weekday != current_weekday) {
			/* don't create forecast if we don't have any info
			 * (e.g., for today if it's almost midnight) */
			if (high != INT_MIN && low != INT_MAX) {
				if (count < days) {
					GDateTime *day;
					char *day_name;

					/* subtract one since we've already
					 * advanced to the next day */
					day = g_date_time_add_days(d, -1);
					day_name = g_date_time_format(day, "%a");
					g_string_append_printf(
						lines,
						"%s - %s. High: %d° Low: %d°\n",
						day_name,
						conditions ? conditions : "",
						high, low);
					g_free(day_name);
					g_date_time_unref(day);
				}
				count++;
			}

			current_weekday = weekday;
			high = INT_MIN;
			low = INT_MAX;
			g_free(conditions);
			conditions = NULL;
		}
		/* skip forecasts with unknown temperatures, which
		 * libgweather reports as 0 */
		if (gweather_info_get_value_temp(gforecasts->data,
						 units, &temp)) {
			if (temp > high)
				high = round(temp);
			if (temp < low)
				low = round(temp);
		}
		g_date_time_unref(d);
	}

	g_free(conditions);
	return count;
}

static char *getForecastText(GWeatherInfo *info, GWeatherInfo *current,
			     Preferences *prefs)
{
	GString *text, *forecasts;
	char retrieved[20], *summary, *temp;
	time_t now;

	now = time(NULL);
	strftime(retrieved, sizeof retrieved, "%l:%M %p %Z", localtime(&now));
	summary = gweather_info_get_weather_summary(current);
	temp = getTemp(current, prefs->units);

	text = g_string_new(NULL);
	g_string_append_printf(text,
			       "\n" PACKAGE_STRING "\n\n"
			       "Retrieved: %s\n\n"
			       "Current Conditions:\n"
			       "%s, %s°\n\n"
			       "Forecast",
			       retrieved, summary, temp);
	g_free(summary);
	g_free(temp);

	forecasts = g_string_new(NULL);
	if (getForecasts(forecasts, gweather_info_get_forecast_list(info),
			 prefs->units, prefs->days) == 0)
		g_string_append(text, " not available.");
	else
		g_string_append_printf(text, ":\n%s", forecasts->str);
	g_string_free(forecasts, TRUE);

	g_string_append_printf(text, "\n%s\n\n",
			       strip_tags(gweather_info_get_attribution(info)));

	return g_string_free(text, FALSE);
}

/* return the current conditions, or the first forecast if they're not
 * available */
static GWeatherInfo *getCurrentConditions(GWeatherInfo *info,
					  GWeatherTemperatureUnit units)
{
	GSList *gforecasts;
	double temp;

	if (gweather_info_get_value_temp(info, units, &temp))
		return info;

	for (gforecasts = gweather_info_get_forecast_list(info); gforecasts;
	     gforecasts = gforecasts->next)
		if (gweather_info_get_value_temp(gforecasts->data, units, &temp))
			return gforecasts->data;

	return NULL;
}

/* load an icon from the icon directory and blend it with the background
 * color; if it's missing and error is non-NULL, set it to an error message */
static WMPixmap *loadIcon(Dockapp *dockapp, const char *name, char **error)
{
	WMColor *background;
	WMPixmap *icon;
	RColor color;
	RImage *image;
	char *filename;

	filename = g_strdup_printf("%s/%s.png", dockapp->prefs->icondir, name);
	image = RLoadImage(WMScreenRContext(dockapp->screen), filename, 0);
	if (!image) {
		if (error)
			*error = g_strconcat(filename, " not found", NULL);
		g_free(filename);
		return NULL;
	}
	g_free(filename);

	background = WMCreateNamedColor(dockapp->screen,
					dockapp->prefs->background, True);
	color = WMGetRColorFromColor(background);
	WMReleaseColor(background);
	RCombineImageWithColor(image, &color);

	icon = WMCreatePixmapFromRImage(dockapp->screen, image, 0);
	RReleaseImage(image);
	return icon;
}

static void getWeather(GWeatherInfo *info, Dockapp *dockapp)
{
	GWeatherInfo *current;
	WMPixmap *icon;
	char *error, *balloon;

	error = NULL;
	icon = NULL;

	if (!gweather_info_is_valid(info)) {
		error = gweather_info_get_weather_summary(info);
		if (!error)
			error = g_strdup("An error occurred");
	}

	current = getCurrentConditions(info, dockapp->prefs->units);
	if (!current) {
		g_free(error);
		error = g_strdup("Retrieval failed");
	} else if (!error)
		icon = loadIcon(dockapp, gweather_info_get_icon_name(current),
				&error);

	if (error) {
		WMSetLabelText(dockapp->text, "ERROR");
		icon = loadIcon(dockapp, "dialog-error", NULL);
		balloon = error;

		/* try again in 1 minute */
		dockapp->minutesLeft = 1;
	} else {
		char *temp, *label;

		temp = getTemp(current, dockapp->prefs->units);
		label = g_strconcat(temp, "°", NULL);
		WMSetLabelText(dockapp->text, label);
		g_free(label);
		g_free(temp);

		if (dockapp->showForecast)
			balloon = getForecastText(info, current, dockapp->prefs);
		else
			balloon = getConditionsText(current);
	}

	if (icon) {
		WMSetLabelImage(dockapp->icon, icon);
		WMReleasePixmap(icon);
	}
	WMSetBalloonTextForView(balloon, WMWidgetView(dockapp->icon));
	g_free(balloon);

	WMRedisplayWidget(dockapp->icon);
	WMRedisplayWidget(dockapp->text);
}

void updateDockapp(void *data)
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
#if HAVE_GWEATHER_VERSION >= 3027004
	info = gweather_info_new(NULL);
#else
	info = gweather_info_new(NULL, GWEATHER_FORECAST_LIST);
#endif
#if HAVE_GWEATHER_VERSION >= 3040000
	gweather_info_set_application_id(info, APPLICATION_ID);
	gweather_info_set_contact_info(info, CONTACT_INFO);
#endif
	gweather_info_set_location(info, loc);
	gweather_info_set_enabled_providers(info, GWEATHER_PROVIDER_ALL);
	g_signal_connect(
		G_OBJECT(info), "updated", G_CALLBACK(getWeather), dockapp);
	gweather_info_update(info);
}
