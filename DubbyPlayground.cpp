
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

// DELAY EFFECT
#define MAX_DELAY static_cast<size_t>(48000 * 6.0f)
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;

float sample_rate, dryL, dryR, delayOutL, delayOutR, delayTimeSecsL, delayTimeSecsR, delayFeedback, stereoSpread;
float currentDelayL, currentDelayR;
float dryWetMixL, dryWetMixR, dryAmplitude, wetAmplitude;

float divisor = 1;
float delayTimeSecs = 0.4f;

float outGain = 1.f;
// FILTER

Svf filterL;
Svf filterR;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {

            // === AUDIO CODE HERE ===================
            dryL = in[0][i];
            dryR = in[1][i];

            // Set the delay time (fonepole/Filter one-pole smooths out the changes)
            fonepole(currentDelayL, delayTimeSecsL, .0001f);

            // Stereo effect: Reduce the delaytime of the Right channel as Pot4
            // increases
            fonepole(currentDelayR, delayTimeSecsR, .0001f);

            delr.SetDelay(currentDelayL);
            dell.SetDelay(currentDelayR);

            delayOutL = dell.Read();
            delayOutR = delr.Read();

            dell.Write((delayFeedback * delayOutL) + (dryL));
            delr.Write((delayFeedback * delayOutR) + (dryR));

            filterL.Process(delayOutL);
            filterR.Process(delayOutR);

            // Create finel dry/wet mix and send to the output with soft limitting
            dryWetMixL = (dryL * dryAmplitude) + (filterL.Low() * wetAmplitude);
            dryWetMixR = (dryR * dryAmplitude) + (filterR.Low() * wetAmplitude);

            out[0][i] = SoftLimit(dryWetMixL)*outGain;
            out[1][i] = SoftLimit(dryWetMixR)*outGain;

            // =======================================
        }
        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    // Init DSP
    dell.Init();
    delr.Init();
    sample_rate = dubby.seed.AudioSampleRate();

    stereoSpread = 0;
    filterL.Init(sample_rate);
    filterL.SetFreq(500.0);
    filterL.SetRes(0.3f);
    filterL.SetDrive(0.8f);

    filterR.Init(sample_rate);
    filterR.SetFreq(500.0);
    filterR.SetRes(0.3f);
    filterR.SetDrive(0.8f);

    dubby.seed.StartAudio(AudioCallback);

    // initLED();
    // setLED(0, BLUE, 0);
    // setLED(1, RED, 0);
    // updateLED();

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        // Set the wet and dry mix
        wetAmplitude = dubby.GetParameterValue(dubby.dubbyParameters[DLY_MIX]);
        dryAmplitude = 1.f - wetAmplitude;

        delayTimeSecs = dubby.GetParameterValue(dubby.dubbyParameters[DLY_TIME]);

        delayFeedback = dubby.GetParameterValue(dubby.dubbyParameters[DLY_FEEDBACK]);

        stereoSpread = 125.f + (dubby.GetParameterValue(dubby.dubbyParameters[DLY_SPREAD]) * 250.f);

        outGain = dubby.GetParameterValue(dubby.dubbyParameters[OUT_GAIN]);
        // Set the delay time based on Pot3
        float pot3Value = dubby.GetParameterValue(dubby.dubbyParameters[DLY_DIVISION]);

        if (pot3Value == 0.0f)
        {
            divisor = 8;
        }
        else if (pot3Value <= 0.1f)
        {
            divisor = 4;
        }
        else if (pot3Value <= 0.2f)
        {
            divisor = 3;
        } // half note triplets
        else if (pot3Value <= 0.3f)
        {
            divisor = 2;
        }
        else if (pot3Value <= 0.4f)
        {
            divisor = 1.5;
        } // whole note triplets
        else if (pot3Value <= 0.5f)
        {
            divisor = 1;
        }
        else if (pot3Value <= 0.6f)
        {
            divisor = 0.5;
        } // x2
        else if (pot3Value <= 0.7f)
        {
            divisor = 0.33;
        } // x3
        else if (pot3Value <= 0.8f)
        {
            divisor = 0.25;
        } // x4
        else if (pot3Value <= 0.9f)
        {
            divisor = 0.2;
        } // x5
        else
        {
            divisor = 0.125;
        } // x8

        delayTimeSecsL = (delayTimeSecs ) / divisor;
        delayTimeSecsR = (delayTimeSecs + stereoSpread) / divisor;

        filterL.SetFreq(dubby.GetParameterValue(dubby.dubbyParameters[FLT_CUTOFF]));
        filterR.SetFreq(dubby.GetParameterValue(dubby.dubbyParameters[FLT_CUTOFF]));

        filterL.SetRes(dubby.GetParameterValue(dubby.dubbyParameters[FLT_RESONANCE]));
        filterR.SetRes(dubby.GetParameterValue(dubby.dubbyParameters[FLT_RESONANCE]));

        
    }
}

void HandleMidiMessage(MidiEvent m)
{
    switch (m.type)
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
    default:
        break;
    }
}

void MonitorMidi()
{
    // Handle USB MIDI Events
    while (dubby.midi_usb.HasEvents())
    {
        MidiEvent m = dubby.midi_usb.PopEvent();
        HandleMidiMessage(m);
    }

    // Handle UART MIDI Events
    while (dubby.midi_uart.HasEvents())
    {
        MidiEvent m = dubby.midi_uart.PopEvent();
        HandleMidiMessage(m);
    }
}
