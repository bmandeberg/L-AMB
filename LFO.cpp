#include <Arduino.h>
#include <limits.h>
#include "LFO.h"

long LFO::slowPeriodLogTable[1024];
long LFO::fastPeriodLogTable[1024];

long periodLogValue(int knobVal, long slowestPeriod, long fastestPeriod) {
  double scale = log(slowestPeriod / fastestPeriod) / ADC_RES;
  return fastestPeriod * exp((ADC_RES - knobVal) * scale);
}

void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin) {
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

int LFO::tick() {
  // progress the wave
  currentValue += rising ? periodIncrement[0] : -periodIncrement[1];
  if (currentValue >= scaledDacResolution) {
    currentValue = scaledDacResolution - (currentValue - scaledDacResolution); // mirror overflow
    rising = false;
  } else if (currentValue <= 0) {
    currentValue = -currentValue; // mirror overflow
    rising = true;
  }

  // update the DAC value
  currentValueDescaled = currentValue >> scalingFactor;
  return triangleWaveSelected ?
    constrain(currentValueDescaled, 0, DAC_RES) :
    rising ? DAC_RES : 0;
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
      int coefficient = clockDivMultOptions[freq / knobRange];
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
    long dutyPeriod = multWithoutOverflow(period, dutyCycle); // make sure period * duty doesn't overflow
    long dutyPeriodSteps = dutyPeriod / lfoUpdateResolution;
    long oppositeDutyPeriodSteps = (period - dutyPeriod) / lfoUpdateResolution;
    periodIncrementCopy[0] = scaledDacResolution / max(dutyPeriodSteps, 1);
    periodIncrementCopy[1] = scaledDacResolution / max(oppositeDutyPeriodSteps, 1);

    noInterrupts();
    periodIncrement[0] = periodIncrementCopy[0];
    periodIncrement[1] = periodIncrementCopy[1];
    interrupts();

    lastDutyCycle = dutyCycle;
  }
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

void LFO::reset() {
  noInterrupts();
  currentValue = 0;
  rising = true;
  interrupts();
}

int LFO::getValue() {
  return currentValueDescaled;
}

void LFO::setTriangleWave() {
  triangleWaveSelected = true;
}

void LFO::setSquareWave() {
  triangleWaveSelected = false;
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

long multWithoutOverflow(long valA, long valB) {
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
