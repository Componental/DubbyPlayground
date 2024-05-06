
#include "daisysp.h"
#include "Dubby.h"
#include "Hihat.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

int selectedPage = 0;

float defaultKnobValue = 0.2f;
SyntheticBassDrum DSY_SDRAM_BSS bassDrum, tomDrum;
SyntheticSnareDrum DSY_SDRAM_BSS snareDrum;
Hihat DSY_SDRAM_BSS hihat;

std::vector<float> knobValues;
// Define a tolerance for knob adjustment
const float knobTolerance = 0.05f; // Adjust as needed

bool triggerBassDrum = false;
bool triggerTomDrum = false;
bool triggerSnareDrum = false;

float bassDrumAmplitude = 1.f;
float snareDrumAmplitude = 1.f;
float tomDrumAmplitude = 1.f;
float hihatAmplitude = 1.f;
const int NUM_PAGES = 4; // assuming 4 types of drums: bass, snare, tom, hihat


// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"BASS DRUM", "SNARE DRUM", "TOM DRUM", "HI-HAT"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"FREQ", "DECAY", "TONE", "DIRT"},
    {"FREQ", "DECAY", "ACCENT", "SNAPPY"},
    {"FREQ", "DECAY", "TONE", "DIRT"},
    {"ATTACK", "DECAY", "COLOUR", "RESO"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {defaultKnobValue, defaultKnobValue, defaultKnobValue, defaultKnobValue},
    {defaultKnobValue, defaultKnobValue, defaultKnobValue, defaultKnobValue},
    {defaultKnobValue, defaultKnobValue, defaultKnobValue, defaultKnobValue},
    {defaultKnobValue, defaultKnobValue, defaultKnobValue, defaultKnobValue}
};
bool knobWithinTolerance[NUM_PAGES][NUM_KNOBS] = {
    {false, false, false, false},
    {false, false, false, false},
    {false, false, false, false},
    {false, false, false, false}
};

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    loadMeter.OnBlockStart();

    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================

            out[0][i] = out[1][i] = (bassDrum.Process(triggerBassDrum) * bassDrumAmplitude + tomDrum.Process(triggerTomDrum) * tomDrumAmplitude + snareDrum.Process(triggerSnareDrum) * snareDrumAmplitude + hihat.Process() * hihatAmplitude) * 0.3f;
            // Reset all drum triggers
            
            triggerBassDrum = false;
            triggerTomDrum = false;
            triggerSnareDrum = false;

            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        }
        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
    loadMeter.OnBlockEnd();
}
bool withinTolerance(float value1, float value2)
{
    return fabs(value1 - value2) <= knobTolerance;
}

void handleKnobs()
{
    int rotationDirection = dubby.encoder.Increment();
    if (rotationDirection != 0)
    {
        // Reset all knobs to false for all drums except the selected one
        for (int i = 0; i < NUM_PAGES; i++)
        {
            if (i != selectedPage)
            {
                for (int j = 0; j < NUM_KNOBS; j++)
                {
                    knobWithinTolerance[i][j] = false;
                }
            }
        }

        // Change the selected page
        selectedPage = (selectedPage + rotationDirection + NUM_PAGES) % NUM_PAGES;
    }

    // Update the algorithm title and custom labels for the selected page
    dubby.algorithmTitle = algorithmTitles[selectedPage];
    dubby.customLabels.assign(customLabels[selectedPage], customLabels[selectedPage] + NUM_KNOBS);
    dubby.UpdateAlgorithmTitle();

    // Process knobs for the selected page
    for (int j = 0; j < NUM_KNOBS; j++)
    {
        float knobValue = dubby.GetKnobValue(static_cast<daisy::Dubby::Ctrl>(j));
        // Only check for tolerance if the knob is not already within tolerance
        if (!knobWithinTolerance[selectedPage][j])
        {
            if (withinTolerance(knobValue, savedKnobValues[selectedPage][j]))
            {
                knobWithinTolerance[selectedPage][j] = true;
            }
        }
        // Update the saved value only if it's within tolerance
        if (knobWithinTolerance[selectedPage][j])
        {
            savedKnobValues[selectedPage][j] = knobValue;
        }
        knobValues[j] = savedKnobValues[selectedPage][j];
        dubby.savedKnobValuesForVisuals[j] = savedKnobValues[selectedPage][j];
    }

    // Update the knob values for the visual representation
    dubby.updateKnobValues(knobValues);
}

