
#include "daisysp.h"
#include "Dubby.h"
#include "Hihat.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

float defaultKnobValue = 0.2f;
SyntheticBassDrum DSY_SDRAM_BSS bassDrum, tomDrum;
SyntheticSnareDrum DSY_SDRAM_BSS snareDrum;
Hihat DSY_SDRAM_BSS hihat;

bool triggerBassDrum = false;
bool triggerTomDrum = false;
bool triggerSnareDrum = false;

float bassDrumAmplitude = 1.f;
float snareDrumAmplitude = 1.f;
float tomDrumAmplitude = 1.f;
float hihatAmplitude = 1.f;
const int NUM_PAGES = 5; // assuming 4 types of drums: bass, snare, tom, hihat


// Vector of vectors to store whether each knob is within tolerance for each drum
// Booleans to track tolerance for each knob of each drum

const char *algorithmTitles[NUM_PAGES] = {"BASS DRUM", "SNARE DRUM", "TOM DRUM", "HI-HAT", "VOLUME"};
const char *customLabels[NUM_PAGES][NUM_KNOBS] = {
    {"FREQ", "DECAY", "TONE", "DIRT"},
    {"FREQ", "DECAY", "ACCENT", "SNAPPY"},
    {"FREQ", "DECAY", "TONE", "DIRT"},
    {"ATTACK", "DECAY", "COLOUR", "RESO"},
    {"BD", "SNARE", "TOM", "HI-HAT"}};

float savedKnobValues[NUM_PAGES][NUM_KNOBS] = {
    {.4f, .6f, .4f, .0f}, // default value bass drum
    {.2f, .4f, .6f, .7f}, // default value snare
    {.2f, .2f, .2f, .2f}, // default value tom
    {.0f, .2f, .5f, .3f}, // default value hihat
    {.7f, .3f, .7f, .7f}}; // default value volume

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

            out[0][i] = out[1][i] = out[2][i] = out[3][i] = (bassDrum.Process(triggerBassDrum) * bassDrumAmplitude + tomDrum.Process(triggerTomDrum) * tomDrumAmplitude + snareDrum.Process(triggerSnareDrum) * snareDrumAmplitude + hihat.Process() * hihatAmplitude);
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

int main(void)
{

    Init(dubby);
    InitMidiClock(dubby);

    dubby.seed.StartAudio(AudioCallback);

    float sample_rate = dubby.seed.AudioSampleRate();
    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());

    setNumPages(NUM_PAGES);

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
        handleKnobs(dubby, algorithmTitles, customLabels, savedKnobValues);

        // Inside the while loop
        bassDrum.SetFreq(savedKnobValues[0][0] * 150.0f);
        bassDrum.SetDecay(savedKnobValues[0][1]);
        bassDrum.SetTone(savedKnobValues[0][2]);
        bassDrum.SetDirtiness(savedKnobValues[0][3]);

        snareDrum.SetFreq((savedKnobValues[1][0] * 300.f) + 10.f);
        snareDrum.SetDecay(savedKnobValues[1][1]);
        snareDrum.SetAccent(savedKnobValues[1][2]);
        snareDrum.SetSnappy(savedKnobValues[1][3]);

        tomDrum.SetFreq((savedKnobValues[2][0] * 250.f) + 200.f);
        tomDrum.SetDecay(savedKnobValues[2][1]);
        tomDrum.SetTone(savedKnobValues[2][2]);
        tomDrum.SetDirtiness(savedKnobValues[2][3]);

        hihat.SetAttack(savedKnobValues[3][0]);
        hihat.SetDecay(savedKnobValues[3][1]);
        hihat.SetFreq(savedKnobValues[3][2]);
        hihat.SetFilterRes(savedKnobValues[3][3]);

        bassDrumAmplitude = savedKnobValues[4][0];
        snareDrumAmplitude = savedKnobValues[4][1];
        tomDrumAmplitude = savedKnobValues[4][2];
        hihatAmplitude = savedKnobValues[4][3];

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
               // bassDrumAmplitude = p.velocity / 127.f;
                break;
            case 61: // Snare drum
                triggerSnareDrum = true;
              //  snareDrumAmplitude = p.velocity / 127.f;
                break;
            case 62: // Tom
                triggerTomDrum = true;
             //   tomDrumAmplitude = p.velocity / 127.f;
                break;
            case 63: // Hi-hat
                hihat.Trigger();
            //    hihatAmplitude = p.velocity / 127.f;
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
