#include <Arduino.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

void setupWebServer();    // call from setup()
void handleWebServer();   // call from loop() to keep server alive