#ifndef CALLBACK_H
#define CALLBACK_H

class LFO;

union CallbackUnion {
  void (*fn)();
  struct {
    void (LFO::*memberFn)();
    LFO* obj;
  } bound;
};

enum class CallbackType {
  FUNCTION,
  MEMBER_FUNCTION
};

struct Callback {
  CallbackUnion cb;
  CallbackType type;

  void invoke() {
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
