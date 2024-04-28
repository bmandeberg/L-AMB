#ifndef LAMB_H
#define LAMB_H

#include "Arduino.h"
#include <Adafruit_MCP4728.h>

extern Adafruit_MCP4728 mcp;
extern const int DAC_RESOLUTION;
extern const int clockInPin;
extern const int clockSelectPin;
extern const int led1Pin;
extern const int led2Pin;
extern const int led3Pin;
extern const int led4Pin;
extern bool clockSelected;
extern volatile long clockPeriod;
extern volatile long lastClockTime;
extern const long minClockPeriod;

#endif // LAMB_H