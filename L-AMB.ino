#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"
#include <arduino-timer.h>

Adafruit_MCP4728 mcp;
const int DAC_RESOLUTION = 4095;
const int ADC_RESOLUTION = 1023;
const int PWM_RESOLUTION = 255;
const int clockInPin = 7;
const int clockSelectPin = 9;
const int led1Pin = 10;
const int led2Pin = 11;
const int led3Pin = 12;
const int led4Pin = 13;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 250;
const long clockResolution = 25;

auto timer = timer_create_default();

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

void toggleClockSelected() {
  clockSelected = !clockSelected;
  if (!clockSelected) {
    lastClockTime = 0;
    clockPeriod = 0;
  }
}

void setup() {
  mcp.begin();
  Serial.begin(9600);

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  // pinMode(led3Pin, OUTPUT);
  // pinMode(led4Pin, OUTPUT);

  Callback toggleClockCallback(toggleClockSelected);
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 15, 16, 14, 8, MCP4728_CHANNEL_A);
  // lfo2.setup(A2, A3, 6, 4, 0, 1, MCP4728_CHANNEL_B);
  // TODO: access extra pins on the 32u4! Can't use lfo3 until that happens
  // lfo3.setup(A4, A5, 5, PE2, PB0, PD5, MCP4728_CHANNEL_C);
}

long time = 0;

void loop() {
  clockSelectSwitch.check();
  time = micros();
  checkLFOs();
  Serial.println(micros() - time);
  updateLEDs();
  delay(1000);
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}

void updateLEDs() {
  int lfo1Value = lfo1.getValue();
  int inverseLfo1Value = DAC_RESOLUTION - lfo1Value;
  // float lfo2Value = lfo2.getValue();
  // float inverseLfo2Value = DAC_RESOLUTION - lfo2Value;
  // float lfo3Value = lfo3.getValue();
  // float inverseLfo3Value = DAC_RESOLUTION - lfo3Value;
  // lfo2Value = lfo2Value * lfo3Value / DAC_RESOLUTION;
  // inverseLfo2Value = inverseLfo2Value * lfo3Value / DAC_RESOLUTION;
  // lfo1Value = lfo1Value * lfo2Value / DAC_RESOLUTION;
  // inverseLfo1Value = inverseLfo1Value * lfo2Value / DAC_RESOLUTION;
  analogWrite(led1Pin, scaleFromDACtoPWM(lfo1Value));
  analogWrite(led2Pin, scaleFromDACtoPWM(inverseLfo1Value));
  // analogWrite(led3Pin, scaleFromDACtoPWM(inverseLfo2Value));
  // analogWrite(led4Pin, scaleFromDACtoPWM(inverseLfo3Value));
}

int scaleFromDACtoPWM(int value) {
  return value * PWM_RESOLUTION / DAC_RESOLUTION;
}

// check input at a rate of 1ms
void checkLFOs() {
  lfo1.check();
  // lfo2.check();
  // lfo3.check();
}

// tick LFOs within the ISR
void tickLFOs() {
  lfo1.tick();
  // lfo2.tick();
  // lfo3.tick();
}

// write LFOs to the DAC
void updateLFOs() {
  lfo1.update();
  // lfo2.update();
  // lfo3.update();
}