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

#include <stdlib.h>

#include "wmforecast.h"

static void do_glib_loop(void *data)
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
