#ifndef CHORDS_H
#define CHORDS_H


#include <array>

// Define the default note
#define DEFAULT_NOTE 60 // Change this value to your desired default note

// Define the maximum number of rhythms
#define MAX_RHYTHMS 8 // Change this value to your desired maximum number of rhythms


// Define the chromatic chord
uint8_t chromatic_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                        DEFAULT_NOTE + 1,
                                        DEFAULT_NOTE + 2,
                                        DEFAULT_NOTE + 3,
                                        DEFAULT_NOTE + 4,
                                        DEFAULT_NOTE + 5,
                                        DEFAULT_NOTE + 6,
                                        DEFAULT_NOTE + 7};

// Define the major chord
uint8_t major_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                    DEFAULT_NOTE + 4,
                                    DEFAULT_NOTE + 7,
                                    DEFAULT_NOTE + 11,
                                    DEFAULT_NOTE + 14,
                                    DEFAULT_NOTE + 17,
                                    DEFAULT_NOTE + 21,
                                    DEFAULT_NOTE + 24};

// Define the minor chord
uint8_t minor_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                    DEFAULT_NOTE + 3,
                                    DEFAULT_NOTE + 7,
                                    DEFAULT_NOTE + 10,
                                    DEFAULT_NOTE + 14,
                                    DEFAULT_NOTE + 17,
                                    DEFAULT_NOTE + 21,
                                    DEFAULT_NOTE + 24};

// Define the pentatonic chord
uint8_t pentatonic_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                         DEFAULT_NOTE + 5,
                                         DEFAULT_NOTE + 7,
                                         DEFAULT_NOTE + 12,
                                         DEFAULT_NOTE + 15,
                                         DEFAULT_NOTE + 19,
                                         DEFAULT_NOTE + 24,
                                         DEFAULT_NOTE + 27};

// Define the minor 7th chord
uint8_t minor7_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                     DEFAULT_NOTE + 3,
                                     DEFAULT_NOTE + 7,
                                     DEFAULT_NOTE + 10,
                                     DEFAULT_NOTE + 14,
                                     DEFAULT_NOTE + 17,
                                     DEFAULT_NOTE + 21,
                                     DEFAULT_NOTE + 24};

// Define the major 7th chord
uint8_t major7_chord[MAX_RHYTHMS] = {DEFAULT_NOTE,
                                     DEFAULT_NOTE + 4,
                                     DEFAULT_NOTE + 7,
                                     DEFAULT_NOTE + 11,
                                     DEFAULT_NOTE + 14,
                                     DEFAULT_NOTE + 17,
                                     DEFAULT_NOTE + 21,
                                     DEFAULT_NOTE + 25};



// Define enums for scale types and octave shift levels
enum ScaleType {
    CHROMATIC = 0,
    MAJOR,
    MINOR,
    PENTATONIC,
    MINOR7,
    MAJOR7
};

#endif CHORDS_H
