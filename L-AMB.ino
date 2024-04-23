#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

void setup() {
  mcp.begin();
  // DAC channel D is set to 0.5V, which is added to the final output to center it in the 6Vpp analog range
  mcp.setChannelValue(MCP4728_CHANNEL_D, DAC_RESOLUTION * 0.1);

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(led4Pin, OUTPUT);

  auto toggleClockSelected = []() {
    clockSelected = !clockSelected;
    if (!clockSelected) {
      lastClockTime = 0;
      clockPeriod = 0;
    }
  };
  clockSelectSwitch.setup(clockSelectPin, true, toggleClockSelected, toggleClockSelected);
  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 15, 16, 14, MCP4728_CHANNEL_A);
  lfo2.setup(A2, A3, 8, 6, 4, MCP4728_CHANNEL_B);
  lfo3.setup(A4, A5, 0, 1, 5, MCP4728_CHANNEL_C);
}

void loop() {
  clockSelectSwitch.check();
  lfo1.update();
  lfo2.update();
  lfo3.update();
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