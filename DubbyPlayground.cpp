
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
    int outChannel;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = { 0.0f };
    if(dubby.GetKnobValue(dubby.CTRL_1) > 0.5f){
        outChannel = 0;
    } else if(dubby.GetKnobValue(dubby.CTRL_1) <= 0.5f){
        outChannel = 1;
    }
	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) 
        {
            //float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================
            out[0][i] = in[0][i];
            // =======================================

            //CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
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
    // setLED(0, BLUE, 100);
    // setLED(1, RED, 100);
    // updateLED();

	while(1) { 
        Monitor(dubby);
        MonitorMidi();
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

