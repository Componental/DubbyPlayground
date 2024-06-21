
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
    int outChannel;
    int inChannel = 0;


bool channelMapping[NUM_AUDIO_CHANNELS][NUM_AUDIO_CHANNELS] = {
    // Input channels:      0, 1, 2, 3
    /* Output channel 0 */ {1, 0, 0, 0}, 
    /* Output channel 1 */ {0, 1, 0, 0},  
    /* Output channel 2 */ {1, 0, 0, 0},   
    /* Output channel 3 */ {0, 1, 0, 0}  
};



void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    // Loop through each sample
    for (size_t i = 0; i < size; i++)
    {
        // Clear the output buffer for each sample
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            out[j][i] = 0.0f; // Initialize output to 0 for each sample
        }

        // Loop through each output channel
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // Loop through each input channel
            for (int inChannel = 0; inChannel < NUM_AUDIO_CHANNELS; inChannel++)
            {
                // Check if the input channel is mapped to the output channel
                if (channelMapping[j][inChannel])
                {
                    // Assign input value to the corresponding output channel
                    out[j][i] += in[inChannel][i];
                }
            }
        }
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);

    dubby.seed.StartAudio(AudioCallback);

    while (1) 
    { 
        Monitor(dubby);
        MonitorMidi();
        if (dubby.buttons[3].TimeHeldMs() > 400) 
        {
            dubby.ResetToBootloader();
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
