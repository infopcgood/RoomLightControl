#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "defines.h"

WiFiUDP Udp;

CRGB leds[NUM_LEDS];
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// ---------- Wi-Fi diagnostics (Serial Monitor at 115200 baud) ----------

static const char* wifiStatusName(int status) {
    switch (status) {
        case 0: return "IDLE";
        case 1: return "NO_SSID_AVAIL - network not found (5 GHz only? typo in SSID? out of range?)";
        case 2: return "SCAN_COMPLETED";
        case 3: return "CONNECTED";
        case 4: return "CONNECT_FAILED - router rejected it (WPA3-only? MAC filter?)";
        case 5: return "CONNECTION_LOST";
        case 6: return "WRONG_PASSWORD";
        case 7: return "DISCONNECTED - still trying";
        default: return "UNKNOWN";
    }
}

static const char* encryptionName(uint8_t enc) {
    switch (enc) {
        case ENC_TYPE_NONE: return "open";
        case ENC_TYPE_WEP:  return "WEP";
        case ENC_TYPE_TKIP: return "WPA/TKIP";
        case ENC_TYPE_CCMP: return "WPA2/CCMP";
        case ENC_TYPE_AUTO: return "WPA/WPA2";
        default:            return "other";
    }
}

static void scanAndPrintNetworks() {
    Serial.println("[WiFi] Networks this board can see (ESP8266 = 2.4 GHz only):");
    int count = WiFi.scanNetworks();
    if (count <= 0) {
        Serial.println("         none - check the antenna / power supply");
        return;
    }
    bool found = false;
    for (int i = 0; i < count; i++) {
        bool mine = WiFi.SSID(i) == WIFI_SSID;
        found = found || mine;
        Serial.printf("         %-32s ch %2d  %4d dBm  %-9s%s\n",
                      WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i),
                      encryptionName(WiFi.encryptionType(i)), mine ? "  <-- WIFI_SSID" : "");
    }
    if (!found) {
        Serial.printf("[WiFi] \"%s\" is NOT in the list. It is probably a 5 GHz network, or the name in defines.h does not match exactly (case and spaces count).\n", WIFI_SSID);
    }
    WiFi.scanDelete();
}

static void showStatusLed(const CRGB& color) {
    leds[0] = color;
    FastLED.show();
}

// ------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("\n\n[RoomLight] %s booting: %d LEDs on GPIO%d, UDP port %d\n",
                  DEVICE_NAME, NUM_LEDS, DATA_PIN, UDP_PORT);

    // Init LEDs first so the first LED can show Wi-Fi state:
    // blue = connecting, red = problem (see Serial Monitor), green flash = connected.
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.clear();
    showStatusLed(CRGB(0, 0, 40));

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(DEVICE_NAME);
    Serial.printf("[WiFi] Connecting to \"%s\" (password length %u)\n",
                  WIFI_SSID, (unsigned) strlen(WIFI_PASSWORD));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long attemptStart = millis();
    int lastStatus = -1;
    while (WiFi.status() != WL_CONNECTED) {
        int status = WiFi.status();
        if (status != lastStatus) {
            Serial.printf("[WiFi] status %d: %s\n", status, wifiStatusName(status));
            lastStatus = status;
        }
        if (millis() - attemptStart > 20000) {
            Serial.println("[WiFi] Not connected after 20 s.");
            showStatusLed(CRGB(40, 0, 0));
            WiFi.disconnect();
            delay(100);
            scanAndPrintNetworks();
            Serial.println("[WiFi] Retrying...");
            showStatusLed(CRGB(0, 0, 40));
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            attemptStart = millis();
            lastStatus = -1;
        }
        delay(250);
    }

    Serial.printf("[WiFi] Connected! IP %s  (RSSI %d dBm, channel %d)\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    Serial.printf("[WiFi] Add this strip in RoomLightSoftware as %s port %d\n",
                  WiFi.localIP().toString().c_str(), UDP_PORT);

    Udp.begin(UDP_PORT);

    showStatusLed(CRGB(0, 40, 0));
    delay(1000);
    FastLED.clear();
    FastLED.show();
}

void loop() {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        int len = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        if (len > 0) {
            packetBuffer[len] = 0;
        }

        switch (packetBuffer[0]) {
            case 'p': {
                // ping, return pong with device information
                char* packet = (char*) calloc(sizeof(char) * (5 + sizeof(DEVICE_NAME)), sizeof(char));
                packet[0] = (NUM_LEDS >> 8) & 0xFF;
                packet[1] = NUM_LEDS & 0xFF;
                packet[2] = DATA_PIN;
                packet[3] = sizeof(DEVICE_NAME);
                memcpy(packet + 4, DEVICE_NAME, sizeof(DEVICE_NAME));
                Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
                Udp.write(packet, 5 + sizeof(DEVICE_NAME));
                Udp.endPacket();
                free(packet);
                break;
            }
            case 'u': {
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
            }

            case 'c': {
                // clear LEDs
                FastLED.clear();
                FastLED.show();
                break;
            }
        }
    }
}
