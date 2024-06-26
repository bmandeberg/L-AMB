#include <Arduino.h>
#include "L-AMB.h"
#include "Switch.h"

void Switch::setup(int pinNum, bool invertedRead, bool pullup, Callback engageCallback, Callback disengageCallback) {
  buttonPin = pinNum;
  inverted = invertedRead;
  engage = engageCallback;
  disengage = disengageCallback;
  lastButtonState = inverted ? HIGH : LOW;
  pinMode(pinNum, pullup ? INPUT_PULLUP : INPUT);
}

void Switch::check() {
  int buttonRead = digitalRead(buttonPin);
  if (buttonRead != lastButtonState) {
    lastDebounceTime = millis();
  }
  if (millis() - lastDebounceTime > debounceDelay) {
    handlePress(buttonRead);
  }
  lastButtonState = buttonRead;
}

void Switch::handlePress(int buttonRead) {
  if (buttonRead != buttonState) {
    buttonState = buttonRead;
    int onState = inverted ? LOW : HIGH;
    if (buttonState == onState) {
      // button pressed
      engage.invoke();
    } else {
      // button released
      disengage.invoke();
    }
  }
}
