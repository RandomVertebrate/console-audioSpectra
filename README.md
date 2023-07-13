# console-audioSpectra
Printing spectral histograms of incoming audio to console.
Essentially an excuse to learn and implement the (simplest version of the) FFT algorithm.

Uses SDL2 to record audio and performs FFT on it. Then "plots" a spectral histogram to the console with linear, semilog, or log-log scaling. And repeat.

Uses `GetConsoleScreenBufferInfo()` to find console dimensions and scales graph accordingly. Uses `system("cls")` to refresh the console between frames. Which is not great, but eh.

<img width="663" alt="sc2" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/afde5d69-ce84-4a63-a891-2932ca87a1c1">

## Guitar Tuner Mode
Guitar tuner mode is semilog scaling that wraps around at the octave.
i.e., The coefficient of 55Hz (A1) adds to 110Hz (A2), 220Hz (A3) etc.
Octave-marking information is thus effectively discarded, but information of flatness or sharpness is retained.

<img width="663" alt="sc1" src="https://github.com/RandomVertebrate/console-audioSpectra/assets/54997017/2e570cc0-9821-4324-b7a0-697a37705966">
