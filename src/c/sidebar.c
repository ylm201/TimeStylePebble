#include <pebble.h>
#include <ctype.h>
#include <math.h>
#include "settings.h"
#include "weather.h"
#include "languages.h"
#include "sidebar.h"
#include "sidebar_widgets.h"

#define V_PADDING 8
#define SCREEN_HEIGHT 168

// "private" functions
// layer update callbacks
void updateRectSidebar(Layer *l, GContext* ctx);

#ifdef PBL_ROUND
  void updateRoundSidebarLeft(Layer *l, GContext* ctx);
  void updateRoundSidebarRight(Layer *l, GContext* ctx);

  // shared drawing stuff between all layers
  void drawRoundSidebar(GContext* ctx, GRect bgBounds, SidebarWidgetType widgetType, int widgetXOffset);
#endif

bool isSecondaryDisplay=false;
Layer* sidebarLayer;

#ifdef PBL_ROUND
  Layer* sidebarLayer2;
#endif

void Sidebar_init(Window* window) {
  // init the sidebar layer
  GRect screenRect = layer_get_bounds(window_get_root_layer(window));
  GRect bounds;

  #ifdef PBL_ROUND
    GRect bounds2;
    bounds = GRect(0, 0, 40, screenRect.size.h);
    bounds2 = GRect(screenRect.size.w - 40, 0, 40, screenRect.size.h);
  #else
    if(!globalSettings.sidebarOnLeft) {
      bounds = GRect(114, 0, 30, screenRect.size.h);
    } else {
      bounds = GRect(0, 0, 30, screenRect.size.h);
    }
  #endif

  // init the widgets
  SidebarWidgets_init();

  sidebarLayer = layer_create(bounds);
  layer_add_child(window_get_root_layer(window), sidebarLayer);

  #ifdef PBL_ROUND
    layer_set_update_proc(sidebarLayer, updateRoundSidebarLeft);
  #else
    layer_set_update_proc(sidebarLayer, updateRectSidebar);
  #endif

  #ifdef PBL_ROUND
    sidebarLayer2 = layer_create(bounds2);
    layer_add_child(window_get_root_layer(window), sidebarLayer2);
    layer_set_update_proc(sidebarLayer2, updateRoundSidebarRight);
  #endif
}

void Sidebar_deinit() {
  layer_destroy(sidebarLayer);

  SidebarWidgets_deinit();
}

void Sidebar_redraw() {
  #ifndef PBL_ROUND
    // reposition the sidebar if needed
    if(globalSettings.sidebarOnLeft) {
      layer_set_frame(sidebarLayer, GRect(0, 0, 30, SCREEN_HEIGHT));
    } else {
      layer_set_frame(sidebarLayer, GRect(114, 0, 30, SCREEN_HEIGHT));
    }
  #endif

  // redraw the layer
  layer_mark_dirty(sidebarLayer);

  #ifdef PBL_ROUND
    layer_mark_dirty(sidebarLayer2);
  #endif
}

void Sidebar_draw_second() {
  isSecondaryDisplay=true;
  Sidebar_redraw();
}

void Sidebar_draw_origin() {
  isSecondaryDisplay=false;
  Sidebar_redraw();
}

void Sidebar_updateTime(struct tm* timeInfo) {
  SidebarWidgets_updateTime(timeInfo);

  // redraw the sidebar in case it changed in any way
  Sidebar_redraw();
}

bool isAutoBatteryShown() {
  if(!globalSettings.disableAutobattery) {
    BatteryChargeState chargeState = battery_state_service_peek();

    if(globalSettings.enableAutoBatteryWidget) {
      if(chargeState.charge_percent <= 10 || chargeState.is_charging) {
        return true;
      }
    }
  }

  return false;
}


#ifdef PBL_ROUND

// returns the best candidate widget for replacement by the auto battery
// or the disconnection icon
int getReplacableWidget() {
  if(globalSettings.widgets[0] == EMPTY) {
    return 0;
  } else if(globalSettings.widgets[2] == EMPTY) {
    return 2;
  }

  if(globalSettings.widgets[0] == WEATHER_CURRENT || globalSettings.widgets[0] == WEATHER_FORECAST_TODAY) {
    return 0;
  } else if(globalSettings.widgets[2] == WEATHER_CURRENT || globalSettings.widgets[2] == WEATHER_FORECAST_TODAY) {
    return 2;
  }

  // if we don't have any of those things, just replace the left widget
  return 0;
}

#else

// returns the best candidate widget for replacement by the auto battery
// or the disconnection icon
int getReplacableWidget() {
  // if any widgets are empty, it's an obvious choice
  for(int i = 0; i < 3; i++) {
    if(globalSettings.widgets[i] == EMPTY) {
      return i;
    }
  }

  // are there any bluetooth-enabled widgets? if so, they're the second-best
  // candidates
  for(int i = 0; i < 3; i++) {
    if(globalSettings.widgets[i] == WEATHER_CURRENT || globalSettings.widgets[i] == WEATHER_FORECAST_TODAY) {
      return i;
    }
  }

  // if we don't have any of those things, just replace the middle widget
  return 1;
}

