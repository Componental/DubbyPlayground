
#include "daisysp.h"
#include "Dubby.h"
#include "Chords.h"
#include <array>
#include <unordered_set> // Include the unordered_set header for efficiently storing active notes

using namespace daisy;
using namespace daisysp;
// Audio and MIDI components
Dubby dubby;
CpuLoadMeter loadMeter;

// Constants and configurations
const float beatsPerMinuteToInterval = 60.0f; // Conversion factor from BPM to seconds

// Time and control variables
int countdown = 1000; // Initial countdown value
int buttonPressedTimeBeforeSwitching = 200;
int lastTimeUpdate;
bool buttonPressed = false;
bool shiftIsOn;

// Knob and button adjustments
bool adjustBpm = false;
bool adjustVelocityBaseline = false;
bool adjustVelocityRandomnessAmount = false;

// Knob values and configurations
int knobValue1, knobValue2, knobValue3, knobValue4;
int prevKnobValue1 = 0, prevKnobValue2 = 0;

// Velocity and randomness settings
std::array<int, MAX_RHYTHMS> baselineVelocities = {110, 110, 110, 110, 110, 110, 110, 110}; // Baseline velocities for each rhythm
int velocities[MAX_RHYTHMS] = {0};                                                          // Current velocities for each rhythm
float velocityRandomAmounts[MAX_RHYTHMS] = {0.0f};                                          // Randomness amount for each rhythm

// Beat timing and counters
float bpm = 120.0f;         // Initial BPM
float beatInterval;         // Interval between beats in seconds
int midiMessageCounter = 0; // Counter for MIDI messages

// MIDI notes and events
int maxLengthsVal = 32;
int maxEventsVal = 32;
uint8_t notes[MAX_RHYTHMS];     // MIDI notes for each rhythm
int lengths[MAX_RHYTHMS] = {0}; // Lengths of each rhythm
int events[MAX_RHYTHMS] = {0};  // Event counts for each rhythm
int offsets[MAX_RHYTHMS] = {0}; // Offsets for each rhythm

// Active note tracking
std::unordered_set<int> activeNotesSet; // Set to store active MIDI notes

// Note shifting and octave adjustment
int octaveShift = 0; // Octave shift amount
int noteShift = 0;   // Note shift amount
int numOctaves = 7;
int notesInOctave = 12;

// MIDI message handling
struct MidiNote
{
    uint8_t note;
    uint8_t velocity;

    float startTime; // Time when the note was triggered
};

std::vector<MidiNote> activeNotes; // Active MIDI notes
int nextNoteIndex = 0;             // Index to keep track of the next note to play

// UI and display variables
std::string scaleForPrint;                                     // Current scale for display
std::string octaveShiftForPrint;                               // Octave shift indication for display
std::string printStuffLeft, printStuffMiddle, printStuffRight; // Strings for display information

////////////////////////////

uint8_t lastNote = 0;
int selectedRhythm = 0;

enum OctaveShiftLevel
{
    OCTAVE_DOWN_3 = 0,
    OCTAVE_DOWN_2,
    OCTAVE_DOWN_1,
    NO_OCTAVE_SHIFT,
    OCTAVE_UP_1,
    OCTAVE_UP_2,
    OCTAVE_UP_3

};

// BPM implement
bool isBeat = false; // Flag to indicate a beat

float nextBeatTime = 0.0f; // Time of the next beat

// Define tolerance for matching knob values
const int knobTolerance = 0.1;

// Function to check if knob values match within tolerance
bool knobValueMatches(int currentValue, int prevValue)
{
    return abs(currentValue - prevValue) <= knobTolerance;
}

int eventsTemp[MAX_RHYTHMS] = {0};

int lengthX, eventsXTemp, offsetX;
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
    switch (m.type)
    {
    case NoteOn:
    {
        NoteOnEvent p = m.AsNoteOn();
        p = m.AsNoteOn(); // p.note, p.velocity
                MIDIUartSendNoteOn(0, p.note, p.velocity);

        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        break;
    }
    default:
        break;
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
    switch (m.type)
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
    default:
        break;
    }
}

