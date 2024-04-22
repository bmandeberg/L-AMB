#include "L-AMB.h"
#include "Switch.h"
#include "Arduino.h"

void Switch::setup(int pinNum, bool invertedRead, callback engageCallback, callback disengageCallback) {
  buttonPin = pinNum;
  inverted = invertedRead;
  engage = engageCallback;
  disengage = disengageCallback;
  lastButtonState = inverted ? HIGH : LOW;
  pinMode(pinNum, INPUT);
}

void Switch::check() {
  int buttonRead = digitalRead(buttonPin);
  if (buttonRead != lastButtonState) {
    lastDebounceTime = millis();
  }
  if (_debounceDifference() > debounceDelay) {
    _handlePress(buttonRead);
  }
  lastButtonState = buttonRead;
}

void Switch::_handlePress(int buttonRead) {
  if (buttonRead != buttonState) {
    buttonState = buttonRead;
    int onState = inverted ? LOW : HIGH;
    if (buttonState == onState) {
      // button pressed
      engage();
    } else {
      // button released
      if (disengage) {
        disengage();
      }
    }
  }
}

unsigned long Switch::_debounceDifference() {
  return millis() - lastDebounceTime;
}
