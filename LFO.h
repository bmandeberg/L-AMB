#ifndef LFO_H
#define LFO_H

#include <I2C_DMAC.h>
#include <Adafruit_ZeroDMA.h>
#include "Switch.h"
#include "L-AMB.h"

#define MCP4725_CMD_WRITEDAC (0x40)

static const long scalingFactor = 17;
static const long scaledDacResolution = DAC_RES << scalingFactor;  // multiply by 131072
static const int squareBuffer = DAC_RES / 8;                       // attenuate pulse wave by this much on + or - swing
static const long lowSlowestPeriod = 20000000;                     // 0.05 Hz
static const long lowFastestPeriod = 200000;                       // 5 Hz
static const long highSlowestPeriod = 20000;                       // 50 Hz
static const long highFastestPeriod = 300;                         // 3333.33 Hz

class LFO {

public:
  void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int dacChan, Adafruit_ZeroDMA* dmaRef);
  void setup(int freqPin, int dutyPin, int wavePin, int rangePin, int dacAddr, I2C_DMAC* i2cRef);
  void tick();
  void check(bool usingClockIn);
  void setHigh();
  void setLow();
  void setTriangleWave();
  void setSquareWave();
  void reset();
  int getValue();

private:
  long period = 1000000;
  int lastFreq;
  volatile long currentValue = 0;
  int currentValueDescaled = 0;
  volatile uint16_t dacValue;
  int dacChannel;
  volatile long periodIncrement[2] = {53673, 53673}; // 0: pulse high, 1: pulse low
  long periodIncrementCopy[2];
  volatile bool rising = true;
  int freqInPin;
  int dutyInPin;
  int waveSwitchPin;
  bool triangleWaveSelected = true;
  int rangeSwitchPin;
  bool highRange = false;
  bool lastRange = false;
  int lastDutyCycle;
  uint8_t dacAddress;
  uint8_t i2cPacket[3] = {MCP4725_CMD_WRITEDAC, 0, 0};
  Adafruit_ZeroDMA* dma;
  I2C_DMAC* i2c;
  Switch waveSwitch;
  Switch rangeSwitch;
  void init(int freqPin, int dutyPin, int wavePin, int rangePin);
  void (LFO::*write)(int dacValue);
  void writeDAC(int dacValue);
  void writeI2C(int dacValue);
};

bool knobChanged(int thisKnob, int lastKnob);

long multKnobWithoutOverflow(long valA, long valB);

int bufferedKnob(int knobVal);

#endif  // LFO_H