std::vector<int> euclideanRhythm(int length, int events, int offset)
{
    std::vector<int> rhythm(length,
                            0); // Initialize the rhythm array with all 0s
    if (events <= 0 || length <= 0)
    {
        return rhythm; // Invalid parameters
    }

    int eventsPerStep = length / events; // Number of events per step
    int extraEvents = length % events;   // Extra events that need to be distributed

    // Calculate the adjusted offset
    offset %= length;
    if (offset < 0)
    {
        offset += length; // Ensure offset is positive
    }

    // Distribute the events as evenly as possible over the length with offset
    int position = offset;
    for (int i = 0; i < events; ++i)
    {
        rhythm[position % length] = 1; // Set event at current position
        position += eventsPerStep;
        if (extraEvents > 0)
        {
            position++; // Distribute extra events
            extraEvents--;
        }
    }

    return rhythm;
}

std::string noteShiftForPrint(int noteShift)
{
    std::vector<std::string> noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    if (noteShift < 0 || noteShift > 11)
    {
        return ""; // Return empty string for out-of-range shifts
    }

    return noteNames[noteShift];
}

void handleEncoder()
{

    // Check for falling edge of encoder press
    if (dubby.encoder.FallingEdge())
    {
        // Toggle the encoderPressed state for the active rhythm
        dubby.encoderPressed[selectedRhythm] = !dubby.encoderPressed[selectedRhythm];

        if (dubby.encoderPressed[selectedRhythm])
        {
            // Reset events to 0 when encoder is pressed
            dubby.prevEventsValues[selectedRhythm] = events[selectedRhythm];
            dubby.preMuteRhythm[selectedRhythm] = dubby.rhythms[selectedRhythm];
            events[selectedRhythm] = 0;
        }
        else
        {
            // Restore previous events value when encoder is pressed again
            events[selectedRhythm] = dubby.prevEventsValues[selectedRhythm];
            dubby.rhythms[selectedRhythm] = dubby.preMuteRhythm[selectedRhythm];
        }
    }

    int rotationDirection = dubby.encoder.Increment();


////////////////
// ADJUST BPM //
////////////////
    if (dubby.buttons[2].Pressed() && dubby.buttons[2].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        adjustBpm = true;
        if (rotationDirection != 0)
        {
            bpm = bpm + rotationDirection;
        }
    }

/////////////////////
// ADJUST VELOCITY //
/////////////////////    
    else if (dubby.buttons[0].Pressed() && dubby.buttons[0].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        adjustVelocityBaseline = true;
        if (rotationDirection != 0)
        {
            baselineVelocities[selectedRhythm] = std::max(0, std::min(127, baselineVelocities[selectedRhythm] + rotationDirection));
        }
    }


////////////////////////////////
// ADJUST VELOCITY RANDOMNESS //
////////////////////////////////
    else if (dubby.buttons[1].Pressed() && dubby.buttons[1].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        adjustVelocityRandomnessAmount = true;
        if (rotationDirection != 0)
        {
            velocityRandomAmounts[selectedRhythm] = std::max(0.0f, std::min(1.0f, velocityRandomAmounts[selectedRhythm] + rotationDirection * 0.01f));
        }
    }


///////////////////
// SELECT RHYTHM //
///////////////////
    else
    {   
        if (rotationDirection != 0)
        {
            selectedRhythm = selectedRhythm + rotationDirection;
        }
        
        selectedRhythm = (selectedRhythm % MAX_RHYTHMS + MAX_RHYTHMS) % MAX_RHYTHMS;
        dubby.selectedRhythm = selectedRhythm;
    }
}
// Define a function to get a random integer within a range
int getRandom(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void sendMidiBasedOnRhythms()
{
    if (dubby.elapsedTime >= nextBeatTime)
    {
        // Keep track of previously active notes
        std::unordered_set<int> prevActiveNotesSet = activeNotesSet;

        // Clear the set of active notes
        activeNotesSet.clear();

        // Beat processing logic here
        for (int i = 0; i < MAX_RHYTHMS; ++i)
        {
            // Check if the current subdivisions should play a note
            if (dubby.rhythms[i][midiMessageCounter % dubby.lengths[i]] == 1)
            {
                int note = notes[i];
                activeNotesSet.insert(
                    note); // Add the note to the set of active notes
            }
        }

        // Send note off for notes that were active but are no longer active
        for (int note : prevActiveNotesSet)
        {
            if (activeNotesSet.find(note) == activeNotesSet.end())
            {
//                int channel = (note - notes[0]) % 16; // Calculate channel based on note position (0 to 7)
                int channel = 0; // Calculate channel based on note position (0 to 7)

                MIDIUsbSendNoteOff(channel, note);  // Sending note off for USB
                MIDIUartSendNoteOff(channel, note); // Sending note off for UART
            }
        }

        // Calculate new velocity based on baseline and randomness
        int velocityIndex = 0; // Initialize the index to zero
        for (int note : activeNotesSet)
        {
            for (int i = 0; i < MAX_RHYTHMS; ++i)
            {
                if (notes[i] == note)
                {
                   int channel = 0;
                   // int channel = (i % 16); // Calculate the channel for this note
                    velocities[velocityIndex] = std::max(0, std::min(127, baselineVelocities[i] + getRandom(-velocityRandomAmounts[i] * baselineVelocities[i], velocityRandomAmounts[i] * baselineVelocities[i])));
                    MIDIUsbSendNoteOn(channel, note, velocities[velocityIndex]);
                    MIDIUartSendNoteOn(channel, note, velocities[velocityIndex]);
                    ++velocityIndex; // Increment the index after assigning velocity
                }
            }
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
    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        if (selectedRhythm == i && dubby.encoderPressed[i] == false)
        {
            lengthX = lengths[i];
            eventsXTemp = events[i];
            offsetX = offsets[i];

            if (!knobValueMatches(knobValue1, prevKnobValue1))
            {
                lengths[i] = knobValue1;
                prevKnobValue1 = knobValue1;
                //                midiMessageCounter = 0;
                // dubby.startIndex[i] = 0;
                dubby.endIndex[i] = (dubby.startIndex[i] + dubby.stepsOnDisplay);
                dubby.desiredLength[i] = dubby.lengths[i] = dubby.stepsOnDisplay;
            }

            if (!knobValueMatches(knobValue2, prevKnobValue2))
            {
                events[i] = knobValue2;
                prevKnobValue2 = knobValue2;
                //                    midiMessageCounter = 0;

                //   dubby.startIndex[i] = 0;
                dubby.endIndex[i] = (dubby.startIndex[i] + dubby.stepsOnDisplay);
                dubby.desiredLength[i] = dubby.lengths[i] = dubby.stepsOnDisplay;
            }
            // Perform actions using knobValue2

            if (dubby.buttons[0].FallingEdge())
            {
                if (adjustVelocityBaseline)
                {
                    adjustVelocityBaseline = false;
                }
                else
                {
                    offsets[i]++;
                }
            }

            if (dubby.buttons[1].FallingEdge())
            {
                if (adjustVelocityRandomnessAmount)
                {
                    adjustVelocityRandomnessAmount = false;
                }
                else
                {
                    offsets[i]--;
                }
            }

            break;
        }
    }
}

void handleScales()
{
    const uint8_t *chord = nullptr;
    switch (static_cast<ScaleType>(knobValue3))
    {
    case CHROMATIC:
        chord = chromatic_chord;
        scaleForPrint = "CHRO";
        break;
    case MAJOR:
        chord = major_chord;
        scaleForPrint = "MAJ";
        break;
    case MINOR:
        chord = minor_chord;
        scaleForPrint = "MIN";
        break;
    case PENTATONIC:
        chord = pentatonic_chord;
        scaleForPrint = "PEN";
        break;
    case MINOR7:
        chord = minor7_chord;
        scaleForPrint = "MIN7";
        break;
    case MAJOR7:
        chord = major7_chord;
        scaleForPrint = "MAJ7";
        break;
    default:
        chord = chromatic_chord;
        scaleForPrint = "CHRO";
        break;
    }
    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        notes[i] = chord[i] + noteShift + octaveShift;
    }
}

void handleKnobs()
{
    knobValue1 = (dubby.GetKnobValue(dubby.CTRL_1) * maxLengthsVal) + 1; // Adjust knob 1 to start from 1 and have a maximum of 32
    knobValue2 = dubby.GetKnobValue(dubby.CTRL_2) * maxEventsVal;
    knobValue3 = (dubby.GetKnobValue(dubby.CTRL_3) * 6); // Adjust knob 3 to start from 1 and have a maximum of 32
    knobValue4 = dubby.GetKnobValue(dubby.CTRL_4) * numOctaves;

    // Limit the knob values to 32
    knobValue1 = std::min(knobValue1, maxLengthsVal);
    knobValue2 = std::min(knobValue2, maxEventsVal);
    knobValue3 = std::min(knobValue3, 6 - 1);
    knobValue4 = std::min(knobValue4, numOctaves - 1);

    // Ensure events do not exceed lengths
    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        if (events[i] > lengths[i])
        {
            events[i] = lengths[i];
        }
    }
}

