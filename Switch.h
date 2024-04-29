#ifndef SWITCH_H
#define SWITCH_H

#include "Callback.h"

class Switch {

  public:
    void setup(int pinNum, bool invertedRead, bool pullup, Callback engageCallback, Callback disengageCallback);
    void check();

  private:
    static const unsigned long debounceDelay = 50;
    unsigned long lastDebounceTime = 0;
    int buttonPin;
    int buttonState;
    bool inverted;
    int lastButtonState;
    void _handlePress(int buttonRead);
    unsigned long _debounceDifference();
    Callback engage;
    Callback disengage;
};

#endif // SWITCH_H