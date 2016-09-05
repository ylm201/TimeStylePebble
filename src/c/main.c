#include <pebble.h>
#include "clock_digit.h"
#include "messaging.h"
#include "settings.h"
#include "weather.h"
#include "sidebar.h"
#include "util.h"

// windows and layers
static Window* mainWindow;
static Layer* windowLayer;

// current bluetooth state
static bool isPhoneConnected;

// current time service subscription
static bool updatingEverySecond;

// the four digits on the clock, ordered h1 h2, m1 m2
static ClockDigit clockDigits[4];

// try to randomize when watches call the weather API
static uint8_t weatherRefreshMinute;

void update_clock();
void redrawScreen();
void tick_handler(struct tm *tick_time, TimeUnits units_changed);
void bluetoothStateChanged(bool newConnectionState);


void update_clock() {
  time_t rawTime;
  struct tm* timeInfo;

  time(&rawTime);
  timeInfo = localtime(&rawTime);

  // DEBUG: use fake time for screenshots
  // timeInfo->tm_hour = 6;
  // timeInfo->tm_min = 23;

  // debug: set fake condition for screenshots
  // Weather_setConditions(44, false, 44);
  // Weather_weatherInfo.currentTemp = 21;
  // Weather_weatherForecast.highTemp = 22;
  // Weather_weatherForecast.lowTemp = 16;

  int hour = timeInfo->tm_hour;

  if (!clock_is_24h_style()) {
    if(hour > 12) {
      hour %= 12;
    } else if(timeInfo->tm_hour == 0) {
      hour = 12;
    }
  }

  uint8_t current_font = globalSettings.clockFontId;

  if(globalSettings.clockFontId == FONT_SETTING_BOLD_H) {
    current_font = FONT_SETTING_BOLD;
  } else if(globalSettings.clockFontId == FONT_SETTING_BOLD_M) {
    current_font = FONT_SETTING_DEFAULT;
  }

  // use the blank image for the leading hour digit if needed
  if(globalSettings.showLeadingZero || hour / 10 != 0) {
    ClockDigit_setNumber(&clockDigits[0], hour / 10, current_font);
  } else {
    ClockDigit_setBlank(&clockDigits[0]);
  }

  ClockDigit_setNumber(&clockDigits[1], hour % 10, current_font);

  if(globalSettings.clockFontId == FONT_SETTING_BOLD_H) {
    current_font = FONT_SETTING_DEFAULT;
  } else if(globalSettings.clockFontId == FONT_SETTING_BOLD_M) {
    current_font = FONT_SETTING_BOLD;
  }

  ClockDigit_setNumber(&clockDigits[2], timeInfo->tm_min  / 10, current_font);
  ClockDigit_setNumber(&clockDigits[3], timeInfo->tm_min  % 10, current_font);

  Sidebar_updateTime(timeInfo);
}

/* forces everything on screen to be redrawn -- perfect for keeping track of settings! */
void redrawScreen() {

  // check if the tick handler frequency should be changed
  if(globalSettings.updateScreenEverySecond != updatingEverySecond) {
    tick_timer_service_unsubscribe();

    if(globalSettings.updateScreenEverySecond) {
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
      updatingEverySecond = true;
    } else {
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
      updatingEverySecond = false;
    }
  }

  // maybe the colors changed!
  for(int i = 0; i < 4; i++) {
    ClockDigit_setColor(&clockDigits[i], globalSettings.timeColor, globalSettings.timeBgColor);
    // ClockDigit_setColor(&clockDigits[i], globalSettings.timeColor, GColorWhite);
  }

  window_set_background_color(mainWindow, globalSettings.timeBgColor);

  // or maybe the sidebar position changed!
  int digitOffset = (globalSettings.sidebarOnLeft) ? 30 : 0;

  for(int i = 0; i < 4; i++) {
    ClockDigit_offsetPosition(&clockDigits[i], digitOffset);
  }

  // maybe the language changed!
  update_clock();

  // update the sidebar
  Sidebar_redraw();
}

