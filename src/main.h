#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ModbusMaster.h>

#include "comms/wifi.h"

#include "webserver/webserver.h"

#pragma once

extern float currentTemp;
extern float currentHumi;
extern String currentSoil;

extern uint8_t tempGraph[];
extern uint8_t humiGraph[];

#define GRAPH_WIDTH 128
#define GRAPH_HEIGHT 30
#define GRAPH_STEP 4
#define GRAPH_POINTS (GRAPH_WIDTH / GRAPH_STEP)
