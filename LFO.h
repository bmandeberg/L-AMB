#ifndef LFO_H
#define LFO_H

#include "Switch.h"
#include "L-AMB.h"

#define MCP4725_CMD_WRITEDAC (0x40)    // writes data to the DAC

static const long scalingFactor = 17;
static const long scaledDacResolution = DAC_RES << scalingFactor; // multiply by 131072
static const long lowSlowestPeriod = 20000000; // 0.05 Hz
static const long lowFastestPeriod = 200000; // 5 Hz
static const long highSlowestPeriod = 20000; // 50 Hz
static const long highFastestPeriod = 250; // 4000 Hz

class LFO {

  public:
    void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int dacChan);
    void tick();
    void check(bool usingClockIn);
    void setHigh();
    void setLow();
    void toggleWave();

  private:
    long period;
    int dutyCycle;
    int lastFreq;
    int dacChannel; // 0, 1, or -1 if not using DAC
    bool usingDac = false;
    uint8_t i2cPacket[3] = {MCP4725_CMD_WRITEDAC, 0, 0};
    volatile long currentValue;
    volatile long periodIncrement[2]; // 0: pulse high, 1: pulse low
    long periodIncrementCopy[2];
    volatile bool rising = false;
    int freqInPin;
    int dutyInPin;
    int waveSwitchPin;
    bool triangleWaveSelected = false;
    int rangeSwitchPin;
    bool highRange = false;
    int lastDutyCycle;
    Switch waveSwitch;
    Switch rangeSwitch;
    void write(int dacValue);
};

#endif // LFO_H