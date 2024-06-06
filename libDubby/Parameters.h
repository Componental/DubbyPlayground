
#pragma once

#include <iostream>
#include "Controls.h"
#include "MidiSettingsMenu.h"

namespace daisy
{
class Parameters
{
  public:

    Controls dubbyCtrl[CONTROLS_LAST];


    void Init() {
      dubbyCtrl[0].Init(KN1, TIME, 0, 0, 1);
      dubbyCtrl[1].Init(KN2, FEEDBACK, 0, 0, 1);
      dubbyCtrl[2].Init(KN3, MIX, 0, 0, 1);
      dubbyCtrl[3].Init(KN4, CUTOFF, 0, 0, 1);
      dubbyCtrl[4].Init(BTN1, IN_GAIN, 0, 0, 1);
      dubbyCtrl[5].Init(BTN2, OUT_GAIN, 0, 0, 1);
      dubbyCtrl[6].Init(BTN3, FREEZE, 0, 0, 1);
      dubbyCtrl[7].Init(BTN4, MUTE, 0, 0, 1);
      dubbyCtrl[8].Init(JSX, LOOP, 0, 0, 1);
      dubbyCtrl[9].Init(JSY, RESONANCE, 0, 0, 1);
      dubbyCtrl[10].Init(JSSW, SCRUB, 0, 0, 1);
      dubbyCtrl[11].Init(CONTROLS_NONE, PARAMS_NONE, 0, 0, 0);
    }

    Controls GetControlValues(DubbyControls ctrl) {
      for (int i = 1; i < CONTROLS_LAST; i++) {
        if (ctrl == dubbyCtrl[i].controls)
          return dubbyCtrl[i];
      }

      return dubbyCtrl[0];
    }

  private:
    
    
};

}

