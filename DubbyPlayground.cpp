#include "Dubby.h"
#include "implementations/includes.h"
#include "reverbsc.h"
#define MAX_DELAY static_cast<size_t>(24000)

using namespace daisy;
using namespace daisysp;

Dubby dubby;

const float smoothingFactor = .0002f;
ReverbSc DSY_SDRAM_BSS verbLeft, verbRight;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS preDelayLeft, preDelayRight;
float sample_rate;
static float smoothedPreDelayValue = 0.0f;
float wetVolumeAdjustment = 0.5f;

Svf lpfLeft, lpfRight; // Array for filters
Svf hpfLeft, hpfRight; // Array for filters

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

        float dryLeft = in[2][i];
        float dryRight = in[3][i];

        // Reverb processing
        float processedSampleLeft, processedSampleRight;
        verbLeft.Process(dryLeft, &processedSampleLeft);
        verbRight.Process(dryRight, &processedSampleRight);

        float wetLeft = processedSampleLeft;
        float wetRight = processedSampleRight;

        // Filter processing
        lpfLeft.Process(wetLeft);
        lpfRight.Process(wetRight);
        hpfLeft.Process(lpfLeft.Low()); // Apply high-pass filter
        hpfRight.Process(lpfRight.Low());

        float filteredWetLeft = hpfLeft.High();
        float filteredWetRight = hpfRight.High();

        fonepole(smoothedPreDelayValue, dubby.dubbyParameters[PREDELAY].value, smoothingFactor);
        preDelayLeft.SetDelay(smoothedPreDelayValue);
        preDelayRight.SetDelay(smoothedPreDelayValue);

        float preDelayedWetLeft = preDelayLeft.Read() * wetVolumeAdjustment;
        float preDelayedWetRight = preDelayRight.Read() * wetVolumeAdjustment;

        float dryWetLeft = (1.0f - dubby.dubbyParameters[MIX].value) * dryLeft + dubby.dubbyParameters[MIX].value * preDelayedWetLeft;
        float dryWetRight = (1.0f - dubby.dubbyParameters[MIX].value) * dryRight + dubby.dubbyParameters[MIX].value * preDelayedWetRight;

        // Output to both stereo channels
        out[0][i] = out[2][i] = dryWetLeft;
        out[1][i] = out[3][i] = dryWetRight;

        preDelayLeft.Write(filteredWetLeft);
        preDelayRight.Write(filteredWetRight);
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    InitPersistantMemory(dubby, SavedParameterSettings);
    sample_rate = dubby.seed.AudioSampleRate();

    dubby.joystickIdleX = dubby.GetKnobValue(dubby.CTRL_5);
    dubby.joystickIdleY = dubby.GetKnobValue(dubby.CTRL_6);

    setLED(1, NO_COLOR, 0);
    setLED(0, NO_COLOR, 0);
    updateLED();
    // setup reverb
    verbLeft.Init(sample_rate);
    verbLeft.SetFeedback(0.5f);
    verbLeft.SetLpFreq(20000.f);
    // setup reverb
    verbRight.Init(sample_rate);
    verbRight.SetFeedback(0.5f);
    verbRight.SetLpFreq(20000.f);
    dubby.seed.StartAudio(AudioCallback);

    preDelayLeft.Init();
    preDelayRight.Init();
    preDelayLeft.SetDelay(dubby.dubbyParameters[PREDELAY].value);
    preDelayRight.SetDelay(dubby.dubbyParameters[PREDELAY].value);

    lpfLeft.Init(sample_rate);
    lpfLeft.SetFreq(500.0);
    lpfLeft.SetRes(0.3f);
    lpfLeft.SetDrive(0.8f);

    lpfRight.Init(sample_rate);
    lpfRight.SetFreq(500.0);
    lpfRight.SetRes(0.3f);
    lpfRight.SetDrive(0.8f);

    hpfLeft.Init(sample_rate);
    hpfLeft.SetFreq(500.0);
    hpfLeft.SetRes(0.3f);
    hpfLeft.SetDrive(0.8f);

    hpfRight.Init(sample_rate);
    hpfRight.SetFreq(500.0);
    hpfRight.SetRes(0.3f);
    hpfRight.SetDrive(0.8f);

    while (1)
    {

        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);
        setLED(1, RED, abs(0.5 + dubby.lfo1Value) * 50);
        setLED(0, RED, abs(0.5 + dubby.lfo2Value) * 50);
        updateLED();

        verbLeft.SetFeedback(dubby.dubbyParameters[LUSH].value);
        // verbLeft.SetLpFreq(dubby.dubbyParameters[COLOUR].value);
        verbRight.SetFeedback(dubby.dubbyParameters[LUSH].value);
        // verbRight.SetLpFreq(dubby.dubbyParameters[COLOUR].value);

        verbLeft.SetFreeze(dubby.buttons[2].Pressed());
        verbRight.SetFreeze(dubby.buttons[2].Pressed());

        // Apply filtering based on COLOUR parameter
        float colourValue = dubby.dubbyParameters[COLOUR].value;


        // If COLOUR is 0 to 0.5, control LPF (500 Hz to 20000 Hz) exponentially
        if (colourValue <= 0.5f)
        {
            // Map the range 0 to 0.5 to an exponential scale for the LPF
            float scaledValue = powf(colourValue / 0.5f, 2.0f); // Apply exponential curve
            float lpfFreq = 500.0f + scaledValue * (20000.0f - 500.0f);
            lpfLeft.SetFreq(lpfFreq);
            lpfRight.SetFreq(lpfFreq);
        }
        // If COLOUR is 0.5 to 1, control HPF (0 Hz to 10000 Hz) exponentially
        else
        {
            // Map the range 0.5 to 1 to an exponential scale for the HPF
            float scaledValue = powf((colourValue - 0.5f) / 0.5f, 2.0f); // Apply exponential curve
            float hpfFreq = scaledValue * 10000.0f;
            hpfLeft.SetFreq(hpfFreq);
            hpfRight.SetFreq(hpfFreq);
        }

        // verbLeft.MakeBubbles(dubby.buttons[3].Pressed());
        // verbRight.MakeBubbles(dubby.buttons[3].Pressed());
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
