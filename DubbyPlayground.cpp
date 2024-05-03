
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

SyntheticBassDrum DSY_SDRAM_BSS bassDrum, tomDrum;
SyntheticSnareDrum DSY_SDRAM_BSS snareDrum;
// HiHat<> DSY_SDRAM_BSS hihat;
std::vector<float> knobValues;
// Define a tolerance for knob adjustment
const float knobTolerance = 0.05f; // Adjust as needed



bool bassdrumSelected = true;
bool snaredrumSelected = false;
bool tomDrumSelected = false;

bool triggerBassDrum = false;
bool triggerTomDrum = false;
bool triggerSnareDrum = false;

// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum
const int NUM_PAGES = 3; // assuming 3 types of drums: bass, snare, and tom
float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {{0.5f}}; // Initialized to 0.5
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

            out[0][i] = out[1][i] = (bassDrum.Process(triggerBassDrum) + tomDrum.Process(triggerTomDrum) + snareDrum.Process(triggerSnareDrum)) * 0.3f; // + hihat.Process(triggerHihat);
            // Reset all drum triggers
            triggerBassDrum= false;
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

    // Get knob values
    float knob1Value = dubby.GetKnobValue(dubby.CTRL_1);
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2);
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3);
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4);

    // Check if a button is pressed and update the selected drum accordingly

    if (dubby.buttons[0].Pressed() && !bassdrumSelected)
    {

        bassdrumSelected = true;
        snaredrumSelected = false;
        tomDrumSelected = false;
    }
    if (dubby.buttons[1].Pressed() && !snaredrumSelected)
    {
        bassdrumSelected = false;
        snaredrumSelected = true;
        tomDrumSelected = false;
    }
    if (dubby.buttons[2].Pressed() && !tomDrumSelected)
    {
        bassdrumSelected = false;
        snaredrumSelected = false;
        tomDrumSelected = true;
    }

    // Set drum parameters based on selected drum and knob values
    if (bassdrumSelected)
    {
        dubby.algorithmTitle = "BASS DRUM";
        dubby.UpdateAlgorithmTitle();

        // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++) {
            knobWithinTolerance[1][i] = false; // Snare
            knobWithinTolerance[2][i] = false; // Tom

            knobValues[i] = savedKnobValues[0][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[0][i];

        }

        
        // Check knob values for bass drum and update parameters accordingly
        if (knobWithinTolerance[0][0] || withinTolerance(knob1Value, savedKnobValues[0][0]))
        {
            knobWithinTolerance[0][0] = true;

            bassDrum.SetFreq(knob1Value * 200.0f);
            savedKnobValues[0][0] = knob1Value;
        }
        if (knobWithinTolerance[0][1] || withinTolerance(knob2Value, savedKnobValues[0][1]))
        {
            knobWithinTolerance[0][1] = true;

            bassDrum.SetDecay(knob2Value);
            savedKnobValues[0][1] = knob2Value;
        }
        if (knobWithinTolerance[0][2] || withinTolerance(knob3Value, savedKnobValues[0][2]))
        {
            knobWithinTolerance[0][2] = true;

            bassDrum.SetTone(knob3Value);
            savedKnobValues[0][2] = knob3Value;
        }
        if (knobWithinTolerance[0][3] || withinTolerance(knob4Value, savedKnobValues[0][3]))
        {
            knobWithinTolerance[0][3] = true;

            bassDrum.SetDirtiness(knob4Value);
            savedKnobValues[0][3] = knob4Value;
        }
    }
    if (snaredrumSelected)
    {
        dubby.algorithmTitle = "SNARE DRUM";
        dubby.UpdateAlgorithmTitle();

  // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++) {
            knobWithinTolerance[0][i] = false; // bassdrum
            knobWithinTolerance[2][i] = false; // Tom

            knobValues[i] = savedKnobValues[1][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[1][i];

        }

        // Check knob values for snare drum and update parameters accordingly
        if (knobWithinTolerance[1][0] || withinTolerance(knob1Value, savedKnobValues[1][0]))
        {
            knobWithinTolerance[1][0] = true;

            snareDrum.SetFreq((knob1Value * 300.f) + 10.f);
            savedKnobValues[1][0] = knob1Value;
        }
        if (knobWithinTolerance[1][1] || withinTolerance(knob2Value, savedKnobValues[1][1]))
        {
            knobWithinTolerance[1][1] = true;

            savedKnobValues[1][1] = knob2Value;
            snareDrum.SetAccent(knob2Value);
        }
        if (knobWithinTolerance[1][2] || withinTolerance(knob3Value, savedKnobValues[1][2]))
        {
            knobWithinTolerance[1][2] = true;

            savedKnobValues[1][2] = knob3Value;
            snareDrum.SetDecay(knob3Value);
        }
        if (knobWithinTolerance[1][3] || withinTolerance(knob4Value, savedKnobValues[1][3]))
        {
            knobWithinTolerance[1][3] = true;

            savedKnobValues[1][3] = knob4Value;
            snareDrum.SetSnappy(knob4Value);
        }
    }
    if (tomDrumSelected)
    {
        dubby.algorithmTitle = "TOM DRUM";
        dubby.UpdateAlgorithmTitle();

 // Reset tolerance flags for other drums
        for (int i = 0; i < NUM_KNOBS; i++) {
            knobWithinTolerance[0][i] = false; // bassdrum
            knobWithinTolerance[1][i] = false; // snare


            knobValues[i] = savedKnobValues[2][i];
            dubby.savedKnobValuesForVisuals[i] = savedKnobValues[2][i];

        }        
        

        // Check knob values for tom drum and update parameters accordingly
        if (knobWithinTolerance[2][0] || withinTolerance(knob1Value, savedKnobValues[2][0]))
        {
            knobWithinTolerance[2][0] = true;

            savedKnobValues[2][0] = knob1Value;
            tomDrum.SetFreq((knob1Value * 200.f) + 200.f);
        }
        if (knobWithinTolerance[2][1] || withinTolerance(knob2Value, savedKnobValues[2][1]))
        {
            knobWithinTolerance[2][1] = true;

            savedKnobValues[2][1] = knob2Value;
            tomDrum.SetDecay(knob2Value);
        }
        if (knobWithinTolerance[2][2] || withinTolerance(knob3Value, savedKnobValues[2][2]))
        {
            knobWithinTolerance[2][2] = true;
            savedKnobValues[2][2] = knob3Value;
            tomDrum.SetTone(knob3Value);
        }
        if (knobWithinTolerance[2][3] || withinTolerance(knob4Value, savedKnobValues[2][3]))
        {
            knobWithinTolerance[2][3] = true;
            savedKnobValues[2][3] = knob4Value;
            tomDrum.SetDirtiness(knob4Value);
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
for (int i = 0; i < NUM_PAGES; ++i) {
    for (int j = 0; j < NUM_KNOBS; ++j) {
        knobWithinTolerance[i][j] = false;
        savedKnobValues[i][j] = 0.5f;

    }
}
    bassDrum.Init(sample_rate);
    tomDrum.Init(sample_rate);
    snareDrum.Init(sample_rate);

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
                break;
            case 61: // Snare drum
                triggerSnareDrum = true;
                break;
            case 62: // Tom
                triggerTomDrum = true;
                break;
            case 63: // Hi-hat
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
