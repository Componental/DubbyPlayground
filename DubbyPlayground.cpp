
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = { 0.0f };

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) 
        {
            float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================

            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        } 
        AssignScopeData(dubby, i, in, out);
	}

    SetRMSValues(dubby, sumSquared);
}


 // Previous knob value to detect changes
float prevKnobValue = 0.0f;

int main(void) {
  Init(dubby);
  InitMidiClock(dubby);

  dubby.seed.StartAudio(AudioCallback);

  while (1) {
    Monitor(dubby);
    MonitorMidi();

    // Get current knob value
    float currentKnobValue = dubby.GetKnobValue(dubby.CTRL_1);

    // Send MIDI only if value changed significantly (e.g., by more than 0.1)
    if (fabs(currentKnobValue - prevKnobValue) > 0.001f) {
      MIDISendControlChange(dubby, 0, 1, currentKnobValue * 127);
      prevKnobValue = currentKnobValue;
    }
  }
}

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            break;
        }
        case SystemRealTime:
        {
            HandleSystemRealTime(m.srt_type);
        }
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            break;
        }
        default: break;
    }
}

void MonitorMidi()
{
    // Handle USB MIDI Events
    while(dubby.midi_usb.HasEvents())
    { 
        MidiEvent m = dubby.midi_usb.PopEvent();
        HandleMidiMessage(m);
    }

    // Handle UART MIDI Events
    while(dubby.midi_uart.HasEvents())
    {
        MidiEvent m = dubby.midi_uart.PopEvent();
        HandleMidiMessage(m);
    }
}