void shiftNotesAndOctaves()
{

    if (dubby.buttons[2].FallingEdge())
    {
        if (adjustBpm)
        {
            adjustBpm = false;
        }
        else
        {
            noteShift = std::min(noteShift + 1,
                                 11); // Increment noteShift but limit it to 11
        }
    }

    if (dubby.buttons[3].FallingEdge())
    {
        noteShift = std::max(noteShift - 1,
                             0); // Decrement noteShift but limit it to 0
    }

    OctaveShiftLevel octaveShiftLevel = static_cast<OctaveShiftLevel>(knobValue4);
    switch (octaveShiftLevel)
    {
    case OCTAVE_DOWN_3:
        octaveShift = -notesInOctave * 3;
        octaveShiftForPrint = "---";
        break;
    case OCTAVE_DOWN_2:
        octaveShift = -notesInOctave * 2;
        octaveShiftForPrint = "--";
        break;
    case OCTAVE_DOWN_1:
        octaveShift = -notesInOctave;
        octaveShiftForPrint = "-";
        break;
    case NO_OCTAVE_SHIFT:
        octaveShift = 0;
        octaveShiftForPrint = "";
        break;
    case OCTAVE_UP_1:
        octaveShift = notesInOctave;
        octaveShiftForPrint = "+";
        break;
    case OCTAVE_UP_2:
        octaveShift = notesInOctave * 2;
        octaveShiftForPrint = "++";
        break;
    case OCTAVE_UP_3:
        octaveShift = notesInOctave * 3;
        octaveShiftForPrint = "+++";
        break;
    default:
        octaveShift = 0;
        octaveShiftForPrint = "";
        break;
    }
}

