#include <Adafruit_ZeroTimer.h>
#include <I2C_DMAC.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

#define MCP4728_I2CADDR_DEFAULT 0x64
#define TIMER_NUM 3

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int clockInPin = 1;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long lfoUpdateResolution = 50; // in microseconds. LFOs update at 20KHz
const int maxDivMult = 9;
static const int numOptions = (maxDivMult - 1) * 2 + 1;
const int knobRange = ADC_RES / numOptions;
int clockDivMultOptions[numOptions];
bool lastUsingClockIn = false;
uint8_t i2cBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

Adafruit_ZeroTimer timer = Adafruit_ZeroTimer(TIMER_NUM);
void TC3_Handler() {
  Adafruit_ZeroTimer::timerHandler(TIMER_NUM);
}

// tick LFOs within the ISR
void tickLFOs() {
  fillBuffer(0, lfo1.tick());
  fillBuffer(1, lfo2.tick());
  fillBuffer(2, lfo3.tick());

  I2C.write(); // in parallel via DMA, takes about 40 micros
}

// check LFO inputs, takes about 162 micros
void checkLFOs(bool usingClock) {
  lfo1.check(usingClock);
  lfo2.check(usingClock);
  lfo3.check(usingClock);
}

void setup() {
  initializeClockDivMultOptions();

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A0, A1, 2, 3);
  lfo2.setup(A2, A3, 4, 5);
  lfo3.setup(A4, A5, 7, 9);
  checkLFOs(false);

  // initialize I2C for communicating with DAC via DMA
  I2C.begin(3400000);
  I2C.initWriteBytes(MCP4728_I2CADDR_DEFAULT, i2cBuffer, 8);

  // setup main clock for ticking LFOs
  timer.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  timer.setCompare(0, F_CPU / 2500000 * lfoUpdateResolution);
  timer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, tickLFOs);
  timer.enable(true);
}

void loop() {
  bool usingClock = usingClockIn();
  // disable external clock use if it is idled
  if (usingClock && micros() - lastClockTime > lowSlowestPeriod) {
    clockPeriod = 0;
    usingClock = false;
  }

  checkLFOs(usingClock);

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

void resetLFOs() {
  lfo1.reset();
  lfo2.reset();
  lfo3.reset();
}

bool usingClockIn() {
  return clockPeriod > highFastestPeriod && clockPeriod < lowSlowestPeriod;
}

void updateClockPeriod() {
  if (lastClockTime) {
    clockPeriod = micros() - lastClockTime;

    if (usingClockIn()) {
      resetLFOs();
    }
  }
  lastClockTime = micros();
}

void fillBuffer(int position, int value) {
  int index = position * 2;
  i2cBuffer[index] = value >> 8;
  i2cBuffer[index + 1] = value & 0xFF;
}