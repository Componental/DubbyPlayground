
#include "daisysp.h"
#include "Dubby.h"
#include <array>
#include "Euclidean.h"
using namespace daisy;
using namespace daisysp;

Dubby dubby;
Euclidean eclid;
bool buttonPressed = false;
uint8_t lastNote = 0;

// BPM implement
float currentTime;
bool isBeat = false; // Flag to indicate a beat
float bpm = 120.0f; // Initial BPM

const float beatsPerMinuteToInterval = 60.0f; // Conversion factor from BPM to seconds
float beatInterval; // Interval between beats in seconds
float nextBeatTime = 0.0f; // Time of the next beat
bool midiMessageSentThisBeat = false;

struct MidiNote {
    uint8_t note;
    uint8_t velocity;
    float startTime; // Time when the note was triggered
};

std::vector<MidiNote> activeNotes; // Vector to store active MIDI notes
float noteLength = 0.2f;

// Define an array of MIDI notes to play in order
const std::vector<uint8_t> majorChord = {60, 64, 67}; // C Major chord
const std::vector<uint8_t> minorChord = {57, 60, 64}; // A Minor chord
const std::vector<uint8_t> dominantSeventhChord = {55, 59, 62, 65}; // G Dominant Seventh chord
const std::vector<uint8_t> diminishedChord = {62, 65, 68}; // D Diminished chord
const std::vector<uint8_t> augmentedChord = {65, 69, 73}; // F Augmented chord

size_t nextNoteIndex = 0; // Index to keep track of the next note to play


void MIDIUartSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}

void MIDIUartSendNoteOff(uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}


void HandleMidiUartMessage(MidiEvent m)
{
    
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            p = m.AsNoteOn(); // p.note, p.velocity
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            break;
        }
        default: break;
    }
}


void MIDIUsbSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void MIDIUsbSendNoteOff(uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void HandleMidiUsbMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            p = m.AsNoteOn(); // p.note, p.velocity
            MIDIUsbSendNoteOn(0, p.note, p.velocity);
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            break;
        }
        default: break;
    }
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquaredIns[4] = { 0.0f };
    double sumSquaredOuts[4] = { 0.0f };

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 4; j++) 
        {
            float _in = dubby.audioGains[0][j] * in[j][i];

            out[j][i] = dubby.audioGains[1][j] * _in;

            
            float sample = _in;
            sumSquaredIns[j] += sample * sample;

            sample = out[j][i];
            sumSquaredOuts[j] += sample * sample;
        } 

        if (dubby.scopeSelector == 0) dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f;   
        else if (dubby.scopeSelector == 1) dubby.scope_buffer[i] = (in[2][i] + in[3][i]) * .5f;   
        else if (dubby.scopeSelector == 2) dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f; 
        else if (dubby.scopeSelector == 3) dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f; 
        else if (dubby.scopeSelector == 4) dubby.scope_buffer[i] = in[0][i]; 
        else if (dubby.scopeSelector == 5) dubby.scope_buffer[i] = in[1][i]; 
        else if (dubby.scopeSelector == 6) dubby.scope_buffer[i] = in[2][i]; 
        else if (dubby.scopeSelector == 7) dubby.scope_buffer[i] = in[3][i]; 
        else if (dubby.scopeSelector == 8) dubby.scope_buffer[i] = out[0][i]; 
        else if (dubby.scopeSelector == 9) dubby.scope_buffer[i] = out[1][i]; 
        else if (dubby.scopeSelector == 10) dubby.scope_buffer[i] = out[2][i]; 
        else if (dubby.scopeSelector == 11) dubby.scope_buffer[i] = out[3][i]; 
	}

    for (int j = 0; j < 4; j++) 
    {
        dubby.currentLevels[0][j] = sqrt(sumSquaredIns[j] / AUDIO_BLOCK_SIZE);
        dubby.currentLevels[1][j] = sqrt(sumSquaredOuts[j] / AUDIO_BLOCK_SIZE);
    }



    // Handle MIDI note sending based on beat interval
    currentTime = static_cast<float>(daisy::System::GetNow()) / 1.0e3f; // Current time in seconds
    dubby.elapsedTime = currentTime;
  
    // Check for MIDI notes to turn off
    for (auto it = activeNotes.begin(); it != activeNotes.end(); ) {
        if (currentTime - it->startTime >= noteLength) { // Check if the specified note length has elapsed
            // Send MIDI note off message
            MIDIUsbSendNoteOff(0, it->note);
            // Remove the note from the list of active notes
            it = activeNotes.erase(it);
        } else {
            ++it;
        }
    }

    if (currentTime >= nextBeatTime) {
        // If a MIDI message hasn't been sent this beat yet, send one
        if (!midiMessageSentThisBeat) {
            // Get the next note to play from the predetermined list
            uint8_t note = majorChord[nextNoteIndex];
            // Increment the index for the next note, looping back to the start if necessary
            nextNoteIndex = (nextNoteIndex + 1) % majorChord.size();

            // Send MIDI note on message for the next note
            const uint8_t velocity = 60;
            MIDIUsbSendNoteOn(0, note, velocity);

            // Add the note to the list of active notes
            activeNotes.push_back({note, velocity, currentTime});

            // Update the flag to indicate that a MIDI message has been sent this beat
            midiMessageSentThisBeat = true;

            // Update next beat time to the time of the next beat
            nextBeatTime = currentTime + beatInterval;
        }
    } else {
        // Reset the flag if the current time is not yet at the next beat
        midiMessageSentThisBeat = false;
    }

}


int main(void)
{
	dubby.seed.Init();
	// dubby.seed.StartLog(true);

    dubby.Init();
    
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    dubby.ProcessAllControls();

    beatInterval = beatsPerMinuteToInterval / bpm;


    dubby.DrawLogo(); 
    System::Delay(1000);
	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateWindowSelector(0, false);
    
    dubby.midi_uart.StartReceive();

	while(1) { 

        bpm=40 + dubby.GetKnobValue(dubby.CTRL_1) * (250-40);
                dubby.bpm = bpm;

        noteLength=dubby.GetKnobValue(dubby.CTRL_2)*2.f;
        beatInterval = beatsPerMinuteToInterval / bpm;
        std::string printStuffLeft = std::to_string(dubby.verticalLinePosition);
        std::string printStuffRight = std::to_string(bpm);

        dubby.UpdateStatusBar(&printStuffLeft[0],dubby.LEFT);
        dubby.UpdateStatusBar(&printStuffRight[0],dubby.RIGHT);

        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        dubby.midi_uart.Listen();
        dubby.midi_usb.Listen();

        // Handle UART MIDI Events
        while(dubby.midi_uart.HasEvents())
        {
            HandleMidiUartMessage(dubby.midi_uart.PopEvent());
        }

        // Handle USB MIDI Events
        while(dubby.midi_usb.HasEvents())
        {
            HandleMidiUsbMessage(dubby.midi_usb.PopEvent());
        }
	}

}