int main(void)
{

    Init(dubby);
    InitMidiClock(dubby);

    dubby.seed.StartAudio(AudioCallback);

    float sample_rate = dubby.seed.AudioSampleRate();
    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());

    // Initialize all values to false
    for (int i = 0; i < NUM_PAGES; ++i)
    {
        for (int j = 0; j < NUM_KNOBS; ++j)
        {
            knobWithinTolerance[i][j] = false;
            savedKnobValues[i][j] = defaultKnobValue;
        }
    }
    bassDrum.Init(sample_rate);
    tomDrum.Init(sample_rate);
    snareDrum.Init(sample_rate);
    hihat.Init(sample_rate);

    bassDrum.SetFreq(defaultKnobValue * 150.0f);
    tomDrum.SetFreq((defaultKnobValue * 250.f) + 200.f);
    snareDrum.SetFreq((defaultKnobValue * 300.f) + 10.f);
    hihat.SetFreq(defaultKnobValue);

    // Set default parameters for each drum
    // ...

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs();

        // Inside the while loop
        bassDrum.SetFreq(savedKnobValues[0][0] * 150.0f);
        bassDrum.SetDecay(savedKnobValues[0][1]);
        bassDrum.SetTone(savedKnobValues[0][2]);
        bassDrum.SetDirtiness(savedKnobValues[0][3]);

        snareDrum.SetFreq((savedKnobValues[1][0] * 300.f) + 10.f);
        snareDrum.SetDecay(savedKnobValues[1][1]);
        snareDrum.SetAccent(savedKnobValues[1][2]);
        snareDrum.SetSnappy(savedKnobValues[1][3]);

        tomDrum.SetFreq((savedKnobValues[2][0] * 250.f) + 200.f);
        tomDrum.SetDecay(savedKnobValues[2][1]);
        tomDrum.SetTone(savedKnobValues[2][2]);
        tomDrum.SetDirtiness(savedKnobValues[2][3]);

        hihat.SetAttack(savedKnobValues[3][0]);
        hihat.SetDecay(savedKnobValues[3][1]);
        hihat.SetFreq(savedKnobValues[3][2]);
        hihat.SetFilterRes(savedKnobValues[3][3]);

        // Check for bootloader reset
        if (dubby.buttons[3].TimeHeldMs() > 1000)
        {
            dubby.ResetToBootloader();
        }
    }
}

void HandleMidiMessage(MidiEvent m)
{
    switch (m.type)
    {
    case NoteOn:
    {
        NoteOnEvent p = m.AsNoteOn();
        if (p.velocity != 0) // Check if the note is played (velocity != 0)
        {
            switch (p.note)
            {
            case 60: // Bass drum
                triggerBassDrum = true;
                bassDrumAmplitude = p.velocity / 127.f;
                break;
            case 61: // Snare drum
                triggerSnareDrum = true;
                snareDrumAmplitude = p.velocity / 127.f;
                break;
            case 62: // Tom
                triggerTomDrum = true;
                tomDrumAmplitude = p.velocity / 127.f;
                break;
            case 63: // Hi-hat
                hihat.Trigger();
                hihatAmplitude = p.velocity / 127.f;
                break;
            default:
                break;
            }
        }
        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        switch (p.note)
        {
        case 60:
            triggerBassDrum = false;
            break;
        case 61:
            triggerSnareDrum = false;
            break;
        case 62:
            triggerTomDrum = false;
            break;
        case 63:
            break;
        default:
            break;
        }
        break;
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
