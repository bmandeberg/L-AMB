#include <Arduino.h>
#include <Adafruit_ZeroTimer.h>
#include <I2C_DMAC.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

#define MCP4728_I2CADDR_DEFAULT 0x64

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int clockInPin = 0;
const int clockSelectPin = 1;
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
uint8_t i2cBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

// Adafruit_ZeroTimer zt5 = Adafruit_ZeroTimer(5);
// void TC5_Handler() {
//   Adafruit_ZeroTimer::timerHandler(5);
// }

// tick LFOs within the ISR
void tickLFOs() {
  fillBuffer(0, lfo1.tickDacVal());
  fillBuffer(1, lfo2.tickDacVal());
  fillBuffer(2, lfo3.tickDacVal());
  I2C.write(); // in parallel, takes about 40 micros
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
  initializeClockDivMultOptions();

  Callback toggleClockCallback(toggleClockSelected);
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 24, 25);
  lfo2.setup(A2, A3, 23, 2);
  lfo3.setup(A4, A5, 3, 4);
  checkLFOs();

  // initialize I2C for communicating with DAC
  I2C.begin(3400000);
  I2C.initWriteBytes(MCP4728_I2CADDR_DEFAULT, i2cBuffer, 8);

  // setup main clock for ticking LFOs
  // zt5.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  // zt5.setCompare(0, clockResolution * 120);
  // zt5.setCallback(true, TC_CALLBACK_CC_CHANNEL0, tickLFOs);
  // zt5.enable(true);
}

long time = 0;

void loop() {
  clockSelectSwitch.check();

  time = micros();
  tickLFOs();
  long elapsed = micros() - time;
  Serial.println(elapsed);
  delay(1000);
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;
  }
  lastClockTime = micros();
}

// check LFO inputs
void checkLFOs() {
  bool usingClockIn = clockSelected && clockPeriod > minClockPeriod;
  lfo1.check(usingClockIn);
  lfo2.check(usingClockIn);
  lfo3.check(usingClockIn);

  lastUsingClockIn = usingClockIn;
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

void fillBuffer(int position, int value) {
  int index = position * 2;
  i2cBuffer[index] = value >> 8;
  i2cBuffer[index + 1] = value & 0xFF;
}