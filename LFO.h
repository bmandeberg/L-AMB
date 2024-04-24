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
    void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int dacChan);
    void update();
    float currentValue();

  private:
    long period;
    float dutyCycle;
    bool rising = false;
    int dacChannel;
    int freqInPin;
    int dutyInPin;
    int waveSwitchPin;
    bool triangleWaveSelected = false;
    bool lastTriangleWaveSelected = false;
    int rangeSwitchPin;
    int rangeOutPin;
    bool highRange = false;
    long lastPeriod;
    float lastDutyCycle;
    bool lastClockSelected;
    Timer<1, micros> timer;
    Switch waveSwitch;
    Switch rangeSwitch;
    void _write(float targetVpp);
    void _setTimer(int delay);
    void _writeCycle(bool updatePeriod);
    bool _usingClockIn();
    void _setHighRange();
    float _currentDuty();
};

floatMap(float x, float in_min, float in_max, float out_min, float out_max);

#endif // LFO_H