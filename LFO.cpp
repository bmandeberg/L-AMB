#include "L-AMB.h"
#include "LFO.h"
#include "Arduino.h"

void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int dacChan) {
  freqInPin = freqPin;
  dutyInPin = dutyPin;
  waveSwitchPin = wavePin;
  rangeSwitchPin = rangePin;
  rangeOutPin = rangePinOut;
  dacChannel = dacChan;
  pinMode(rangeOutPin, OUTPUT);

  callback setHigh = [this]() {
    if (!this->_usingClockIn()) {
      this->_setHighRange(true);
    }
  };
  callback setLow = [this]() {
    if (!this->_usingClockIn()) {
      this->_setHighRange(false);
    }
  };
  rangeSwitch.setup(rangeSwitchPin, false, setHigh, setLow);
  this->_setHighRange(digitalRead(rangeSwitchPin) == HIGH);
  
  callback toggleWave = [this]() {
    triangleWaveSelected = !triangleWaveSelected;
  };
  waveSwitch.setup(waveSwitchPin, false, toggleWave, toggleWave);
  triangleWaveSelected = digitalRead(waveSwitchPin) == HIGH;
  lastTriangleWaveSelected = triangleWaveSelected;
}

void LFO::update() {
  rangeSwitch.check();

  // reset automatic range selection if clock input is removed
  if (lastClockSelected != clockSelected && !this->_usingClockIn()) {
    this->_setHighRange(digitalRead(rangeSwitchPin) == HIGH);
  }
  
  // set LFO period
  int freq = analogRead(freqInPin);
  // if using external clock input
  if (this->_usingClockIn()) {
    // automatically update range based on clock frequency
    long crossoverPeriod = (lowFastestPeriod - highSlowestPeriod) * 0.5;
    if (clockPeriod > crossoverPeriod && highRange) {
      this->_setHighRange(false);
    } else if (clockPeriod < crossoverPeriod && !highRange) {
      this->_setHighRange(true);
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
    period = freq < ADC_RESOLUTION * 0.5 ? clockPeriod * coefficient : clockPeriod / coefficient;
  } else {
    long slowestPeriod = highRange ? highSlowestPeriod : lowSlowestPeriod;
    long fastestPeriod = highRange ? highFastestPeriod : lowFastestPeriod;
    period = map(freq, 0, ADC_RESOLUTION, slowestPeriod, fastestPeriod);
  }

  // set LFO duty cycle
  dutyCycle = floatMap((float)analogRead(dutyInPin), 0.0, (float)ADC_RESOLUTION, 0.1, 0.9);

  bool changed = period != lastPeriod || dutyCycle != lastDutyCycle || triangleWaveSelected != lastTriangleWaveSelected;
  if (changed) {
    this->_writeCycle(true);
  }

  lastPeriod = period;
  lastDutyCycle = dutyCycle;
  lastClockSelected = clockSelected;
  lastTriangleWaveSelected = triangleWaveSelected;
  timer.tick();
}

void LFO::_write(float targetVpp) {
  int dacValue = 0;
  int halfDac = DAC_RESOLUTION * 0.5;
  if (triangleWaveSelected) {
    // calculate pulse voltage, either top or bottom
    float pwm = rising ? dutyCycle : 1.0 - dutyCycle;
    float V = rising ? targetVpp : -targetVpp;
    float C = highRange ? highC : lowC;
    float freq = 1 / ((float)period * 1.0e-6);
    float voltage = V * R * C * freq / pwm;
    dacValue = halfDac + voltage * 0.2 * DAC_RESOLUTION;
  } else {
    // 1Vpp square wave
    dacValue = halfDac + DAC_RESOLUTION * 0.1 * (rising ? 1 : -1);
  }
  mcp.setChannelValue(dacChannel, constrain(dacValue, 0, DAC_RESOLUTION));
}

void LFO::_setTimer(long delay) {
  timer.in(delay, [this](void*) -> bool {
    this->_writeCycle(false);
    return false;
  });
}

void LFO::_writeCycle(bool updatePeriod) {
  float targetVpp = 1.0;
  float duty = this->_currentDuty();
  long cyclePeriod = period * duty;
  long timerPeriod = cyclePeriod;

  if (updatePeriod) {
    auto ticksLeft = timer.ticks();
    timer.cancel();
    long lastCyclePeriod = lastPeriod * duty;
    float percentLeft = (float)ticksLeft / (float)lastCyclePeriod;
    bool alreadyGoneTooFar = lastCyclePeriod - ticksLeft > cyclePeriod;
    long newTicksLeft = alreadyGoneTooFar ? cyclePeriod : cyclePeriod * percentLeft;

    if (alreadyGoneTooFar) {
      rising = !rising;
      targetVpp *= 1.0 - percentLeft;
    } else {
      timerPeriod = cyclePeriod * percentLeft;
    }
  } else {
    rising = !rising;
    timerPeriod = period * this->_currentDuty();
  }

  this->_setTimer(timerPeriod);
  this->_write(targetVpp);
}

bool LFO::_usingClockIn() {
  return clockSelected && clockPeriod > minClockPeriod;
}

void LFO::_setHighRange(bool rangeHigh) {
  highRange = rangeHigh;
  digitalWrite(rangeOutPin, rangeHigh ? HIGH : LOW);
}

float LFO::_currentDuty() {
  return duty = rising ? dutyCycle : 1 - dutyCycle;
}

float LFO::currentValue() {
  if (!triangleWaveSelected) {
    return rising ? 1.0 : 0.0;
  } else {
    auto ticksLeft = timer.ticks();
    long cyclePeriod = period * this->_currentDuty();
    float percentLeft = (float)ticksLeft / (float)cyclePeriod;
    return rising ? 1.0 - percentLeft : percentLeft;
  }
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}