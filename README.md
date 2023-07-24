# console-audioSpectra
Printing spectral histograms of incoming audio to console, and basic pitch detection and chord guessing algorithms.
Started as an excuse to learn and implement the (simplest version of the) FFT.
Uses SDL2 to record audio and performs FFT on it. Then interprets and/or visualizes FFT results in hopefully interesting ways.

**VISUALIZER OPTIONS**

**Scaled Spectrum**

1. Fixed semilog
2. Fixed linear
3. Fixed log-log
4. Adaptive semilog
5. Adaptive linear
6. Adaptive log-log

**Wrapped Spectrum  (Spectral Guitar Tuner)**

7. Fixed
8. Adaptive

**Music Algorithms**

9. Pitch recognition (automatic tuner)
10. Chord Guesser

## Scaled Spectrum Mode
"Plots" a spectral histogram to the console with linear, semilog, or log-log scaling. And repeat.

Uses `GetConsoleScreenBufferInfo()` to find console dimensions and scale graph accordingly. Uses `system("cls")` to refresh the console between frames. Which is not great, but eh.

<img width="960" alt="sc1" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/f63131fa-9d64-4799-919c-1b24cc7239d4">

## Spectral Guitar Tuner Mode
Guitar tuner mode is semilog scaling that wraps around at the octave.
i.e., The coefficient of 55Hz (A1) adds to 110Hz (A2), 220Hz (A3) etc.
Octave-marking information is thus effectively discarded, but information of flatness or sharpness is retained. Logic explained in [YouTube Video](https://youtu.be/Ufx_nrxLhq0).

<img width="960" alt="sc2" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/4ef75b59-4d2c-492f-a8ed-6b29b1e8675f">

## "Auto" Guitar Tuner Mode
This performs pitch detection by identifying peaks in the FFT and finding the "approximate HCF" (i.e., fundamental frequency corresponding to an arbitrary subset of harmonics) using continued fractions.
Tuner display is a needle-and-dial type, but the needle stays fixed and the dial moves, to prevent erratic needle motions near quarter-tone points.

<img width="960" alt="sc3" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/25d0b8dc-2f5f-4e90-984d-bebde7e2624c">

## Chord Guesser
This attempts to name a chord by assigning pitch names to spectral spikes. Hit-or-miss, but often guesses something at least close-sounding.

<img width="960" alt="sc4" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/a2cbe5c5-b5a3-4183-9ec7-f45faf34fbf9">
<img width="960" alt="sc8" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/875ae46c-22ed-4d9e-808d-43f796db4603">
<img width="960" alt="sc9" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/fba01e4a-6957-4c6d-9cb0-1f4b33185a86">
<img width="960" alt="sc7" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/4071b711-6255-43e8-82ae-c2c67c8b59c6">
<img width="960" alt="sc5" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/468777dc-5ced-4fcb-9f65-cacc4194c2d6">
