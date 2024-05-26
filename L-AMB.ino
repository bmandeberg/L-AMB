#include <Adafruit_ZeroTimer.h>
#include <Adafruit_ZeroDMA.h>
#include <I2C_DMAC.h>
#include "L-AMB.h"
#include "Switch.h"
#include "LFO.h"

#define MCP4725_I2CADDR_DEFAULT 0x62
#define MCP4725_I2CADDR_ALT 0x63
#define TIMER_NUM 3

const int DAC_RES = 4095;
const int ADC_RES = 1023;
const int PWM_RES = 255;
const int clockInPin = 1;
volatile long clockPeriod = 0;
volatile long lastClockTime = 0;
const long clockResolution = 25; // clock updates at 40KHz
const int maxDivMult = 9;
static const int numOptions = (maxDivMult - 1) * 2 + 1;
const int knobRange = ADC_RES / numOptions;
int clockDivMultOptions[numOptions];
bool lastUsingClockIn = false;
static const int led1Pin = 5;
static const int led2Pin = 7;
static const int led3Pin = 9;

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

// I2C_DMAC I2C1(&sercom3, 13, 12);
Adafruit_ZeroDMA dma;

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

void setup() {
  initializeClockDivMultOptions();

  pinMode(clockInPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(clockInPin), updateClockPeriod, RISING);

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);

  // initialize DAC DMA
  analogWriteResolution(12);
  dma.setTrigger(TC3_DMAC_ID_OVF);
  dma.setAction(DMA_TRIGGER_ACTON_BEAT);
  dma.allocate();

  // setup second I2C
  // pinPeripheral(13, PIO_SERCOM_ALT);
  // pinPeripheral(12, PIO_SERCOM_ALT);

  lfo1.setup(A1, A2, 24, 25, 0, &dma);
  // lfo2.setup(A3, A4, 23, 3, MCP4725_I2CADDR_DEFAULT, &I2C, 2, 3);
  // lfo3.setup(A5, A6, 4, 0, MCP4725_I2CADDR_ALT, &I2C1, 4, 5);
  checkLFOs(false);

  // setup main clock for ticking LFOs
  timer.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  timer.setCompare(0, F_CPU / 2500000 * clockResolution);
  timer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, tickLFOs);
  timer.enable(true);
}

void loop() {
  bool usingClock = usingClockIn();
  // disable external clock use if it's idled
  if (usingClock && micros() - lastClockTime > lowSlowestPeriod) {
    clockPeriod = 0;
    usingClock = false;
  }

  checkLFOs(usingClock);
  // updateLEDs();

  lastUsingClockIn = usingClock;
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
void checkLFOs(bool usingClock) {
  lfo1.check(usingClock);
  // lfo2.check(usingClock);
  // lfo3.check(usingClock);
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
  return clockPeriod > highFastestPeriod && clockPeriod < lowSlowestPeriod;
}

long scaleFromDACtoPWM(long value) {
  return value * PWM_RES / DAC_RES;
}

void updateLEDs() {
  int lfo3Value = lfo3.getValue();
  int inverseLfo3Value = DAC_RES - lfo3Value;
  int lfo2Value = lfo2.getValue();
  int inverseLfo2Value = DAC_RES - lfo2Value;
  int lfo1Value = lfo1.getValue();
  int inverseLfo1Value = DAC_RES - lfo1Value;
  lfo2Value = lfo2Value * inverseLfo3Value / DAC_RES;
  inverseLfo2Value = inverseLfo2Value * inverseLfo3Value / DAC_RES;
  lfo1Value = lfo1Value * inverseLfo2Value / DAC_RES;
  inverseLfo1Value = inverseLfo1Value * inverseLfo2Value / DAC_RES;
  
  // led4 is hooked up directly to lfo3Value, not controlled digitally
  analogWrite(led3Pin, scaleFromDACtoPWM(lfo2Value));
  analogWrite(led2Pin, scaleFromDACtoPWM(lfo1Value));
  analogWrite(led1Pin, scaleFromDACtoPWM(inverseLfo1Value));
}
