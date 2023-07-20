#include <iostream>
#include <math.h>
#include <complex>
#include <windows.h>
#include <SDL2/SDL.h>
#include "visualizer.h"

float echoVolume;                       /// Anything recorded is immediately (-ish) played back at this volume.

AudioQueue MainAudioQueue(10000000);    /// Main queue. Recorded Audio is pushed, popped audio is played, and peeked audio is FFT'd.

/**
--------------------------
----Callback Functions----
--------------------------
Callback functions are called by the sound card from a parallel thread.

When the sound card has new mic data to report, it calls the recording
callback function and passes it a pointer Uint8* stream to this new data.
It also passes int streamLength, the number of bytes of new data.

When the sound card requires new data to play, it calls the playback
callback function and passes it a pointer Uint8* stream to where it wishes
this data to be written.
It also passes int streamLength, the number of bytes of data required.
**/
void RecCallback(void* userdata, Uint8* stream, int streamLength)
{
    Uint32 length = (Uint32)streamLength;
    MainAudioQueue.push((sample*)stream, length/sizeof(sample));
}
void PlayCallback(void* userdata, Uint8* stream, int streamLength)
{
    Uint32 length = (Uint32)streamLength;
    MainAudioQueue.pop((sample*)stream, length/sizeof(sample), ::echoVolume);
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_AUDIO);                                       /// Initialize SDL audio

    SDL_AudioSpec RecAudiospec, PlayAudiospec;                      /// SDL_AudioSpec objects used to tell SDL sample rate, buffer size etc.

    /// Setting audio parameters
    RecAudiospec.freq = RATE;
    RecAudiospec.format = AUDIO_S16SYS;
    RecAudiospec.samples = CHUNK;
    RecAudiospec.callback = RecCallback;                            /// Callback functions also passed to SDL through SDL_AudioSpec objects
    PlayAudiospec = RecAudiospec;
    PlayAudiospec.callback = PlayCallback;

    /// Opening two audio devices, one for recording and one for playback.
    SDL_AudioDeviceID PlayDevice = SDL_OpenAudioDevice(NULL, 0, &PlayAudiospec, NULL, 0);
    SDL_AudioDeviceID RecDevice = SDL_OpenAudioDevice(NULL, 1, &RecAudiospec, NULL, 0);

    /// Error messages if audio devices could not be opened
    if(PlayDevice <= 0)
        std::cerr<<"Playback device not opened: "<<SDL_GetError()<<"\n";
    if(RecDevice <= 0)
    {
        std::cerr<<"Recording device not opened: "<<SDL_GetError()<<"\n";
        return -1;
    }

    SDL_PauseAudioDevice(RecDevice, 0);                             /// Start recording
    SDL_Delay(1500);                                                /// Allow some time for audio queue to fill up
    SDL_PauseAudioDevice(PlayDevice, 0);                            /// Start playback

    /// Menu
    int ans, lim1, lim2;
    std::cout<<"\nVisualizer Options\n------------------\n"
        <<"\nScaled Spectrum:\n"
        <<"\n1 . Fixed semilog"
        <<"\n2 . Fixed linear"
        <<"\n3 . Fixed log-log"
        <<"\n4 . Adaptive semilog"
        <<"\n5 . Adaptive linear"
        <<"\n6 . Adaptive log-log"
        <<"\n7 . Octave-wrapped semilog (guitar tuner)"
        <<"\n8 . Adaptive guitar tuner"
        <<"\n\nOther:\n"
        <<"\n9 . Pitch recognition (auto tuner)"
        <<"\n10. Spike enumeration (chord speller)"
        <<"\n\nEnter choice: ";
    std::cin>>ans;
    if(ans<7)
    {
        std::cout<<"\nEnter lower frequency limit: ";
        std::cin>>lim1;
        std::cout<<"\nEnter upper frequency limit: ";
        std::cin>>lim2;
    }
    std::cout<<"\nEnter echo volume (0 = no echo): ";
    std::cin>>::echoVolume;
    switch(ans)
    {
        case 1 : startSemilogVisualizer(lim1, lim2, MainAudioQueue, 1000); break;
        case 2 : startLinearVisualizer(lim1, lim2, MainAudioQueue, 1000); break;
        case 3 : startLoglogVisualizer(lim1, lim2, MainAudioQueue, 1000); break;
        case 4 : startSemilogVisualizer(lim1, lim2, MainAudioQueue, 1000, true); break;
        case 5 : startLinearVisualizer(lim1, lim2, MainAudioQueue, 1000, true); break;
        case 6 : startLoglogVisualizer(lim1, lim2, MainAudioQueue, 1000, true); break;
        case 7 : startTuner(MainAudioQueue, 1000); break;
        case 8 : startTuner(MainAudioQueue, 1000, true); break;
        case 9 : startAutoTuner(MainAudioQueue, 1000); break;
        case 10: startChordSpeller(MainAudioQueue, 1000); break;
        default: return 0;
    }

    /// Close audio devices
    SDL_CloseAudioDevice(PlayDevice);
    SDL_CloseAudioDevice(RecDevice);
    /**
    ISSUE:
    no clean way to exit during execution.
    Audio devices expected not to be closed properly if the process is killed.
    Thankfully Windows seems to make it alright (daemons or something idk)
    **/

    return 0;
}
