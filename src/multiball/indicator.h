#pragma once

#include <Arduino.h>
#include <FastLED.h>

// start the indicator LED
CRGB* indicator_setup(uint8_t num_leds);
void indicator_handle();
