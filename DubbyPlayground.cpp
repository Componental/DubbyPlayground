
#include "daisysp.h"
#include "Dubby.h"
#include "Compressor.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

Overdrive drive[4]; // Array of Overdrive objects for each channel

float wetDry;
float gainReductionVal;
float inputGain;
float outputVolume;

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
            
            float inSignal = in[j][i] * inputGain;

            // Process input signal for each channel
            float wetSignal = drive[j].Process(inSignal);

            // Apply dry/wet mix
            const float dryMix = 1.0f - wetDry; // 0 to 1, where 0 is fully wet and 1 is fully dry
            const float wetMix = wetDry;

            // Combine dry and wet signals using the mix values
            float outputSignal = wetSignal * gainReductionVal;
            out[j][i] = ((inSignal * dryMix) + (outputSignal * wetMix)) * outputVolume;

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
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G SOMETHING ELSE

const float newMinDriveVal = 0.4f;
const float newMaxDriveVal = 0.99f;
float driveVal = (knob2Value * (newMaxDriveVal - newMinDriveVal)) + newMinDriveVal;

    //gainReductionVal = 1;
        gainReductionVal = knob2Value > 0.5f ? (1.f-(1.3f*0.5f))+knob2Value*0.3f : 1.f - (1.3f*knob2Value)+knob2Value*0.3f;

    // Update the PhaserCustom parameters
         wetDry = knob3Value;
        outputVolume = knob4Value;


/*
    // Map the knob value to a logarithmic scale for cutoff frequency
    float minCutoff = 5.0f; // Minimum cutoff frequency in Hz
    float maxCutoff = 7000.0f; // Maximum cutoff frequency in Hz
    float mappedCutoff = daisysp::fmap(cutOffKnobValue, minCutoff, maxCutoff, daisysp::Mapping::LOG);

*/
float minGain = 0.001f; // Minimum gain value
float maxGain = 3.0f;    // Maximum gain value
// Logarithmic mapping
inputGain = minGain * exp(knob1Value * log(maxGain / minGain));


for (int i = 0; i < 4; i++)
    {
        drive[i].SetDrive(driveVal);
        // Set other parameters for each Overdrive object if needed
    }




    std::vector<float>    knobValues = {inputGain, knob2Value, knob3Value, knob4Value};

    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);


}


int main(void)
{
    Init(dubby);
	dubby.seed.StartAudio(AudioCallback);
    float sample_rate = dubby.seed.AudioSampleRate();
for (int i = 0; i < 4; i++)
    {
        drive[i].Init();
        // Set other parameters for each Overdrive object if needed
    }
/*
 comp.Init(sample_rate);
    comp.SetThreshold(-5.f);
    comp.SetRatio(80 .f);
    comp.SetAttack(0.05f);
    comp.SetRelease(0.05f);
*/
   


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