
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"
#include "reverbsc.h"
using namespace daisy;
using namespace daisysp;

Dubby dubby;
ReverbSc DSY_SDRAM_BSS reverbLeft, reverbRight;
Overdrive driveReverbLeft, driveReverbRight, driveDelayLeft, driveDelayRight;

// DELAY EFFECT
#define MAX_DELAY static_cast<size_t>(48000 * 6.0f)
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayLineLeft, delayLineRight;

float sample_rate, dryL, dryR, delayOutL, delayOutR, delayTimeMillisL, delayTimeMillisR, delayFeedback, stereoSpread, delaySamplesL, delaySamplesR;
float currentDelayL, currentDelayR;
float reverbDelayDryWetMixL, reverbDelayDryWetMixR, delayDryAmplitude, delayWetAmplitude;

float reverbDryAmplitude, reverbWetAmplitude;

float reverbAmplitudeAdjustment = 0.7f;

float divisor = 1;
float delayTimeMillis = 400.f;
float distortedReverbLeft, distortedReverbRight, distortedDelayLeft, distortedDelayRight, driveAmount, driveGainCompensation;
float outGain = 1.f;
// FILTER

Svf filterL;
Svf filterR;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

// Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<PersistantMemoryParameterSettings> SavedParameterSettings(dubby.seed.qspi);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};
    bool freeze = dubby.dubbyParameters[DLY_FREEZE].value > 0.5f;

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS - 2; j++)
        {
            // Dry input signal
            dryL = in[2][i];
            dryR = in[3][i];

            // === REVERB EFFECT ===
            float processedSampleLeft;
            float processedSampleRight;

            reverbLeft.Process(dryL, &processedSampleLeft);
            reverbRight.Process(dryR, &processedSampleRight);

            float reverbWetLeft = processedSampleLeft * reverbWetAmplitude;
            float reverbWetRight = processedSampleRight * reverbWetAmplitude;

                        // Apply overdrive to the mixed signal
            distortedReverbLeft = driveReverbLeft.Process(reverbWetLeft);
            distortedReverbRight = driveReverbRight.Process(reverbWetRight);

    
            // Mix the dry signal with the reverb signal
            float reverbOutputLeft = (dryL * reverbDryAmplitude) + (distortedReverbLeft*driveGainCompensation);
            float reverbOutputRight = (dryR * reverbDryAmplitude) + (distortedReverbRight*driveGainCompensation);

            // === DELAY EFFECT ===
            if (freeze)
            {
                delayFeedback = 1.0f;
                delayOutL = delayLineLeft.Read();
                delayOutR = delayLineRight.Read();
                delayLineLeft.Write(delayOutL);
                delayLineRight.Write(delayOutR);
            }
            else
            {
                fonepole(currentDelayL, delayTimeMillisL, 0.0001f);
                fonepole(currentDelayR, delayTimeMillisR, 0.0001f);

                delaySamplesL = currentDelayL * (sample_rate / 1000);
                delaySamplesR = currentDelayR * (sample_rate / 1000);

                delayLineLeft.SetDelay(delaySamplesL);
                delayLineRight.SetDelay(delaySamplesR);

                delayOutL = delayLineLeft.Read();
                delayOutR = delayLineRight.Read();

                delayLineLeft.Write((delayFeedback * delayOutL) + reverbOutputLeft);
                delayLineRight.Write((delayFeedback * delayOutR) + reverbOutputRight);
            }

            // Process the delay outputs through the filters
            filterL.Process(delayOutL);
            filterR.Process(delayOutR);

            // Final mix of delay and filter outputs with the dry signal
            float delayWetOutputLeft = filterL.Low() * delayWetAmplitude;
            float delayWetOutputRight = filterR.Low() * delayWetAmplitude;

            distortedDelayLeft = driveDelayLeft.Process(delayWetOutputLeft)*driveGainCompensation;
            distortedDelayRight = driveDelayRight.Process(delayWetOutputRight)*driveGainCompensation;
            reverbDelayDryWetMixL = distortedDelayLeft * delayWetAmplitude + reverbOutputLeft * delayDryAmplitude;
            reverbDelayDryWetMixR = distortedDelayRight * delayWetAmplitude + reverbOutputRight * delayDryAmplitude;


            // Output the final processed signal with gain and soft limiting
            out[2][i] = SoftLimit(reverbDelayDryWetMixL) * outGain;
            out[3][i] = SoftLimit(reverbDelayDryWetMixR) * outGain;
        }
        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    InitPersistantMemory(dubby, SavedParameterSettings);
    // Init DSP
    sample_rate = dubby.seed.AudioSampleRate();

    delayLineLeft.Init();
    delayLineRight.Init();

    stereoSpread = 0;

    // REVERB
    reverbLeft.Init(sample_rate);
    reverbRight.Init(sample_rate);
    reverbLeft.SetFeedback(0.5f);
    reverbRight.SetFeedback(0.5f);
    reverbLeft.SetLpFreq(20000.f);
    reverbRight.SetLpFreq(20000.f);

    filterL.Init(sample_rate);
    filterL.SetFreq(500.0);
    filterL.SetRes(0.3f);
    filterL.SetDrive(0.8f);

    filterR.Init(sample_rate);
    filterR.SetFreq(500.0);
    filterR.SetRes(0.3f);
    filterR.SetDrive(0.8f);

    driveReverbLeft.Init();
    driveReverbRight.Init();
    driveDelayLeft.Init();
    driveDelayRight.Init();

    dubby.seed.StartAudio(AudioCallback);

    // initLED();
    // setLED(0, BLUE, 0);
    // setLED(1, RED, 0);
    // updateLED();

    // DELETE MEMORY
    SavedParameterSettings.RestoreDefaults();

    while (1)
    {
        MonitorPersistantMemory(dubby, SavedParameterSettings);

        Monitor(dubby);
        MonitorMidi();
        // Set the wet and dry mix based on the delay mix parameter
        delayWetAmplitude = dubby.dubbyParameters[DLY_MIX].value;
        delayDryAmplitude = 1.f - delayWetAmplitude;

        // Check if DLY_MAXWET is greater than 0.5
        if (dubby.dubbyParameters[DLY_MAXWET].value > 0.5f)
        {
            delayDryAmplitude = 0.f; // Set dry amplitude to 0
            delayWetAmplitude = 1.f; // Set wet amplitude to 1 (fully wet)
        }

        // Retrieve the delay time, feedback, and stereo spread parameters
        delayTimeMillis = dubby.dubbyParameters[DLY_TIME].value;
        delayFeedback = dubby.dubbyParameters[DLY_FEEDBACK].value;
        stereoSpread = dubby.dubbyParameters[DLY_SPREAD].value;

        // Retrieve the output gain parameter
        outGain = dubby.dubbyParameters[OUT_GAIN].value;

        // Set the delay time divisor based on the Pot3 value (DLY_DIVISION)
        float pot3Value = dubby.dubbyParameters[DLY_DIVISION].value;

        divisor =
            pot3Value <= 0.05f   ? 8.0f
            : pot3Value <= 0.15f ? 4.0f
            : pot3Value <= 0.25f ? 3.0f
            : pot3Value <= 0.35f ? 2.0f
            : pot3Value <= 0.45f ? 1.5f
            : pot3Value <= 0.55f ? 1.0f
            : pot3Value <= 0.65f ? 0.5f
            : pot3Value <= 0.75f ? 0.33f
            : pot3Value <= 0.85f ? 0.25f
            : pot3Value <= 0.95f ? 0.2f
                                 : 0.125f;

        // Calculate the delay times for left and right channels
        delayTimeMillisL = (delayTimeMillis - stereoSpread) / divisor;
        delayTimeMillisR = (delayTimeMillis + stereoSpread) / divisor;

        // Ensure that delay times don't go lower than 1 millisecond
        delayTimeMillisL = fmaxf(delayTimeMillisL, 1.0f);
        delayTimeMillisR = fmaxf(delayTimeMillisR, 1.0f);

        // REVERB
        reverbWetAmplitude = dubby.dubbyParameters[RVB_MIX].value;
        reverbDryAmplitude = 1.f - reverbWetAmplitude;
        reverbLeft.SetFeedback(dubby.dubbyParameters[RVB_FEEDBACK].value);
        reverbRight.SetFeedback(dubby.dubbyParameters[RVB_FEEDBACK].value);
        reverbLeft.SetLpFreq(dubby.dubbyParameters[RVB_LPF_CUTOFF].value);
        reverbRight.SetLpFreq(dubby.dubbyParameters[RVB_LPF_CUTOFF].value);
        // Set the filter frequency and resonance for left and right channels
        float cutoffFreq = dubby.dubbyParameters[FLT_CUTOFF].value;
        float resonance = dubby.dubbyParameters[FLT_RESONANCE].value;

        filterL.SetFreq(cutoffFreq);
        filterR.SetFreq(cutoffFreq);

        filterL.SetRes(resonance);
        filterR.SetRes(resonance);

        driveAmount = dubby.dubbyParameters[DRV_AMOUNT].value;

        driveReverbLeft.SetDrive(driveAmount);
        driveReverbRight.SetDrive(driveAmount);
        driveDelayLeft.SetDrive(driveAmount);
        driveDelayRight.SetDrive(driveAmount); 

        driveGainCompensation = dubby.dubbyParameters[DRV_GAIN_COMPENSATION].value;
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
}