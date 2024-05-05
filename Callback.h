#ifndef CALLBACK_H
#define CALLBACK_H

class LFO;

class Callback {
public:
  // define a union to hold either a free function or a bound member function
  union CallbackUnion {
    void (*fn)();
    struct {
      void (LFO::*memberFn)();
      LFO* obj;
    } bound;

    CallbackUnion() : fn(nullptr) {} // initialize to nullptr to avoid undefined behavior
  };

  // define an enum class to distinguish between callback types
  enum class CallbackType {
    FUNCTION,
    MEMBER_FUNCTION
  };

private:
  CallbackUnion cb;
  CallbackType type;

public:
  Callback() : type(CallbackType::FUNCTION) {
    cb.fn = nullptr;
  }

  // constructor for free functions
  Callback(void (*function)()) : type(CallbackType::FUNCTION) {
    cb.fn = function;
  }

  // constructor for member functions
  Callback(void (LFO::*memberFunction)(), LFO* object)
    : type(CallbackType::MEMBER_FUNCTION) {
    cb.bound.memberFn = memberFunction;
    cb.bound.obj = object;
  }

  // function to invoke the callback
  void invoke() const {
    switch (type) {
      case CallbackType::FUNCTION:
        if (cb.fn) cb.fn();
        break;
      case CallbackType::MEMBER_FUNCTION:
        if (cb.bound.obj && cb.bound.memberFn) {
          (cb.bound.obj->*(cb.bound.memberFn))();
        }
        break;
    }
  }
};

#endif // CALLBACK_H
