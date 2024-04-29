#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"
#include <arduino-timer.h>

const int ADC_RESOLUTION = pow(2, 10) - 1;
const float R = 47000.0;
const float lowC = 1.0e-6; // 1uF
const float highC = 1.2e-9; // 1.2nF
const long lowSlowestPeriod = 20000000; // 0.05 Hz
const long lowFastestPeriod = 200000; // 5 Hz
const long highSlowestPeriod = 20000; // 50 Hz
const long highFastestPeriod = 250; // 4000 Hz
const long resetPulseDuration = 28;

class LFO {

  public:
    static LFO* instance; // instance pointer for binding to Timer.in
    static bool timerCallback(void* arg);
    void writeCycle(bool updatePeriod);
    void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int resetPin, int dacChan);
    void update();
    float currentValue();
    void setHigh();
    void setLow();
    void toggleWave();

  private:
    long period;
    float dutyCycle;
    bool rising = false;
    bool lastRising = false;
    int dacChannel;
    int freqInPin;
    int dutyInPin;
    int waveSwitchPin;
    bool triangleWaveSelected = false;
    bool lastTriangleWaveSelected = false;
    int rangeSwitchPin;
    int rangeOutPin;
    int resetPulsePin;
    bool highRange = false;
    long lastPeriod;
    float lastDutyCycle;
    bool lastClockSelected;
    bool resetting = false;
    long resetTime;
    Timer<1, micros> timer;
    Switch waveSwitch;
    Switch rangeSwitch;
    void _write(float targetVpp);
    void _setTimer(long delay);
    bool _usingClockIn();
    void _setRange(bool rangeHigh);
    float _currentDuty();
};

float floatMap(float x, float in_min, float in_max, float out_min, float out_max);

#endif // LFO_H