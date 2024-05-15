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

  bool updatePeriod = knobChanged(freq, lastFreq) || lastUsingClockIn != usingClockIn;
  if (updatePeriod) {
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
      // period = map(freq, 0, ADC_RES, slowestPeriod, fastestPeriod);
      period = map(freq, 0, ADC_RES, slowestPeriod, fastestPeriod);
      Serial.println(freq);
    }

    lastFreq = freq;
  }

  period = 1000000;
  dutyCycle = ADC_RES / 4;

  // if anything has changed, update the periodIncrement
  if (updatePeriod || knobChanged(dutyCycle, lastDutyCycle)) {
    int duty = rising ? dutyCycle : ADC_RES - dutyCycle;
    // make sure period * duty doesn't overflow
    long dutyPeriod = duty && period > LONG_MAX / duty ?
      period / ADC_RES * duty :
      period * duty / ADC_RES;
    long dutyPeriodIncrement = dutyPeriod / clockResolution;
    long oppositeDutyPeriodIncrement = (period - dutyPeriod) / clockResolution;
    periodIncrementCopy[0] = scaledDacResolution / max(dutyPeriodIncrement, 1);
    periodIncrementCopy[1] = scaledDacResolution / max(oppositeDutyPeriodIncrement, 1);

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
