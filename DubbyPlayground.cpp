
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
Chorus DSY_SDRAM_BSS chorus[4];
float knob4Value;
float dry[NUM_AUDIO_CHANNELS];
float wet[NUM_AUDIO_CHANNELS];


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
            // Process the dry signal through chorus
    dry[j] = in[j][i];
    wet[j] = chorus[j].Process(dry[j]);

            // Mix dry and wet signals based on knob4Value
            float dryWetMix = knob4Value; // Assuming knob4Value ranges from 0.0 to 1.0
    out[j][i] = (1.0f - dryWetMix) * dry[j] + dryWetMix * wet[j];


            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        } 
        AssignScopeData(dubby, i, in, out);
	}

    SetRMSValues(dubby, sumSquared);
}

void handleKnobs(){
    float knob1Value = dubby.GetKnobValue(dubby.CTRL_1); // E.G GAIN
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2) ; // E.G RESONANCE
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3); // E.G CUTOFF
    knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G SOMETHING ELSE

    // Map the knob value to a logarithmic scale for cutoff frequency
    float minRate = 0.01f; // Minimum cutoff frequency in Hz
    float maxRate = 5.f; // Maximum cutoff frequency in Hz
    float mappedRate = daisysp::fmap(knob2Value, minRate, maxRate, daisysp::Mapping::LOG);

for (int j = 0; j < NUM_AUDIO_CHANNELS; j++){
    chorus[j].SetLfoDepth(knob1Value);
    chorus[j].SetLfoFreq(mappedRate);
    chorus[j].SetFeedback(knob3Value);

} 

    std::vector<float>    knobValues = {knob1Value, mappedRate, knob3Value, knob4Value};

    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);


}


int main(void)
{
    Init(dubby);
	dubby.seed.StartAudio(AudioCallback);
    float sample_rate = dubby.seed.AudioSampleRate();
    
    for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) {
chorus[j].Init(sample_rate);
chorus[j].SetDelay(0.6+j*0.1f);
        }

    

	while(1) { 
        Monitor(dubby);
        MonitorMidi();

        handleKnobs();
                 if(dubby.buttons[3].TimeHeldMs() > 1000){dubby.ResetToBootloader();}


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