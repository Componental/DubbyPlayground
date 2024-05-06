
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
// HiHat<> DSY_SDRAM_BSS hihat;
std::vector<float> knobValues;
// Define a tolerance for knob adjustment
const float knobTolerance = 0.05f; // Adjust as needed

bool page0Selected = true;
bool page1Selected = false;
bool page2Selected = false;
bool page3Selected = false;

bool triggerBassDrum = false;
bool triggerTomDrum = false;
bool triggerSnareDrum = false;

float bassDrumAmplitude = 1.f;
float snareDrumAmplitude = 1.f;
float tomDrumAmplitude = 1.f;
float hihatAmplitude = 1.f;

// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum
const int NUM_PAGES = 4;                                // assuming 4 types of drums: bass, snare, tom, hihat
float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {{defaultKnobValue}}; // Initialized to 0.5
bool knobWithinTolerance[NUM_PAGES][NUM_KNOBS] = {{false}};

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

    // Get encoder value
    int rotationDirection = dubby.encoder.Increment();
    if (rotationDirection != 0)
    {
        selectedPage = (selectedPage + rotationDirection + NUM_PAGES) % NUM_PAGES;
    }
    // Check if a button is pressed and update the selected drum accordingly
    // Set drum parameters based on selected drum and knob values
    page0Selected = selectedPage == 0;
    page1Selected = selectedPage == 1;
    page2Selected = selectedPage == 2;
    page3Selected = selectedPage == 3;

    // Get knob values
    float knob1Value = dubby.GetKnobValue(dubby.CTRL_1);
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2);
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3);
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4);

    // Set drum parameters based on selected drum and knob values
    if (page0Selected)
    {
        dubby.algorithmTitle = "BASS DRUM";
        dubby.customLabels = {"FREQ", "DECAY", "TONE", "DIRT"};
        dubby.UpdateAlgorithmTitle();

        // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++)
        {
            knobWithinTolerance[1][i] = false; // Snare
            knobWithinTolerance[2][i] = false; // Tom
            knobWithinTolerance[3][i] = false; // hihat

            knobValues[i] = savedKnobValues[0][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[0][i];
        }

        // Check knob values for bass drum and update parameters accordingly
        if (knobWithinTolerance[0][0] || withinTolerance(knob1Value, savedKnobValues[0][0]))
        {
            knobWithinTolerance[0][0] = true;

            // PAGE 1, KNOB 1
            ////////////////////////////////////////////////

            bassDrum.SetFreq(knob1Value * 150.0f);

            ////////////////////////////////////////////////

            savedKnobValues[0][0] = knob1Value;
        }
        if (knobWithinTolerance[0][1] || withinTolerance(knob2Value, savedKnobValues[0][1]))
        {
            knobWithinTolerance[0][1] = true;

            // PAGE 1, KNOB 2
            ////////////////////////////////////////////////

            bassDrum.SetDecay(knob2Value);

            ////////////////////////////////////////////////

            savedKnobValues[0][1] = knob2Value;
        }
        if (knobWithinTolerance[0][2] || withinTolerance(knob3Value, savedKnobValues[0][2]))
        {
            knobWithinTolerance[0][2] = true;

            // PAGE 1, KNOB 3
            ////////////////////////////////////////////////

            bassDrum.SetTone(knob3Value);

            ////////////////////////////////////////////////

            savedKnobValues[0][2] = knob3Value;
        }
        if (knobWithinTolerance[0][3] || withinTolerance(knob4Value, savedKnobValues[0][3]))
        {
            knobWithinTolerance[0][3] = true;

            // PAGE 1, KNOB 4
            ////////////////////////////////////////////////

            bassDrum.SetDirtiness(knob4Value);

            ////////////////////////////////////////////////

            savedKnobValues[0][3] = knob4Value;
        }
    }
    if (page1Selected)
    {
        dubby.algorithmTitle = "SNARE DRUM";
        dubby.customLabels = {"FREQ", "DECAY", "ACCENT", "SNAPPY"};
        dubby.UpdateAlgorithmTitle();

        // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++)
        {
            knobWithinTolerance[0][i] = false; // bassdrum
            knobWithinTolerance[2][i] = false; // Tom
            knobWithinTolerance[3][i] = false; // hihat

            knobValues[i] = savedKnobValues[1][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[1][i];
        }

        // Check knob values for snare drum and update parameters accordingly
        if (knobWithinTolerance[1][0] || withinTolerance(knob1Value, savedKnobValues[1][0]))
        {
            knobWithinTolerance[1][0] = true;

            // PAGE 2, KNOB 1
            ////////////////////////////////////////////////

            snareDrum.SetFreq((knob1Value * 300.f) + 10.f);

            ////////////////////////////////////////////////

            savedKnobValues[1][0] = knob1Value;
        }
        if (knobWithinTolerance[1][1] || withinTolerance(knob2Value, savedKnobValues[1][1]))
        {
            knobWithinTolerance[1][1] = true;

            // PAGE 2, KNOB 2
            ////////////////////////////////////////////////

            snareDrum.SetDecay(knob2Value);

            ////////////////////////////////////////////////

            savedKnobValues[1][1] = knob2Value;
        }
        if (knobWithinTolerance[1][2] || withinTolerance(knob3Value, savedKnobValues[1][2]))
        {
            knobWithinTolerance[1][2] = true;

            // PAGE 2, KNOB 3
            ////////////////////////////////////////////////

            snareDrum.SetAccent(knob3Value);

            ////////////////////////////////////////////////

            savedKnobValues[1][2] = knob3Value;
        }
        if (knobWithinTolerance[1][3] || withinTolerance(knob4Value, savedKnobValues[1][3]))
        {
            knobWithinTolerance[1][3] = true;

            // PAGE 2, KNOB 4
            ////////////////////////////////////////////////

            snareDrum.SetSnappy(knob4Value);

            ////////////////////////////////////////////////

            savedKnobValues[1][3] = knob4Value;
        }
    }
    if (page2Selected)
    {
        dubby.algorithmTitle = "TOM DRUM";
        dubby.customLabels = {"FREQ", "DECAY", "TONE", "DIRT"};
        dubby.UpdateAlgorithmTitle();

        // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++)
        {
            knobWithinTolerance[0][i] = false; // bassdrum
            knobWithinTolerance[1][i] = false; // snare
            knobWithinTolerance[3][i] = false; // hihat

            knobValues[i] = savedKnobValues[2][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[2][i];
        }

        // Check knob values for tom drum and update parameters accordingly
        if (knobWithinTolerance[2][0] || withinTolerance(knob1Value, savedKnobValues[2][0]))
        {
            knobWithinTolerance[2][0] = true;

            // PAGE 3, KNOB 1
            ////////////////////////////////////////////////

            tomDrum.SetFreq((knob1Value * 250.f) + 200.f);

            ////////////////////////////////////////////////

            savedKnobValues[2][0] = knob1Value;
        }
        if (knobWithinTolerance[2][1] || withinTolerance(knob2Value, savedKnobValues[2][1]))
        {
            knobWithinTolerance[2][1] = true;

            // PAGE 3, KNOB 2
            ////////////////////////////////////////////////

            tomDrum.SetDecay(knob2Value);

            ////////////////////////////////////////////////

            savedKnobValues[2][1] = knob2Value;
        }
        if (knobWithinTolerance[2][2] || withinTolerance(knob3Value, savedKnobValues[2][2]))
        {
            knobWithinTolerance[2][2] = true;

            // PAGE 3, KNOB 3
            ////////////////////////////////////////////////

            tomDrum.SetTone(knob3Value);

            ////////////////////////////////////////////////

            savedKnobValues[2][2] = knob3Value;
        }
        if (knobWithinTolerance[2][3] || withinTolerance(knob4Value, savedKnobValues[2][3]))
        {
            knobWithinTolerance[2][3] = true;

            // PAGE 3, KNOB 4
            ////////////////////////////////////////////////

            tomDrum.SetDirtiness(knob4Value);

            ////////////////////////////////////////////////

            savedKnobValues[2][3] = knob4Value;
        }
    }

    if (page3Selected)
    {
        dubby.algorithmTitle = "HI-HAT";
        dubby.customLabels = {"ATTACK", "DECAY", "COLOUR", "RESO"};
        dubby.UpdateAlgorithmTitle();

        // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++)
        {
            knobWithinTolerance[0][i] = false; // bassdrum
            knobWithinTolerance[1][i] = false; // snare
            knobWithinTolerance[2][i] = false; // tom
            // knobWithinTolerance[3][i] = true;  // hihat

            knobValues[i] = savedKnobValues[3][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[3][i];
        }

        // Check knob values for hi-hat and update parameters accordingly
        if (knobWithinTolerance[3][0] || withinTolerance(knob1Value, savedKnobValues[3][0]))
        {
            knobWithinTolerance[3][0] = true;

            // PAGE 4, KNOB 1
            ////////////////////////////////////////////////

            hihat.SetAttack(knob1Value);

            ////////////////////////////////////////////////

            savedKnobValues[3][0] = knob1Value;
        }
        if (knobWithinTolerance[3][1] || withinTolerance(knob2Value, savedKnobValues[3][1]))
        {
            knobWithinTolerance[3][1] = true;

            // PAGE 4, KNOB 2
            ////////////////////////////////////////////////

            hihat.SetDecay(knob2Value);

            ////////////////////////////////////////////////

            savedKnobValues[3][1] = knob2Value;
        }
        if (knobWithinTolerance[3][2] || withinTolerance(knob3Value, savedKnobValues[3][2]))
        {
            knobWithinTolerance[3][2] = true;

            // PAGE 4, KNOB 3
            ////////////////////////////////////////////////

            hihat.SetFreq(knob3Value);

            ////////////////////////////////////////////////

            savedKnobValues[3][2] = knob3Value;
        }
        if (knobWithinTolerance[3][3] || withinTolerance(knob4Value, savedKnobValues[3][3]))
        {
            knobWithinTolerance[3][3] = true;

            // PAGE 4, KNOB 4
            ////////////////////////////////////////////////

            hihat.SetFilterRes(knob4Value);

            ////////////////////////////////////////////////

            savedKnobValues[3][3] = knob4Value;
        }
    }
    // Update knob values in Dubby class
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
