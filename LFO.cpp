#include <Arduino.h>
#include <limits.h>
#include "LFO.h"

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

int LFO::tickDacVal() {
  // progress the wave
  currentValue += rising ? periodIncrement[0] : -periodIncrement[1];
  if (currentValue >= scaledDacResolution) {
    currentValue = scaledDacResolution - (currentValue - scaledDacResolution);
    rising = false;
  } else if (currentValue <= 0) {
    currentValue = -currentValue;
    rising = true;
  }

  // update the DAC value
  int currentValueDescaled = currentValue >> scalingFactor;
  return triangleWaveSelected ?
    constrain(currentValueDescaled, 0, DAC_RES) :
    rising ? DAC_RES : 0;
}

void LFO::check(bool usingClockIn) {
  rangeSwitch.check();
  waveSwitch.check();

  int freq = analogRead(freqInPin);
  int dutyCycle = analogRead(dutyInPin);

  // if using external clock input
  if (usingClockIn) {
    // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
    int coefficient = clockDivMultOptions[freq / knobRange];
    long newPeriod = freq <= ADC_RES / 2 ? clockPeriod * coefficient : clockPeriod / coefficient;
    period = constrain(newPeriod, highFastestPeriod, lowSlowestPeriod);
  } else {
    // set LFO period based on frequency knob
    long slowestPeriod = highRange ? highSlowestPeriod : lowSlowestPeriod;
    long fastestPeriod = highRange ? highFastestPeriod : lowFastestPeriod;
    // make sure freq -> period mapping doesn't overflow
    long periodInterpolation = multKnobWithoutOverflow(slowestPeriod - fastestPeriod, freq);
    period = slowestPeriod - periodInterpolation;
  }

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

bool knobChanged(int thisKnob, int lastKnob) {
  const int minKnobDiff = 5;
  return thisKnob < lastKnob - minKnobDiff || thisKnob > lastKnob + minKnobDiff;
}

long multKnobWithoutOverflow(long valA, long valB) {
  return valB && valA > LONG_MAX / valB ?
    valA / ADC_RES * valB :
    valA * valB / ADC_RES;
}
