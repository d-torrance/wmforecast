/***********************************************************************
    Copyright (C) 2014 Doug Torrance

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <WINGs/WINGs.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <getopt.h>
#include <pthread.h>

#define color(c) WMCreateNamedColor(screen,c,True)

typedef struct {
	WMLabel *icon;
	WMLabel *text;
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
	char *title;
	ForecastArray *forecasts;
	RImage *icon;
	int errorFlag;
	char *errorText;
} Weather;

typedef struct {
	char *units;
	char *woeid;
	char *zip;
	char *woeid_or_zip;
	long int interval;
} Preferences;

typedef struct {
	WMScreen *screen;
	Dockapp *dockapp;
	Preferences *prefs;
} ThreadData;

Forecast *newForecast()
{
	Forecast *forecast = malloc(sizeof(Forecast));
	forecast->day = NULL;
	forecast->low = NULL;
	forecast->high = NULL;
	forecast->text = NULL;
	return forecast;
}

ForecastArray *newForecastArray()
{
	ForecastArray *array = malloc(sizeof(ForecastArray));
	array->length = 0;
	array->forecasts = NULL;
	return array;
}

void appendForecast(ForecastArray *array, Forecast *forecast)
{
	array->length++;
	array->forecasts = (Forecast *)realloc(array->forecasts, sizeof(Forecast)*(array->length));
	array->forecasts[(array->length)-1] = *forecast;
}

Weather *newWeather()
{
	Weather *weather = malloc(sizeof(Weather));
	weather->temp = NULL;
	weather->text = NULL;
	weather->title = NULL;
	weather->icon = NULL;
	weather->forecasts = newForecastArray();
	weather->errorFlag = 0;
	weather->errorText = NULL;
	return weather;
}

void freeForecast(Forecast *forecast)
{
	if (forecast->day)
		free(forecast->day);
	if (forecast->low)
		free(forecast->low);
	if (forecast->high)
		free(forecast->high);
	if (forecast->text)
		free(forecast->text);

}

void freeForecastArray(ForecastArray *array)
{
	int i;
	for (i = 0; i < array->length; i++)
		if (&array->forecasts[i])
			freeForecast(&array->forecasts[i]);
	if (array)
		free(array);
}

void freeWeather(Weather *weather)
{
	if (weather->temp)
		free(weather->temp);
	if (weather->text)
		free(weather->text);
	if (weather->title)
		free(weather->title);
	if (weather->forecasts)
		freeForecastArray(weather->forecasts);
	if (weather->errorText)
		free(weather->errorText);
	free(weather);
}

void setTitle(Weather *weather, const char *title)
{
	weather->title = realloc(weather->title, strlen(title) + 1);
	strcpy(weather->title, title);
}

void setConditions(Weather *weather,
		   WMScreen *screen,
		   const char *temp,
		   const char *text,
		   const char *code
	)
{
	RContext *context;
	char *filename;

	weather->temp = realloc(weather->temp, strlen(temp) + 1);
	strcpy(weather->temp, temp);
	weather->text = realloc(weather->text, strlen(text) + 1);
	strcpy(weather->text, text);

	context = WMScreenRContext(screen);
	filename = wstrconcat(wstrconcat(DATADIR"/",code),".png");
	weather->icon = RLoadImage(context,filename,0);
}

void setForecast(Forecast *forecast,
		 const char *day,
		 const char *low,
		 const char *high,
		 const char *text
	)
{
	forecast->day = realloc(forecast->day, strlen(day) + 1);
	strcpy(forecast->day, day);
	forecast->low = realloc(forecast->low, strlen(low) + 1);
	strcpy(forecast->low, low);
	forecast->high = realloc(forecast->high, strlen(high) + 1);
	strcpy(forecast->high, high);
	forecast->text = realloc(forecast->text, strlen(text) + 1);
	strcpy(forecast->text, text);
}

WMWindow *WMCreateDockapp(WMScreen *screen, const char *name, int argc, char **argv)
{
	WMWindow *dockapp;
	XWMHints *hints;
	Display *display;
	Window window;
	
	display = WMScreenDisplay(screen);
	dockapp = WMCreateWindow(screen,name);
	WMRealizeWidget(dockapp);
	
	window = WMWidgetXID(dockapp);

	hints = XGetWMHints(display, window);
	hints->flags |= WindowGroupHint|IconWindowHint|StateHint;
	hints->window_group = window;
    	hints->icon_window = window;
    	hints->initial_state = WithdrawnState;

	XSetWMHints(display, window, hints);
	XFree(hints);

	XSetCommand(display, window, argv, argc);

	WMResizeWidget(dockapp,56,56);
	return dockapp;
}

Dockapp *newDockapp(WMScreen *screen, int argc, char **argv)
{
	Dockapp *dockapp = malloc(sizeof(Dockapp));
	WMFrame *frame;
	WMWindow *window;

	window = WMCreateDockapp(screen, "", argc, argv);

	frame = WMCreateFrame(window);
	WMSetFrameRelief(frame,WRSunken);
	WMResizeWidget(frame,56,56);
	WMSetWidgetBackgroundColor(frame,color("black"));
	WMRealizeWidget(frame);

	dockapp->text = WMCreateLabel(frame);
	WMSetWidgetBackgroundColor(dockapp->text, color("black"));
	WMSetLabelFont(dockapp->text, WMCreateFont(screen, "-Misc-Fixed-Medium-R-SemiCondensed--13-120-75-75-C-60-ISO10646-1"));
	WMSetLabelTextColor(dockapp->text, color("Light sea green"));
	WMSetLabelTextAlignment (dockapp->text, WACenter);
	WMResizeWidget(dockapp->text,52,14);
	WMMoveWidget(dockapp->text,2,40);
	WMRealizeWidget(dockapp->text);

	dockapp->icon = WMCreateLabel(frame);
	WMSetWidgetBackgroundColor(dockapp->icon,color("black"));
	WMRealizeWidget(dockapp->icon);

	WMSetLabelImagePosition(dockapp->icon,WIPImageOnly);
	WMResizeWidget(dockapp->icon,32,32);
	WMMoveWidget(dockapp->icon,12,5);

	WMMapWidget(window);
	WMMapWidget(frame);
	WMMapSubwidgets(frame);

	return dockapp;
}

char *getBalloonText(Weather *weather) 
{
	char *text;
	int i;

	text = wstrconcat("\n",weather->title);
	text = wstrappend(text,"\n\nCurrent Conditions:\n");
	text = wstrappend(text, weather->text);
	text = wstrappend(text, ", ");
	text = wstrappend(text, weather->temp);
	text = wstrappend(text, "°\n\nForecast:\n");
	for (i = 0; i < weather->forecasts->length; i++) {
		text = wstrappend(text,weather->forecasts->forecasts[i].day);
		text = wstrappend(text," - ");
		text = wstrappend(text,weather->forecasts->forecasts[i].text);
		text = wstrappend(text,". High: ");
		text = wstrappend(text,weather->forecasts->forecasts[i].high);
		text = wstrappend(text,"° Low: ");
		text = wstrappend(text,weather->forecasts->forecasts[i].low);
		text = wstrappend(text,"°\n");
	}
// as per Yahoo's Attribution Guidelines
// (https://developer.yahoo.com/attribution/)
	text = wstrappend(text,"\nPowered by Yahoo!\n\n");
	return text;
}

void setError(Weather *weather, WMScreen *screen, const char *errorText)
{
	weather->errorFlag = 1;
	weather->errorText = realloc(weather->errorText, strlen(errorText) + 1);
	strcpy(weather->errorText, errorText);

	
	

}
	



/**************************************************
from http://curl.haxx.se/libcurl/c/getinmemory.html
***************************************************/
struct MemoryStruct {
	char *memory;
	size_t size;
};
 
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
 
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
 
	return realsize;
}

