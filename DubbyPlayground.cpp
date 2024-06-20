
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
    int outChannel;
    int inChannel = 3;


// Define a mapping table where inChannel is the row and outChannel is the column
// Define a mapping table where inChannel is the row and outChannel is the column

// Define a mapping table where inChannel is the row and outChannel is the column
int channelMapping[NUM_AUDIO_CHANNELS][NUM_AUDIO_CHANNELS] = {
    // inChannel = 0
    {0, 1, 1, 1},   // input 0 -> output 0, input 0 -> output 1, input 0 -> output 2, input 0 -> output 3
    
    // inChannel = 1
    {1, 1, 1, 1},   // input 1 -> output 0, input 1 -> output 1, input 1 -> output 2, input 1 -> output 3
    
    // inChannel = 2
    {2, 2, 2, 2},   // input 2 -> output 0, input 2 -> output 1, input 2 -> output 2, input 2 -> output 3
    
    // inChannel = 3
    {1, 3, 0, 2}    // input 3 -> output 0, input 3 -> output 1, input 3 -> output 2, input 3 -> output 3
};


void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    // Loop through all input channels
    for (size_t i = 0; i < size; i++)
    {
        // Loop through all output channels
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // Determine the target output channel for the current input channel
            int targetOutput = channelMapping[j][inChannel]; // Adjust inChannel as needed

            // Assign input value to the corresponding output channel
            out[targetOutput][i] = in[j][i];

            // Additional audio processing code can be added here
        }
    }
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
                 if(dubby.buttons[3].TimeHeldMs() > 400){dubby.ResetToBootloader();}

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

