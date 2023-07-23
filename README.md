# console-audioSpectra
Printing spectral histograms of incoming audio to console, and basic pitch detection and chord guessing algorithms.
Started as an excuse to learn and implement the (simplest version of the) FFT.

Uses SDL2 to record audio and performs FFT on it. Then "plots" a spectral histogram to the console with linear, semilog, or log-log scaling. And repeat.

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
This attempts to name a chord by assigning pitch names to spectral spikes. Hit-or-miss, but often guesses something close-sounding at least.

<img width="960" alt="sc7" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/8cb0062b-b8ca-4028-bb6c-99fa6b5cb089">
<img width="960" alt="sc6" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/ee3d1edc-a2d4-48cf-ba8d-fa0c7a24b666">
<img width="960" alt="sc5" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/45917220-6752-4296-9ca5-e37e30c80c5d">
<img width="960" alt="sc4" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/52a18b76-67f5-4455-aca0-6c2ce125a57e">
