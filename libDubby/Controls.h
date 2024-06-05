
#pragma once

#include <iostream>

  enum DubbyControls
  {
    CONTROLS_NONE,
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

  enum Params {
    PARAMS_NONE,
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


namespace daisy
{
class Controls
{
  public:

    DubbyControls controls;
    int param;
    float value;
    float min;
    float max;


    const char * ParamsStrings[PARAMS_LAST] = 
    { 
      "-",
      "TIME", 
      "FEEDBACK", 
      "MIX", 
      "CUTOFF", 
      "IN GAIN", 
      "OUT GAIN", 
      "FREEZE", 
      "MUTE", 
      "LOOP", 
      "RES", 
      "SCRUB",
      "RATIO",
      "PREDELAY",
      "AMOUNT"
    };


    const char * ControlsStrings[CONTROLS_LAST] = 
    { 
        "-",
        "KNOB1",
        "KNOB2", 
        "KNOB3", 
        "KNOB4",
        "BTN1",
        "BTN2",
        "BTN3",
        "BTN4",
        "JSX",
        "JSY",
        "JSSW",
    };

    void Init(DubbyControls c, int p, float val, float mi, float ma) {
      controls = c;
      param = p;
      value =  val;
      min = mi;
      max = ma;
    }

    // void SetCurrentValue(float val)
    // {
    //   value = val;
    // }


  private:
    
    
};

}

