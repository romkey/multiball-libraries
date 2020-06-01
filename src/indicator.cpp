#if 0

#include "multiball/indicator.h"
#include "multiball/mqtt.h"

#ifdef ESP32
#include <WiFi.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include <Ticker.h>
#include <FastLED.h>

#define INDICATOR_INTERVAL 0.5

static Ticker indicator_ticker;

static bool indicator_flash = true;
static bool indicator_on = true;

static CRGB* leds;

static CRGB green(0, 255, 0), red(255, 0, 0), amber(255, 194, 0), black(0, 0, 0), blue(0, 0, 255);

void indicator_callback();

CRGB* indicator_setup(uint8_t num_leds) {
  leds = (CRGB*)malloc(sizeof(CRGB) * num_leds);
  for(int i = 0; i<num_leds; i++)
	leds[i] = CRGB(0, 0, 0);

  //  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, 1);

  FastLED.setBrightness(64);

#ifdef VERBOSE
  leds[0] = red;
  FastLED.show();
  Serial.println("red");
  delay(2000);

  leds[0] = green;
  FastLED.show();
  Serial.println("green");
  delay(2000);

  leds[0] = blue;
  FastLED.show();
  Serial.println("blue");
  delay(2000);
#endif

  indicator_ticker.attach(INDICATOR_INTERVAL, indicator_callback);

  return leds;
}

void indicator_handle() {
  if(WiFi.status() == WL_CONNECTED && mqtt_is_connected()) {
    leds[0] = green;
    FastLED.show();

    indicator_flash = false;
    return;
  }

  indicator_flash = true;
  if(WiFi.status() != WL_CONNECTED) {
    leds[0] = red;
    FastLED.show();

    return;
  }

  leds[0] = amber;
  FastLED.show();
}

void indicator_callback() {
  if(!indicator_flash)
    return;

  if(indicator_on) {
    indicator_on = false;
    leds[0] = black;

    FastLED.show();
    return;
  }

  indicator_on = true;
  indicator_handle();
}

#endif
