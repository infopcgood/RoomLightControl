#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "defines.h"

WiFiUDP Udp;

CRGB leds[NUM_LEDS];
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

void setup() {
    WiFi.hostname(DEVICE_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    Udp.begin(UDP_PORT);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        int len = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        if (len > 0) {
            packetBuffer[len] = 0;
        }

        switch (packetBuffer[0]) {
            case 'p': 
                // ping, return pong with device information
                char* packet = (char*) calloc(sizeof(char) * (5 + sizeof(DEVICE_NAME)), sizeof(char));
                packet[0] = (NUM_LEDS >> 8) & 0xFF;
                packet[1] = NUM_LEDS & 0xFF;
                packet[2] = DATA_PIN;
                packet[3] = sizeof(DEVICE_NAME);
                memcpy(packet + 4, DEVICE_NAME, sizeof(DEVICE_NAME));
                Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
                Udp.write(packet, 5 + sizeof(DEVICE_NAME));
                free(packet);
                break;

            case 'u':
                // update LEDs.
                // packet format: u ( [led index (2 bytes)] [R] [G] [B] ) * n
                for (int i = 1; i < len; i += 5) {
                    int ledIndex = (packetBuffer[i] << 8) | packetBuffer[i + 1];
                    if (ledIndex < NUM_LEDS) {
                        leds[ledIndex] = CRGB(packetBuffer[i + 2], packetBuffer[i + 3], packetBuffer[i + 4]);
                    }
                }
                FastLED.show();
                break;

            case 'c':
                // clear LEDs
                FastLED.clear();
                FastLED.show();
                break;
        }
    }
}
