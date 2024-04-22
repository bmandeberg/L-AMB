#ifndef SWITCH_H
#define SWITCH_H

using callback = std::function<void()>;

class Switch {

  public:
    void setup(int pinNum, bool invertedRead, callback engageCallback, callback disengageCallback);
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
    callback engage;
    callback disengage;
};

#endif // SWITCH_H