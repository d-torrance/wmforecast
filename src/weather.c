/* Copyright (C) 2014-2026 Doug Torrance <dtorrance9@gatech.edu>
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

#include <string.h>
#include <time.h>

#include "wmforecast.h"

#define APPLICATION_ID "org.friedcheese.wmforecast"
#define CONTACT_INFO "dtorrance9@gatech.edu"

#define round(x) (int)(x + 0.5)

/* append a line to the conditions text, freeing the value from libgweather */
static void appendCondition(GString *text, const char *label, char *value)
{
	g_string_append_printf(text, "%s%s\n", label, value);
	g_free(value);
}

static char *getConditionsText(GWeatherInfo *info)
{
	GString *text;

	text = g_string_new("\n");
	appendCondition(text, "City:\t\t\t\t",
			gweather_info_get_location_name(info));
	appendCondition(text, "Last update:\t\t",
			gweather_info_get_update(info));
	appendCondition(text, "Conditions:\t\t",
			gweather_info_get_conditions(info));
	appendCondition(text, "Sky:\t\t\t\t", gweather_info_get_sky(info));
	appendCondition(text, "Temperature:\t\t",
			gweather_info_get_temp(info));
	appendCondition(text, "Feels like:\t\t\t",
			gweather_info_get_apparent(info));
	appendCondition(text, "Dew point:\t\t", gweather_info_get_dew(info));
	appendCondition(text, "Relative humidity:\t",
			gweather_info_get_humidity(info));
	appendCondition(text, "Wind:\t\t\t", gweather_info_get_wind(info));
	appendCondition(text, "Pressure:\t\t\t",
			gweather_info_get_pressure(info));
	appendCondition(text, "Visibility:\t\t\t",
			gweather_info_get_visibility(info));
	appendCondition(text, "Sunrise:\t\t\t",
			gweather_info_get_sunrise(info));
	appendCondition(text, "Sunset:\t\t\t",
			gweather_info_get_sunset(info));
	g_string_append(text, "\n");

	return g_string_free(text, FALSE);
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
	char *stripped, *end;
	gboolean in_tag;

	if (!to_strip)
		return g_strdup("");

	stripped = end = g_strdup(to_strip);
	in_tag = FALSE;
	for (; *to_strip; to_strip++) {
		if (*to_strip == '<')
			in_tag = TRUE;
		else if (*to_strip == '>')
			in_tag = FALSE;
		else if (!in_tag)
			*end++ = *to_strip;
	}
	*end = '\0';

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
	char retrieved[20], *summary, *temp, *attribution;
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

	attribution = strip_tags(gweather_info_get_attribution(info));
	g_string_append_printf(text, "\n%s\n\n", attribution);
	g_free(attribution);

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

static gboolean unrefInfo(gpointer info)
{
	g_object_unref(info);
	return G_SOURCE_REMOVE;
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

	/* we're done with this update, but libgweather may still be using
	 * info while emitting this signal, so free it once we're idle */
	g_signal_handlers_disconnect_by_func(info, G_CALLBACK(getWeather),
					     dockapp);
	g_idle_add(unrefInfo, info);
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
	WMReleaseColor(background);
	WMReleaseColor(text);

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
#if HAVE_GWEATHER_VERSION >= 4000000
	g_object_unref(loc);
	g_object_unref(world);
#else
	gweather_location_unref(loc);
#endif
	gweather_info_set_enabled_providers(info, GWEATHER_PROVIDER_ALL);
	g_signal_connect(
		G_OBJECT(info), "updated", G_CALLBACK(getWeather), dockapp);
	gweather_info_update(info);
}
