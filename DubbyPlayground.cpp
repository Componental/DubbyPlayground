
#include "daisysp.h"
#include "Dubby.h"
#include "Compressor.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

Compressor comp;

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

            out[j][i
             ] = comp.Process(_in);

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
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G CUTOFF

    float minThreshold = -80.f; // Minimum cutoff frequency in Hz
    float maxThreshold = -0.f; // Maximum cutoff frequency in Hz
    float mappedThreshold = daisysp::fmap(knob1Value, minThreshold, maxThreshold, daisysp::Mapping::LINEAR);


    // Map the knob value to a logarithmic scale for cutoff frequency
    float minRatio = 1.0f; // Minimum cutoff frequency in Hz
    float maxRatio= 40.0f; // Maximum cutoff frequency in Hz
    float mappedRatio = daisysp::fmap(knob2Value, minRatio, maxRatio, daisysp::Mapping::LOG);

    
    float minAttack = 0.001f; // Minimum cutoff frequency in Hz
    float maxAttack = 2.0f; // Maximum cutoff frequency in Hz
    float mappedAttack = daisysp::fmap(knob3Value, minAttack, maxAttack, daisysp::Mapping::LINEAR);

    float minRelease = 0.001f; // Minimum cutoff frequency in Hz
    float maxRelease = 2.0f; // Maximum cutoff frequency in Hz
    float mappedRelease = daisysp::fmap(knob4Value, minRelease, maxRelease, daisysp::Mapping::LINEAR);

    comp.SetThreshold(mappedThreshold);
    comp.SetRatio(mappedRatio);
    comp.SetAttack(mappedAttack);
    comp.SetRelease(mappedRelease);
    

    std::vector<float>    knobValues = {mappedThreshold, mappedRatio, mappedAttack, mappedRelease};

    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);


}


int main(void)
{
    Init(dubby);
	dubby.seed.StartAudio(AudioCallback);
    float sample_rate = dubby.seed.AudioSampleRate();
comp.Init(sample_rate);
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