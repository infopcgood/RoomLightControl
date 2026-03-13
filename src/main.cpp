#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 200
#define DATA_PIN 4

CRGB leds[NUM_LEDS];

void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
    for(int i = 0; i < NUM_LEDS; i++) {
        for(int j = 0; j <= i; j++) {
            leds[i-j] = (j/3)%3 == 0 ? CRGB::Red : (j/3)%3 == 1 ? CRGB::Green : CRGB::Blue;
        }
        FastLED.show();
        delay(500);
    }
    delay(2000);
}
