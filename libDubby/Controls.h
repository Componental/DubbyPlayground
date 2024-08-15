
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

