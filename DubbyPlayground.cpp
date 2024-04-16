
#include "daisysp.h"
#include "Dubby.h"
#include "reverbsc.h"
#include "implementations/includes.h"

#define MAX_DELAY static_cast<size_t>(24000)

using namespace daisy;
using namespace daisysp;
float sample_rate;
Dubby dubby;

const float smoothingFactor =  .0002f;
static float smoothedKnob1Value = 0.0f;
float knob1Value;
ReverbSc DSY_SDRAM_BSS verbLeft, verbRight;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS preDelayLeft, preDelayRight;

float wetVolumeAdjustment = 0.8f; 

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = { 0.0f };


	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 2; j++) 
        {
            float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================

      // Process input samples through the reverb effect
            float processedSampleLeft;
            float processedSampleRight;

            float dryLeft = in[0][i];
            float dryRight = in[1][i];

          //  float shiftedLeft = pitchshiftLeft.Process(dryLeft);
          //  float shiftedRight = pitchshiftRight.Process(dryRight);
        
            verbLeft.Process(dryLeft, &processedSampleLeft);
            verbRight.Process(dryRight, &processedSampleRight);


            float wetLeft = processedSampleLeft;
            float wetRight = processedSampleRight;

    fonepole(smoothedKnob1Value, knob1Value, smoothingFactor);
            if (smoothedKnob1Value < 0.01f) {
                smoothedKnob1Value = 0.01f;
            }

            preDelayLeft.SetDelay(smoothedKnob1Value*0.5f*sample_rate);
            preDelayRight.SetDelay(smoothedKnob1Value*0.5f*sample_rate);

            float preDelayedWetLeft = preDelayLeft.Read()*wetVolumeAdjustment;
            float preDelayedWetRight = preDelayRight.Read()*wetVolumeAdjustment;

            // Dry/Wet mix for left and right channels
            float dryWetLeft = (1.0f - dubby.GetKnobValue(dubby.CTRL_4)) * dryLeft + dubby.GetKnobValue(dubby.CTRL_4) * preDelayedWetLeft;
            float dryWetRight = (1.0f - dubby.GetKnobValue(dubby.CTRL_4)) * dryRight + dubby.GetKnobValue(dubby.CTRL_4) * preDelayedWetRight;

            // Output to both stereo channels
            out[0][i] = dryWetLeft;
            out[1][i] = dryWetRight;            // Mix the processed sample with the original input
            //out[j][i] = in[j][i] + processed_sample;
            preDelayLeft.Write(wetLeft);
            preDelayRight.Write(wetRight);
            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        } 
        AssignScopeData(dubby, i, in, out);
	}

    SetRMSValues(dubby, sumSquared);
}

void handleKnobs(){
     knob1Value = dubby.GetKnobValue(dubby.CTRL_1); // E.G GAIN
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2) ; // E.G RESONANCE
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3); // E.G CUTOFF
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G CUTOFF
    

 // Map the knob value to a logarithmic scale for cutoff frequency
    float minFeedback = 0.5f; // Minimum cutoff frequency in Hz
    float maxFeedback = 0.999f; // Maximum cutoff frequency in Hz
    float invLogMappedFeedback = maxFeedback - (maxFeedback - minFeedback) * powf(10, -knob2Value * 2); // Adjust exponent as needed


    // Map the knob value to a logarithmic scale for cutoff frequency
    float minCutoff = 100.0f; // Minimum cutoff frequency in Hz
    float maxCutoff = 20000.0f; // Maximum cutoff frequency in Hz
    float mappedCutoff = daisysp::fmap(knob3Value, minCutoff, maxCutoff, daisysp::Mapping::LOG);

    verbLeft.SetFeedback(invLogMappedFeedback);
    verbLeft.SetLpFreq(mappedCutoff);
    verbRight.SetFeedback(invLogMappedFeedback);
    verbRight.SetLpFreq(mappedCutoff);

    std::vector<float>    knobValues = {smoothedKnob1Value, knob2Value, knob3Value, knob4Value};

    // Update knob values in Dubby class
    dubby.updateKnobValues(knobValues);


}


int main(void)
{
    Init(dubby);
	dubby.seed.StartAudio(AudioCallback);
     sample_rate = dubby.seed.AudioSampleRate();


    //setup reverb
    verbLeft.Init(sample_rate);
    verbLeft.SetFeedback(0.5f);
    verbLeft.SetLpFreq(20000.0f);
  //setup reverb
    verbRight.Init(sample_rate);
    verbRight.SetFeedback(0.5f);
    verbRight.SetLpFreq(20000.0f);
    
    preDelayLeft.Init();
    preDelayRight.Init();
        preDelayLeft.SetDelay(10.f);
    preDelayRight.SetDelay(10.f);

    //pitchshiftLeft.Init(sample_rate);
    //pitchshiftRight.Init(sample_rate);

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