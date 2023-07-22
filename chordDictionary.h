#include <iostream>

#define CHORD_NAME_SIZE 15
#define CHORD_MAX_NOTES 5
#define NUM_CHORD_TYPES 10

/**

NOTE REPRESENTATION: PITCH NUMBERS

Here, notes/pitches are represented by numbers as follows:
1 = A, 2 = A#, 3 = B etc.

**/

struct chord
{
    int notes[CHORD_MAX_NOTES];
    int num_notes;
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
**/
static chord A_root_chords[] = {

    {{1, 5, 8, 12}, 4, "A Maj7"},
    {{1, 4, 8, 11}, 4, "A min7"},
    {{1, 5, 8, 11}, 4, "A dom7"},
    {{1, 5, 8, 12}, 4, "A minMaj7"},
    {{1, 4, 7}, 3, "A dim"},
    {{1, 6, 8}, 3, "A sus4"},
    {{1, 3, 8}, 3, "A sus2"},
    {{1, 5, 8}, 3, "A Maj"},
    {{1, 4, 8}, 3, "A min"},
    {{1, 8}, 2, "A 5"}

};

/// After initialization, this will hold all transpositions of A_root_chords
static chord all_chords[NUM_CHORD_TYPES*12];

/// Initialization function: Performs transpositions to populate all_chords from A_root_chords
void initialize_chord_dictionary();

/// Looks for a chord that contains ALL provided input notes, and writes the name of the chord to name_out
int what_chord_is(char* name_out, int notes[], int num_notes);