Weather *getWeather(WMScreen *screen, Preferences *prefs)
{
	char *url;
	CURL *curl_handle;
  	CURLcode res;
 	struct MemoryStruct chunk;
	Weather *weather;
	xmlDocPtr doc;
	xmlNodePtr cur;

	url = wstrconcat("http://weather.yahooapis.com/forecastrss?u=",prefs->units);
	if (strcmp(prefs->woeid_or_zip,"w") == 0) {
		url = wstrappend(url,"&w=");
		url = wstrappend(url, prefs->woeid);
	}
	else {
		url = wstrappend(url,"&p=");
		url = wstrappend(url, prefs->zip);
	}
		
	weather = newWeather();
	chunk.memory = malloc(1);
	chunk.size = 0;
 
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK) {
		setError(weather, screen, curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		curl_global_cleanup();
		if(chunk.memory)
			free(chunk.memory);
		return weather;
	}
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	doc = xmlParseMemory(chunk.memory, chunk.size);
	if (doc == NULL) {
		setError(weather, screen, "Document not parsed successfully");
		xmlFreeDoc(doc);
		return;
	}
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		setError(weather, screen,"Empty document");
		xmlFreeDoc(doc);
		return;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "rss")) {
		setError(weather, screen,"Empty document");
		fprintf(stderr,"document of the wrong type, root node != rss");
		xmlFreeDoc(doc);
		return;
	}
		
	cur = cur->children;
	cur = cur->next;
	cur = cur->children;

	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"item"))) {
			cur = cur->children;
			while (cur != NULL) {
				if ((!xmlStrcmp(cur->name, (const xmlChar *)"title"))) {
					if ((!xmlStrcmp(xmlNodeListGetString(doc, cur->children, 1), (const xmlChar *)"City not found"))) {
						printf("Location not found\n");
						exit(0);
					}
					setTitle(
						weather,
						xmlNodeListGetString(doc, cur->children, 1)
						);
					
				}
				if ((!xmlStrcmp(cur->name, (const xmlChar *)"condition"))) {
					setConditions(
						weather,
						screen,
						xmlGetProp(cur,"temp"),
						xmlGetProp(cur,"text"),
						xmlGetProp(cur,"code")
						);
				}
				if ((!xmlStrcmp(cur->name, (const xmlChar *)"forecast"))) {
					Forecast *forecast = newForecast();
					setForecast(
						forecast,
						xmlGetProp(cur,"day"),
						xmlGetProp(cur,"low"),
						xmlGetProp(cur,"high"),
						xmlGetProp(cur,"text")
						);
					appendForecast(weather->forecasts,forecast);
				}

				
				cur = cur->next;
			}
		}
		else cur = cur->next;
	}


	xmlFreeDoc(doc);
