
#include "daisysp.h"
#include "Dubby.h"
#include <array>
#include <unordered_set> // Include the unordered_set header for efficiently storing active notes

// Define a set to store active notes
std::unordered_set<int> activeNotesSet;

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

bool        buttonPressed  = false;
bool        encoderPressed = false;
bool        shiftIsOn;
uint8_t     lastNote     = 0;
int         activeRhythm = 0;
uint8_t     notes[MAX_RHYTHMS];
std::string scaleForPrint;
std::string octaveShiftForPrint;
//std::string noteShiftForPrint;
//int totalShift = 0;
int octaveShift = 0;
int noteShift   = 0;


uint8_t chromatic_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                        DEFAULT_NOTE + 1,
                                        DEFAULT_NOTE + 2,
                                        DEFAULT_NOTE + 3,
                                        DEFAULT_NOTE + 4,
                                        DEFAULT_NOTE + 5,
                                        DEFAULT_NOTE + 6,
                                        DEFAULT_NOTE + 7};

uint8_t major_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                    DEFAULT_NOTE + 4,
                                    DEFAULT_NOTE + 7,
                                    DEFAULT_NOTE + 11,
                                    DEFAULT_NOTE + 14,
                                    DEFAULT_NOTE + 17,
                                    DEFAULT_NOTE + 21,
                                    DEFAULT_NOTE + 24};

uint8_t minor_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                    DEFAULT_NOTE + 3,
                                    DEFAULT_NOTE + 7,
                                    DEFAULT_NOTE + 10,
                                    DEFAULT_NOTE + 14,
                                    DEFAULT_NOTE + 17,
                                    DEFAULT_NOTE + 21,
                                    DEFAULT_NOTE + 24};

uint8_t pentatonic_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                         DEFAULT_NOTE + 5,
                                         DEFAULT_NOTE + 7,
                                         DEFAULT_NOTE + 12,
                                         DEFAULT_NOTE + 15,
                                         DEFAULT_NOTE + 19,
                                         DEFAULT_NOTE + 24,
                                         DEFAULT_NOTE + 27};

// Minor 7th chord
uint8_t minor7_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                     DEFAULT_NOTE + 3,
                                     DEFAULT_NOTE + 7,
                                     DEFAULT_NOTE + 10,
                                     DEFAULT_NOTE + 14,
                                     DEFAULT_NOTE + 17,
                                     DEFAULT_NOTE + 21,
                                     DEFAULT_NOTE + 24};

// Major 7th chord
uint8_t major7_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                     DEFAULT_NOTE + 4,
                                     DEFAULT_NOTE + 7,
                                     DEFAULT_NOTE + 11,
                                     DEFAULT_NOTE + 14,
                                     DEFAULT_NOTE + 17,
                                     DEFAULT_NOTE + 21,
                                     DEFAULT_NOTE + 25};


const uint8_t velocity = 110;
// Variable to store previous knob values
int prevKnobValue1 = 0;
int prevKnobValue2 = 0;

int knobValue1, knobValue2, knobValue3, knobValue4;


// BPM implement
bool  isBeat = false;  // Flag to indicate a beat
float bpm    = 120.0f; // Initial BPM

int midiMessageCounter = 0;


const float beatsPerMinuteToInterval
    = 60.0f;               // Conversion factor from BPM to seconds
float beatInterval;        // Interval between beats in seconds
float nextBeatTime = 0.0f; // Time of the next beat

// Define tolerance for matching knob values
const int knobTolerance = 0.001;

// Function to check if knob values match within tolerance
bool knobValueMatches(int currentValue, int prevValue)
{
    return abs(currentValue - prevValue) <= knobTolerance;
}

// Declare arrays to store lengths, events, and offsets
int lengths[MAX_RHYTHMS]
    = {0}; // Assuming initial values are 0, adjust as needed
int events[MAX_RHYTHMS]
    = {0}; // Assuming initial values are 0, adjust as needed
int offsets[MAX_RHYTHMS]
    = {0}; // Assuming initial values are 0, adjust as needed
int eventsTemp[MAX_RHYTHMS] = {0};


int  lengthX, eventsXTemp, offsetX;
void MIDIUartSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x90; // limit channel byte, add status byte
    data[1] = note & 0x7F;             // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}

void MIDIUartSendNoteOff(uint8_t channel, uint8_t note)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x80; // limit channel byte, add status byte
    data[1] = note & 0x7F;             // remove MSB on data
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
            p             = m.AsNoteOn(); // p.note, p.velocity
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


void MIDIUsbSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x90; // limit channel byte, add status byte
    data[1] = note & 0x7F;             // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void MIDIUsbSendNoteOff(uint8_t channel, uint8_t note)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x80; // limit channel byte, add status byte
    data[1] = note & 0x7F;             // remove MSB on data
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
            p             = m.AsNoteOn(); // p.note, p.velocity
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

