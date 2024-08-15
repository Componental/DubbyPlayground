// HOW TO USE PARAMS: 
// 1. ADD PARAMS TO ENUM LIST BELOW
// 2. UPDATE PARAMSSTRINGS IN Dubby.h WITH THE SAME ORDER AS THE ENUM
// 3. INITIALIZE IN USING .Init() IN InitDubbyParameters() IN Dubby.cpp
// 4. EVENTUALLY ASSIGN THE PARAMETER TO A CONTROL IN InitDubbyControls();
// 5. USE THE VARIABLE IN DubbyPlayground.cpp, GET THE VALUE USING: dubby.GetParameterValue(dubby.dubbyParameters[FLT_CUTOFF]

#pragma once

#include <iostream>

enum Params {
  PARAM_NONE,
  DLY_TIME,
  DLY_FEEDBACK,
  DLY_DIVISION,
  DLY_SPREAD,
  DLY_MIX,
  FLT_CUTOFF,
  FLT_RESONANCE,
  OUT_GAIN,
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
        float normalizedValue = (controlValue - 0.0f) / (1.0f - 0.0f);

        switch(curve)
        {
            case LINEAR:
                value = min + normalizedValue * (max - min);
                break;
            case LOGARITHMIC:
                if (normalizedValue <= 0.0f) {
                    value = min;
                } else {
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
        return value;
    }

  private:
    
};

}