static void main_window_load(Window *window) {

  #ifdef PBL_ROUND
    GPoint digitPoints[4] = {GPoint(40, 17), GPoint(90, 17), GPoint(40, 92), GPoint(90, 92)};
  #else
    GPoint digitPoints[4] = {GPoint(7, 7), GPoint(60, 7), GPoint(7, 90), GPoint(60, 90)};
  #endif

  ClockDigit_construct(&clockDigits[0], digitPoints[0]);
  ClockDigit_construct(&clockDigits[1], digitPoints[1]);
  ClockDigit_construct(&clockDigits[2], digitPoints[2]);
  ClockDigit_construct(&clockDigits[3], digitPoints[3]);

  for(int i = 0; i < 4; i++) {
    ClockDigit_setColor(&clockDigits[i], globalSettings.timeColor, globalSettings.timeBgColor);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(clockDigits[i].imageLayer));
  }

  window_set_background_color(window, globalSettings.timeBgColor);

  // create the sidebar
  Sidebar_init(window);

  // Make sure the time is displayed from the start
  redrawScreen();
  update_clock();
}

static void main_window_unload(Window *window) {
  for(int i = 0; i < 4; i++) {
    ClockDigit_destruct(&clockDigits[i]);
  }

  Sidebar_deinit();
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  // every 30 minutes, request new weather data
  if(!globalSettings.disableWeather) {
    if(tick_time->tm_min == weatherRefreshMinute && tick_time->tm_sec == 0) {
      messaging_requestNewWeatherData();
    }
  }

  // every hour, if requested, vibrate
  if(tick_time->tm_sec == 0) {
    if(globalSettings.hourlyVibe == 1) { // hourly vibes only
      if(tick_time->tm_min % 60 == 0) {
        vibes_double_pulse();
      }
    } else if(globalSettings.hourlyVibe == 2) {  // hourly and half-hourly
      if(tick_time->tm_min % 60 == 0) {
        vibes_double_pulse();
      } else if(tick_time->tm_min % 30 == 0) {
        vibes_short_pulse();
      }
    }
  }

  update_clock();
}

void bluetoothStateChanged(bool newConnectionState) {
  // if the phone was connected but isn't anymore and the user has opted in,
  // trigger a vibration
  if(isPhoneConnected && !newConnectionState && globalSettings.btVibe) {
    static uint32_t const segments[] = { 200, 100, 100, 100, 500 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
      };
    vibes_enqueue_custom_pattern(pat);
  }

  // if the phone was disconnected and isn't anymore, update the data
  if(!isPhoneConnected && newConnectionState) {
    messaging_requestNewWeatherData();
  }

  isPhoneConnected = newConnectionState;

  Sidebar_redraw();
}

// force the sidebar to redraw any time the battery state changes
void batteryStateChanged(BatteryChargeState charge_state) {
  Sidebar_redraw();
}

// fixes for disappearing elements after notifications
// (from http://codecorner.galanter.net/2016/01/08/solved-issue-with-pebble-framebuffer-after-notification-is-dismissed/)
static void app_focus_changing(bool focusing) {
  if (focusing) {
     layer_set_hidden(windowLayer, true);
  }
}

static void app_focus_changed(bool focused) {
  if (focused) {
     layer_set_hidden(windowLayer, false);
     layer_mark_dirty(windowLayer);
  }
}

static void init() {
  setlocale(LC_ALL, "");

  srand(time(NULL));

  weatherRefreshMinute = rand() % 60;

  // init settings
  Settings_init();

  // init weather system
  Weather_init();

  // init the messaging thing
  messaging_init(redrawScreen);

  // Create main Window element and assign to pointer
  mainWindow = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(mainWindow, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(mainWindow, true);

  windowLayer = window_get_root_layer(mainWindow);

  // Register with TickTimerService
  if(globalSettings.updateScreenEverySecond) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    updatingEverySecond = true;
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    updatingEverySecond = false;
  }

  bool connected = bluetooth_connection_service_peek();
  bluetoothStateChanged(connected);
  bluetooth_connection_service_subscribe(bluetoothStateChanged);

  // register with battery service
  battery_state_service_subscribe(batteryStateChanged);

  // set up focus change handlers
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_changed,
    .will_focus = app_focus_changing
  });
}

static void deinit() {
  // Destroy Window
  window_destroy(mainWindow);

  // unload weather stuff
  Weather_deinit();
  Settings_deinit();

  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
