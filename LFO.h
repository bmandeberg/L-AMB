#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"

static const long scalingFactor = 17;
static const long scaledDacResolution = DAC_RES << scalingFactor;  // multiply by 131072
static const long lowSlowestPeriod = 20000000;                     // 0.05 Hz
static const long lowFastestPeriod = 200000;                       // 5 Hz
static const long highSlowestPeriod = 20000;                       // 50 Hz
static const long highFastestPeriod = 300;                         // 3333.33 Hz

class LFO {

public:
  void setup(int freqPin, int dutyPin, int wavePin, int rangePin);
  int tickDacVal();
  void check(bool usingClockIn);
  void setHigh();
  void setLow();
  void toggleWave();

private:
  long period = 1000000;
  int lastFreq;
  volatile long currentValue = 0;
  volatile long periodIncrement[2] = { 53673, 53673 }; // 0: pulse high, 1: pulse low
  long periodIncrementCopy[2];
  volatile bool rising = true;
  int freqInPin;
  int dutyInPin;
  int waveSwitchPin;
  bool triangleWaveSelected = true;
  int rangeSwitchPin;
  bool highRange = false;
  int lastDutyCycle;
  Switch waveSwitch;
  Switch rangeSwitch;
};

#endif // LFO_H