#include "Dubby.h"
#include "implementations/includes.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
int outChannel;
int inChannel = 0;

bool midiClockStarted = false;
bool midiClockStoppedByButton2 = false;


void MonitorMidi();
void HandleMidiUartMessage(MidiEvent m);
void HandleMidiUsbMessage(MidiEvent m);

// Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<PersistantMemoryParameterSettings> SavedParameterSettings(dubby.seed.qspi);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    dubby.ProcessLFO(); 

    for (size_t i = 0; i < size; i++)
    {
        for (int j = 0; j < NUM_AUDIO_CHANNELS; j++)
        {

            out[j][i] = 0.0f; // Clear output buffer for each sample

            for (int inChannel = 0; inChannel < NUM_AUDIO_CHANNELS; inChannel++)
            {
                switch (dubby.channelMapping[j][inChannel])
                {
                case PASS:
                    out[j][i] += in[inChannel][i];
                    break;
                case EFCT:
                    out[j][i] += 0; // flt[inChannel].Process(in[inChannel][i]);

                    break;
                case SNTH:
                    out[j][i] += /* Synth processing here, if applicable */ 0.0f;
                    break;
                default:
                    out[j][i] += 0;
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
    
    setLED(1, NO_COLOR, 0);
    setLED(0, NO_COLOR, 0);
    updateLED();

    dubby.seed.StartAudio(AudioCallback);

    while (1)
    {

        Monitor(dubby);
        MonitorMidi();
        MonitorPersistantMemory(dubby, SavedParameterSettings);
        setLED(1, RED, abs(0.5 + dubby.lfo1Value) * 50);
        setLED(0, RED, abs(0.5 + dubby.lfo2Value) * 50);
        updateLED();
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
                (void)p; // Suppress unused variable warning
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
                (void)p; // Suppress unused variable warning
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
