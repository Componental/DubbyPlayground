
#pragma once

#include <iostream>
#include <vector>
#include <algorithm> // for std::find

#include "Parameters.h"

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
    std::vector<Params> param;
    int paramArraySize = PARAMS_LAST;

    float value;
    float tempValue = 0;

    void Init(DubbyControls c, float val) {
      control = c;
      value =  val;
      
      param.resize(PARAMS_LAST, PARAM_NONE);
    }


    void addParamValue(Params value) {
        // Find the first default value position
        auto it = std::find(param.begin(), param.end(), PARAM_NONE);
        if (it != param.end()) {
            *it = value;
        } else {
            std::cerr << "Array is full, cannot add more important values.\n";
        }
    }

    void removeParamValue(Params value) {
      auto it = std::find(param.begin(), param.end(), value);
      if (it != param.end()) {
          *it = PARAM_NONE;
      } else {
          std::cerr << "Value not found in the array.\n";
      }

      // After removing, we need to shift important values to the left
      std::stable_partition(param.begin(), param.end(), [](Params x) { return x != PARAM_NONE; });
  }

    // void SetCurrentValue(float val)
    // {
    //   value = val;
    // }


  private:
    
    
};

}

