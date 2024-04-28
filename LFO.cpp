#include "LFO.h"
#include "Arduino.h"

LFO* LFO::instance = nullptr;

void LFO::setup(int freqPin, int dutyPin, int wavePin, int rangePin, int rangePinOut, int dacChan) {
  freqInPin = freqPin;
  dutyInPin = dutyPin;
  waveSwitchPin = wavePin;
  rangeSwitchPin = rangePin;
  rangeOutPin = rangePinOut;
  dacChannel = dacChan;
  pinMode(rangeOutPin, OUTPUT);

  Callback setHigh;
  setHigh.type = CallbackType::MEMBER_FUNCTION;
  setHigh.cb.bound.obj = this;
  setHigh.cb.bound.mfunc = &LFO::setHigh;
  Callback setLow;
  setLow.type = CallbackType::MEMBER_FUNCTION;
  setLow.cb.bound.obj = this;
  setLow.cb.bound.mfunc = &LFO::setLow;
  rangeSwitch.setup(rangeSwitchPin, false, setHigh, setLow);
  this->_setRange(digitalRead(rangeSwitchPin) == HIGH);
  
  Callback toggleWave;
  toggleWave.type = CallbackType::MEMBER_FUNCTION;
  toggleWave.cb.bound.obj = this;
  toggleWave.cb.bound.mfunc = &LFO::toggleWave;
  waveSwitch.setup(waveSwitchPin, false, toggleWave, toggleWave);
  triangleWaveSelected = digitalRead(waveSwitchPin) == HIGH;
  lastTriangleWaveSelected = triangleWaveSelected;
}

void LFO::update() {
  rangeSwitch.check();
  waveSwitch.check();

  // reset automatic range selection if clock input is removed
  if (lastClockSelected != clockSelected && !this->_usingClockIn()) {
    this->_setRange(digitalRead(rangeSwitchPin) == HIGH);
  }
  
  // set LFO period
  int freq = analogRead(freqInPin);
  // if using external clock input
  if (this->_usingClockIn()) {
    // automatically update range based on clock frequency
    long crossoverPeriod = (lowFastestPeriod - highSlowestPeriod) * 0.5;
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
    period = freq < ADC_RESOLUTION * 0.5 ? clockPeriod * coefficient : clockPeriod / coefficient;
  } else {
    // set LFO period based on frequency knob
    long slowestPeriod = highRange ? highSlowestPeriod : lowSlowestPeriod;
    long fastestPeriod = highRange ? highFastestPeriod : lowFastestPeriod;
    period = map(freq, 0, ADC_RESOLUTION, slowestPeriod, fastestPeriod);
  }

  // set LFO duty cycle
  float minDutyCycle = 0.1;
  float maxDutyCycle = 0.9;
  float mappedDutyCycle = floatMap((float)analogRead(dutyInPin), 0.0, (float)ADC_RESOLUTION, minDutyCycle, maxDutyCycle);
  dutyCycle = constrain(mappedDutyCycle, minDutyCycle, maxDutyCycle);

  // schedule DAC update if necessary
  if (period != lastPeriod || dutyCycle != lastDutyCycle || triangleWaveSelected != lastTriangleWaveSelected) {
    this->writeCycle(true);
  }

  lastPeriod = period;
  lastDutyCycle = dutyCycle;
  lastClockSelected = clockSelected;
  lastTriangleWaveSelected = triangleWaveSelected;
  timer.tick();
}

// set DAC output
void LFO::_write(float targetVpp) {
  int halfDac = DAC_RESOLUTION * 0.5;
  int dacValue = halfDac;
  if (triangleWaveSelected) {
    // calculate pulse voltage (either top or bottom) that is fed to the integrator
    float pwm = rising ? dutyCycle : 1.0 - dutyCycle;
    float V = rising ? targetVpp : -targetVpp;
    float C = highRange ? highC : lowC;
    float freq = 1 / ((float)period * 1.0e-6);
    float voltage = V * R * C * freq / pwm;
    dacValue += voltage * 0.2 * DAC_RESOLUTION;
  } else {
    // 1Vpp square wave
    dacValue += DAC_RESOLUTION * 0.1 * (rising ? 1 : -1);
  }
  mcp.setChannelValue(dacChannel, constrain(dacValue, 0, DAC_RESOLUTION));
}

bool LFO::timerCallback(void *arg) {
  if (instance) {
    instance->writeCycle(false);
  }
  return false;
}

void LFO::_setTimer(long delay) {
  instance = this;
  timer.in(delay, timerCallback);
}

// one "cycle" is either the duty cycle or the inverse of the duty cycle (either rising or falling part of the wave)
void LFO::writeCycle(bool updatePeriod) {
  float targetVpp = 1.0;
  float duty = this->_currentDuty();
  long cyclePeriod = period * duty;
  long timerPeriod = cyclePeriod;

  // cancel and restart scheduled DAC update if period changes
  if (updatePeriod) {
    auto ticksLeft = timer.ticks();
    timer.cancel();
    long lastCyclePeriod = lastPeriod * duty;
    float percentLeft = (float)ticksLeft / (float)lastCyclePeriod;
    bool alreadyGoneTooFar = lastCyclePeriod - ticksLeft > cyclePeriod;

    if (alreadyGoneTooFar) {
      rising = !rising;
      timerPeriod = period * this->_currentDuty();
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

void LFO::_setRange(bool rangeHigh) {
  highRange = rangeHigh;
  digitalWrite(rangeOutPin, rangeHigh ? HIGH : LOW);
}

float LFO::_currentDuty() {
  return rising ? dutyCycle : 1 - dutyCycle;
}

// current normalized value of the LFO
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

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}