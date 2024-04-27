#include "Arduino.h"
#include <Adafruit_MCP4728.h>

#ifndef LAMB_H
#define LAMB_H

Adafruit_MCP4728 mcp;
const int DAC_RESOLUTION = pow(2, 12) - 1;
const int clockInPin = 7;
const int clockSelectPin = 9;
const int led1Pin = 10;
const int led2Pin = 11;
const int led3Pin = 12;
const int led4Pin = 13;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 12;

#endif // LAMB_H