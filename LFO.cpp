#include "LFO.h"
#include "Arduino.h"
#include <limits.h>

void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin) {
  freqInPin = freqPin;
  dutyInPin = dutyPin;
  waveSwitchPin = wavePin;
  rangeSwitchPin = rangePin;

  Callback setHighCallback(&LFO::setHigh, this);
  Callback setLowCallback(&LFO::setLow, this);
  rangeSwitch.setup(rangeSwitchPin, false, false, setHighCallback, setLowCallback);
  highRange = digitalRead(rangeSwitchPin) == HIGH;
  
  Callback toggleWaveCallback(&LFO::toggleWave, this);
  waveSwitch.setup(waveSwitchPin, false, false, toggleWaveCallback, toggleWaveCallback);
  triangleWaveSelected = digitalRead(waveSwitchPin) == HIGH;
  // TODO: remove this test code
  triangleWaveSelected = true;
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

  // update the DAC value
  if (triangleWaveSelected) {
    // triangle
    int currentValueDescaled = currentValue >> scalingFactor;
    dacValue = constrain(currentValueDescaled, 0, DAC_RES);
  } else if (rising != lastRising) {
    // pulse
    dacValue = rising ? DAC_RES : 0;
  }

  lastRising = rising;
}

void LFO::check() {
  rangeSwitch.check();
  waveSwitch.check();
  
  // set LFO period
  int freq = analogRead(freqInPin);
  // if using external clock input
  if (usingClockIn() && clockPeriod != lastClockPeriod) {
    // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
    int coefficient = clockDivMultOptions[freq / knobRange];
    period = freq <= ADC_RES / 2 ? clockPeriod * coefficient : clockPeriod / coefficient;
  } else {
    // set LFO period based on frequency knob
    long slowestPeriod = highRange ? highSlowestPeriod : lowSlowestPeriod;
    long fastestPeriod = highRange ? highFastestPeriod : lowFastestPeriod;
    period = map(freq, 0, ADC_RES, slowestPeriod, fastestPeriod);
  }

  // set LFO duty cycle
  int dutyCycle = analogRead(dutyInPin);

  // if anything has changed, update the periodIncrement and triangle value
  if (period != lastPeriod || dutyCycle != lastDutyCycle) {
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
  }

  lastPeriod = period;
  lastDutyCycle = dutyCycle;

  // TODO: remove this test code
  period = 1000000;
  dutyCycle = ADC_RES / 2;
}

bool LFO::usingClockIn() {
  return clockSelected && clockPeriod > minClockPeriod;
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

void LFO::toggleWave() {
  triangleWaveSelected = !triangleWaveSelected;
}
