#include "chordDictionary.h"
#include "helper.h"

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

    new_chord.name[1] = ' ';
    pitchName(new_chord.name, new_chord.notes[0]);

    return new_chord;
}

/// Create chord dictionary, i.e., populate all_chords[] with transposed up versions of A_root_chords
void initialize_chord_dictionary()
{
    for(int i=0; i<NUM_CHORD_TYPES; i++)
        for(int j=0; j<12; j++)
            all_chords[i*12+j] = transpose_chord(A_root_chords[i], j);
    chord_dictionary_initialized = true;
}

/// Given a set of notes (pitch numbers 1 = A, 2 = A#, 3 = B, etc.),
/// this function finds a chord that contains all those notes
/// and writes the chord's name to char* name_out
int what_chord_is(char* name_out, int notes[], int num_notes)
{
    int chord_index;                                                /// This will store the guess (its index in all_chords[])

    /// Iterate through all_chords looking for chords that contains all input notes
    int candidates[NUM_CHORD_TYPES*12];
    int num_candidates = 0;
    for(int i=0; i<NUM_CHORD_TYPES*12; i++)
        /// If a chord is found that contains all input notes, add its index to candidates[]
        if(all_chords[i].contains(notes, num_notes))
            candidates[num_candidates++] = i;

    /// If not found return 0
    if(num_candidates == 0)
        return 0;

    /// If there is exactly one candidate, it is the answer.
    if(num_candidates == 1)
        chord_index = candidates[0];
    /// Otherwise, trim down the list of candidates by the following criteria (in order of importance):
    /// 1. Based on chord::num_notes (smaller preferred).
    /// 2. Based on root note.
    else
    {
        /// Find minimum chord size in list of candidates
        int min_size = all_chords[candidates[0]].num_notes;
        for(int i=0; i<num_candidates; i++)
            if(all_chords[candidates[i]].num_notes < min_size)
                min_size = all_chords[candidates[i]].num_notes;
        /// Disregard (delete) all candidates with more notes than min_size
        for(int i=0; i<num_candidates; i++)
            if(all_chords[candidates[i]].num_notes > min_size)
            {
                for(int j=i; j<num_candidates-1; j++)
                    candidates[j] = candidates[j+1];
                num_candidates--;
            }
        /// Now guess the first remaining candidate by default
        chord_index = candidates[0];
        /// But if the root note of some other candidate matches the first input note, guess that instead.
        for(int i=0; i<num_candidates; i++)
            if(all_chords[candidates[i]].notes[0] == notes[0])
                chord_index = candidates[i];

    }

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
