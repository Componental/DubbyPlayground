#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"
#include "SynthVoice.h"
using namespace daisy;
using namespace daisysp;

Dubby dubby;

SynthVoice synthVoice;

bool gate;
bool note_on = false; // Track if a note is currently on

float sample_rate;
float outGain = 0.1f;

const int NUM_PAGES = 5; // assuming 4 types of drums: bass, snare, tom, hihat

// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"OSC 1", "OSC 2", "FILTER", "FILTER ENV", "AMP. ENV"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"CUTOFF", "RES", "DRIVE", "ENV"},
    {"ATTACK", "DECAY", "SUS", "REL"},
    {"ATTACK", "DECAY", "SUS", "REL"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {.4f, .6f, .4f, .0f}, // default value bass drum
    {.2f, .4f, .6f, .7f}, // default value snare
    {.7f, .2f, .2f, .2f}, // default value tom
    {.0f, .2f, .5f, .3f}};

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {

         synthVoice.TriggerEnv(gate);

        float output = synthVoice.Process();

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

    synthVoice.Init(sample_rate);
    synthVoice.SetFreq(440.f * savedKnobValues[0][2]);


    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        synthVoice.SetOsc1Shape(savedKnobValues[0][0]);
        synthVoice.SetOsc2Shape(savedKnobValues[1][0]);

        synthVoice.SetFilterDrive(savedKnobValues[2][2]);
        synthVoice.SetFilterRes(savedKnobValues[2][1]);
        synthVoice.SetFilterCutoff(savedKnobValues[2][0], savedKnobValues[2][3]);

        // FILTER ENV

        // Map the knob value to a logarithmic scale for cutoff frequency

        synthVoice.SetFilterADSR(savedKnobValues[3][0], savedKnobValues[3][1], savedKnobValues[3][2], savedKnobValues[3][3]);
        synthVoice.SetAmpADSR(savedKnobValues[4][0], savedKnobValues[4][1], savedKnobValues[4][2], savedKnobValues[4][3]);

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
        if (p.velocity != 0)
        {
            if (!note_on)
            {
                // Note is currently off, so turn it on and trigger gate
                gate = true;
                note_on = true;
            }
            // Set the frequency regardless
            synthVoice.SetFreq(mtof(p.note));
        }
        else
        {
            // Treat velocity 0 as Note Off
            if (p.velocity == 0)
            {
                NoteOffEvent p = m.AsNoteOff();
                // Check if the note off matches the note currently playing (optional, based on your polyphony requirements)
                gate = false;
                note_on = false;
            }
        }
        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        // Check if the note off matches the note currently playing (optional, based on your polyphony requirements)
        gate = false;
        note_on = false;
        break;
    }
    case SystemRealTime:
    {
        HandleSystemRealTime(m.srt_type);
        break;
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