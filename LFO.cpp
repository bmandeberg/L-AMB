#include <Arduino.h>
#include <limits.h>
#include "LFO.h"

#define I2C_FREQ 3200000

long LFO::slowPeriodLogTable[1024];
long LFO::fastPeriodLogTable[1024];

long periodLogValue(int knobVal, long slowestPeriod, long fastestPeriod) {
  double scale = log(slowestPeriod / fastestPeriod) / ADC_RES;
  return fastestPeriod * exp((ADC_RES - knobVal) * scale);
}

// setup internal DAC, currently hardcoded to A0
void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin, int dacChan, Adafruit_ZeroDMA* dmaRef) {
  init(freqPin, dutyPin, wavePin, rangePin);
  dacChannel = dacChan;
  analogWrite(A0, 0);
  dma = dmaRef;
  dma->addDescriptor((void *)&dacValue, (void *)&DAC->DATA[dacChannel].reg, 1, DMA_BEAT_SIZE_HWORD, false, false);
  dma->loop(true);
  dma->startJob();
  write = &LFO::writeDAC;
}

// setup external DAC via I2C
void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin, int dacAddr, I2C_DMAC* i2cRef, int writeChannel, int readChannel) {
  init(freqPin, dutyPin, wavePin, rangePin);
  dacAddress = dacAddr;
  i2c = i2cRef;
  i2c->begin(I2C_FREQ);
  i2c->setWriteChannel(writeChannel);
  i2c->setReadChannel(readChannel);
  i2c->initWriteBytes(dacAddress, i2cPacket, 3);
  write = &LFO::writeI2C;
}

void LFO::init(int freqPin, int dutyPin, int wavePin, int rangePin) {
  freqInPin = freqPin;
  dutyInPin = dutyPin;
  waveSwitchPin = wavePin;
  rangeSwitchPin = rangePin;

  Callback setHighCallback(&LFO::setHigh, this);
  Callback setLowCallback(&LFO::setLow, this);
  rangeSwitch.setup(rangeSwitchPin, false, false, setHighCallback, setLowCallback);
  highRange = digitalRead(rangeSwitchPin) == HIGH;
  
  Callback triangleWaveCallback(&LFO::setTriangleWave, this);
  Callback squareWaveCallback(&LFO::setSquareWave, this);
  waveSwitch.setup(waveSwitchPin, false, false, triangleWaveCallback, squareWaveCallback);
  triangleWaveSelected = digitalRead(waveSwitchPin) == HIGH;
}

void LFO::tick() {
  // progress the wave
  currentValue += rising ? periodIncrement[0] : -periodIncrement[1];
  if (currentValue >= scaledDacResolution) {
    currentValue = scaledDacResolution - (currentValue - scaledDacResolution);
    rising = false;
  } else if (currentValue <= 0) {
    currentValue = -currentValue;
    rising = true;
  }

  // write the DAC value
  currentValueDescaled = triangleWaveSelected ?
    currentValue >> scalingFactor :
    rising ? DAC_RES - squareBuffer : squareBuffer;
  (this->*write)(currentValueDescaled);
}

void LFO::writeDAC(int dacVal) {
  dacValue = dacVal;
}

void LFO::writeI2C(int dacVal) {
  i2cPacket[1] = (dacVal / 16) & 0xFF; // upper 8 bits
  i2cPacket[2] = (dacVal % 16) << 4;   // lower 4 bits
  i2c->write(); // takes about 20 micros in parallel
}

void LFO::check(bool usingClockIn) {
  rangeSwitch.check();
  waveSwitch.check();

  int freq = bufferedKnob(analogRead(freqInPin));
  int dutyCycle = bufferedKnob(analogRead(dutyInPin));

  bool updatePeriod = knobChanged(freq, lastFreq) ||
    lastUsingClockIn != usingClockIn ||
    lastRange != highRange;
  if (updatePeriod) {
    // if using external clock input
    if (usingClockIn) {
      // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
      int coefficient = clockDivMultTable[freq / knobRange];
      long newPeriod = freq <= ADC_RES / 2 ?
        clockPeriod * coefficient :
        clockPeriod / coefficient;
      period = constrain(newPeriod, highFastestPeriod, lowSlowestPeriod);
    } else {
      // set LFO period based on frequency knob
      period = highRange ?
        fastPeriodLogTable[freq] :
        slowPeriodLogTable[freq];
    }

    lastFreq = freq;
    lastRange = highRange;
  }

  // if anything has changed, update the periodIncrement
  if (updatePeriod || knobChanged(dutyCycle, lastDutyCycle)) {
    // make sure period * duty doesn't overflow
    long dutyPeriod = multKnobWithoutOverflow(period, dutyCycle);
    long dutyPeriodSteps = dutyPeriod / clockResolution;
    long oppositeDutyPeriodSteps = (period - dutyPeriod) / clockResolution;
    periodIncrementCopy[0] = scaledDacResolution / max(dutyPeriodSteps, 1);
    periodIncrementCopy[1] = scaledDacResolution / max(oppositeDutyPeriodSteps, 1);

    noInterrupts();
    periodIncrement[0] = periodIncrementCopy[0];
    periodIncrement[1] = periodIncrementCopy[1];
    interrupts();

    lastDutyCycle = dutyCycle;
  }
}

int LFO::getValue() {
  return currentValueDescaled;
}

void LFO::setHigh() {
  if (!usingClockIn()) {
    highRange = true;
  }
}

void LFO::setLow() {
  if (!usingClockIn()) {
    highRange = false;
  }
}

void LFO::setTriangleWave() {
  triangleWaveSelected = true;
}

void LFO::setSquareWave() {
  triangleWaveSelected = false;
}

void LFO::reset() {
  noInterrupts();
  currentValue = 0;
  rising = true;
  interrupts();
}

void LFO::initializePeriodTables() {
  for (int i = 0; i < 1024; i++) {
    slowPeriodLogTable[i] = periodLogValue(i, lowSlowestPeriod, lowFastestPeriod);
    fastPeriodLogTable[i] = periodLogValue(i, highSlowestPeriod, highFastestPeriod);
  }
}

bool knobChanged(int thisKnob, int lastKnob) {
  const int minKnobDiff = 5;
  return thisKnob < lastKnob - minKnobDiff || thisKnob > lastKnob + minKnobDiff;
}

long multKnobWithoutOverflow(long valA, long valB) {
  return valB && valA > LONG_MAX / valB ?
    valA / ADC_RES * valB :
    valA * valB / ADC_RES;
}

int bufferedKnob(int knobVal) {
  int knobBuffer = 10;
  return knobVal < knobBuffer ? 0 :
    knobVal > ADC_RES - knobBuffer ? ADC_RES :
    knobVal;
}
