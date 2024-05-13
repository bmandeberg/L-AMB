#include <Arduino.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"
#include "Adafruit_ZeroTimer.h"

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int clockInPin = 1;
const int clockSelectPin = 5;
bool clockSelected = false;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long minClockPeriod = 250;
const long clockResolution = 25;
const int maxDivMult = 9;
static const int numOptions = (maxDivMult - 1) * 2 + 1;
const int knobRange = ADC_RES / numOptions;
int clockDivMultOptions[numOptions];
bool lastUsingClockIn = false;

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

Adafruit_ZeroTimer zt5 = Adafruit_ZeroTimer(5);
void TC5_Handler(){
  Adafruit_ZeroTimer::timerHandler(5);
}

// tick LFOs within the ISR
void tickLFOs() {
  lfo1.tick();
  lfo2.tick();
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
  Serial.begin(9600);
  initializeClockDivMultOptions();

  Callback toggleClockCallback(toggleClockSelected);
  clockSelectSwitch.setup(clockSelectPin, false, true, toggleClockCallback, toggleClockCallback);

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  lfo1.setup(A2, A3, 24, 25, 0);
  lfo2.setup(A4, A5, 23, 3, 1);
  // TODO: access extra pins! Can't use lfo3 until that happens, need extra analog in
  // lfo3.setup(A6, A7, 4, 0, -1);

  // initialize DAC
  analogWriteResolution(12);
  analogWrite(A0, DAC_RES / 2);
  analogWrite(A1, DAC_RES / 2);

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
  // lfo3.check(usingClockIn);

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