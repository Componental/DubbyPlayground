
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

static LadderFilter flt[4]; // Four filters, one for each channel

float outGain;


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
            float output = flt[j].Process(_in);

            out[j][i] = output * outGain;

            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        } 
        AssignScopeData(dubby, i, in, out);
	}

    SetRMSValues(dubby, sumSquared);
}

void handleKnobs(){
   // Get the knob values
       float inGain = dubby.GetKnobValue(dubby.CTRL_1)*4;
    float res = dubby.GetKnobValue(dubby.CTRL_2) ;
    float cutOffKnobValue = dubby.GetKnobValue(dubby.CTRL_3);
     outGain = dubby.GetKnobValue(dubby.CTRL_4)*3;

    // Map the knob value to a logarithmic scale for cutoff frequency
    float minCutoff = .05f; // Minimum cutoff frequency in Hz
    float maxCutoff = 17000.f; // Maximum cutoff frequency in Hz
    float mappedCutoff = daisysp::fmap(cutOffKnobValue, minCutoff, maxCutoff, daisysp::Mapping::LOG);

    float roundedCutoff = round(cutOffKnobValue * 1000 + 0.5f)*20.f;


    // Update the filter parameters for each filter
    for (int i = 0; i < 4; i++) {
        flt[i].SetInputDrive(inGain);
        flt[i].SetRes(res);
        flt[i].SetFreq(mappedCutoff);
    }

    std::vector<float>    knobValues = {inGain, res, roundedCutoff,outGain};
    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);
}
// Define an array to hold the string representations of the filter modes

// Function to convert FilterMode enum to string
// Function to convert FilterMode enum to string

int main(void)
{
    Init(dubby);
	dubby.seed.StartAudio(AudioCallback);
    float sample_rate = dubby.seed.AudioSampleRate();
  // initialize Moogladder object
    // Initialize the LadderFilter objects
    for (int i = 0; i < 4; i++) {
        flt[i].Init(sample_rate);
    }


    LadderFilter::FilterMode currentMode = LadderFilter::FilterMode::LP24;



	while(1) { 
        Monitor(dubby);
        MonitorMidi();

        handleKnobs();


                 if(dubby.buttons[3].TimeHeldMs() > 3000){dubby.ResetToBootloader();}


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