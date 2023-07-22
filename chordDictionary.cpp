#include "chordDictionary.h"

/// Checks if a chord contains ALL the input notes
bool chord::contains(int notes_in[], int num_notes_in)
{
    for(int i=0; i<num_notes_in; i++)
    {
        bool contains_ith = false;
        for(int j=0; j<num_notes; j++)
            if(notes[j] == notes_in[i])
            {
                contains_ith = true;
                break;
            }
        if(!contains_ith)
            return false;
    }
    return true;
}

/// Transpose a chord up by a certain number of semitones (-ve semitones_up means transpose down)
chord transpose_chord(chord old_chord, int semitones_up)
{
    char note_names[][3] = {"A ", "A#", "B ", "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#"};

    /// If shift is zero return chord unchanged
    if(semitones_up == 0)
        return old_chord;
    /// If shift is more than 11 semitones down the arithmetic doesn't work
    else if(semitones_up < -11)
    {
        std::cout<<"\nInvalid transpose\n";
        return old_chord;
    }

    /// Create a new chord to return
    chord new_chord;

    /// Number of notes is unchanged through transposition
    new_chord.num_notes = old_chord.num_notes;

    /// Iterate through notes and transpose each one
    for(int i=0; i<old_chord.num_notes; i++)
        new_chord.notes[i] = (11 + old_chord.notes[i] + semitones_up)%12 + 1;

    /// Copy the old chord name to the new chord,
    /// then replace the first two characters of the name with the appropriate letter name
    /// i.e., with the letter name corresponding to the first element of the chord
    for(int i=0; i<CHORD_NAME_SIZE; i++)
        new_chord.name[i] = old_chord.name[i];
    new_chord.name[0] = note_names[new_chord.notes[0]-1][0];
    new_chord.name[1] = note_names[new_chord.notes[0]-1][1];

    return new_chord;
}

/// Create chord dictionary, i.e., populate all_chords[] with transposed up versions of A_root_chords
void initialize_chord_dictionary()
{
    for(int i=0; i<NUM_CHORD_TYPES; i++)
        for(int j=0; j<12; j++)
            all_chords[i*12+j] = transpose_chord(A_root_chords[i], j);
}

/// Given a set of notes (pitch numbers 1 = A, 2 = A#, 3 = B, etc.),
/// this function finds a chord that contains all those notes
/// and writes the chord's name to char* name_out
int what_chord_is(char* name_out, int notes[], int num_notes)
{
    /// Iterate through all_chords looking for a chord that contains all input notes
    int chord_index = -1;
    for(int i=0; i<NUM_CHORD_TYPES*12; i++)
        if(all_chords[i].contains(notes, num_notes))
            chord_index = i;

    /// If not found return 0
    if(chord_index == -1)
        return 0;

    /// Write name of chord found to name_out
    int name_length = 0;
    while(all_chords[chord_index].name[name_length] != '\0')
    {
        name_out[name_length] = all_chords[chord_index].name[name_length];
        name_length++;
    }

    /// Return name length so that calling function known how many characters were written
    return name_length;
}
