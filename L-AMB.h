#ifndef LAMB_H
#define LAMB_H

extern const int DAC_RES;
extern const int ADC_RES;
extern const int clockInPin;
extern const int clockSelectPin;
extern bool clockSelected;
extern volatile long clockPeriod;
extern long lastClockPeriod;
extern volatile long lastClockTime;
extern const long minClockPeriod;
extern const long clockResolution;
extern int clockDivMultOptions[];
extern const int knobRange;

#endif // LAMB_H