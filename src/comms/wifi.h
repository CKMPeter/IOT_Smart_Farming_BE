#ifndef COMMS_WIFI_H
#define COMMS_WIFI_H

#include <Arduino.h>

void setupWiFi();               // call from setup()
void handleWifiPortal();        // call from loop() to keep portal alive
bool isWiFiConnected();         // check STA connection
String getLocalIP();            // returns IP as string (STA or AP)

#endif // COMMS_WIFI_H
