#pragma once

#include <iostream>        // Standard input/output stream library
#include <vector>          
#include <algorithm>       // std::find

#include "Parameters.h"    // Include the header file that defines 'Params' and 'PARAMS_LAST'

enum DubbyControls
{
  CONTROL_NONE,  // No control
  KN1,           // Knob 1
  KN2,           // Knob 2
  KN3,           // Knob 3
  KN4,           // Knob 4
  BTN1,          // Button 1
  BTN2,          // Button 2
  BTN3,          // Button 3
  BTN4,          // Button 4
  JSX,           // Joystick X-axis
  JSY,           // Joystick Y-axis
  JSSW,          // Joystick switch
  CONTROLS_LAST  // Placeholder for the last control
};

namespace daisy
{
class Controls
{
  public:

    DubbyControls control;              // Variable to hold the type of control
    std::vector<Params> param;          // Vector to store associated parameters
    int paramArraySize = PARAMS_LAST;   // Size of the parameter array (initialized to the last parameter index)

    float value;                        // Current value associated with the control
    int page;                           // Page number or context where the control is active
    float tempValue = 0;                // Temporary value storage for intermediate operations

    // Initializes the control with a specified type, page, and value
    void Init(DubbyControls c, float val) {
      control = c;                           // Set control type
      value =  val;                          // Set initial value
      
      param.resize(PARAMS_LAST, PARAM_NONE); // Initialize the vector with default parameter values (PARAM_NONE)
    }

    // Adds a parameter value to the first available slot in the vector
    void addParamValue(Params value) {
        auto it = std::find(param.begin(), param.end(), PARAM_NONE); // Find the first occurrence of PARAM_NONE
        if (it != param.end()) {
            *it = value;  // Replace the first PARAM_NONE with the new parameter value
        } else {
            std::cerr << "Array is full, cannot add more important values.\n"; // Error message if vector is full
        }
    }

    // Removes a specific parameter value from the vector
    void removeParamValue(Params value) {
      auto it = std::find(param.begin(), param.end(), value); // Find the parameter value in the vector
      if (it != param.end()) {
          *it = PARAM_NONE;  // Set the found parameter to PARAM_NONE
      } else {
          std::cerr << "Value not found in the array.\n"; // Error message if value is not found
      }

      // Shift non-PARAM_NONE values to the left, maintaining order
      std::stable_partition(param.begin(), param.end(), [](Params x) { return x != PARAM_NONE; });
  }

    // This function is commented out but would set the current value of the control
    // void SetCurrentValue(float val)
    // {
    //   value = val;
    // }

  private:
    // No private members in this class
};

}
