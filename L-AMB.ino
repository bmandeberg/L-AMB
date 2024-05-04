#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

Adafruit_MCP4728 mcp;
const int DAC_RESOLUTION = pow(2, 12) - 1;
const int clockInPin = 7;
const int clockSelectPin = 9;
const int led1Pin = 10;
const int led2Pin = 11;
const int led3Pin = 12;
const int led4Pin = 13;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 12;

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

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  // pinMode(led3Pin, OUTPUT);
  // pinMode(led4Pin, OUTPUT);

  Callback toggleClockCallback;
  toggleClockCallback.type = CallbackType::FUNCTION;
  toggleClockCallback.cb.fn = toggleClockSelected;
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 15, 16, 14, 8, MCP4728_CHANNEL_A);
  // lfo2.setup(A2, A3, 6, 4, 0, 1, MCP4728_CHANNEL_B);
  // TODO: access extra pins on the 32u4! Can't use lfo3 until that happens
  // lfo3.setup(A4, A5, 5, PE2, PB0, PD5, MCP4728_CHANNEL_C);
}

void loop() {
  clockSelectSwitch.check();
  lfo1.update();
  // lfo2.update();
  // lfo3.update();
  updateLEDs();
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}

void updateLEDs() {
  float lfo1Value = lfo1.currentValue();
  float inverseLfo1Value = 1.0 - lfo1Value;
  // float lfo2Value = lfo2.currentValue();
  // float inverseLfo2Value = 1.0 - lfo2Value;
  // float lfo3Value = lfo3.currentValue();
  // float inverseLfo3Value = 1.0 - lfo3Value;
  // lfo2Value *= lfo3Value;
  // inverseLfo2Value *= lfo3Value;
  // lfo1Value *= lfo2Value;
  // inverseLfo1Value *= lfo2Value;
  analogWrite(led1Pin, lfo1Value * 255);
  analogWrite(led2Pin, inverseLfo1Value * 255);
  // analogWrite(led3Pin, inverseLfo2Value * 255);
  // analogWrite(led4Pin, inverseLfo3Value * 255);
}