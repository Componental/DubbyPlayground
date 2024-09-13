
#pragma once

#include <iostream>
#include <vector>
#include <algorithm> // for std::find

enum DubbyControls
{
  CONTROL_NONE,
  KN1,
  KN2, 
  KN3, 
  KN4,
  BTN1,
  BTN2,
  BTN3,
  BTN4,
  JSX,
  JSY,
  JSSW,
  CONTROLS_LAST
};


inline const char* ctrlEnumToString(DubbyControls control) {
    switch (control) {
        case CONTROL_NONE: return "CONTROL_NONE";
        case KN1: return "KN1";
        case KN2: return "KN2";
        case KN3: return "KN3";
        case KN4: return "KN4";
        case BTN1: return "BTN1";
        case BTN2: return "BTN2";
        case BTN3: return "BTN3";
        case BTN4: return "BTN4";
        case JSX: return "JSX";
        case JSY: return "JSY";
        case JSSW: return "JSSW";
        case CONTROLS_LAST: return "CONTROLS_LAST";
        default: return "UNKNOWN_CONTROL";
    }
}

namespace daisy
{
class Controls
{
  public:

    DubbyControls control;

    float value;
    float tempValue = 0;

    void Init(DubbyControls c, float val) {
      control = c;
      value =  val;
    }

  private:
  
};

}

