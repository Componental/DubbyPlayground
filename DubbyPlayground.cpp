#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

// SYNTH
VariableShapeOscillator osc1, osc2;
LadderFilter flt;
const int NUM_PAGES = 4; // assuming 4 types of drums: bass, snare, tom, hihat

// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"OSC. 1", "OSC 2", "FILTER", "AMP"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"CUTOFF", "RES", "DRIVE", "ENV"},
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
    for (size_t i = 0; i < size; i++)
    {
        float oscOut = osc1.Process() + osc2.Process(); // Combine oscillator outputs


        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // === AUDIO CODE HERE ===================
            out[0][i] = out [1][i] = out[2][i] = out[3][i] = oscOut * 0.1f;
            // =======================================
        }
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    setNumPages(NUM_PAGES);

    float sample_rate = dubby.seed.AudioSampleRate();

    osc1.Init(sample_rate);
    osc1.SetWaveshape(0);
    osc1.SetSync(false);
    osc1.SetFreq(440.f); // Set an initial frequency for osc1

    osc2.Init(sample_rate);
    osc2.SetWaveshape(0);
    osc2.SetSync(false);
    osc2.SetFreq(440.f); // Set an initial frequency for osc2
    
flt.Init(sample_rate);
flt.SetFreq(5000.f);
flt.SetRes(0.7f);

    dubby.seed.StartAudio(AudioCallback);

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        // Update oscillators
        osc1.SetPW(savedKnobValues[0][0]);
        osc1.SetSyncFreq(440.f * savedKnobValues[0][2]);

        osc2.SetPW(savedKnobValues[1][0]);
        osc2.SetSyncFreq(440.f * savedKnobValues[1][2]);

        flt.SetFreq(5000.f);
flt.SetRes(0.7f);


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