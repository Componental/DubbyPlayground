
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
  PARAMS_2ND_LAST, // because of a bug with last element of list, made this one but stays hidden
  PARAMS_LAST
};

enum ParameterOptions 
{
  PARAM,
  CTRL,
  PAGE,
  VALUE,
  MIN,
  MAX, 
  CURVE,
  POPTIONS_LAST
};

enum Pages 
{
  ALL,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX, 
  SEVEN,
  EIGHT, 
  NINE, 
  TEN,
  ELEVEN, 
  TWELWE, 
  THIRTEEN,
  FOURTEEN,
  FIFTEEN,
  SIXTEEN,
  PAGES_LAST
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
    int page;
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

    void setPage(int pg){
      page = pg;
    }

  private:
    
};

}

