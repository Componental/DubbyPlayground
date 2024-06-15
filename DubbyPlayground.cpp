
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

const int NUM_PAGES = 4; // assuming 4 types of drums: bass, snare, tom, hihat

float prevKnobValues[NUM_PAGES][NUM_KNOBS + 2] = {};
int buttonStates[NUM_PAGES][NUM_BUTTONS] = {};

// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"MIDI CTRL P1", "MIDI CTRL P2", "MIDI CTRL P3", "MIDI CTRL P4"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"PRM 1", "PRM 2", "PRM 3", "PRM 4"},
    {"PRM 5", "PRM 6", "PRM 7", "PRM 8"},
    {"PRM 9", "PRM 10", "PRM 11", "PRM 12"},
    {"PRM 13", "PRM 14", "PRM 15", "PRM 16"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {.4f, .6f, .4f, .0f}, // default value bass drum
    {.2f, .4f, .6f, .7f}, // default value snare
    {.2f, .2f, .2f, .2f}, // default value tom
    {.0f, .2f, .5f, .3f}};

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            float _in = SetGains(dubby, j, i, in, out);

            // === AUDIO CODE HERE ===================

            // =======================================

            CalculateRMS(dubby, _in, out[j][i], j, sumSquared);
        }
        AssignScopeData(dubby, i, in, out);
    }

    SetRMSValues(dubby, sumSquared);
}

// Previous knob value to detect changes

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    setNumPages(NUM_PAGES);

    dubby.seed.StartAudio(AudioCallback);

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        // Get current knob value
        // Determine the current page
        int currentPage = getSelectedPage(); // Assuming you have a function to get the current page
        float currentKnobValues[NUM_KNOBS + 2];

        // Get current knob values
        enum Ctrl
        {
            CTRL_1,
            CTRL_2,
            CTRL_3,
            CTRL_4
        };

        // Handle buttons
        for (int buttonIndex = 0; buttonIndex < NUM_BUTTONS; buttonIndex++)
        {
            if (dubby.buttons[buttonIndex].RisingEdge())
            {
                // Toggle the button state
                buttonStates[currentPage][buttonIndex] = !buttonStates[currentPage][buttonIndex];
                // Send MIDI control change
                int value = buttonStates[currentPage][buttonIndex] ? 127 : 0;
                MIDISendControlChange(dubby, 0, 16 + (currentPage * NUM_BUTTONS + buttonIndex), value); // Assuming CC numbers 16-x for the buttons
            }
        }

        // HANDLE KNOBS
        for (int knobIndex = 0; knobIndex < NUM_KNOBS; knobIndex++)
        {
            currentKnobValues[knobIndex] = dubby.GetKnobValue(static_cast<daisy::Dubby::Ctrl>(knobIndex)); // Assuming Ctrl enum values correspond to knob indices

            if (fabs(currentKnobValues[knobIndex] - prevKnobValues[currentPage][knobIndex]) > 0.005f)
            {
                // Calculate the MIDI control change number based on the page and knob index
                int ccNumber = currentPage * NUM_KNOBS + knobIndex;
                MIDISendControlChange(dubby, 0, ccNumber, savedKnobValues[currentPage][knobIndex] * 127);
                prevKnobValues[currentPage][knobIndex] = currentKnobValues[knobIndex];
            }
        }

        // HANDLE JOYSTICK
        for (int i = 0; i < numPages; i++)
        {
            if (currentPage == i)
            {
                if (fabs(dubby.GetKnobValue(dubby.CTRL_5) - prevKnobValues[currentPage][4]) > 0.005f)
                {
                    MIDISendControlChange(dubby, 0, 102 + (i * 2), 127 - (dubby.GetKnobValue(dubby.CTRL_5) * 127));
                    prevKnobValues[currentPage][4] = dubby.GetKnobValue(dubby.CTRL_5);
                }
                if (fabs(dubby.GetKnobValue(dubby.CTRL_6) - prevKnobValues[currentPage][5]) > 0.005f)
                {
                    MIDISendControlChange(dubby, 0, 103 + (i * 2), 127 - (dubby.GetKnobValue(dubby.CTRL_6) * 127));
                    prevKnobValues[currentPage][5] = dubby.GetKnobValue(dubby.CTRL_6);
                }
            }
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
    case ControlChange:
    {
        ControlChangeEvent p = m.AsControlChange();
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
