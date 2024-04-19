
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

SyntheticBassDrum DSY_SDRAM_BSS bassDrum;
SyntheticBassDrum DSY_SDRAM_BSS tomDrum;
SyntheticSnareDrum DSY_SDRAM_BSS snareDrum;
// HiHat<> DSY_SDRAM_BSS hihat;
std::vector<float> knobValues;
// Define a tolerance for knob adjustment
const float knobTolerance = 0.05f; // Adjust as needed

// Define vectors to store saved knob values for each drum
std::vector<float> savedKnobValuesBassDrum;
std::vector<float> savedKnobValuesSnareDrum;
std::vector<float> savedKnobValuesTomDrum;

bool bassdrumSelected = true;
bool snaredrumSelected = false;
bool tomDrumSelected = false;

bool triggerBassDrum = false;
bool triggerTomDrum = false;
bool triggerSnareDrum = false;

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
    // Get knob values
    float knob1Value = dubby.GetKnobValue(dubby.CTRL_1);
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2);
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3);
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4);

    // Check if a button is pressed and update the selected drum accordingly
    if (dubby.buttons[0].Pressed() && !bassdrumSelected)
    {
        if (bassdrumSelected)
            savedKnobValuesBassDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (snaredrumSelected)
            savedKnobValuesSnareDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (tomDrumSelected)
            savedKnobValuesTomDrum = {knob1Value, knob2Value, knob3Value, knob4Value};

        bassdrumSelected = true;
        snaredrumSelected = false;
        tomDrumSelected = false;
    }
    if (dubby.buttons[1].Pressed() && !snaredrumSelected)
    {
        if (bassdrumSelected)
            savedKnobValuesBassDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (snaredrumSelected)
            savedKnobValuesSnareDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (tomDrumSelected)
            savedKnobValuesTomDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        bassdrumSelected = false;
        snaredrumSelected = true;
        tomDrumSelected = false;
    }
    if (dubby.buttons[2].Pressed() && !tomDrumSelected)
    {
        if (bassdrumSelected)
            savedKnobValuesBassDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (snaredrumSelected)
            savedKnobValuesSnareDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        else if (tomDrumSelected)
            savedKnobValuesTomDrum = {knob1Value, knob2Value, knob3Value, knob4Value};
        bassdrumSelected = false;
        snaredrumSelected = false;
        tomDrumSelected = true;
    }

    // Set drum parameters based on selected drum and knob values
    if (bassdrumSelected)
    {
knobValues =  savedKnobValuesBassDrum;
dubby.savedKnobValuesForVisuals = savedKnobValuesBassDrum;

        // Check knob values for bass drum and update parameters accordingly
        if (withinTolerance(knob1Value, savedKnobValuesBassDrum[0]))
        {
            bassDrum.SetFreq(knob1Value * 200.0f);
            savedKnobValuesBassDrum[0] = knob1Value;
        }
        if (withinTolerance(knob2Value, savedKnobValuesBassDrum[1]))
        {
            bassDrum.SetDecay(knob2Value);
            savedKnobValuesBassDrum[1] = knob2Value;
        }
        if (withinTolerance(knob3Value, savedKnobValuesBassDrum[2]))
        {
            bassDrum.SetTone(knob3Value);
            savedKnobValuesBassDrum[2] = knob3Value;
        }
        if (withinTolerance(knob4Value, savedKnobValuesBassDrum[3]))
        {
            bassDrum.SetDirtiness(knob4Value);
            savedKnobValuesBassDrum[3] = knob4Value;
        }
    }
    if (snaredrumSelected)
    {
knobValues =  savedKnobValuesSnareDrum;
dubby.savedKnobValuesForVisuals = savedKnobValuesSnareDrum;

        // Check knob values for snare drum and update parameters accordingly
        if (withinTolerance(knob1Value, savedKnobValuesSnareDrum[0]))
        {
            snareDrum.SetFreq((knob1Value * 300.f) + 10.f);
            savedKnobValuesSnareDrum[0] = knob1Value;
        }
        if (withinTolerance(knob2Value, savedKnobValuesSnareDrum[1]))
        {
            savedKnobValuesSnareDrum[1] = knob2Value;

            snareDrum.SetAccent(knob2Value);
        }
        if (withinTolerance(knob3Value, savedKnobValuesSnareDrum[2]))
        {
            savedKnobValuesSnareDrum[2] = knob3Value;

            snareDrum.SetDecay(knob3Value);
        }
        if (withinTolerance(knob4Value, savedKnobValuesSnareDrum[3]))
        {
            savedKnobValuesSnareDrum[3] = knob4Value;

            snareDrum.SetSnappy(knob4Value);
        }
    }
    if (tomDrumSelected)
    {
        knobValues =  savedKnobValuesTomDrum;
dubby.savedKnobValuesForVisuals = savedKnobValuesTomDrum;

        // Check knob values for tom drum and update parameters accordingly
        if (withinTolerance(knob1Value, savedKnobValuesTomDrum[0]))
        {
            savedKnobValuesTomDrum[0] = knob1Value;

            tomDrum.SetFreq((knob1Value * 200.f) + 200.f);

        }
        if (withinTolerance(knob2Value, savedKnobValuesTomDrum[1]))
        {
            savedKnobValuesTomDrum[1] = knob2Value;

            tomDrum.SetDecay(knob2Value);
        }
        if (withinTolerance(knob3Value, savedKnobValuesTomDrum[2]))
        {
            savedKnobValuesTomDrum[2] = knob3Value;

            tomDrum.SetTone(knob3Value);
        }
        if (withinTolerance(knob4Value, savedKnobValuesTomDrum[3]))
        {
            savedKnobValuesTomDrum[3] = knob4Value;

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
    // Initialize saved knob values to 0.5 for each drum
    savedKnobValuesBassDrum = {0.5f, 0.5f, 0.5f, 0.5f};
    savedKnobValuesSnareDrum = {0.5f, 0.5f, 0.5f, 0.5f};
    savedKnobValuesTomDrum = {0.5f, 0.5f, 0.5f, 0.5f};

    float sample_rate = dubby.seed.AudioSampleRate();
    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());

    bassDrum.Init(sample_rate);
    tomDrum.Init(sample_rate);
    snareDrum.Init(sample_rate);
    //  hihat.Init(sample_rate);

    bassDrum.SetDecay(0.2);
    bassDrum.SetTone(1.f);
    bassDrum.SetDirtiness(0.5f);
    bassDrum.SetFreq(100.f);
    bassDrum.SetFmEnvelopeAmount(0.7f);
    bassDrum.SetFmEnvelopeDecay(0.7f);

    tomDrum.SetDecay(0.7);
    tomDrum.SetTone(1.f);
    tomDrum.SetDirtiness(0.5f);
    tomDrum.SetFreq(300.f);
    tomDrum.SetFmEnvelopeAmount(0.7f);
    tomDrum.SetFmEnvelopeDecay(0.7f);

    snareDrum.SetAccent(0.7f);
    snareDrum.SetDecay(.7f);
    snareDrum.SetSnappy(.7f);
    snareDrum.SetFreq(.7f);

    //   hihat.SetDecay(0.7f);
    //  hihat.SetSustain(0.8f);
    //  hihat.SetTone(0.7f);
    //  hihat.SetNoisiness(0.3f);

    // initLED();
    // setLED(0, BLUE, 100);
    // setLED(1, RED, 100);
    // updateLED();

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();

        std::string str = std::to_string(int(loadMeter.GetAvgCpuLoad() * 100.0f)) + "%";
        dubby.UpdateStatusBar(&str[0], dubby.MIDDLE);

        handleKnobs();
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
