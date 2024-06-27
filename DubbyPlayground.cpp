
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
bool midiClockStarted = false;
bool midiClockStoppedByButton2 = false;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================

            out[j][i] = _in;

            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        }
        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);

    dubby.seed.StartAudio(AudioCallback);

    // initLED();
    // setLED(0, BLUE, 0);
    // setLED(1, RED, 0);
    // updateLED();

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        MIDISendNoteOn(dubby, 46, 120);

      if (dubby.buttons[dubby.CTRL_1].FallingEdge())
    {
        if (midiClockStarted)
        {
            MIDISendStop(dubby);
            midiClockStarted = false;
        }
        else
        {
            MIDISendContinue(dubby);
            midiClockStarted = true;
        }
    }

    if (dubby.buttons[dubby.CTRL_2].FallingEdge())
    {
        if (midiClockStarted)
        {
            MIDISendStop(dubby);
            midiClockStarted = false;
        }
        else
        {
            MIDISendStart(dubby);
            midiClockStarted = true;
        }
    }

        if (dubby.buttons[dubby.CTRL_3].TimeHeldMs() > 1000)
        {
            dubby.ResetToBootloader();
        }
    }
}

void HandleMidiMessage(MidiEvent m)
{

    switch (m.type)
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
    default:
        break;
    }
}

void MonitorMidi()
{
    // Handle USB MIDI Events
    while (dubby.midi_usb.HasEvents())
    {
        MidiEvent m = dubby.midi_usb.PopEvent();
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                HandleMidiMessage(m);
            }
        }
    }

    // Handle UART MIDI Events
    while (dubby.midi_uart.HasEvents())
    {
        MidiEvent m = dubby.midi_uart.PopEvent();
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                HandleMidiMessage(m);
            }
        }
    }
}
