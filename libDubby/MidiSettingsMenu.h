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

enum MidiInOptions
{
  MIDIIN_ON,
  MIDIIN_OFF,
  MIDIINOPTIONS_LAST
};

enum MidiInChannelOptions
{
  MIDI_IN_CHN1,
  MIDI_IN_CHN2,
  MIDI_IN_CHN3,
  MIDI_IN_CHN4,
  MIDI_IN_CHN5,
  MIDI_IN_CHN6,
  MIDI_IN_CHN7,
  MIDI_IN_CHN8,
  MIDI_IN_CHN9,
  MIDI_IN_CHN10,
  MIDI_IN_CHN11,
  MIDI_IN_CHN12,
  MIDI_IN_CHN13,
  MIDI_IN_CHN14,
  MIDI_IN_CHN15,
  MIDI_IN_CHN16,
  MIDIINCHNOPTIONS_LAST
};

enum MidiOutOptions
{
  MIDIOUT_ON,
  MIDIOUT_OFF,
  MIDIOUTOPTIONS_LAST
};

enum MidiOutChannelOptions
{
  MIDI_OUT_CHN1,
  MIDI_OUT_CHN2,
  MIDI_OUT_CHN3,
  MIDI_OUT_CHN4,
  MIDI_OUT_CHN5,
  MIDI_OUT_CHN6,
  MIDI_OUT_CHN7,
  MIDI_OUT_CHN8,
  MIDI_OUT_CHN9,
  MIDI_OUT_CHN10,
  MIDI_OUT_CHN11,
  MIDI_OUT_CHN12,
  MIDI_OUT_CHN13,
  MIDI_OUT_CHN14,
  MIDI_OUT_CHN15,
  MIDI_OUT_CHN16,
  MIDIOUTCHNOPTIONS_LAST
};

enum MidiThruOutOptions
{
  MIDI_THRU,
  MIDI_OUT,
  MIDITHRUOUT_LAST
};

namespace daisy
{
  class MidiSettingsMenu
  {
  public:
    int currentMidiClockOption = 0;
    int currentMidiInOption = 0;
    int currentMidiInChannelOption = 0;
    int currentMidiOutOption = 0;
    int currentMidiOutChannelOption = 0;
    int currentMidiThruOutOption = 0;

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

    const char *MidiInOptionsStrings[MIDIINOPTIONS_LAST] =
        {
            "ON",
            "OFF"};

    const char *MidiInChannelOptionsStrings[MIDIINCHNOPTIONS_LAST] =
        {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"};

    const char *MidiOutOptionsStrings[MIDIOUTOPTIONS_LAST] =
        {
            "ON",
            "OFF"};

    const char *MidiOutChannelOptionsStrings[MIDIOUTCHNOPTIONS_LAST] =
        {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"};

    const char *MidiThruOutOptionsStrings[MIDITHRUOUT_LAST] =
        {
            "OUT",
            "THRU"};

    void Init(MidiSettings c)
    {
      setting = c;
      
    }

  private:
    MidiSettings setting;
  };
}
