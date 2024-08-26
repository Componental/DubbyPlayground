
#include "daisysp.h"
#include "Dubby.h"

#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
int outChannel;
int inChannel = 0;

static LadderFilter flt[4]; // Four filters, one for each channel

bool midiClockStarted = false;
bool midiClockStoppedByButton2 = false;

void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

// Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<PersistantMemoryParameterSettings> SavedParameterSettings(dubby.seed.qspi);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    // Loop through each sample
    for (size_t i = 0; i < size; i++)
    {
        // Clear the output buffer for each sample
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            out[j][i] = 0.0f; // Initialize output to 0 for each sample
        }

        // Loop through each output channel
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // Loop through each input channel
            for (int inChannel = 0; inChannel < NUM_AUDIO_CHANNELS; inChannel++)
            {
                // Use switch statement to handle different channel mappings
                switch (dubby.channelMapping[j][inChannel])
                {
                case PASS:
                    // Directly assign input to output channel
                    out[j][i] += in[inChannel][i];
                    break;

                case EFCT:
                {
                    // Apply effect (like filter) to input and then assign to output channel
                    float output = flt[j].Process(in[inChannel][i]);
                    out[j][i] += output;
                    break;
                }

                case SNTH:
                    // Apply input to synth
                    // float output = synth[j].SidechainInput(in[inChannel][i]);
                    // out[j][i] += output;
                    break;

                default:
                    // Do nothing (channel not assigned)
                    break;
                }
            }
        }
    }
}

int main(void)
{
    Init(dubby);
    InitMidiClock(dubby);
    InitPersistantMemory(dubby, SavedParameterSettings);

    dubby.seed.StartAudio(AudioCallback);

    //  initLED();
    //  setLED(0, BLUE, 0);
    // setLED(1, RED, 0);
    //  updateLED();
    // Update the filter parameters for each filter
    float sample_rate = dubby.seed.AudioSampleRate();

    for (int i = 0; i < 4; i++)
    {
        // initialize Moogladder object
        // Initialize the LadderFilter objects
        flt[i].Init(sample_rate);

        flt[i].SetInputDrive(1.f);
        flt[i].SetRes(0.f);
        flt[i].SetFreq(2000.f);
    }

    LadderFilter::FilterMode currentMode = LadderFilter::FilterMode::LP24;

    // DELETE MEMORY
    // SavedParameterSettings.RestoreDefaults();

    while (1)
    {
        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);

        //   MIDISendNoteOn(dubby, 46, 120);

        if (dubby.buttons[dubby.CTRL_1].FallingEdge())
        {
            if (midiClockStarted)
            {
                MIDISendStop(dubby);
                midiClockStarted = false;
            }
            else
            {
                if (midiClockStoppedByButton2)
                {
                    MIDISendStart(dubby);
                    midiClockStoppedByButton2 = false;
                }
                else
                {
                    MIDISendContinue(dubby);
                }
                midiClockStarted = true;
            }
        }

        if (dubby.buttons[dubby.CTRL_2].FallingEdge())
        {
            MIDISendStop(dubby);
            midiClockStarted = false;
            midiClockStoppedByButton2 = true;
        }

        if (dubby.buttons[dubby.CTRL_3].TimeHeldMs() > 1000)
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
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                NoteOnEvent p = m.AsNoteOn();
            }
        }
        break;
    }
    case NoteOff:
    {
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                NoteOffEvent p = m.AsNoteOff();
            }
        }
        break;
    }
    case SystemRealTime:
    {
        if (dubby.dubbyMidiSettings.currentMidiClockOption == FOLLOWER)
        {

            HandleSystemRealTime(m.srt_type, dubby);
            // std::string stra = std::to_string(dubby.receivedBPM);
            // dubby.UpdateStatusBar(&stra[0], dubby.MIDDLE, 127);
        }
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
        if (dubby.dubbyMidiSettings.currentMidiInOption == MIDIIN_ON)
        {
            if (m.channel == dubby.dubbyMidiSettings.currentMidiInChannelOption)
            {
                HandleMidiMessage(m);
            }
        }
    }
}