void handleJoystick()
{
    int maxMultiplicationFactor = 6;
    int halfMaxMultiplicationFactor = maxMultiplicationFactor / 2;

    float firstThreshold = 0.2f;
    float firstThresholdInverse = 1.0f - firstThreshold;
    float secondThreshold = 0.4f;
    float secondThresholdInverse = 1.0f - secondThreshold;

    // Adjust events1Temp based on knob value for CTRL_6
    if (dubby.GetKnobValue(dubby.CTRL_6) < firstThreshold)
    {
        // Quadruple the events1 value temporarily
        eventsTemp[0] = std::min(events[0] * maxMultiplicationFactor, lengths[0]);
        eventsTemp[4] = std::min(events[4] * maxMultiplicationFactor, lengths[4]);
    }
    else if (dubby.GetKnobValue(dubby.CTRL_6) < secondThreshold)
    {
        // Double the events1 value temporarily
        eventsTemp[0] = std::min(events[0] * halfMaxMultiplicationFactor, lengths[0]);
        eventsTemp[4] = std::min(events[4] * halfMaxMultiplicationFactor, lengths[4]);
    }
    else
    {
        // Reset events1 to its original value
        eventsTemp[0] = events[0];
        eventsTemp[4] = events[4];
    }

    // Adjust events2Temp based on knob value for CTRL_5
    if (dubby.GetKnobValue(dubby.CTRL_5) < firstThreshold)
    {
        // Quadruple the events2 value temporarily
        eventsTemp[1] = std::min(events[1] * maxMultiplicationFactor, lengths[1]);
        eventsTemp[5] = std::min(events[5] * maxMultiplicationFactor, lengths[5]);
    }
    else if (dubby.GetKnobValue(dubby.CTRL_5) < secondThreshold)
    {
        // Double the events2 value temporarily
        eventsTemp[1] = std::min(events[1] * halfMaxMultiplicationFactor, lengths[1]);
        eventsTemp[5] = std::min(events[5] * halfMaxMultiplicationFactor, lengths[5]);
    }
    else
    {
        // Reset events2 to its original value
        eventsTemp[1] = events[1];
        eventsTemp[5] = events[5];
    }

    // Adjust events3Temp based on knob value for CTRL_6
    if (dubby.GetKnobValue(dubby.CTRL_6) > firstThresholdInverse)
    {
        // Quadruple the events3 value temporarily
        eventsTemp[2] = std::min(events[2] * maxMultiplicationFactor, lengths[2]);
        eventsTemp[6] = std::min(events[6] * maxMultiplicationFactor, lengths[6]);
    }
    else if (dubby.GetKnobValue(dubby.CTRL_6) > secondThresholdInverse)
    {
        // Double the events3 value temporarily
        eventsTemp[2] = std::min(events[2] * halfMaxMultiplicationFactor, lengths[2]);
        eventsTemp[6] = std::min(events[6] * halfMaxMultiplicationFactor, lengths[6]);
    }
    else
    {
        // Reset events3 to its original value
        eventsTemp[2] = events[2];
        eventsTemp[6] = events[6];
    }

    // Adjust events4Temp based on knob value for CTRL_5
    if (dubby.GetKnobValue(dubby.CTRL_5) > firstThresholdInverse)
    {
        // Quadruple the events4 value temporarily
        eventsTemp[3] = std::min(events[3] * maxMultiplicationFactor, lengths[3]);
        eventsTemp[7] = std::min(events[7] * maxMultiplicationFactor, lengths[7]);
    }
    else if (dubby.GetKnobValue(dubby.CTRL_5) > secondThresholdInverse)
    {
        // Double the events4 value temporarily
        eventsTemp[3] = std::min(events[3] * halfMaxMultiplicationFactor, lengths[3]);
        eventsTemp[7] = std::min(events[7] * halfMaxMultiplicationFactor, lengths[7]);
    }
    else
    {
        // Reset events4 to its original value
        eventsTemp[3] = events[3];
        eventsTemp[7] = events[7];
    }
}

