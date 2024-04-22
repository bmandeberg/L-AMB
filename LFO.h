#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include <arduino-timer.h>

const int ADC_RESOLUTION = pow(2, 10) - 1;
const float R = 47000.0;
const float lowC = 1.0e-6;
const float highC = 1.2e-9
const long lowSlowestPeriod = 20000000; // 0.05 Hz
const long lowFastestPeriod = 200000; // 5 Hz
const long highSlowestPeriod = 20000; // 50 Hz
const long highFastestPeriod = 250; // 4000 Hz

class LFO {

  public:
    void setup(int freqPin, int dutyPin, int squarePin, int rangePin, int rangePinOut, int dacChan);
    void update();
    long period;
    float dutyCycle;
    bool rising = false;

  private:
    int dacChannel;
    int freqInPin;
    int dutyInPin;
    int pulseOutPin;
    int rangeInPin;
    int rangeOutPin;
    bool highRange = false;
    long lastPeriod;
    float lastDutyCycle;
    bool lastClockSelected;
    Switch rangeSwitch;
    Timer<1, micros> timer;
    void _write(float targetVpp);
    void _setTimer(int delay);
    void _writeCycle(bool updatePeriod);
    bool _usingClockIn();
    void _setHighRange();
};

#endif // LFO_H