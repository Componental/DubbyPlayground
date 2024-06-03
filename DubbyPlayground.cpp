#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

// SYNTH
// VariableShapeOscillator osc1, osc2;
Oscillator osc1, osc2;

static LadderFilter filter;

static Adsr filterEnv, ampEnv;
static Metro tick;
bool gate;
float filterEnvOut, ampEnvOut;

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

        // When the metro ticks, trigger the envelope to start.
        if (tick.Process())
        {
            gate = !gate;
        }

        // Use envelope to control the amplitude of the oscillator.
        filterEnvOut = filterEnv.Process(gate);
        ampEnvOut = ampEnv.Process(gate);

        osc1.SetAmp(ampEnvOut);
        osc2.SetAmp(ampEnvOut);

        float osc1Out = osc1.Process();
        float osc2Out = osc2.Process();

        float oscCombinedOut = osc1Out + osc2Out;



        // left out
        out[0][i] = filter.Process(oscCombinedOut);

        // right out
        out[1][i] = filter.Process(oscCombinedOut);
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    setNumPages(NUM_PAGES);
    dubby.seed.StartAudio(AudioCallback);

    sample_rate = dubby.seed.AudioSampleRate();

    osc1.Init(sample_rate);
    osc1.SetFreq(250.f); // Set an initial frequency for osc1
    osc1.SetAmp(0.1);

    osc2.Init(sample_rate);
    osc2.SetFreq(250.f); // Set an initial frequency for osc2
    osc2.SetAmp(0.1);

    filter.Init(sample_rate);

    filterEnv.Init(sample_rate);
    ampEnv.Init(sample_rate);
    // Set up metro to pulse every second
    tick.Init(.7f, sample_rate);

    LadderFilter::FilterMode currentMode = LadderFilter::FilterMode::LP24;

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);
        // Update oscillators
        osc1.SetFreq(440.f * savedKnobValues[0][2]);
        osc1.SetWaveform(savedKnobValues[0][0] * 6);

        osc2.SetFreq(440.f * savedKnobValues[1][2]);
        osc2.SetWaveform(savedKnobValues[1][0] * 6);

        // Get the knob values
        float inGain = savedKnobValues[2][2] * 4;
        float res = savedKnobValues[2][1];
        float cutOffKnobValue = savedKnobValues[2][0] + (savedKnobValues[2][3]*filterEnvOut);

     // FILTER ENV

        // Map the knob value to a logarithmic scale for cutoff frequency

        float minAttackFilter = .01f; // Minimum cutoff frequency in Hz
        float maxAttackFilter = 3.f;  // Maximum cutoff frequency in Hz
        float mappedAttackFilter = daisysp::fmap(savedKnobValues[3][0], minAttackFilter, maxAttackFilter, daisysp::Mapping::LOG);

        // Set envelope parameters
        filterEnv.SetTime(ADSR_SEG_ATTACK, mappedAttackFilter);
        filterEnv.SetTime(ADSR_SEG_DECAY, savedKnobValues[3][1]);
        filterEnv.SetSustainLevel(savedKnobValues[3][2]);
        filterEnv.SetTime(ADSR_SEG_RELEASE, savedKnobValues[3][3]);


        // Map the knob value to a logarithmic scale for cutoff frequency
        float minCutoff = .05f;    // Minimum cutoff frequency in Hz
        float maxCutoff = 17000.f; // Maximum cutoff frequency in Hz
        float mappedCutoff = daisysp::fmap(cutOffKnobValue, minCutoff, maxCutoff, daisysp::Mapping::LOG);

        // Update the filter parameters
        filter.SetInputDrive(inGain);
        filter.SetRes(res);
        filter.SetFreq(mappedCutoff);


   

        // AMP ENV

        // Map the knob value to a logarithmic scale for cutoff frequency
        float minAttackAmp = .01f; // Minimum cutoff frequency in Hz
        float maxAttackAmp = 3.f;  // Maximum cutoff frequency in Hz
        float mappedAttackAmp = daisysp::fmap(savedKnobValues[4][0], minAttackAmp, maxAttackAmp, daisysp::Mapping::LOG);

        // Set envelope parameters
        ampEnv.SetTime(ADSR_SEG_ATTACK, mappedAttackAmp);
        ampEnv.SetTime(ADSR_SEG_DECAY, savedKnobValues[4][1]);
        ampEnv.SetSustainLevel(savedKnobValues[4][2]);
        ampEnv.SetTime(ADSR_SEG_RELEASE, savedKnobValues[4][3]);


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