#include "Dubby.h"
#include "implementations/includes.h"
#include "reverbsc.h"
using namespace daisy;
using namespace daisysp;

Dubby dubby;
int outChannel;
int inChannel = 0;

bool midiClockStarted = false;
bool midiClockStoppedByButton2 = false;
ReverbSc DSY_SDRAM_BSS reverb[2];        // Array for reverb channels
Overdrive driveReverb[2], driveDelay[2]; // Arrays for overdrive

// DELAY EFFECT
#define MAX_DELAY static_cast<size_t>(48000 * 6.0f)
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayLine[2]; // Array for delay lines

float sample_rate, dry[2], delayOut[2], delayTimeMillis[2], delayFeedback, delaySamples[2];
float currentDelay[2];
float reverbDelayDryWetMix[2], delayDryAmplitude, delayWetAmplitude;

float reverbDryAmplitude;

float reverbAmplitudeAdjustment = 0.7f;

float divisor = 1;
float distortedReverb[2], distortedDelay[2], driveGainCompensation;

// FILTER
Svf filter[2]; // Array for filters

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

// Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<PersistantMemoryParameterSettings> SavedParameterSettings(dubby.seed.qspi);
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    dubby.ProcessLFO();
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};
    bool freeze = dubby.dubbyParameters[DLY_FREEZE].value > 0.5f;

    for (size_t i = 0; i < size; i++)
    {
        AssignScopeData(dubby, i, in, out);

        float processedSample[2];

        dry[0] = in[2][i];
        dry[1] = in[3][i];

        for (int j = 0; j < 2; j++)
        {
            // === REVERB EFFECT ===
            reverb[j].Process(dry[j], &processedSample[j]);
            float reverbWet = processedSample[j] * dubby.dubbyParameters[RVB_MIX].value;

            // Apply overdrive to the mixed signal
            distortedReverb[j] = driveReverb[j].Process(reverbWet);

            // Mix the dry signal with the reverb signal
            dry[j] = (dry[j] * reverbDryAmplitude) + (distortedReverb[j] * driveGainCompensation);

            // === DELAY EFFECT ===
            if (freeze)
            {
                delayFeedback = 1.0f;
                delayOut[j] = delayLine[j].Read();
                delayLine[j].Write(delayOut[j]);
            }
            else
            {
                fonepole(currentDelay[j], delayTimeMillis[j], 0.0001f);

                delaySamples[j] = currentDelay[j] * (sample_rate / 1000);
                delayLine[j].SetDelay(delaySamples[j]);

                delayOut[j] = delayLine[j].Read();

                delayLine[j].Write((delayFeedback * delayOut[j]) + dry[j]);
            }

            // Process the delay outputs through the filters and overdrive
            filter[j].Process(delayOut[j]);
            float delayWetOutput = filter[j].Low() * delayWetAmplitude;

            distortedDelay[j] = driveDelay[j].Process(delayWetOutput) * driveGainCompensation;
            reverbDelayDryWetMix[j] = distortedDelay[j] * delayWetAmplitude + dry[j] * delayDryAmplitude;

            // Update output channels 1 and 3
            if (j == 0)
            {
                out[0][i] = SoftLimit(reverbDelayDryWetMix[j]) * dubby.dubbyParameters[OUT_GAIN].value;
            }
            else
            {
                out[1][i] = SoftLimit(reverbDelayDryWetMix[j]) * dubby.dubbyParameters[OUT_GAIN].value;
            }
        }

        // Clear channels 2 and 4 (since we're not using them)
        out[2][i] = 0;
        out[3][i] = 0;

        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
  //  InitPersistantMemory(dubby, SavedParameterSettings);

    dubby.joystickIdleX = dubby.GetKnobValue(dubby.CTRL_5);
    dubby.joystickIdleY = dubby.GetKnobValue(dubby.CTRL_6);

    setLED(1, NO_COLOR, 0);
    setLED(0, NO_COLOR, 0);
    updateLED();
    // Init DSP
    sample_rate = dubby.seed.AudioSampleRate();

    for (int j = 0; j < 2; j++)
    {
        delayLine[j].Init();
        reverb[j].Init(sample_rate);
        reverb[j].SetFeedback(0.5f);
        reverb[j].SetLpFreq(20000.f);

        filter[j].Init(sample_rate);
        filter[j].SetFreq(500.0);
        filter[j].SetRes(0.3f);
        filter[j].SetDrive(0.8f);

        driveReverb[j].Init();
        driveDelay[j].Init();
    }

    dubby.seed.StartAudio(AudioCallback);
    float sample_rate = dubby.seed.AudioSampleRate();

    while (1)
    {
        //MonitorPersistantMemory(dubby, SavedParameterSettings);

        Monitor(dubby);
        MonitorMidi();
        setLED(1, TURQUOISE, abs(0.5 + dubby.lfo1Value) * 50);
        setLED(0, TURQUOISE, abs(0.5 + dubby.lfo2Value) * 50);
        updateLED();

        delayWetAmplitude = dubby.dubbyParameters[DLY_MIX].value;
        delayDryAmplitude = 1.f - delayWetAmplitude;

        if (dubby.dubbyParameters[DLY_MAXWET].value > 0.5f)
        {
            delayDryAmplitude = 0.f;
            delayWetAmplitude = 1.f;
        }

        delayFeedback = dubby.dubbyParameters[DLY_FEEDBACK].value;

        reverbDryAmplitude = 1.f - dubby.dubbyParameters[RVB_MIX].value;

        divisor = dubby.dubbyParameters[DLY_DIVISION].value <= 0.05f   ? 8.0f
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

        delayTimeMillis[0] = (dubby.dubbyParameters[DLY_TIME].value - dubby.dubbyParameters[DLY_SPREAD].value) / divisor;
        delayTimeMillis[1] = (dubby.dubbyParameters[DLY_TIME].value + dubby.dubbyParameters[DLY_SPREAD].value) / divisor;

        for (int j = 0; j < 2; j++)
        {
            delayTimeMillis[j] = fmaxf(delayTimeMillis[j], 1.0f);

            reverb[j].SetFeedback(dubby.dubbyParameters[RVB_FEEDBACK].value);
            reverb[j].SetLpFreq(dubby.dubbyParameters[RVB_LPF_CUTOFF].value);

            filter[j].SetFreq(dubby.dubbyParameters[FLT_CUTOFF].value);
            filter[j].SetRes(dubby.dubbyParameters[FLT_RESONANCE].value);

            driveReverb[j].SetDrive(dubby.dubbyParameters[DRV_AMOUNT].value);
            driveDelay[j].SetDrive(dubby.dubbyParameters[DRV_AMOUNT].value);
        }

        driveGainCompensation = dubby.dubbyParameters[DRV_GAIN_COMPENSATION].value;
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