#endif

#ifdef PBL_ROUND

void updateRoundSidebarRight(Layer *l, GContext* ctx) {
  GRect bounds = layer_get_bounds(l);
  GRect bgBounds = GRect(bounds.origin.x, bounds.size.h / -2, bounds.size.h * 2, bounds.size.h * 2);

  bool showDisconnectIcon = !bluetooth_connection_service_peek();
  bool showAutoBattery = isAutoBatteryShown();

  SidebarWidgetType displayWidget = globalSettings.widgets[2];
  
  if(isSecondaryDisplay){
    displayWidget=WEATHER_FORECAST_TODAY;   
  }

  if((showAutoBattery || showDisconnectIcon) && getReplacableWidget() == 2) {
    if(showAutoBattery) {
      displayWidget = BATTERY_METER;
    } else if(showDisconnectIcon) {
      displayWidget = BLUETOOTH_DISCONNECT;
    }
  }

  drawRoundSidebar(ctx, bgBounds, displayWidget, 3);
}

void updateRoundSidebarLeft(Layer *l, GContext* ctx) {
  GRect bounds = layer_get_bounds(l);
  GRect bgBounds = GRect(bounds.origin.x - bounds.size.h * 2 + bounds.size.w, bounds.size.h / -2, bounds.size.h * 2, bounds.size.h * 2);

  bool showDisconnectIcon = !bluetooth_connection_service_peek();
  bool showAutoBattery = isAutoBatteryShown();
  SidebarWidgetType displayWidget = globalSettings.widgets[0];

  if((showAutoBattery || showDisconnectIcon) && getReplacableWidget() == 0) {
    if(showAutoBattery) {
      displayWidget = BATTERY_METER;
    } else if(showDisconnectIcon) {
      displayWidget = BLUETOOTH_DISCONNECT;
    }
  }
  if(isSecondaryDisplay){
    displayWidget=DATE;   
  }
  drawRoundSidebar(ctx, bgBounds, displayWidget, 7);
}

void drawRoundSidebar(GContext* ctx, GRect bgBounds, SidebarWidgetType widgetType, int widgetXOffset) {
  SidebarWidgets_updateFonts();

  graphics_context_set_fill_color(ctx, globalSettings.sidebarColor);

  graphics_fill_radial(ctx,
                       bgBounds,
                       GOvalScaleModeFillCircle,
                       100,
                       DEG_TO_TRIGANGLE(0),
                       TRIG_MAX_ANGLE);

  SidebarWidgets_xOffset = widgetXOffset;
  SidebarWidget widget = getSidebarWidgetByType(widgetType);

  // calculate center position of the widget
  int widgetPosition = bgBounds.size.h / 4 - widget.getHeight() / 2;
  widget.draw(ctx, widgetPosition);
}
#endif





void updateRectSidebar(Layer *l, GContext* ctx) {
  SidebarWidgets_updateFonts();

  graphics_context_set_fill_color(ctx, globalSettings.sidebarColor);
  graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone);

  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  // if the pebble is disconnected, show the disconnect icon
  bool showDisconnectIcon = !bluetooth_connection_service_peek();
  bool showAutoBattery = isAutoBatteryShown();

  SidebarWidget displayWidgets[3];

  displayWidgets[0] = getSidebarWidgetByType(globalSettings.widgets[0]);
  displayWidgets[1] = getSidebarWidgetByType(globalSettings.widgets[1]);
  displayWidgets[2] = getSidebarWidgetByType(globalSettings.widgets[2]);

  // do we need to replace a widget?
  // if so, determine which widget should be replaced
  if(showAutoBattery || showDisconnectIcon) {
    int widget_to_replace = getReplacableWidget();

    if(showAutoBattery) {
      displayWidgets[widget_to_replace] = getSidebarWidgetByType(BATTERY_METER);
    } else if(showDisconnectIcon) {
      displayWidgets[widget_to_replace] = getSidebarWidgetByType(BLUETOOTH_DISCONNECT);
    }
  }

  // if the widgets are too tall, enable "compact mode"
  SidebarWidgets_useCompactMode = false; // ensure that we compare the non-compacted heights
  int totalHeight = displayWidgets[0].getHeight() + displayWidgets[1].getHeight() + displayWidgets[2].getHeight();
  SidebarWidgets_useCompactMode = (totalHeight > 142) ? true : false;

  // calculate the three widget positions
  int topWidgetPos = V_PADDING;
  int lowerWidgetPos = SCREEN_HEIGHT - V_PADDING - displayWidgets[2].getHeight();

  // vertically center the middle widget using MATH
  int middleWidgetPos = ((lowerWidgetPos - displayWidgets[1].getHeight()) + (topWidgetPos + displayWidgets[0].getHeight())) / 2;

  // draw the widgets
  displayWidgets[0].draw(ctx, topWidgetPos);
  displayWidgets[1].draw(ctx, middleWidgetPos);
  displayWidgets[2].draw(ctx, lowerWidgetPos);

}
