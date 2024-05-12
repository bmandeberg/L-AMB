#ifndef LAMB_H
#define LAMB_H

extern const int DAC_RES;
extern const int ADC_RES;
extern const int clockInPin;
extern const int clockSelectPin;
extern bool clockSelected;
extern volatile long clockPeriod;
extern volatile long lastClockTime;
extern const long minClockPeriod;
extern const long clockResolution; // in microseconds
extern const int knobRange;
extern int clockDivMultOptions[];
extern bool lastUsingClockIn;

bool usingClockIn();

#endif // LAMB_H