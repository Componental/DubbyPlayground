#include "Dubby.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
int outChannel;
int inChannel = 0;

bool midiClockStarted = false;
bool midiClockStoppedByButton2 = false;

// DELAY EFFECT
#define MAX_DELAY static_cast<size_t>(48000 * 20.0f)
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
    dubby.ProcessLFO();

    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    // Retrieve the FREEZE parameter
    bool freeze = dubby.dubbyParameters[DLY_FREEZE].value > 0.5f;

    for (size_t i = 0; i < size; i++)
    {
        AssignScopeData(dubby, i, in, out);

        // === AUDIO PROCESSING CODE ===
        dryL = in[0][i] + in[2][i]; // Left channel dry input
        dryR = in[1][i] + in[3][i]; // Right channel dry input

        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {

            // Set and smooth the delay time for the left channel
            fonepole(currentDelayL, delayTimeMillisL, 0.0001f);

            // Set and smooth the delay time for the right channel
            fonepole(currentDelayR, delayTimeMillisR, 0.0001f);

            delaySamplesL = currentDelayL * (sample_rate / 1000);
            delaySamplesR = currentDelayR * (sample_rate / 1000);

            // Check if freeze is activated
            if (freeze)
            {
                // Set feedback to 1 and prevent writing new data to delay buffers
                delayFeedback = 1.0f;
                delayLineLeft.SetDelay(delaySamplesL);
                delayLineRight.SetDelay(delaySamplesR);

                delayOutL = delayLineLeft.Read();
                delayOutR = delayLineRight.Read();
                delayLineLeft.Write(delayOutL);  // Just keep the old data
                delayLineRight.Write(delayOutR); // Just keep the old data
            }
            else
            {

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
            out[0][i] = out[2][i] = SoftLimit(dryWetMixL) * outGain;
            out[1][i] = out[3][i] = SoftLimit(dryWetMixR) * outGain;

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

    dubby.joystickIdleX = dubby.GetKnobValue(dubby.CTRL_5);
    dubby.joystickIdleY = dubby.GetKnobValue(dubby.CTRL_6);

    setLED(1, NO_COLOR, 0);
    setLED(0, NO_COLOR, 0);
    updateLED();
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

    while (1)
    {

        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);
        setLED(1, RED, abs(0.5 + dubby.lfo1Value) * 50);
        setLED(0, RED, abs(0.5 + dubby.lfo2Value) * 50);
        updateLED();
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

        if (dubby.dubbyParameters[DLY_SYNC].value > 0.5)
        {
            dubby.dubbyParameters[DLY_TIME_FREE].control = CONTROL_NONE;
            delayTimeMillis = 60000 / dubby.dubbyMidiSettings.currentBpm;
        }
        else
        {
            delayTimeMillis = dubby.dubbyParameters[DLY_TIME_FREE].value;
        }

        delayFeedback = dubby.dubbyParameters[DLY_FEEDBACK].value;
        stereoSpread = dubby.dubbyParameters[DLY_SPREAD].value;

        // Retrieve the output gain parameter
        outGain = dubby.dubbyParameters[OUT_GAIN].value;

        divisor =
            dubby.dubbyParameters[DLY_DIVISION].value <= 0.05f   ? 8.0f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.15f ? 4.0f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.25f ? 3.0f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.35f ? 2.0f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.45f ? 1.5f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.55f ? 1.0f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.65f ? 0.5f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.75f ? 0.33f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.85f ? 0.25f
            : dubby.dubbyParameters[DLY_DIVISION].value <= 0.95f ? 0.2f
                                                                 : 0.125f;

        // Calculate the delay times for left and right channels
        // delayTimeMillisL = (delayTimeMillis - stereoSpread) / divisor;
        // delayTimeMillisR = (delayTimeMillis + stereoSpread) / divisor;
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
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                NoteOnEvent p = m.AsNoteOn();
                (void)p; // Suppress unused variable warning
            }
        }
        break;
    }
    case NoteOff:
    {
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                NoteOffEvent p = m.AsNoteOff();
                (void)p; // Suppress unused variable warning
            }
        }
        break;
    }
    case SystemRealTime:
    {
        if (dubby.dubbyMidiSettings.currentMidiClockOption == FOLLOWER)
        {
            HandleSystemRealTime(m.srt_type, dubby);
        }
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
