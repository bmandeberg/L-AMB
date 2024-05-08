#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"

const long scalingFactor = 100000;
const long scaledDacResolution = DAC_RESOLUTION * scalingFactor;
const float R = 47000.0;
const float lowC = 1.0e-6; // 1uF
const float highC = 1.2e-9; // 1.2nF
const long lowSlowestPeriod = 20000000; // 0.05 Hz
const long lowFastestPeriod = 200000; // 5 Hz
const long highSlowestPeriod = 20000; // 50 Hz
const long highFastestPeriod = 250; // 4000 Hz
const long crossoverPeriod = (lowFastestPeriod - highSlowestPeriod) / 2;
const int minDutyCycle = ADC_RESOLUTION / 10;
const int maxDutyCycle = ADC_RESOLUTION * 9 / 10;
// int minDutyCycle = 0;
// int maxDutyCycle = ADC_RESOLUTION;

class LFO {

  public:
    int getValue();
    int update();
    void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int squarePinOut);
    void tick();
    void check();
    void setHigh();
    void setLow();
    void toggleWave();

  private:
    long period;
    int dutyCycle;
    volatile long currentValue;
    long currentValueCopy;
    volatile long periodIncrement;
    long periodIncrementCopy;
    volatile bool rising = false;
    bool risingCopy = false;
    bool lastRising = false;
    int freqInPin;
    int dutyInPin;
    int waveSwitchPin;
    bool triangleWaveSelected = false;
    int rangeSwitchPin;
    int rangeOutPin;
    int squareOutPin;
    volatile uint8_t &port = PORTB;
    uint8_t bit = 4;
    bool highRange = false;
    long lastPeriod;
    int lastDutyCycle;
    bool lastClockSelected;
    bool resetting = false;
    long resetTime;
    int lastDacValue = 0;
    Switch waveSwitch;
    Switch rangeSwitch;
    int _getPulseForTriangle();
    void _setTimer(long delay);
    bool _usingClockIn();
    void _setRange(bool rangeHigh);
    int _currentDuty();
    long _calculatePeriodIncrement();
};

#endif // LFO_H