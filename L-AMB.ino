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
int clockDivMultTable[numOptions];
bool lastUsingClockIn = false;
const int stepPin = 5;
int currentStep = 0;
const int setStepPin = 7;

LFO lfo1, lfo2, lfo3;
Switch clockSelectSwitch;

Adafruit_ZeroDMA dma;

Adafruit_ZeroTimer timer = Adafruit_ZeroTimer(TIMER_NUM);
void TC3_Handler() {
  Adafruit_ZeroTimer::timerHandler(TIMER_NUM);
}

// tick LFOs within the ISR
void tickLFOs() {
  lfo1.tick();
}

void setup() {
  Serial.begin(9600);
  initializeClockDivMultOptions();

  pinMode(stepPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(stepPin), stepSequence, RISING);

  // initialize DAC DMA
  analogWriteResolution(12);
  analogWrite(A1, 0);
  dma.setTrigger(TC3_DMAC_ID_OVF);
  dma.setAction(DMA_TRIGGER_ACTON_BEAT);
  dma.allocate();

  // setup LFOs
  LFO::initializePeriodTables();
  lfo1.setup(A3, A2, 24, 25, 0, &dma);
  checkLFOs(false);

  // setup main clock for ticking LFOs
  timer.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  timer.setCompare(0, F_CPU / 2500000 * clockResolution);
  timer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, tickLFOs);
  timer.enable(true);
  
  pinMode(setStepPin, OUTPUT);
  digitalWrite(setStepPin, HIGH);
}

int oneVoltADC = ADC_RES / 6.6f;
int oneVoltDAC = 4096 / 6.6f;
int lastVoiceVal = 0;

void loop() {
  bool usingClock = usingClockIn();
  // disable external clock use if it's idled
  if (usingClock && micros() - lastClockTime > lowSlowestPeriod) {
    clockPeriod = 0;
    usingClock = false;
  }

  checkLFOs(usingClock);

  lastUsingClockIn = usingClock;

  int voiceVal = analogRead(A4) / oneVoltADC;
  if (voiceVal != lastVoiceVal) {
    analogWrite(A1, min(voiceVal, 6) * oneVoltDAC);
    lastVoiceVal = voiceVal;
  }
}

void stepSequence() {
  currentStep++;
  
  if (currentStep > 1) {
    currentStep = 0;
  }
  
  digitalWrite(setStepPin, currentStep == 0 ? LOW : HIGH);
}

// check LFO inputs, takes about 162 micros
void checkLFOs(bool usingClock) {
  lfo1.check(usingClock);
}

void resetLFOs() {
  lfo1.reset();
}

void initializeClockDivMultOptions() {
  // freq knob sweeps from divide by 9 to multiply by 9 of clock frequency
  bool descending = true;
  int optionIndex = 0;
  for (int i = maxDivMult; i < maxDivMult + 1; descending ? i-- : i++) {
    if (i < 2) {
      descending = false;
    }
    clockDivMultTable[optionIndex] = i;
    optionIndex++;
  }
}

bool usingClockIn() {
  return clockPeriod > highFastestPeriod && clockPeriod < lowSlowestPeriod;
}

long scaleFromDACtoPWM(long value) {
  return value * PWM_RES / DAC_RES;
}
