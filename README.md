# console-audioSpectra
Printing spectral histograms of incoming audio to console.
Essentially an excuse to learn and implement the (simplest version of the) FFT algorithm.

Uses SDL2 to record audio and performs FFT on it. Then "plots" a spectral histogram to the console with linear, semilog, or log-log scaling. And repeat.

Uses `GetConsoleScreenBufferInfo()` to find console dimensions and scale graph accordingly. Uses `system("cls")` to refresh the console between frames. Which is not great, but eh.

<img width="960" alt="sc1" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/f63131fa-9d64-4799-919c-1b24cc7239d4">

## Spectral Guitar Tuner Mode
Guitar tuner mode is semilog scaling that wraps around at the octave.
i.e., The coefficient of 55Hz (A1) adds to 110Hz (A2), 220Hz (A3) etc.
Octave-marking information is thus effectively discarded, but information of flatness or sharpness is retained.

<img width="960" alt="sc2" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/4ef75b59-4d2c-492f-a8ed-6b29b1e8675f">

## 'Auto' Guitar Tuner Mode
This performs pitch detection by identifying peaks in the FFT and finding the "approximate HCF" (i.e., fundamental frequency corresponding to an arbitrary subset of harmonics) using continued fractions.
Tuner display is a needle-and-dial type, but the needle stays fixed and the dial moves, to prevent erratic needle motions near quarter-tone points.

<img width="960" alt="sc3" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/105a587b-7851-4dd9-95ca-40a994bc3395">

