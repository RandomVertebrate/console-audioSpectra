#include <iostream>
#include <math.h>
#include <complex>
#include <windows.h>
#include <conio.h>
#include <SDL2/SDL.h>
#include "visualizer.h"

#define REFRESH_TIME 10                 /// Time in milliseconds. Sets (maximum) refresh rate.

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

/// Function to determine whether x key is currently pressed (exit condition)
char capture_button_press()
{
    if(_kbhit())
        return getch();
    return 0;
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

    /// Unpause recording and playback devices
    SDL_PauseAudioDevice(RecDevice, 0);                             /// Start recording
    SDL_Delay(2000);                                                /// Allow some time for audio queue to fill up
    SDL_PauseAudioDevice(PlayDevice, 0);                            /// Start playback

    /// Menu
    int ans, lim1, lim2;

    MAIN_MENU:

    std::cout<<"VISUALIZER OPTIONS\n"
        <<"\nScaled Spectrum\n----------------"
        <<"\n1 . Fixed semilog"
        <<"\n2 . Fixed linear"
        <<"\n3 . Fixed log-log"
        <<"\n4 . Adaptive semilog"
        <<"\n5 . Adaptive linear"
        <<"\n6 . Adaptive log-log"
        <<"\n\nWrapped Spectrum (Spectral Guitar Tuner)\n----------------------------------------"
        <<"\n7 . Fixed"
        <<"\n8 . Adaptive"
        <<"\n\nMusic Algorithms\n----------------"
        <<"\n9 . Pitch recognition (automatic tuner)"
        <<"\n10. Chord Guesser"
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

    int consoleWidth = 0;
    int consoleHeight = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbi;                                /// Console object to retrieve console window size for graph scaling
    int new_consoleWidth;
    int new_consoleHeight;

    std::cout<<"\nStarting...\nDuring execution, press x to exit or m to return to menu";
    SDL_Delay(1000);
    system("cls");

    /// Screen refresh loop. Run for at least 10 minutes or until x is pressed
    for(int i=0; i<600000/REFRESH_TIME; i++)
    {
        bool windowChanged = false;

        if(i%10==0)
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            new_consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left;
            new_consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top;

            if(new_consoleHeight!=consoleHeight || new_consoleWidth!=consoleWidth)
                windowChanged = true;

            consoleWidth = new_consoleWidth;
            consoleHeight = new_consoleHeight;
        }

        switch(ans)
        {
            case 1 :
                {
                    SemilogVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 2 :
                {
                    LinearVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 3 :
                {
                    LoglogVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 4 :
                {
                    SemilogVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight, true);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 5 :
                {
                    LinearVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight, true);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 6 :
                {
                    LoglogVisualizer(lim1, lim2, MainAudioQueue, consoleWidth, consoleHeight, true);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 7 :
                {
                    SpectralTuner(MainAudioQueue, consoleWidth, consoleHeight);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 8 :
                {
                    SpectralTuner(MainAudioQueue, consoleWidth, consoleHeight, true);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 9 :
                {
                    if(windowChanged)
                        system("cls");
                    AutoTuner(MainAudioQueue, consoleWidth, windowChanged);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            case 10:
                {
                    ChordGuesser(MainAudioQueue);
                    SDL_Delay(REFRESH_TIME);
                    break;
                }
            default: return 0;
        }

        char button_press = capture_button_press();
        if(button_press == 'x')
            break;
        else if(button_press == 'm')
        {
            system("cls");
            goto MAIN_MENU;
        }
    }

    /// Close audio devices
    SDL_CloseAudioDevice(PlayDevice);
    SDL_CloseAudioDevice(RecDevice);

    return 0;
}
