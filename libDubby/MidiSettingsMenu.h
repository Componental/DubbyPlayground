// midiSettingsMenu.h

#pragma once

#include <iostream>

enum MidiSettings
{
  MIDICLOCK,
  MIDIIN,
  MIDIINCHN,
  MIDIOUT,
  MIDIOUTCHN,
  MIDITHRUOUT,
  MIDISETTINGS_LAST
};

enum MidiClockOptions
{
  LEADER,
  FOLLOWER,
  MIDICLOCKOPTIONS_LAST
};

namespace daisy
{
  class MidiSettingsMenu
  {
  public:
    MidiSettings midiSettings;
    const char *MidiSettingsStrings[MIDISETTINGS_LAST] =
        {
            "MIDI CLOCK",
            "MIDI IN",
            "MIDI IN CHN",
            "MIDI OUT",
            "MIDI OUT CHN",
            "MIDI THRU/OUT"};

    const char *MidiClockOptionsStrings[MIDICLOCKOPTIONS_LAST] =
        {
            "LEADER",
            "FOLLOWER"};

    void Init(MidiSettings c)
    {
      setting = c;
    }

  private:
    MidiSettings setting;
    int param;
    float value;
    float min;
    float max;
  };
}
