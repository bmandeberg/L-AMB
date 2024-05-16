#include <Adafruit_ZeroTimer.h>
#include <I2C_DMAC.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

#define MCP4725_I2CADDR_DEFAULT 0x62
#define MCP4725_I2CADDR_ALT 0x63
#define TIMER_NUM 3

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int clockInPin = 1;
const int clockSelectPin = 5;
volatile bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long clockResolution = 25; // clock updates at 40KHz
const int maxDivMult = 9;
static const int numOptions = (maxDivMult - 1) * 2 + 1;
const int knobRange = ADC_RES / numOptions;
int clockDivMultOptions[numOptions];
bool lastUsingClockIn = false;

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

// I2C_DMAC I2C1(&sercom3, 13, 12);

Adafruit_ZeroTimer timer = Adafruit_ZeroTimer(TIMER_NUM);
void TC3_Handler() {
  Adafruit_ZeroTimer::timerHandler(TIMER_NUM);
}

// tick LFOs within the ISR
void tickLFOs() {
  lfo1.tick();
  // lfo2.tick();
  // lfo3.tick();
}

void toggleClockSelected() {
  clockSelected = !clockSelected;
  if (!clockSelected) {
    lastClockTime = 0;
    clockPeriod = 0;
  }
}

void setup() {
  initializeClockDivMultOptions();

  Callback toggleClockCallback(toggleClockSelected);
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  // setup second I2C
  // pinPeripheral(13, PIO_SERCOM_ALT);
  // pinPeripheral(12, PIO_SERCOM_ALT);
  // I2C1.setWriteChannel(2);
  // I2C1.setReadChannel(3);

  lfo1.setup(A1, A2, 24, 25);
  // lfo2.setup(A3, A4, 23, 3, MCP4725_I2CADDR_DEFAULT, &I2C);
  // lfo3.setup(A5, A6, 4, 0, MCP4725_I2CADDR_ALT, &I2C1);
  checkLFOs();

  // setup main clock for ticking LFOs
  timer.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  timer.setCompare(0, F_CPU / 2500000 * clockResolution);
  timer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, tickLFOs);
  timer.enable(true);
}

void loop() {
  clockSelectSwitch.check();
  checkLFOs();
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

// check LFO inputs, takes about 162 micros
void checkLFOs() {
  bool usingClock = usingClockIn();
  lfo1.check(usingClock);
  // lfo2.check(usingClock);
  // lfo3.check(usingClock);

  lastUsingClockIn = usingClock;
}

void resetLFOs() {
  lfo1.reset();
  // lfo2.reset();
  // lfo3.reset();
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
  return clockSelected && clockPeriod > highFastestPeriod && clockPeriod < lowSlowestPeriod;
}
