
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
    int outChannel;
    int inChannel = 0;


static LadderFilter flt[4]; // Four filters, one for each channel




void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);





void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // Loop through each sample
    for (size_t i = 0; i < size; i++) {
        // Clear the output buffer for each sample
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) {
            out[j][i] = 0.0f; // Initialize output to 0 for each sample
        }

        // Loop through each output channel
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) {
            // Loop through each input channel
            for (int inChannel = 0; inChannel < NUM_AUDIO_CHANNELS; inChannel++) {
                // Check if the input channel is mapped to the output channel
                if (dubby.channelMapping[j][inChannel] == PASS) {
                    // Directly assign input to output channel
                    out[j][i] += in[inChannel][i];
                } else if (dubby.channelMapping[j][inChannel] == EFCT) {
                    // Apply effect (like filter) to input and then assign to output channel
                    float output = flt[j].Process(in[inChannel][i]);
                    out[j][i] += output;
                } else if (dubby.channelMapping[j][inChannel] == SNTH) {
                    // Apply input to synth
                    //float output = synth[j].SidechainInput(in[inChannel][i]);
                    //out[j][i] += output;
                }
                // If channelMapping[j][inChannel] == 0, do nothing (channel not assigned)
            }
        }
    }
}
int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);

    dubby.seed.StartAudio(AudioCallback);

     initLED();   
     setLED(0, BLUE, 0);
    setLED(1, RED, 0);
     updateLED();
    // Update the filter parameters for each filter
              float sample_rate = dubby.seed.AudioSampleRate();

    for (int i = 0; i < 4; i++) {
  // initialize Moogladder object
    // Initialize the LadderFilter objects
        flt[i].Init(sample_rate);
    

  flt[i].SetInputDrive(1.f);
        flt[i].SetRes(0.f);
        flt[i].SetFreq(2000.f);

    }

                LadderFilter::FilterMode currentMode = LadderFilter::FilterMode::LP24;

	while(1) { 
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
