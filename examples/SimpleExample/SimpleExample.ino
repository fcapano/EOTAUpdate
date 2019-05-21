// This example shows how to autoupdate the firmware of an ESP microcontroller.

#include <WiFi.h>
#include <EOTAUpdate.h>

const char * const   VERSION_STRING = "1.0";
const unsigned short VERSION_NUMBER = 1;
const char * const   UPDATE_URL     = "https://myserver/ota/cfg.txt";

const char *ssid = "WIFI_SSID";
const char *password = "WIFI_PASSWORD";

EOTAUpdate updater(UPDATE_URL, VERSION_NUMBER);

void setup() {
    Serial.printf("Hello! My version is: [%u] %s\n", VERSION_NUMBER, VERSION_STRING);

    Serial.print("Connecting to WiFi... Remember to set the SSID and password!");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to network");
}

void loop() {
    System.println("Checking for updates. Remember to set the URL!");
    updater.CheckAndUpdate();
    delay(3000);
}