#include <iostream>

#define CHORD_NAME_SIZE 15
#define CHORD_MAX_NOTES 5
#define NUM_CHORD_TYPES 10

/**
INFO

Notes/pitches are represented by pitch numbers:
1 = A, 2 = A#, 3 = B etc.

All chords must be defined in root position.
**/

struct chord
{
    int num_notes;
    int notes[CHORD_MAX_NOTES];                                             /// Must be in root position
    char name[CHORD_NAME_SIZE];

    bool contains(int notes_in[], int num_notes_in);
};

chord transpose_chord(chord old_chord, int semitones_up);

/**
A_root_chords will be transposed up to find all other chords.
i.e., A_root_chords defines all chord types that the program will look for.

Chords with greater numbers of notes should appear earlier, as should chords
less likely to occur. This way more likely outcomes (later entries) get
preference.

NUM_CHORD_TYPES should reflect how many chords are in the array below.

All chords must be defined in root position.
**/
static chord A_root_chords[] = {

    {4, {1, 5, 8, 12}, "A Maj7"},
    {4, {1, 4, 8, 11}, "A min7"},
    {4, {1, 5, 8, 11}, "A dom7"},
    {4, {1, 4, 8, 12}, "A minMaj7"},
    {3, {1, 4, 7}, "A dim"},
    {3, {1, 6, 8}, "A sus4"},
    {3, {1, 3, 8}, "A sus2"},
    {3, {1, 5, 8}, "A Maj"},
    {2, {1, 4, 8}, "A min"},
    {2, {1, 8}, "A 5"}

};

/// After initialization, this will hold all transpositions of A_root_chords
static chord all_chords[NUM_CHORD_TYPES*12];

/// Initialization function: Performs transpositions to populate all_chords from A_root_chords
void initialize_chord_dictionary();

/// Looks for a chord that contains ALL provided input notes, and writes the name of the chord to name_out
int what_chord_is(char* name_out, int notes[], int num_notes);