struct MidiNote
{
    uint8_t note;
    uint8_t velocity;

    float startTime; // Time when the note was triggered
};


std::vector<int> euclideanRhythm(int length, int events, int offset)
{
    std::vector<int> rhythm(length,
                            0); // Initialize the rhythm array with all 0s
    if(events <= 0 || length <= 0)
    {
        return rhythm; // Invalid parameters
    }

    int eventsPerStep = length / events; // Number of events per step
    int extraEvents
        = length % events; // Extra events that need to be distributed

    // Calculate the adjusted offset
    offset %= length;
    if(offset < 0)
    {
        offset += length; // Ensure offset is positive
    }

    // Distribute the events as evenly as possible over the length with offset
    int position = offset;
    for(int i = 0; i < events; ++i)
    {
        rhythm[position % length] = 1; // Set event at current position
        position += eventsPerStep;
        if(extraEvents > 0)
        {
            position++; // Distribute extra events
            extraEvents--;
        }
    }

    return rhythm;
}

std::vector<MidiNote> activeNotes; // Vector to store active MIDI notes
float                 noteLength = 0.2f;


size_t nextNoteIndex = 0; // Index to keep track of the next note to play


std::string noteShiftForPrint(int noteShift)
{
    std::vector<std::string> noteNames
        = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    if(noteShift < 0 || noteShift > 11)
    {
        return ""; // Return empty string for out-of-range shifts
    }

    return noteNames[noteShift];
}


void handleEncoder()
{
    if(dubby.encoder.FallingEdge())
    {
        encoderPressed = !encoderPressed;
    }


    int rotationDirection = dubby.encoder.Increment();
    if(rotationDirection == 1)
    {
        activeRhythm += 1;
    }
    else if(rotationDirection == -1)
    {
        activeRhythm -= 1;
    }
    activeRhythm = (activeRhythm % MAX_RHYTHMS + MAX_RHYTHMS) % MAX_RHYTHMS;
    dubby.activeRhythm = activeRhythm;

    dubby.bpm = bpm;
}


void sendMidiBasedOnRhythms()
{
    if(dubby.elapsedTime >= nextBeatTime)
    {
        // Keep track of previously active notes
        std::unordered_set<int> prevActiveNotesSet = activeNotesSet;

        // Clear the set of active notes
        activeNotesSet.clear();

        // Beat processing logic here
        for(int i = 0; i < MAX_RHYTHMS; ++i)
        {
            // Check if the current subdivisions should play a note
            if(dubby.rhythms[i][midiMessageCounter % dubby.lengths[i]] == 1)
            {
                int note = notes[i];
                activeNotesSet.insert(
                    note); // Add the note to the set of active notes
            }
        }

        // Send note off for notes that were active but are no longer active
        for(int note : prevActiveNotesSet)
        {
            if(activeNotesSet.find(note) == activeNotesSet.end())
            {
                MIDIUsbSendNoteOff(0, note);  // Sending note off for USB
                MIDIUartSendNoteOff(0, note); // Sending note off for UART
            }
        }

        // Send note on for all active notes
        for(int note : activeNotesSet)
        {
            MIDIUsbSendNoteOn(
                0,
                note,
                velocity); // Sending all notes to MIDI channel 0 for USB
            MIDIUartSendNoteOn(
                0,
                note,
                velocity); // Sending all notes to MIDI channel 0 for UART
        }

        float subdivisionInterval = beatInterval / 4.0f;
        nextBeatTime += subdivisionInterval;
        ++midiMessageCounter;
        dubby.midiCounter = midiMessageCounter;
    }
}


void selectRhythm()
{
    // Switch between rhythms
    for(int i = 0; i < MAX_RHYTHMS; ++i)
    {
        if(activeRhythm == i)
        {
            lengthX     = lengths[i];
            eventsXTemp = events[i];
            offsetX     = offsets[i];

            if(!knobValueMatches(knobValue1, prevKnobValue1))
            {
                lengths[i]     = knobValue1;
                prevKnobValue1 = knobValue1;
            }

            if(!knobValueMatches(knobValue2, prevKnobValue2))
            {
                events[i]      = knobValue2;
                prevKnobValue2 = knobValue2;
            }

            if(dubby.buttons[0].FallingEdge())
            {
                offsets[i]++;
            }

            if(dubby.buttons[1].FallingEdge())
            {
                offsets[i]--;
            }

            break;
        }
    }
}

