#include "wifi.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

static WebServer server(80);
static Preferences prefs;
static bool apMode = false;
static unsigned long connectTimeoutMs = 8000; // try 8 seconds to connect

// AP settings (static IP for the portal)
const char* AP_SSID = "ESP32-Setup";
const char* AP_PASS = "12345678";
IPAddress apIP(192,168,4,1);
IPAddress apGW(192,168,4,1);
IPAddress apSN(255,255,255,0);

static String savedSSID;
static String savedPass;

static void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { font-family: Arial, Helvetica, sans-serif; padding: 10px; }
          input { width: 100%; padding: 8px; margin: 6px 0; box-sizing: border-box; }
          button { padding: 10px; width: 100%; }
          .small { font-size: 0.9em; color: #666; }
        </style>
      </head>
      <body>
        <h3>ESP32 WiFi Setup</h3>
        <form method="POST" action="/save">
          <label>SSID</label><br>
          <input name="ssid" placeholder="Your network name" autocomplete="on"><br>
          <label>Password</label><br>
          <input name="pass" type="password" placeholder="Password (leave blank for open networks)"><br>
          <button type="submit">Save & Connect</button>
        </form>
        <p class="small">Connect to the AP <b>ESP32-Setup</b> (password <b>12345678</b>) then open <b>http://192.168.4.1</b></p>
      </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

static void handleSave() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  // Save to Preferences
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  String resp = "<html><body><h3>Saved. Rebooting and attempting to connect...</h3></body></html>";
  server.send(200, "text/html", resp);
  delay(1200);
  ESP.restart();
}

static void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.softAPConfig(apIP, apGW, apSN); // set static IP for AP

  Serial.println();
  Serial.println("=== SoftAP mode started ===");
  Serial.print("AP SSID: "); Serial.println(AP_SSID);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // start webserver
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  apMode = true;
}

static bool tryConnectSTA(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long start = millis();
  Serial.print("Trying to connect to ");
  Serial.print(ssid);
  Serial.print(" ... ");

  while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    apMode = false;
    return true;
  }

  Serial.println();
  Serial.println("Connection attempt failed.");
  return false;
}

void setupWiFi() {
  // read saved credentials
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();

  if (savedSSID.length() > 0) {
    if (tryConnectSTA(savedSSID.c_str(), savedPass.c_str())) {
      // connected as STA
      apMode = false;
      return;
    }
    // else fallthrough to AP
  }

  // start AP portal if no saved creds or connection failed
  startAPMode();
}

void handleWifiPortal() {
  if (apMode) {
    server.handleClient();
  } else {
    // Maintain STA connection; attempt reconnect in background if needed
    if (WiFi.status() != WL_CONNECTED) {
      static unsigned long lastAttempt = 0;
      unsigned long now = millis();
      if (now - lastAttempt > 5000) {
        lastAttempt = now;
        Serial.println("STA lost connection — attempting reconnect...");
        // read stored creds and reconnect
        prefs.begin("wifi", true);
        savedSSID = prefs.getString("ssid", "");
        savedPass = prefs.getString("pass", "");
        prefs.end();
        if (savedSSID.length() > 0) {
          WiFi.disconnect();
          WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        } else {
          // no creds → go to AP
          startAPMode();
        }
      }
    }
    // if still connected nothing to handle here
  }
}

bool isWiFiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

String getLocalIP() {
  if (apMode) {
    return WiFi.softAPIP().toString();
  } else if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  } else {
    return String("0.0.0.0");
  }
}