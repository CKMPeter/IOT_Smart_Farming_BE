#include "webserver.h"
#include <ArduinoJson.h>
#include "../main.h"     // ensure access to globals

AsyncWebServer server(80);

void handleApiData(AsyncWebServerRequest *request) {
    JsonDocument doc;

    doc["temperature"] = currentTemp;
    doc["humidity"] = currentHumi;
    doc["soil"] = currentSoil;

    // New JSON array API
    JsonArray tArr = doc["tempGraph"].to<JsonArray>();
    JsonArray hArr = doc["humiGraph"].to<JsonArray>();

    for (int i = 0; i < GRAPH_POINTS; i++) {
        tArr.add(tempGraph[i]);
        hArr.add(humiGraph[i]);
    }

    String json;
    serializeJson(doc, json);

    request->send(200, "application/json", json);
}

void setupWebServer() {

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
        return;
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/api/data", HTTP_GET, handleApiData);

    server.begin();
    Serial.println("Webserver started on port 80");
}

void handleWebServer() {
    // nothing needed, async server
}
