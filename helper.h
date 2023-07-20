#include <iostream>
#include <math.h>
#include <complex>
#include <windows.h>
#include "audioDSP.h"

void show_bargraph(int bars[], int n_bars, int height=50,               /// Histogram plotter
                   int hScale = 1, float vScale = 1, char symbol='|');

float index2freq(int index);

float freq2index(float freq);

/**
----float mapLin2Log()----
Maps linear axis to log axis.
If graph A has a linearly scaled x-axis and graph B has a logarithmically scaled x-axis
then this function gives the x-coordinate in B corresponding to some x-coordinate in A.
**/
float mapLin2Log(float LinMin, float LinRange, float LogMin, float LogRange, float LinVal);

/**
----float approx_hcf()----
Finds approximate HCF of numbers,
e.g. finds a fundamental frequency given an arbitrary subset of its harmonics. Must be
approximate because input will be real measured data, so not exact. Returns 0 if no HCF
is found (input is float, so 1 is not always a factor).
**/
float approx_hcf(float inputs[], int num_inputs, int max_iter = 5, int accuracy_threshold = 10);

/**
----Find_n_Largest()----
Finds indices of n_out largest samples in input array.
**/
void Find_n_Largest(int* output, sample* input, int n_out, int n_in, float MinRatio = 1);

/**
----int pitchNumber()----
Given a frequency freq, this function finds and returns the closest pitch number,
which is an integer between 1 and 12 where 1 refers to A and 12 to G#.
The number of cents between the input frequency and the 'correct' pitch is written
to centsSharp
**/
int pitchNumber(float freq, float *centsSharp = nullptr);

/**
----int pitchName()----
Given a pitch number from 1 to 12 where 1 refers to A and 12 to G#
This function writes the pitch name to char* name and returns the length of the name.
**/
int pitchName(char* name, int pitch_num);

