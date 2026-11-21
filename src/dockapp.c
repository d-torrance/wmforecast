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
#include <X11/Xatom.h>

#include "wmforecast.h"

static void close_window(WMWidget *self, void *data)
{
	(void)data;
	WMDestroyWidget(self);
	exit(0);
}

/* Create and map the application window and return the window that the
 * dockapp's contents should be placed in.  In windowed mode, this is the
 * application window itself.  Otherwise, it is a separate icon window,
 * which the window manager will reparent into the dock.  (The application
 * window must not be its own icon window: Window Maker skips icon windows
 * when it adopts existing windows on restart, which would leave the old
 * dockapp stranded on the root window.) */
static WMWindow *WMCreateDockapp(WMScreen *screen, const char *name,
				 const char *title, int argc, char **argv,
				 Bool windowed)
{
	WMWindow *dockapp, *iconwin;
	XWMHints *hints;
	Display *display;
	Window window;

	display = WMScreenDisplay(screen);
	dockapp = WMCreateWindow(screen, name);
	WMSetWindowTitle(dockapp, title);
	WMRealizeWidget(dockapp);

	window = WMWidgetXID(dockapp);

	hints = XGetWMHints(display, window);
	hints->flags |= WindowGroupHint;
	hints->window_group = window;

	if (windowed)
		iconwin = dockapp;
	else {
		iconwin = WMCreateWindow(screen, name);
		WMResizeWidget(iconwin, 56, 56);
		WMRealizeWidget(iconwin);
		/* the icon window shouldn't look like a client window */
		XDeleteProperty(display, WMWidgetXID(iconwin), XA_WM_HINTS);

		hints->flags |= IconWindowHint | StateHint;
		hints->icon_window = WMWidgetXID(iconwin);
		hints->initial_state = WithdrawnState;
	}

	XSetWMHints(display, window, hints);
	XFree(hints);

	WMSetWindowCloseAction(dockapp, close_window, NULL);

	XSetCommand(display, window, argv, argc);

	WMResizeWidget(dockapp, 56, 56);
	WMMapWidget(dockapp);
	return iconwin;
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
	dockapp->showForecast = 1;

	window = WMCreateDockapp(screen, "", "wmforecast", argc, argv,
				 prefs->windowed);

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

	WMMapWidget(dockapp->frame);
	WMMapSubwidgets(dockapp->frame);

	return dockapp;
}

void refresh(XEvent *event, void *data)
{
	Dockapp *d = (Dockapp *)data;

	switch (event->xbutton.button) {
	case Button1:
		if (WMIsDoubleClick(event)) {
			d->minutesLeft = d->prefs->interval;
			updateDockapp(d);
		}
		break;

	case Button2:
		d->showForecast = 1 - d->showForecast;
		updateDockapp(d);
		break;

	case Button3:
		if (!d->prefsWindowPresent)
			editPreferences(d);
		break;

	default:
		break;
	}
}

void timerHandler(void *data)
{
	Dockapp *d = (Dockapp *)data;

	d->minutesLeft--;
	if (d->minutesLeft == 0) {
		d->minutesLeft = d->prefs->interval;
		updateDockapp(data);
	}
}
