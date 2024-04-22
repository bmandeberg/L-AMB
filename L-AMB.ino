#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

LFO lfo1 = LFO(A0, A1, 15, 16, 14, MCP4728_CHANNEL_A);
LFO lfo2 = LFO(A2, A3, 8, 6, 4, MCP4728_CHANNEL_B);
LFO lfo3 = LFO(A4, A5, 0, 1, 5, MCP4728_CHANNEL_C);

auto toggleClockSelected = []() {
  clockSelected = !clockSelected;
  if (!clockSelected) {
    lastClockTime = 0;
    clockPeriod = 0;
  }
};
Switch clockSelectSwitch = Switch(clockSelectPin, true, toggleClockSelected, toggleClockSelected);

void setup() {
  mcp.begin();
  mcp.setChannelValue(MCP4728_CHANNEL_D, DAC_RESOLUTION / 10);
  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);
}

void loop() {
  clockSelectSwitch.check();
  lfo1.update();
  lfo2.update();
  lfo3.update();
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}