// finishing parsing xml

	if(chunk.memory)
		free(chunk.memory);

	return weather;
}

void updateDockapp(WMScreen *screen, Dockapp *dockapp, Preferences *prefs)
{
	Weather *weather;
	WMPixmap *icon;

	weather = getWeather(screen, prefs);

	if (weather->errorFlag == 1) {
		RContext *context;

		WMSetLabelText(dockapp->text,"ERROR");

		context = WMScreenRContext(screen);
		weather->icon = RLoadImage(context,DATADIR"/na.png",0);

		icon = WMCreatePixmapFromRImage(screen,weather->icon,0);
		WMSetLabelImage(dockapp->icon,icon);

		WMSetBalloonTextForView(weather->errorText, WMWidgetView(dockapp->icon)); 

	}
	else {
		WMSetLabelText(dockapp->text,wstrconcat(weather->temp,"°"));
	
		icon = WMCreatePixmapFromRImage(screen,weather->icon,0);
		WMSetLabelImage(dockapp->icon,icon);
	
		WMSetBalloonTextForView(getBalloonText(weather), WMWidgetView(dockapp->icon)); 
	}

	WMRedisplayWidget(dockapp->icon);
	WMRedisplayWidget(dockapp->text);

	freeWeather(weather);
}

Preferences *setPreferences(int argc, char **argv)
{
	int c;
	Preferences *prefs = malloc(sizeof(Preferences));
	
	//set defaults
	prefs->units = "f";
	prefs->woeid = "2502265";
	prefs->zip = NULL;
	prefs->woeid_or_zip = NULL;
	prefs->interval = 60;

	while (1)
	{
		static struct option long_options[] =
			{
				{"version", no_argument, 0, 'v'},
				{"help", no_argument, 0, 'h'},
				{"woeid", required_argument, 0, 'w'},
				{"units", required_argument, 0, 'u'},
				{"zip", required_argument, 0, 'z'},
				{"interval", required_argument, 0, 'i'},
				{0, 0, 0, 0}
			};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "vhw:u:z:i:",
				 long_options, &option_index);
     
		if (c == -1)
			break;
     
		switch (c)
		{
		case 0:
			if (long_options[option_index].flag != 0)
				break;
			printf ("option %s", long_options[option_index].name);
			if (optarg)
				printf (" with arg %s", optarg);
			printf ("\n");
			break;
     
		case 'v':
			printf("wmforecast %s\n\n"
			       "Copyright (C) 2014 Doug Torrance\n"
			       "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
			       "This is free software: you are free to change and redistribute it.\n"
			       "There is NO WARRANTY, to the extent permitted by law.\n\n"		       
			       "Written by Doug Torrance\n"
			       , VERSION);
			exit(0);
     
		case 'h':
			printf("a weather dockapp for Window Maker using the Yahoo Weather API\n\n"
			       "Usage: wmforecast [OPTIONS]\n\n"
			       "Options:\n"
			       "    -v, --version       print the version number\n"
			       "    -h, --help          print this help screen\n" 
			       "    -u, --units <c|f>   whether to use Celsius or Fahrenheit (default is f)\n"
			       "    -w, --woeid <woeid> Where on Earth ID (default is 2502265 for\n"
			       "                        Sunnyvale, CA -- to find your WOEID, search\n"
			       "                        for your city at http://weather.yahoo.com and\n"
			       "                        look in the URL.)\n"
			       "    -z, --zip <zip>     ZIP code or Location ID (Yahoo has deprecated this\n"
			       "                        option and it is not guaranteed to work)\n"
			       "(note that only one of -w or -z may be used, not both)\n\n"
			       "Report bugs to %s\n",
			       PACKAGE_BUGREPORT
				);
			
			exit(0);
     
		case 'w':
			prefs->woeid_or_zip = "w";
			prefs->woeid = optarg;
			break;
     
		case 'u':
			if ((strcmp(optarg,"f") != 0)&&(strcmp(optarg,"c") != 0)) {
				printf("units must be 'f' or 'c'\n");
				exit(0);
			}
			prefs->units = optarg;
			break;
     
		case 'z':
			if (prefs->woeid_or_zip) {
				printf("only one of -w or -z may be used, not both\n");
				exit(0);
			}
			prefs->woeid_or_zip = "z";
			prefs->zip = optarg;
			break;

		case 'i':
			prefs->interval = strtol(optarg,NULL,10);
			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;
     
		default:
			abort ();
		}
	}
	
	if (!prefs->woeid_or_zip) {
		prefs->woeid_or_zip = "w";
	}

	return prefs;
}

ThreadData *newThreadData(WMScreen *screen, Dockapp *dockapp, Preferences *prefs)
{
	ThreadData *data = malloc(sizeof(ThreadData));
	data->screen = screen;
	data->dockapp = dockapp;
	data->prefs = prefs;
	return data;
}
	
void *timerLoop(void *args)
{
	ThreadData *data = args;
	for (;;) {
		updateDockapp(data->screen, data->dockapp, data->prefs);
		sleep(60*data->prefs->interval);
	}
}

int main(int argc, char **argv)
{
	Display *display;
	Dockapp *dockapp;
	Preferences *prefs;
	pthread_t thread;
	ThreadData *data;
	WMScreen *screen;

	prefs = setPreferences(argc, argv);

	XInitThreads();

	WMInitializeApplication("wmforecast", &argc, argv);
	display = XOpenDisplay("");

	screen = WMCreateScreen(display, DefaultScreen(display));
	dockapp = newDockapp(screen, argc, argv);
	data = newThreadData(screen, dockapp, prefs);

	pthread_create(&thread, NULL, timerLoop, data);
	WMScreenMainLoop(screen);
}