void handleScales()
{
    switch(knobValue3)
    {
        case 0:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = chromatic_chord[i] + noteShift + octaveShift;
                scaleForPrint = "CHRO";
            }
            break;
        case 1:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = major_chord[i] + noteShift + octaveShift;
                scaleForPrint = "MAJ";
            }
            break;
        case 2:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = minor_chord[i] + noteShift + octaveShift;
                scaleForPrint = "MIN";
            }
            break;
        case 3:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = pentatonic_chord[i] + noteShift + octaveShift;
                scaleForPrint = "PEN";
            }
            break;
        case 4:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = minor7_chord[i] + noteShift + octaveShift;
                scaleForPrint = "MIN7";
            }
            break;
        case 5:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = major7_chord[i] + noteShift + octaveShift;
                scaleForPrint = "MAJ7";
            }
            break;
        default:
            for(int i = 0; i < MAX_RHYTHMS; ++i)
            {
                notes[i]      = chromatic_chord[i] + noteShift + octaveShift;
                scaleForPrint = "CHRO";
            }
            break;
    }
}

void handleKnobs()
{
    knobValue1 = (dubby.GetKnobValue(dubby.CTRL_1) * 32)
                 + 1; // Adjust knob 1 to start from 1 and have a maximum of 32
    knobValue2 = dubby.GetKnobValue(dubby.CTRL_2) * 33;
    knobValue3
        = (dubby.GetKnobValue(dubby.CTRL_3)
           * 6); // Adjust knob 3 to start from 1 and have a maximum of 32
    knobValue4 = dubby.GetKnobValue(dubby.CTRL_4) * 5;


    // Limit the knob values to 32
    knobValue1 = std::min(knobValue1, 32);
    knobValue2 = std::min(knobValue2, 32);
    knobValue3 = std::min(knobValue3, 5);
    knobValue4 = std::min(knobValue4, 4);
}

void shiftNotesAndOctaves()
{
    if(dubby.buttons[2].FallingEdge())
    {
        noteShift = std::min(noteShift + 1,
                             11); // Increment noteShift but limit it to 11
    }

    if(dubby.buttons[3].FallingEdge())
    {
        noteShift = std::max(noteShift - 1,
                             0); // Decrement noteShift but limit it to 0
    }


    switch(knobValue4)
    {
        case 0:
            octaveShift         = -24;
            octaveShiftForPrint = "--";

            break;
        case 1:
            octaveShift = -12;

            octaveShiftForPrint = "-";

            break;
        case 2:
            octaveShift = 0;

            octaveShiftForPrint = "";

            break;
        case 3:
            octaveShift         = 12;
            octaveShiftForPrint = "+";

            break;
        case 4:
            octaveShift         = 24;
            octaveShiftForPrint = "++";

            break;
            ;
        default:
            octaveShift         = 0;
            octaveShiftForPrint = "";


            break;
    }
}


void handleJoystick()
{
    // Adjust events1Temp based on knob value for CTRL_6
    if(dubby.GetKnobValue(dubby.CTRL_6) < 0.2)
    {
        // Quadruple the events1 value temporarily
        eventsTemp[0] = std::min(events[0] * 4, 32);
    }
    else if(dubby.GetKnobValue(dubby.CTRL_6) < 0.4)
    {
        // Double the events1 value temporarily
        eventsTemp[0] = std::min(events[0] * 2, 32);
    }
    else
    {
        // Reset events1 to its original value
        eventsTemp[0] = events[0];
    }

    // Adjust events2Temp based on knob value for CTRL_5
    if(dubby.GetKnobValue(dubby.CTRL_5) < 0.2)
    {
        // Quadruple the events2 value temporarily
        eventsTemp[1] = std::min(events[1] * 4, 32);
    }
    else if(dubby.GetKnobValue(dubby.CTRL_5) < 0.4)
    {
        // Double the events2 value temporarily
        eventsTemp[1] = std::min(events[1] * 2, 32);
    }
    else
    {
        // Reset events2 to its original value
        eventsTemp[1] = events[1];
    }

    // Adjust events3Temp based on knob value for CTRL_6
    if(dubby.GetKnobValue(dubby.CTRL_6) > 0.8)
    {
        // Quadruple the events3 value temporarily
        eventsTemp[2] = std::min(events[2] * 4, 32);
    }
    else if(dubby.GetKnobValue(dubby.CTRL_6) > 0.6)
    {
        // Double the events3 value temporarily
        eventsTemp[2] = std::min(events[2] * 2, 32);
    }
    else
    {
        // Reset events3 to its original value
        eventsTemp[2] = events[2];
    }

    // Adjust events4Temp based on knob value for CTRL_5
    if(dubby.GetKnobValue(dubby.CTRL_5) > 0.8)
    {
        // Quadruple the events4 value temporarily
        eventsTemp[3] = std::min(events[3] * 4, 32);
    }
    else if(dubby.GetKnobValue(dubby.CTRL_5) > 0.6)
    {
        // Double the events4 value temporarily
        eventsTemp[3] = std::min(events[3] * 2, 32);
    }
    else
    {
        // Reset events4 to its original value
        eventsTemp[3] = events[3];
    }
}

