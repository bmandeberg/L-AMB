#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"
#include <Adafruit_ZeroDMA.h>

static const long scalingFactor = 17;
static const long scaledDacResolution = DAC_RES << scalingFactor; // multiply by 131072
static const long lowSlowestPeriod = 20000000; // 0.05 Hz
static const long lowFastestPeriod = 200000; // 5 Hz
static const long highSlowestPeriod = 20000; // 50 Hz
static const long highFastestPeriod = 250; // 4000 Hz

class LFO {

  public:
    void setupDAC(int freqPin, int dutyPin, int wavePin, int rangePin, int dacChannel);
    void setupI2C(int freqPin, int dutyPin, int wavePin, int rangePin);
    void tick();
    void check(bool usingClockIn);
    void setHigh();
    void setLow();
    void toggleWave();

  private:
    long period;
    int dutyCycle;
    int lastFreq;
    volatile uint16_t dacValue;
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
    int lastDutyCycle;
    Switch waveSwitch;
    Switch rangeSwitch;
    Adafruit_ZeroDMA dma;
    void init(int freqPin, int dutyPin, int wavePin, int rangePin);
};

#endif // LFO_H