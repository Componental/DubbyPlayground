#include "daisysp.h"
#include "Dubby.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
int outChannel;
int inChannel = 0;

static LadderFilter flt[NUM_AUDIO_CHANNELS]; // One filter for each input channel

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
        // Set all audio samples for the current frame (index i) across all channels to 0.0f
          std::fill_n(&out[0][i], NUM_AUDIO_CHANNELS, 0.0f);

        // Loop through each output channel
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {
            // Loop through each input channel
            for (int inChannel = 0; inChannel < NUM_AUDIO_CHANNELS; inChannel++)
            {
                switch (dubby.channelMapping[j][inChannel]) {
                    case PASS: out[j][i] += in[inChannel][i]; break;
                    case EFCT: out[j][i] += flt[inChannel].Process(in[inChannel][i]); break;
                    case SNTH: /* Synth code placeholder */ break;
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

    // Update the filter parameters for each filter
    float sample_rate = dubby.seed.AudioSampleRate();

    for (int i = 0; i < NUM_AUDIO_CHANNELS; i++)
    {
        // Initialize the LadderFilter objects
        flt[i].Init(sample_rate);

        flt[i].SetInputDrive(1.f);
        flt[i].SetRes(0.f);
        flt[i].SetFreq(2000.f);
    }

    LadderFilter::FilterMode currentMode = LadderFilter::FilterMode::LP24;

    while (1)
    {
            (void)currentMode; // suppress unused variable warning

        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);

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
                (void)p;  // Suppress unused variable warning
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
                (void)p;  // Suppress unused variable warning
            }
        }
        break;
    }
    case SystemRealTime:
    {
        if (dubby.dubbyMidiSettings.currentMidiClockOption == FOLLOWER)
        {
            HandleSystemRealTime(m.srt_type, dubby);
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
