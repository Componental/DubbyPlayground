
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

// SYNTH
VariableShapeOscillator osc1, osc2;



const int NUM_PAGES = 4; // assuming 4 types of drums: bass, snare, tom, hihat


// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"OSC. 1", "OSC 2", "MIDI CTRL P3", "MIDI CTRL P4"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
    {"SHAPE", "OCTAVE", "FINE", "AMPL."},
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
                out[0][i] =out[1][i] =out[2][i] = out[3][i] = (osc1.Process() + osc2.Process()) *0.1f;
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


    osc1.Init(dubby.seed.AudioSampleRate());
    osc1.SetWaveshape(0);
    osc1.SetSync(false);

    osc2.Init(dubby.seed.AudioSampleRate());
    osc2.SetWaveshape(0);
    osc2.SetSync(false);
    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

osc1.SetPW(savedKnobValues[0][0]);
osc1.SetSyncFreq(440.f*savedKnobValues[0][2]);


osc2.SetPW(savedKnobValues[1][0]);
osc2.SetSyncFreq(440.f*savedKnobValues[1][2]);


        

  

                      if(dubby.buttons[3].TimeHeldMs() > 300){dubby.ResetToBootloader();}

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
