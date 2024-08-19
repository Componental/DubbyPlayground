
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

// DELAY EFFECT
#define MAX_DELAY static_cast<size_t>(48000 * 6.0f)
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayLineLeft;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayLineRight;

float sample_rate, dryL, dryR, delayOutL, delayOutR, delayTimeMillisL, delayTimeMillisR, delayFeedback, stereoSpread, delaySamplesL, delaySamplesR;
float currentDelayL, currentDelayR;
float dryWetMixL, dryWetMixR, dryAmplitude, wetAmplitude;

float divisor = 1;
float delayTimeMillis = 400.f;

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

    // Retrieve the FREEZE parameter
    bool freeze = dubby.dubbyParameters[DLY_FREEZE].value > 0.5f;

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // === AUDIO PROCESSING CODE ===
            dryL = in[0][i]; // Left channel dry input
            dryR = in[1][i]; // Right channel dry input

            // Check if freeze is activated
            if (freeze)
            {
                // Set feedback to 1 and prevent writing new data to delay buffers
                delayFeedback = 1.0f;
                delayOutL = delayLineLeft.Read();
                delayOutR = delayLineRight.Read();
                delayLineLeft.Write(delayOutL);  // Just keep the old data
                delayLineRight.Write(delayOutR); // Just keep the old data
            }
            else
            {
                // Set and smooth the delay time for the left channel
                fonepole(currentDelayL, delayTimeMillisL, 0.0001f);

                // Set and smooth the delay time for the right channel
                fonepole(currentDelayR, delayTimeMillisR, 0.0001f);

                delaySamplesL = currentDelayL * (sample_rate / 1000);
                delaySamplesR = currentDelayR * (sample_rate / 1000);

                // Set delay times for delay lines
                delayLineLeft.SetDelay(delaySamplesL);
                delayLineRight.SetDelay(delaySamplesR);

                // Read delay outputs
                delayOutL = delayLineLeft.Read();
                delayOutR = delayLineRight.Read();

                // Write the feedback and current input to the delay lines
                delayLineLeft.Write((delayFeedback * delayOutL) + dryL);
                delayLineRight.Write((delayFeedback * delayOutR) + dryR);
            }

            // Process the delay outputs through the filters
            filterL.Process(delayOutL);
            filterR.Process(delayOutR);

            // Mix the dry and wet signals and apply soft limiting
            dryWetMixL = (dryL * dryAmplitude) + (filterL.Low() * wetAmplitude);
            dryWetMixR = (dryR * dryAmplitude) + (filterR.Low() * wetAmplitude);

            // Output the final processed signals with gain and soft limiting
            out[0][i] = SoftLimit(dryWetMixL) * outGain;
            out[1][i] = SoftLimit(dryWetMixR) * outGain;

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
    InitPersistantMemory(dubby, SavedParameterSettings);
    // Init DSP
    delayLineLeft.Init();
    delayLineRight.Init();
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

    // DELETE MEMORY
    SavedParameterSettings.RestoreDefaults();

    while (1)
    {
        MonitorPersistantMemory(dubby, SavedParameterSettings);

        Monitor(dubby);
        MonitorMidi();
        // Set the wet and dry mix based on the delay mix parameter
        // Set the wet and dry mix based on the delay mix parameter
        wetAmplitude = dubby.dubbyParameters[DLY_MIX].value;
        dryAmplitude = 1.f - wetAmplitude;

        // Check if DLY_MAXWET is greater than 0.5
        if (dubby.dubbyParameters[DLY_MAXWET].value > 0.5f)
        {
            dryAmplitude = 0.f; // Set dry amplitude to 0
            wetAmplitude = 1.f; // Set wet amplitude to 1 (fully wet)
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

        // Set the filter frequency and resonance for left and right channels
        float cutoffFreq = dubby.dubbyParameters[FLT_CUTOFF].value;
        float resonance = dubby.dubbyParameters[FLT_RESONANCE].value;

        filterL.SetFreq(cutoffFreq);
        filterR.SetFreq(cutoffFreq);

        filterL.SetRes(resonance);
        filterR.SetRes(resonance);
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