#include "main.h"

// ======================================================
//                 OLED SH1106 128x64
// ======================================================
U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// ======================================================
//                Soil + Relay Pump + Relay Fan
// ======================================================
const int moistD = 4;           // Soil sensor digital output
const int relayPinPump = 23;    // Relay Pump
const int relayPinFan  = 19;    // Relay Fan

// ======================================================
//                Fan Control Thresholds
// ======================================================
#define TEMP_FAN_ON   30.0
#define TEMP_FAN_OFF  28.0

#define HUMI_FAN_ON   75.0
#define HUMI_FAN_OFF  70.0

// ======================================================
//                Graph Settings
// ======================================================
#define GRAPH_WIDTH 128
#define GRAPH_HEIGHT 30
#define GRAPH_STEP 4
#define GRAPH_POINTS (GRAPH_WIDTH / GRAPH_STEP)

uint8_t tempGraph[GRAPH_POINTS];
uint8_t humiGraph[GRAPH_POINTS];

// ======================================================
//                Runtime Sensor State
// ======================================================
float currentTemp = 25.0;
float currentHumi = 60.0;
String currentSoil = "UNKNOWN";
String currentIP   = "0.0.0.0";

unsigned long lastUpdate     = 0;
unsigned long lastModbusRead = 0;

// ======================================================
//           Modbus RS485 (HardwareSerial on ESP32)
// ======================================================
HardwareSerial modbusSerial(2);   // UART2
ModbusMaster node;
uint8_t modbusID = 1;

// UART pins for RS485 → ESP32
#define RS485_RX 16
#define RS485_TX 17

// ======================================================
//                  MODBUS SETUP
// ======================================================
void setupModbus() {
  modbusSerial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  node.begin(modbusID, modbusSerial);
  Serial.println("Modbus Ready (UART2 RX=16, TX=17)");
}

// ======================================================
//              READ TEMPERATURE & HUMIDITY
// ======================================================
bool readModbus(float &temperature, float &humidity) {
  uint8_t result = node.readInputRegisters(1, 2);

  if (result == node.ku8MBSuccess) {
    temperature = node.getResponseBuffer(0) / 10.0;
    humidity    = node.getResponseBuffer(1) / 10.0;
    return true;
  }

  Serial.print("Modbus Error: ");
  Serial.println(result);
  return false;
}

// ======================================================
//                      SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(50);

  // ---------------- WiFi ----------------
  setupWiFi();

  if (isWiFiConnected()) {
    currentIP = getLocalIP();    // STA IP
    Serial.print("WiFi STA IP: ");
  } else {
    currentIP = getLocalIP();    // AP IP
    Serial.print("AP mode IP: ");
  }
  Serial.println(currentIP);

  setupWebServer();

  // ---------------- GPIO ----------------
  pinMode(moistD, INPUT);

  // Relays OFF at boot (safe state)
  pinMode(relayPinPump, INPUT);
  pinMode(relayPinFan, INPUT);

  // ---------------- OLED ----------------
  oled.begin();
  oled.setFont(u8g2_font_6x10_tr);

  memset(tempGraph, 0, sizeof(tempGraph));
  memset(humiGraph, 0, sizeof(humiGraph));

  // ---------------- Modbus --------------
  setupModbus();

  Serial.println("System Ready");
}

// ======================================================
//                        LOOP
// ======================================================
void loop() {
  handleWifiPortal();
  handleWebServer();

  // ====================================================
  //                    SOIL SENSOR
  // ====================================================
  int dState = digitalRead(moistD);

  // HIGH = DRY → Pump ON
  // LOW  = WET → Pump OFF
  if (dState == HIGH) {
    pinMode(relayPinPump, OUTPUT);
    digitalWrite(relayPinPump, HIGH);
    currentSoil = "DRY";
  } else {
    pinMode(relayPinPump, INPUT_PULLDOWN);
    currentSoil = "WET";
  }

  // ====================================================
  //               MODBUS READ (EVERY 2s)
  // ====================================================
  if (millis() - lastModbusRead >= 2000) {
    lastModbusRead = millis();

    float tVal = currentTemp;
    float hVal = currentHumi;

    if (readModbus(tVal, hVal)) {
      currentTemp = tVal;
      currentHumi = hVal;

      Serial.print("Temp=");
      Serial.print(currentTemp);
      Serial.print("C  Hum=");
      Serial.print(currentHumi);
      Serial.println("%");
    }
  }

  // ====================================================
  //                FAN CONTROL (HYSTERESIS)
  // ====================================================
  static bool fanState = false;

  if (!fanState &&
      (currentTemp >= TEMP_FAN_ON || currentHumi >= HUMI_FAN_ON)) {

    pinMode(relayPinFan, OUTPUT);
    digitalWrite(relayPinFan, HIGH);
    fanState = true;
  }
  else if (fanState &&
           (currentTemp <= TEMP_FAN_OFF && currentHumi <= HUMI_FAN_OFF)) {

    pinMode(relayPinFan, INPUT_PULLDOWN);
    fanState = false;
  }

  // ====================================================
  //               OLED UPDATE (EVERY 700ms)
  // ====================================================
  if (millis() - lastUpdate >= 700) {
    lastUpdate = millis();

    // Shift graph left
    for (int i = 0; i < GRAPH_POINTS - 1; i++) {
      tempGraph[i] = tempGraph[i + 1];
      humiGraph[i] = humiGraph[i + 1];
    }

    // Map sensor values to graph height
    tempGraph[GRAPH_POINTS - 1] =
      constrain(map((int)(currentTemp * 10), 200, 350, 0, GRAPH_HEIGHT),
                0, GRAPH_HEIGHT);

    humiGraph[GRAPH_POINTS - 1] =
      constrain(map((int)(currentHumi * 10), 400, 800, 0, GRAPH_HEIGHT),
                0, GRAPH_HEIGHT);

    // ---------------- OLED DRAW ----------------
    oled.firstPage();
    do {
      oled.setCursor(0, 10);
      oled.print("T:");
      oled.print(currentTemp, 1);
      oled.print("C");

      oled.setCursor(70, 10);
      oled.print("H:");
      oled.print(currentHumi, 1);
      oled.print("%");

      oled.setCursor(0, 20);
      oled.print("Soil:");
      oled.print(currentSoil);

      oled.setCursor(70, 20);
      oled.print("Fan:");
      oled.print(fanState ? "ON" : "OFF");

      // -------- IP ADDRESS --------
      oled.setCursor(0, 30);
      oled.print("IP:");
      oled.print(currentIP);

      // -------- GRAPH FRAME -------
      oled.drawFrame(0, 32, GRAPH_WIDTH, GRAPH_HEIGHT);

      // Temperature graph
      for (int i = 1; i < GRAPH_POINTS; i++) {
        oled.drawLine(
          (i - 1) * GRAPH_STEP,
          32 + GRAPH_HEIGHT - tempGraph[i - 1],
          i * GRAPH_STEP,
          32 + GRAPH_HEIGHT - tempGraph[i]
        );
      }

      // Humidity graph (dashed)
      for (int i = 1; i < GRAPH_POINTS; i++) {
        if (i % 2 == 0) continue;
        oled.drawLine(
          (i - 1) * GRAPH_STEP,
          32 + GRAPH_HEIGHT - humiGraph[i - 1],
          i * GRAPH_STEP,
          32 + GRAPH_HEIGHT - humiGraph[i]
        );
      }

    } while (oled.nextPage());
  }
}
