#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 209
#define DATA_PIN 4

CRGB leds[NUM_LEDS];

void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
    // Make the LEDs continuously strobe between ffc400 and 00ffd0 for every 40 led with an interval of 2s using the HSV color space to achieve a smooth transition between the colors.
    // Smoothly transition between the colors using the HSV color space to achieve a smooth transition between the colors.
    for (int i = 0; i < NUM_LEDS; i++) {
        // Calculate the hue value based on the LED index and the current time
        uint8_t hue = (i * 256 / NUM_LEDS + millis() / 20) % 256;
        leds[i] = CHSV(hue, 255, 255); // Set the LED color using HSV
    }
    FastLED.show();
    delay(20); // Delay to control the speed of the color transition
    
}