void handleButtons()
{
    // Check if button[2] is pressed and held for more than 2000 ms
    if (dubby.buttons[0].TimeHeldMs() > 1000 && dubby.buttons[2].TimeHeldMs() > 1000)
    {
        // Set events to 0 for each rhythm
        for (int i = 0; i < MAX_RHYTHMS; ++i)
        {
            events[i] = 0;
        }
    }

    if (dubby.buttons[3].TimeHeldMs() > 3000 && dubby.buttons[4].TimeHeldMs() > 3000)
    {
        dubby.ResetToBootloader();
    }
}
void generateRhythms()
{
    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        // Ensure events do not exceed lengths
        if (eventsTemp[i] > lengths[i])
        {
            eventsTemp[i] = lengths[i];
        }
        
        dubby.rhythms[i] = euclideanRhythm(lengths[i], eventsTemp[i], offsets[i]);
        dubby.lengths[i] = lengths[i];
    }
}

void printValuesToDisplay()
{

////////////////////////////////////////
// CLEAR ALL EVENTS: PRINT TO DISPLAY //
////////////////////////////////////////
    if (dubby.buttons[0].Pressed() && dubby.buttons[2].Pressed())
    {
        countdown -= daisy::System::GetNow() - lastTimeUpdate;
        lastTimeUpdate = daisy::System::GetNow();

        printStuffLeft = "";
        printStuffRight = "";

        if (countdown > 0)
        {
            printStuffMiddle = "CLEARING EVENTS IN: " + std::to_string(countdown) + " MS";
        }
        else
        {
            printStuffMiddle = "EVENTS SUCCESSFULLY CLEARED";
        }
        dubby.UpdateStatusBar(&printStuffMiddle[0], dubby.MIDDLE, 127);
    }

///////////////////////////////////////
// ADJUST VELOCITY: PRINT TO DISPLAY //
//////////////////////////////////////

    else if (dubby.buttons[0].Pressed() && dubby.buttons[0].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        countdown = 1000;
        lastTimeUpdate = daisy::System::GetNow();

        printStuffLeft = "";
        printStuffRight = "";
        printStuffMiddle = "VELOCITY:" + std::to_string(baselineVelocities[selectedRhythm]);

        dubby.UpdateStatusBar(&printStuffMiddle[0], dubby.MIDDLE, 127);
    }

//////////////////////////////////////////////////
// ADJUST VELOCITY RANDOMNESS: PRINT TO DISPLAY //
//////////////////////////////////////////////////
    else if (dubby.buttons[1].Pressed() && dubby.buttons[1].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        printStuffLeft = "";
        printStuffRight = "";
        printStuffMiddle = "VEL. RANDOM:" + std::to_string((int)(velocityRandomAmounts[selectedRhythm] * 100)) + " %";

        dubby.UpdateStatusBar(&printStuffMiddle[0], dubby.MIDDLE, 127);

    }

//////////////////////////////////
// ADJUST BPM: PRINT TO DISPLAY //
//////////////////////////////////
    else if (dubby.buttons[2].Pressed() && dubby.buttons[2].TimeHeldMs() > buttonPressedTimeBeforeSwitching)
    {
        countdown = 1000;
        lastTimeUpdate = daisy::System::GetNow();
        printStuffLeft = "";
        printStuffRight = "";
        printStuffMiddle = "BPM:" + std::to_string((int)bpm);

        dubby.UpdateStatusBar(&printStuffMiddle[0], dubby.MIDDLE, 127);

    }

///////////////////////////////////////
// DEFAULT DISPLAY: PRINT TO DISPLAY //
///////////////////////////////////////
    else
    {
        countdown = 1000;
        lastTimeUpdate = daisy::System::GetNow();
        printStuffLeft = "LE:" + std::to_string(lengthX) + " EV:" + std::to_string(eventsXTemp) + " OF:" + std::to_string(offsetX);
        printStuffRight = noteShiftForPrint(noteShift) + " " + scaleForPrint + octaveShiftForPrint + " " + std::to_string((int)bpm);

        dubby.UpdateStatusBar(&printStuffLeft[0], dubby.LEFT, 70);
        dubby.UpdateStatusBar(&printStuffRight[0], dubby.RIGHT, 57);
    }
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    loadMeter.OnBlockStart();

    double sumSquaredIns[4] = {0.0f};
    double sumSquaredOuts[4] = {0.0f};

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

        if (dubby.scopeSelector == 0)
            dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f;
        else if (dubby.scopeSelector == 1)
            dubby.scope_buffer[i] = (in[2][i] + in[3][i]) * .5f;
        else if (dubby.scopeSelector == 2)
            dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;
        else if (dubby.scopeSelector == 3)
            dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f;
        else if (dubby.scopeSelector == 4)
            dubby.scope_buffer[i] = in[0][i];
        else if (dubby.scopeSelector == 5)
            dubby.scope_buffer[i] = in[1][i];
        else if (dubby.scopeSelector == 6)
            dubby.scope_buffer[i] = in[2][i];
        else if (dubby.scopeSelector == 7)
            dubby.scope_buffer[i] = in[3][i];
        else if (dubby.scopeSelector == 8)
            dubby.scope_buffer[i] = out[0][i];
        else if (dubby.scopeSelector == 9)
            dubby.scope_buffer[i] = out[1][i];
        else if (dubby.scopeSelector == 10)
            dubby.scope_buffer[i] = out[2][i];
        else if (dubby.scopeSelector == 11)
            dubby.scope_buffer[i] = out[3][i];
    }

    for (int j = 0; j < 4; j++)
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

    dubby.DrawLogo();
    System::Delay(200);
    dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateWindowSelector(0, false);
    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());

    dubby.midi_uart.StartReceive();
    // bpm = 140;

    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        lengths[i] = 32;
        events[i] = 0;
    }

    prevKnobValue1 = knobValue1;
    prevKnobValue2 = knobValue2;

    // Set all indexes of endIndex to 32
    for (int i = 0; i < MAX_RHYTHMS; ++i)
    {
        dubby.endIndex[i] = dubby.stepsOnDisplay;
        //  dubby.startIndex[i] = lengths[i];
    }
    while (1)
    {
        beatInterval = beatsPerMinuteToInterval / bpm;

        handleScales();
        shiftNotesAndOctaves();
        handleKnobs();
        handleEncoder();
        generateRhythms();
        handleJoystick();
        selectRhythm();
        sendMidiBasedOnRhythms();
        printValuesToDisplay();
        handleButtons();

        dubby.elapsedTime = static_cast<float>(daisy::System::GetNow()) / 1.0e3f; // Current time in seconds

        // Button handling for reset to bootloader
        // if(dubby.buttons[3].TimeHeldMs() > 1000){dubby.ResetToBootloader();}

        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        // CPU METER =====
        std::string str = std::to_string(int(loadMeter.GetAvgCpuLoad() * 100.0f)) + "%";
        // dubby.UpdateStatusBar(&str[0], dubby.MIDDLE);
        // ===============

        dubby.midi_uart.Listen();
        dubby.midi_usb.Listen();

        // Handle UART MIDI Events
        while (dubby.midi_uart.HasEvents())
        {
            HandleMidiUartMessage(dubby.midi_uart.PopEvent());
        }

        // Handle USB MIDI Events
        while (dubby.midi_usb.HasEvents())
        {
            HandleMidiUsbMessage(dubby.midi_usb.PopEvent());
        }
    }
}
