#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

void setup() {
  mcp.begin();
  // DAC channel D is set to 0.5V, which is added to the final output to center it in the 6Vpp analog range
  mcp.setChannelValue(MCP4728_CHANNEL_D, DAC_RESOLUTION / 10);

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
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}