void generateRhythms()
{
    for(int i = 0; i < MAX_RHYTHMS; ++i)
    {
        //    std::vector<int> rhythm = euclideanRhythm(lengths[i], events[i], offsets[i]);
        if(i < 4)
        {
            dubby.rhythms[i]
                = euclideanRhythm(lengths[i], eventsTemp[i], offsets[i]);
        }
        else
        {
            dubby.rhythms[i]
                = euclideanRhythm(lengths[i], events[i], offsets[i]);
        }

        dubby.lengths[i] = lengths[i];
    }
}

void printValuesToDisplay()
{
    std::string printStuffLeft = std::to_string(lengthX) + "|"
                                 + std::to_string(eventsXTemp) + "|"
                                 + std::to_string(offsetX);
    std::string printStuffRight = noteShiftForPrint(noteShift) + " "
                                  + scaleForPrint + octaveShiftForPrint + " "
                                  + std::to_string((int)bpm);
    dubby.UpdateStatusBar(&printStuffLeft[0], dubby.LEFT);
    dubby.UpdateStatusBar(&printStuffRight[0], dubby.RIGHT);
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
        loadMeter.OnBlockStart();

    double sumSquaredIns[4]  = {0.0f};
    double sumSquaredOuts[4] = {0.0f};

    for(size_t i = 0; i < size; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            float _in = dubby.audioGains[0][j] * in[j][i];

            out[j][i] = dubby.audioGains[1][j] * _in;


            float sample = _in;
            sumSquaredIns[j] += sample * sample;

            sample = out[j][i];
            sumSquaredOuts[j] += sample * sample;
        }

        if(dubby.scopeSelector == 0)
            dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f;
        else if(dubby.scopeSelector == 1)
            dubby.scope_buffer[i] = (in[2][i] + in[3][i]) * .5f;
        else if(dubby.scopeSelector == 2)
            dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;
        else if(dubby.scopeSelector == 3)
            dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f;
        else if(dubby.scopeSelector == 4)
            dubby.scope_buffer[i] = in[0][i];
        else if(dubby.scopeSelector == 5)
            dubby.scope_buffer[i] = in[1][i];
        else if(dubby.scopeSelector == 6)
            dubby.scope_buffer[i] = in[2][i];
        else if(dubby.scopeSelector == 7)
            dubby.scope_buffer[i] = in[3][i];
        else if(dubby.scopeSelector == 8)
            dubby.scope_buffer[i] = out[0][i];
        else if(dubby.scopeSelector == 9)
            dubby.scope_buffer[i] = out[1][i];
        else if(dubby.scopeSelector == 10)
            dubby.scope_buffer[i] = out[2][i];
        else if(dubby.scopeSelector == 11)
            dubby.scope_buffer[i] = out[3][i];
    }

    for(int j = 0; j < 4; j++)
    {
        dubby.currentLevels[0][j] = sqrt(sumSquaredIns[j] / AUDIO_BLOCK_SIZE);
        dubby.currentLevels[1][j] = sqrt(sumSquaredOuts[j] / AUDIO_BLOCK_SIZE);
    }

    loadMeter.OnBlockEnd();

    // // // // // // // // // // // // // //
}


int main(void)
{
    dubby.seed.Init();
    // dubby.seed.StartLog(true);

    dubby.Init();

    dubby.seed.SetAudioBlockSize(
        AUDIO_BLOCK_SIZE); // number of samples handled per callback
    dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    dubby.ProcessAllControls();

    beatInterval = beatsPerMinuteToInterval / bpm;

    dubby.DrawLogo();
    System::Delay(200);
    dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateWindowSelector(0, false);

    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());

    dubby.midi_uart.StartReceive();


    bpm = 140;


    for(int i = 0; i < MAX_RHYTHMS; ++i)
    {
        lengths[i] = 16;
    }


    while(1)
    {
        handleScales();
        shiftNotesAndOctaves();
        handleKnobs();
        handleEncoder();
        generateRhythms();
        handleJoystick();
        selectRhythm();
        sendMidiBasedOnRhythms();
        printValuesToDisplay();


        dubby.elapsedTime = static_cast<float>(daisy::System::GetNow())
                            / 1.0e3f; // Current time in seconds


        // Button handling for reset to bootloader
        if(dubby.buttons[3].TimeHeldMs() > 1000)
        {
            dubby.ResetToBootloader();
        }


        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        
        // CPU METER =====
        std::string str = std::to_string(int(loadMeter.GetAvgCpuLoad() * 100.0f)) + "%"; 
       // dubby.UpdateStatusBar(&str[0], dubby.MIDDLE);
        // ===============

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
