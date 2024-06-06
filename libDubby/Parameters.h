
#pragma once

#include <iostream>

enum Params {
  PARAM_NONE,
  TIME,
  FEEDBACK,
  MIX, 
  CUTOFF,
  IN_GAIN,
  OUT_GAIN,
  FREEZE,
  MUTE, 
  LOOP,
  RESONANCE,
  SCRUB,
  RATIO,
  PREDELAY,
  AMOUNT,
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
  LOGARITHIMIC,
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
    float value;
    float min;
    float max;
    Curves curve;

    void Init(Params p, float v, float mi, float ma, Curves c) 
    {
      param = p;
      value = v;
      min = mi;
      max = ma;
      curve = c;
    }

    float GetRealValue(float controlValue) 
    {
      value = min + ((controlValue - 0.0f) * (max - min) / (1.0f - 0.0f));
      return value;
    }

  private:
    
    
};

}

