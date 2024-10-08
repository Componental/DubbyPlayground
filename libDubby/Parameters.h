
#pragma once

#include <iostream>
#include <vector>
#include <algorithm> // for std::find
#include "Controls.h"
#include "MidiSettingsMenu.h"
#include "ChannelMappingMenu.h"

enum Params
{
  PARAM_NONE,
  PREDELAY_TIME,
  REVERB_TIME,
  LPF_CUTOFF,
  MIX,
  PARAMS_2ND_LAST, // because of a bug with last element of list, made this one but stays hidden
  PARAMS_LAST
};

enum ParameterOptions
{
  PARAM,
  CTRL,
  VALUE,
  MIN,
  MAX,
  CURVE,
  POPTIONS_LAST
};

enum Curves
{
  LINEAR,
  LOGARITHMIC,
  EXPONENTIAL,
  SIGMOID,
  CURVES_LAST
};

namespace daisy
{
  class Parameters
  {
  public:
    Params param;
    DubbyControls control = CONTROL_NONE;
    float value, min, max, minLimit, maxLimit, baseValue;
    bool hasMinLimit, hasMaxLimit;
    Curves curve;

    // Parameter, control, value, min, max, curve, hasminLimit, minLimit, hasMaxLimit, maxL;
    void Init(Params p, DubbyControls con, float v, float mi, float ma, Curves c, bool hasMinL = false, float minL = 0.0f, bool hasMaxL = false, float maxL = 1.0f)
    {
      param = p;
      control = con;
      value = v;
      baseValue = v;
  
      min = mi;
      max = ma;
      curve = c;

      hasMinLimit = hasMinL;
      hasMaxLimit = hasMaxL;

      if (hasMinL)
        minLimit = minL;
      if (hasMaxL)
        maxLimit = maxL;
    }

    void CalculateRealValue(float normalizedValue)
    {
      switch (curve)
      {
      case LINEAR:
        value = min + normalizedValue * (max - min);
        break;
      case LOGARITHMIC:
        if (normalizedValue <= 0.0f)
        {
          value = min;
        }
        else
        {
          value = min + (std::log(normalizedValue * 9.0f + 1.0f) / std::log(10.0f)) * (max - min);
        }
        break;
      case EXPONENTIAL:
        value = min + ((std::exp(normalizedValue) - 1.0f) / (std::exp(1.0f) - 1.0f)) * (max - min);
        break;
      case SIGMOID:
      {
        float sigmoidValue = 1.0f / (1.0f + std::exp(-12.0f * (normalizedValue - 0.5f)));
        value = min + sigmoidValue * (max - min);
      }
      break;
      default:
        value = min; // Default to minimum value for unknown curve type
        break;
      }
    }

  private:
  };

}
