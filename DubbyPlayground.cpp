
#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"
#include "PhaserX.h"
using namespace daisy;
using namespace daisysp;
Dubby dubby;
WhiteNoise noise;
float outputVolume;

//Phaser  DSY_SDRAM_BSS   phaser;

//PhaserCustom phaser1 (3, 1, 0.5f, 0.4);
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
            out [j][i]= Phaser_compute(_in)*outputVolume;
            
            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        } 
        AssignScopeData(dubby, i, in, out);
	}

    SetRMSValues(dubby, sumSquared);
}

void handleKnobs(){
   // Get the knob values
    float knob1Value = dubby.GetKnobValue(dubby.CTRL_1); // E.G GAIN
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2) ; // E.G RESONANCE
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3); // E.G CUTOFF
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G CUTOFF


    float depth = knob1Value*64;
    float feedback = knob3Value*127;


    float minRate = 0.3f;
    float maxRate = 127.f;
    float rate = daisysp::fmap(knob2Value, minRate, maxRate, daisysp::Mapping::LOG);


    float minOutputVolume = 0.001f;
    float maxOutputVolume = 1.f;
    outputVolume = daisysp::fmap(knob4Value, minOutputVolume, maxOutputVolume, daisysp::Mapping::LOG);

    // Update the PhaserCustom parameters
    
    Phaser_Wet_set(depth);
    Phaser_Rate_set(rate);
    Phaser_Feedback_set(feedback);
      std::vector<float>    knobValues = {knob1Value, knob2Value, knob3Value, knob4Value};

    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);
}   

  

int main(void)
{
    Init(dubby);
         float sample_rate = dubby.seed.AudioSampleRate();

PhaserInit();

	dubby.seed.StartAudio(AudioCallback);
    noise.Init();
    
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