#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"
#include "SynthVoice.h"
using namespace daisy;
using namespace daisysp;

Dubby dubby;
const int NUM_VOICES = 5; // Number of synth voices

SynthVoice synthVoices[NUM_VOICES]; // Array of five synth voices

bool gate[NUM_VOICES];              // Array of gates for each voice
bool note_on[NUM_VOICES] = {false}; // Track if a note is currently on for each voice

float sample_rate;
float outGain = 0.1f;
const int NUM_PAGES = 5; // assuming 4 types of drums: bass, snare, tom, hihat

const char *algorithmTitles[NUM_PAGES] = {"OSC 1", "OSC 2", "FILTER", "FILTER ENV", "AMP. ENV"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"CUTOFF", "RES", "DRIVE", "ENV"},
    {"ATTACK", "DECAY", "SUS", "REL"},
    {"ATTACK", "DECAY", "SUS", "REL"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {.4f, .6f, .0f, .0f},  // default value osc 1
    {.2f, .4f, .02f, .7f}, // default value osc 2
    {.2f, .2f, .1f, .6f},  // default value filter
    {.0f, .3f, .2f, .0f},  // default value filter env
    {.0f, .0f, .2f, .0f}}; // default value for amp env

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        float output = 0.0f; // Initialize output for accumulation of voice outputs

        // Process each synth voice
        for (int v = 0; v < NUM_VOICES; v++)
        {
            synthVoices[v].TriggerEnv(gate[v]);
            output += synthVoices[v].Process();
        }

        // Apply overall gain and output
        output *= outGain;

        // left out
        out[0][i] = output;

        // right out
        out[1][i] = output;
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    setNumPages(NUM_PAGES);

    dubby.seed.StartAudio(AudioCallback);

    sample_rate = dubby.seed.AudioSampleRate();

    // Initialize synth voices
    for (int v = 0; v < NUM_VOICES; v++)
    {
        synthVoices[v].Init(sample_rate);
    }

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        // Handle knob changes, if necessary

        // Check if any button is held for bootloader reset
        for (int v = 0; v < NUM_VOICES; v++)
        {
            synthVoices[v].SetOsc1Shape(savedKnobValues[0][0]);
            synthVoices[v].SetOsc2Shape(savedKnobValues[1][0]);
            synthVoices[v].SetOsc1Tune(savedKnobValues[0][2], savedKnobValues[0][1]);
            synthVoices[v].SetOsc2Tune(savedKnobValues[1][2], savedKnobValues[1][1]);

            synthVoices[v].SetFilterDrive(savedKnobValues[2][2]);
            synthVoices[v].SetFilterRes(savedKnobValues[2][1]);
            synthVoices[v].SetFilterCutoff(savedKnobValues[2][0], savedKnobValues[2][3]);

            // FILTER ENV

            // Map the knob value to a logarithmic scale for cutoff frequency

            synthVoices[v].SetFilterADSR(savedKnobValues[3][0], savedKnobValues[3][1], savedKnobValues[3][2], savedKnobValues[3][3]);
            synthVoices[v].SetAmpADSR(savedKnobValues[4][0], savedKnobValues[4][1], savedKnobValues[4][2], savedKnobValues[4][3]);
        }

        if (dubby.buttons[3].TimeHeldMs() > 300)
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
        int voiceIndex = -1;

        // Find the last note-off voice or the first voice with no note on
        for (int v = NUM_VOICES - 1; v >= 0; v--)
        {
            if (!note_on[v])
            {
                voiceIndex = v;
                break;
            }
        }

        if (voiceIndex != -1)
        {
            gate[voiceIndex] = true;
            note_on[voiceIndex] = true;
            synthVoices[voiceIndex].SetFreq(mtof(p.note));
        }

        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();

        // Find the voice with the matching note
        for (int v = 0; v < NUM_VOICES; v++)
        {
            if (note_on[v] && mtof(p.note) == synthVoices[v].GetFreq())
            {
                gate[v] = false;
                note_on[v] = false;
                break; // Exit loop after releasing the note
            }
        }

        break;
    }
    case SystemRealTime:
    {
        HandleSystemRealTime(m.srt_type);
        break;
    }
    case ControlChange:
    {
        // Handle Control Change messages if needed
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
