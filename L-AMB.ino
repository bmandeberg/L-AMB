#include <I2C_DMAC.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

#define MCP4728_I2CADDR_DEFAULT 0x64
#define TIMER_NUM 3

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int clockInPin = 10;
const int clockSelectPin = 11;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 250;
const long clockResolution = 50; // clock updates at 20KHz
const int maxDivMult = 9;
static const int numOptions = (maxDivMult - 1) * 2 + 1;
const int knobRange = ADC_RES / numOptions;
int clockDivMultOptions[numOptions];
bool lastUsingClockIn = false;

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

// tick LFOs within the ISR
void tickLFOs() {
  // int dacVal = lfo1.tickDacVal();
  // Serial.println(dacVal);
  lfo1.tickDacVal();
}

void toggleClockSelected() {
  clockSelected = !clockSelected;
  if (!clockSelected) {
    lastClockTime = 0;
    clockPeriod = 0;
  }
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  initializeClockDivMultOptions();

  Callback toggleClockCallback(toggleClockSelected);
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 2, 3);
  // lfo2.setup(A2, A3, 4, 5);
  // lfo3.setup(A4, A5, 7, 9);
  checkLFOs();
}

long time = 0;

void loop() {
  clockSelectSwitch.check();
  time = micros();
  checkLFOs();
  long elapsed = micros() - time;
  Serial.println(elapsed);
  tickLFOs();
  delay(500);
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}

// check LFO inputs, takes about 162 micros
void checkLFOs() {
  bool usingClock = usingClockIn();
  lfo1.check(usingClock);
  // lfo2.check(usingClock);
  // lfo3.check(usingClock);

  lastUsingClockIn = usingClock;
}

void initializeClockDivMultOptions() {
  // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
  bool descending = true;
  int optionIndex = 0;
  for (int i = maxDivMult; i < maxDivMult + 1; descending ? i-- : i++) {
    if (i < 2) {
      descending = false;
    }
    clockDivMultOptions[optionIndex] = i;
    optionIndex++;
  }
}

bool usingClockIn() {
  return clockSelected && clockPeriod > minClockPeriod;
}
