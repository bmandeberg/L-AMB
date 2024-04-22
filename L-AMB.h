#include "Arduino.h"
#include <Adafruit_MCP4728.h>

#ifndef LAMB_H
#define LAMB_H

Adafruit_MCP4728 mcp;
const int DAC_RESOLUTION = pow(2, 12) - 1;
int clockInPin = 7;
int clockSelectPin = 9;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 10;

#endif // LAMB_H