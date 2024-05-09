#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"

const long scalingFactor = 17;
const long scaledDacResolution = DAC_RES << scalingFactor; // multiply by 131072
const float R = 47000.0;
const float lowC = 1.0e-6; // 1uF
const float highC = 1.2e-9; // 1.2nF
const long lowSlowestPeriod = 20000000; // 0.05 Hz
const long lowFastestPeriod = 200000; // 5 Hz
const long highSlowestPeriod = 20000; // 50 Hz
const long highFastestPeriod = 250; // 4000 Hz
const long crossoverPeriod = (lowFastestPeriod - highSlowestPeriod) / 2;

class LFO {

  public:
    void setup(int freqPin, int dutyPin, int wavePin, int rangePin);
    void tick();
    void check();
    void setHigh();
    void setLow();
    void toggleWave();

  private:
    long period;
    int dutyCycle;
    volatile int dacValue;
    volatile long currentValue;
    volatile long periodIncrement[2]; // 0: pulse high, 1: pulse low
    long periodIncrementCopy[2];
    volatile bool rising = false;
    volatile bool lastRising = false;
    int freqInPin;
    int dutyInPin;
    int waveSwitchPin;
    bool triangleWaveSelected = false;
    int rangeSwitchPin;
    bool highRange = false;
    long lastPeriod;
    int lastDutyCycle;
    Switch waveSwitch;
    Switch rangeSwitch;
    bool usingClockIn();
};

#endif // LFO_H