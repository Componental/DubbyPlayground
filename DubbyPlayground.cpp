
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"
#define NUM_STRINGS 5 // Define the number of strings

using namespace daisy;
using namespace daisysp;

Dubby dubby;
int oldestPlayingVoice = 0;
bool voiceInUse[NUM_STRINGS] = {false};

StringVoice DSY_SDRAM_BSS strings[NUM_STRINGS];
float freq;
float accent;
float damping;
float structure;
float brightness;
float outputVolume;

float freqs[NUM_STRINGS] = {}; // Array to store frequencies for each string
int notes[NUM_STRINGS] = {};   // Array to store frequencies for each string

bool triggers[NUM_STRINGS] = {}; // Array to store trigger states for each string

bool triggerString;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

float sample_rate;
float outGain = 0.1f;
const int NUM_PAGES = 1; // assuming 4 types of drums: bass, snare, tom, hihat

const char *algorithmTitles[NUM_PAGES] = {"OSC 1"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"DAMP", "STRUC", "BRIGHT", "OUTPUT"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {.4f, .4f, .2f, .4f}}; // default value for amp env

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[2][NUM_AUDIO_CHANNELS] = {0.0f};

    for (size_t i = 0; i < size; i++)
    {

        float _inLeft = SetGains(dubby, 0, i, in, out);
        float _inRight = SetGains(dubby, 1, i, in, out);

        float mix = 0.0f;
        for (int j = 0; j < NUM_STRINGS; j++)
        {
            mix += strings[j].Process(triggers[j]);
        }
        out[0][i] = out[2][i] = (mix * 0.2f * outputVolume) + _inLeft;
        out[1][i] = out[3][i] = (mix * 0.2f * outputVolume) + _inRight;
        // Reset all string triggers
        triggerString = false;
        for (int j = 0; j < NUM_STRINGS; j++)
        {
            triggers[j] = false;
        }
    }

    SetRMSValues(dubby, sumSquared);
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);

    float sample_rate = dubby.seed.AudioSampleRate();

    for (int i = 0; i < NUM_STRINGS; i++)
    {
        strings[i].Init(sample_rate);
    }

    dubby.seed.StartAudio(AudioCallback);

    // initLED();
    // setLED(0, BLUE, 0);
    // setLED(1, RED, 0);
    // updateLED();

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        float minOutput = 0.001f;
        float maxOutput = 1.f;

        // Get the knob values
        // float knob1Value = savedKnobValues[0][0]; // E.G GAIN
        // float knob2Value = savedKnobValues[0][1]; // E.G RESONANCE
        // float knob3Value = savedKnobValues[0][2]; // E.G CUTOFF
        // float knob4Value = savedKnobValues[0][3]; // E.G CUTOFF
        float damp = dubby.GetParameterValue(dubby.dubbyParameters[DAMP]);
        float struc = dubby.GetParameterValue(dubby.dubbyParameters[STRUC]);
        float bright = dubby.GetParameterValue(dubby.dubbyParameters[BRIGHT]);
        float outGain = dubby.GetParameterValue(dubby.dubbyParameters[OUTPUT]);

        outputVolume = daisysp::fmap(outGain, minOutput, maxOutput, Mapping::LOG);

        damp += 1.f - dubby.GetKnobValue(dubby.CTRL_6) - 0.5f;
        bright += 1.f - dubby.GetKnobValue(dubby.CTRL_5) - 0.5f;

        float knob1ValueClamped = std::max(std::min(damp, 1.f), 0.f);
        float knob3ValueClamped = std::max(std::min(bright, 1.f), 0.f);

        //            std::vector<float> knobValues = { knob1ValueClamped, knob2Value, knob3ValueClamped, knob4Value};
        //  dubby.updateKnobValues(knobValues);
        structure = struc;
        brightness = knob3ValueClamped;

        for (int i = 0; i < NUM_STRINGS; i++)
        {
            freq = freqs[i];
            float minFrequency = 0.f;
            float maxFrequency = 500.f;

            float frequencyRatio = (freq - minFrequency) / (maxFrequency - minFrequency);
            damping = 0.9f - (frequencyRatio * 0.58f); // Linear interpolation from 1 to 0.5

            strings[i].SetFreq(freq);
            // strings[i].SetAccent(accent);
            strings[i].SetDamping(damping * (1.f - knob1ValueClamped));
            strings[i].SetSustain(false);
            strings[i].SetStructure(structure);
            strings[i].SetBrightness(brightness);
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
        if (p.velocity != 0 && p.note < 86)
        {
            bool assigned = false;
            // Check if the note is already being played
            for (int i = 0; i < NUM_STRINGS; i++)
            {
                if (notes[i] == p.note)
                {
                    freqs[i] = mtof(p.note);
                    strings[i].SetFreq(freqs[i]);
                    strings[i].SetAccent((p.velocity / 127.f) * dubby.GetParameterValue(dubby.dubbyParameters[ACCENT]));
                    triggers[i] = true;
                    voiceInUse[i] = true;
                    assigned = true;
                    break;
                }
            }

            // If the note is not already being played, find the first unused string
            if (!assigned)
            {
                for (int i = 0; i < NUM_STRINGS; i++)
                {
                    if (!voiceInUse[i])
                    {
                        freqs[i] = mtof(p.note);
                        notes[i] = p.note;
                        strings[i].SetFreq(freqs[i]);
                        strings[i].SetAccent((p.velocity / 127.f) * dubby.GetParameterValue(dubby.dubbyParameters[ACCENT]));
                        triggers[i] = true;
                        voiceInUse[i] = true; // Mark the voice as in use
                        assigned = true;
                        break;
                    }
                }
            }

            // If no unused string is found, assign the note to the oldest playing voice
            if (!assigned)
            {
                freqs[oldestPlayingVoice] = mtof(p.note);
                notes[oldestPlayingVoice] = p.note;
                strings[oldestPlayingVoice].SetFreq(freqs[oldestPlayingVoice]);
                triggers[oldestPlayingVoice] = true;
                // Update the oldest playing voice (round-robin)
                oldestPlayingVoice = (oldestPlayingVoice + 1) % NUM_STRINGS;
            }
        }
        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        // Turn off triggering for the string with matching MIDI note
        for (int i = 0; i < NUM_STRINGS; i++)
        {
            if (notes[i] == p.note)
            {
                triggers[i] = false;
                voiceInUse[i] = false; // Mark the voice as unused
                notes[i] = 0;          // Reset the note value
                break;
            }
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
