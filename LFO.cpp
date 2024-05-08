#include "LFO.h"
#include "Arduino.h"
#include <limits.h>

void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int squarePinOut) {
  freqInPin = freqPin;
  dutyInPin = dutyPin;
  waveSwitchPin = wavePin;
  rangeSwitchPin = rangePin;
  rangeOutPin = rangePinOut;
  squareOutPin = squarePinOut;
  pinMode(rangeOutPin, OUTPUT);
  pinMode(squareOutPin, OUTPUT);
  digitalWrite(squareOutPin, LOW);

  switch (squareOutPin) {
    case 8:
      port = PORTB;
      bit = 4;
      break;
    case 13:
      port = PORTC;
      bit = 7;
      break;
  }

  Callback setHigh(&LFO::setHigh, this);
  Callback setLow(&LFO::setLow, this);
  rangeSwitch.setup(rangeSwitchPin, false, false, setHigh, setLow);
  this->_setRange(digitalRead(rangeSwitchPin) == HIGH);
  
  Callback toggleWave(&LFO::toggleWave, this);
  waveSwitch.setup(waveSwitchPin, false, false, toggleWave, toggleWave);
  // triangleWaveSelected = digitalRead(waveSwitchPin) == HIGH;
  // TODO: remove this test code
  triangleWaveSelected = true;
}

void LFO::tick() {
  currentValue += rising ? periodIncrement : -periodIncrement;
  if (currentValue >= scaledDacResolution) {
    currentValue = scaledDacResolution - (currentValue - scaledDacResolution);
    rising = false;
  } else if (currentValue <= 0) {
    currentValue = -currentValue;
    rising = true;
  }
}

void LFO::check() {
  rangeSwitch.check();
  waveSwitch.check();
  
  // set LFO period
  int freq = analogRead(freqInPin);
  // if using external clock input
  if (this->_usingClockIn()) {
    // automatically check range based on clock frequency
    if (clockPeriod > crossoverPeriod && highRange) {
      this->_setRange(false);
    } else if (clockPeriod < crossoverPeriod && !highRange) {
      this->_setRange(true);
    }

    // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
    int maxDivMult = 9;
    int numOptions = (maxDivMult - 1) * 2 + 1;
    int knobRange = ADC_RESOLUTION / numOptions;
    int knobIndex = freq / knobRange;
    int options[numOptions];
    bool descending = true;
    int optionIndex = 0;
    for (int i = maxDivMult; i < maxDivMult + 1; descending ? i-- : i++) {
      if (i < 2) {
        descending = false;
      }
      options[optionIndex] = i;
      optionIndex++;
    }
    int coefficient = options[knobIndex];
    period = freq < ADC_RESOLUTION / 2 ? clockPeriod * coefficient : clockPeriod / coefficient;
  } else {
    // set LFO period based on frequency knob
    long slowestPeriod = highRange ? highSlowestPeriod : lowSlowestPeriod;
    long fastestPeriod = highRange ? highFastestPeriod : lowFastestPeriod;
    period = map(freq, 0, ADC_RESOLUTION, slowestPeriod, fastestPeriod);
  }

  // set LFO duty cycle
  int dutyRead = analogRead(dutyInPin);
  int mappedDutyCycle = map(dutyRead, 0, ADC_RESOLUTION, minDutyCycle, maxDutyCycle);
  dutyCycle = constrain(mappedDutyCycle, minDutyCycle, maxDutyCycle);

  // TODO: remove this test code
  period = 1000000;
  dutyCycle = ADC_RESOLUTION / 2;
}

int LFO::update() {
  int dacValue = lastDacValue;
  noInterrupts();
  risingCopy = rising;
  currentValueCopy = currentValue;
  interrupts();

  // reset automatic range selection if clock input is removed
  if (lastClockSelected != clockSelected && !this->_usingClockIn()) {
    this->_setRange(digitalRead(rangeSwitchPin) == HIGH);
  }
  
  // if anything has changed, update the periodIncrement and triangle value
  if (period != lastPeriod || dutyCycle != lastDutyCycle || risingCopy != lastRising) {
    periodIncrementCopy = this->_calculatePeriodIncrement();
    if (!ALL_DIGITAL) {
      dacValue = this->_getPulseForTriangle();
    }
  }

  if (ALL_DIGITAL) {
    if (triangleWaveSelected) {
      // triangle
      int currentValueDescaled = currentValueCopy / scalingFactor;
      dacValue = constrain(currentValueDescaled, 0, DAC_RESOLUTION);
    } else if (risingCopy != lastRising) {
      // pulse
      dacValue = risingCopy ? DAC_RESOLUTION : 0;
    }
  } else if (risingCopy != lastRising) {
    // update direct pulse wave output
    if (risingCopy) {
      port |= 1 << bit;
    } else {
      port &= ~(1 << bit);
    }
  }

  noInterrupts();
  periodIncrement = periodIncrementCopy;
  interrupts();

  lastPeriod = period;
  lastDutyCycle = dutyCycle;
  lastClockSelected = clockSelected;
  lastRising = risingCopy;
  lastDacValue = dacValue;

  return dacValue;
}

// calculate either Vtop or Vbottom of pulse wave that hits an integrator to create the variable-duty-cycle triangle wave
int LFO::_getPulseForTriangle() {
  float targetVpp = 1.0;
  int halfDac = DAC_RESOLUTION / 2;
  int dacValue = halfDac;
  float V = risingCopy ? targetVpp : -targetVpp;
  float C = highRange ? highC : lowC;
  float freq = 1 / ((float)period * 1.0e-6);
  float duty = constrain(this->_currentDuty() / (float)ADC_RESOLUTION, 0.1, 0.9);
  float voltage = V * R * C * freq / duty;
  dacValue += voltage * 0.2 * DAC_RESOLUTION;
  return constrain(dacValue, 0, DAC_RESOLUTION);
}

bool LFO::_usingClockIn() {
  return clockSelected && clockPeriod > minClockPeriod;
}

void LFO::_setRange(bool rangeHigh) {
  highRange = rangeHigh;
  digitalWrite(rangeOutPin, rangeHigh ? HIGH : LOW);
}

// current duty cycle, scaled to the ADC resolution
int LFO::_currentDuty() {
  return risingCopy ? dutyCycle : ADC_RESOLUTION - dutyCycle;
}

// current value of the LFO, scaled to the DAC resolution
int LFO::getValue() {
  if (!triangleWaveSelected) {
    // pulse
    return risingCopy ? DAC_RESOLUTION : 0;
  } else {
    // triangle
    int currentValueDescaled = currentValueCopy / scalingFactor;
    return constrain(currentValueDescaled, 0, DAC_RESOLUTION);
  }
}

long LFO::_calculatePeriodIncrement() {
  int duty = this->_currentDuty();
  // make sure period * duty doesn't overflow
  long dutyPeriod = duty && period > LONG_MAX / duty ?
    period / ADC_RESOLUTION * duty :
    period * duty / ADC_RESOLUTION;
  long dutyPeriodIncrement = dutyPeriod / clockResolution;
  return scaledDacResolution / max(dutyPeriodIncrement, 1);
}

void LFO::setHigh() {
  if (!this->_usingClockIn()) {
    this->_setRange(true);
  }
}

void LFO::setLow() {
  if (!this->_usingClockIn()) {
    this->_setRange(false);
  }
}

void LFO::toggleWave() {
  triangleWaveSelected = !triangleWaveSelected;
}
