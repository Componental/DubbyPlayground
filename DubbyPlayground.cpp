#include "Dubby.h"
#include "implementations/includes.h"
#include "reverbsc.h"
#define MAX_DELAY static_cast<size_t>(24000)
const float smoothingFactor = .0002f;
ReverbSc DSY_SDRAM_BSS verbLeft, verbRight;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS preDelayLeft, preDelayRight;
float sample_rate;
static float smoothedPreDelayValue = 0.0f;
float wetVolumeAdjustment = 0.5f;

using namespace daisy;
using namespace daisysp;

Dubby dubby;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

// Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<PersistantMemoryParameterSettings> SavedParameterSettings(dubby.seed.qspi);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    dubby.ProcessLFO();

    for (size_t i = 0; i < size; i++)
    {
        AssignScopeData(dubby, i, in, out);

        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {

            // Process input samples through the reverb effect
            float processedSampleLeft;
            float processedSampleRight;

            float dryLeft = in[2][i];
            float dryRight = in[3][i];

            //  float shiftedLeft = pitchshiftLeft.Process(dryLeft);
            //  float shiftedRight = pitchshiftRight.Process(dryRight);

            verbLeft.Process(dryLeft, &processedSampleLeft);
            verbRight.Process(dryRight, &processedSampleRight);

            float wetLeft = processedSampleLeft;
            float wetRight = processedSampleRight;

            fonepole(smoothedPreDelayValue, PREDELAYTIME, smoothingFactor);
            if (smoothedPreDelayValue < 0.01f)
            {
                smoothedPreDelayValue = 0.01f;
            }

            preDelayLeft.SetDelay(smoothedPreDelayValue * 0.5f * sample_rate);
            preDelayRight.SetDelay(smoothedPreDelayValue * 0.5f * sample_rate);

            float preDelayedWetLeft = preDelayLeft.Read() * wetVolumeAdjustment;
            float preDelayedWetRight = preDelayRight.Read() * wetVolumeAdjustment;

            // Dry/Wet mix for left and right channels
            float dryWetLeft = (1.0f - MIX) * dryLeft + MIX * preDelayedWetLeft;
            float dryWetRight = (1.0f - MIX) * dryRight + MIX * preDelayedWetRight;

            // Output to both stereo channels
            out[0][i] = out[2][i] = dryWetLeft;
            out[1][i] = out[3][i] = dryWetRight; // Mix the processed sample with the original input
            // out[j][i] = in[j][i] + processed_sample;
            preDelayLeft.Write(wetLeft);
            preDelayRight.Write(wetRight);
            // =======================================
        }
    }
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

    dubby.seed.StartAudio(AudioCallback);
    sample_rate = dubby.seed.AudioSampleRate();

    // setup reverb
    verbLeft.Init(sample_rate);
    verbLeft.SetFeedback(0.5f);
    verbLeft.SetLpFreq(20000.0f);
    // setup reverb
    verbRight.Init(sample_rate);
    verbRight.SetFeedback(0.5f);
    verbRight.SetLpFreq(20000.0f);

    preDelayLeft.Init();
    preDelayRight.Init();
    preDelayLeft.SetDelay(10.f);
    preDelayRight.SetDelay(10.f);

    verbLeft.SetFeedback(0.5f);
    verbLeft.SetLpFreq(20.f);
    verbRight.SetFeedback(0.5f);
    verbRight.SetLpFreq(20.f);

    while (1)
    {

        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);
        setLED(1, RED, abs(0.5 + dubby.lfo1Value) * 50);
        setLED(0, RED, abs(0.5 + dubby.lfo2Value) * 50);
        updateLED();

        verbLeft.SetFeedback(feedback);
        verbLeft.SetLpFreq(cutoff);
        verbRight.SetFeedback(feedback);
        verbRight.SetLpFreq(cutoff);
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
        MidiEvent m = dubby.midi_usb.PopEvent();
        HandleMidiMessage(m);
    }

    // Handle UART MIDI Events
    while (dubby.midi_uart.HasEvents())
    {
        MidiEvent m = dubby.midi_uart.PopEvent();
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                HandleMidiMessage(m);
            }
        }
    }
}
