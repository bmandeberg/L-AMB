#ifndef CALLBACK_H
#define CALLBACK_H

class LFO;

union CallbackUnion {
  void (*func)();
  struct {
    void (LFO::*mfunc)();
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
        if (cb.func) cb.func();
        break;
      case CallbackType::MEMBER_FUNCTION:
        if (cb.bound.obj && cb.bound.mfunc) {
          (cb.bound.obj->*(cb.bound.mfunc))();
        }
        break;
    }
  }
};

#endif // CALLBACK_H
