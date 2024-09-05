
#pragma once

#include <iostream>
#include <vector>
#include <algorithm> // for std::find
#include "Controls.h"
#include "MidiSettingsMenu.h"
#include "ChannelMappingMenu.h"

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
    float value, smoothedValue, displayedValue, previousDisplayedValue, min, max, minLimit, maxLimit;
    bool hasMinLimit, hasMaxLimit;
    float threshold;
    float alpha = 0.1f;
    unsigned long lastMovementTime = 0;
    unsigned long movementTimeout = 100; // Time to consider knob as stopped (in milliseconds)
    float movementThreshold = 0.005f; // Threshold for detecting significant change

    Curves curve;

    void Init(Params p, DubbyControls con, float v, float mi, float ma, Curves c, bool hasMinL = false, float minL = 0.0f, bool hasMaxL = false, float maxL = 1.0f) 
    {
      param = p;
      control = con;
      value = v;
      displayedValue = v;
      min = mi;
      max = ma;
      curve = c;
      threshold = 0.01f * abs(max - max);

      hasMinLimit = hasMinL;
      hasMaxLimit = hasMaxL;

      if (hasMinL) minLimit = minL;
      if (hasMaxL) maxLimit = maxL;
    }


    void calculateThreshold() {
        // Single-line equation for dynamic threshold
        threshold = abs(max - min) * (0.01 + 0.02 * log10(abs(max - min)));
    }


    void CalculateRealValue(float normalizedValue) 
    {
        switch(curve)
        {
          case LINEAR:
              value = min + normalizedValue * (max - min);

              calculateThreshold();

              // smoothedValue = (alpha * displayedValue) + ((1 - alpha) * smoothedValue);
              // Only update the displayed value if the change exceeds the threshold
              if (abs(value - previousDisplayedValue) >= threshold) 
              {
                  lastMovementTime = System::GetNow(); // Update last movement time
                  previousDisplayedValue = value;
              }

              if (System::GetNow() - lastMovementTime > movementTimeout) {
                  // Display the constant value when the knob is not turning
                  displayedValue = previousDisplayedValue;
              } else {
                  // Display the smoothed value while the knob is turning
                  displayedValue = value;
              }

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
    }

  private:
    
};

}

