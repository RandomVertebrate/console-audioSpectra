#include <iostream>
#include <math.h>
#include <complex>
#include <windows.h>
#include <SDL2/SDL.h>

#include "helper.h"
#include "chordDictionary.h"

/**
--------------------------------------
----Visualizer Function Parameters----
--------------------------------------
minfreq and maxfreq are the horizontal (frequency) bounds of the spectral histogram.

adaptive sets whether to continually adjust vertical scaling to fit and fill console
window.

iterations is the total number of refresh cycles before function return.

delayMicroseconds sets the visualizer refresh rate or "frame rate".

graphScale sets the vertical scale of data relative to the console window height.
Irrelevant if adaptive is enabled.
**/

void SemilogVisualizer(int minfreq, int maxfreq, AudioQueue &MainAudioQueue, int consoleWidth, int consoleHeight,
                       bool adaptive = false, float graphScale = 0.0008);

void LinearVisualizer(int minfreq, int maxfreq, AudioQueue &MainAudioQueue, int consoleWidth, int consoleHeight,
                       bool adaptive = false, float graphScale = 0.0008);

void LoglogVisualizer(int minfreq, int maxfreq, AudioQueue &MainAudioQueue, int consoleWidth, int consoleHeight,
                       bool adaptive = false, float graphScale = 0.0008);

void SpectralTuner(AudioQueue &MainAudioQueue, int consoleWidth, int consoleHeight, bool adaptive = false,
                   float graphScale = 0.0008);

/**
------------------
----Auto Tuner----
------------------
startAutoTuner() is a more traditional guitar tuner. It performs internal pitch
detection and displays a stationary "needle" and moving note-name "dial". Tuning
can be performed by aligning the note name to the needle.

Pitch detection is performed by finding peaks in the fft and assuming that they
are harmonics of an underlying fundamental. The approximate HCF of the frequencies
therefore gives the pitch.

span_semitones sets the span (and precision) of the dial display, i.e. how many
pitch names are to be shown on screen at once.
**/

void AutoTuner(AudioQueue &MainAudioQueue, int consoleWidth, bool printNeedle = true, int span_semitones = 4);

/**
----Chord Guesser----
ChordGuesser() finds frequency peaks and prints pitch names and, if found, a
chord name as well.
**/

void ChordGuesser(AudioQueue &MainAudioQueue, int max_notes = 4);
