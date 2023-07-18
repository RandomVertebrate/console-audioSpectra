#include <iostream>
#include <math.h>
#include <complex>
#include <windows.h>
#include <SDL2/SDL.h>

#define RATE 44100                      /// Sample rate
#define CHUNK 64                        /// Buffer size
#define CHANNELS 1                      /// Mono audio
#define FORMAT AUDIO_S16SYS             /// Sample format: signed system-endian 16 bit integers
#define MAX_SAMPLE_VALUE 32767          /// Max sample value based on sample datatype

#define FFTLEN 65536                    /// Number of samples to perform FFT on. Must be power of 2.

typedef short sample;                   /// Datatype of samples. Also used to store frequency coefficients.
typedef std::complex<double> cmplx;     /// Complex number datatype for fft

float echoVolume;                       /// Anything recorded is immediately (-ish) played back at this volume.

/**
------------------------
----class AudioQueue----
------------------------
SDL doesn't seem to have internal recording and playback queues, so this.
Why audio queues are needed:
Audio recording/playback happens "in the background", i.e. on a different thread:
    - Whenever new audio data enters the sound card (from the microphone), the
      sound card will call a callback function and pass this data to it.
    - Whenever the sound card requires new data to send (to the speakers), it will
      call a callback function and expect to be provided with as much data as it
      requires.
These things happen at unpredictable times since they are tied to audio sample rates
etc. rather than the clock speed.
Reading and writing audio data from and to a queue helps prevent threading problems
such as skipping/repeating samples or getting more or less data than expected.
**/
class AudioQueue
{
    int len;                                                            /// Maximum length of queue
    sample *audio;                                                      /// Pointer to audio data array
    int inpos;                                                          /// Index in audio[] of back of queue
    int outpos;                                                         /// Index in audio[] of front of queue
  public:
    AudioQueue(int QueueLength = 10000)                                 /// Constructor. Takes maximum length.
    {
        len = QueueLength;
        audio = new sample[len];                                        /// Initializing audio data array.
        inpos = 0;                                                      /// Front and back both set to zero. Setting them 1 sample apart doesn't really make sense
        outpos = 0;                                                     /// because several samples will be pushed or popped at once.
    }
    ~AudioQueue()
    {
        delete[] audio;
    }
    bool data_available(int n_samples = 1)                              /// Check if the queue has n_samples of data in it.
    {
        if(inpos>=outpos)
            return (inpos-outpos)>=n_samples;
        else
            return (inpos+len-outpos)>=n_samples;
    }
    bool space_available(int n_samples = 1)                             /// Check if the queue has space for n_samples of new data.
    {
        if(inpos>=outpos)
            return (outpos+len-inpos)>n_samples;
        else
            return (outpos-inpos)>n_samples;
    }
    void push(sample* input, int n_samples, float volume=1)             /// Push n_samples of new data to the queue
    {
        if(!space_available(n_samples))
        {
            std::cout<<"\n\nAudio queue overflow\n\n";
            return;
        }
        for(int i=0; i<n_samples; i++)
            audio[(inpos+i)%len] = input[i]*volume;
        inpos=(inpos+n_samples)%len;
    }
    void pop(sample* output, int n_samples, float volume=1)             /// Pop n_samples of data from the queue
    {
        if(!data_available(n_samples))
        {
            std::cout<<"\n\nAudio queue underflow\n\n";
            return;
        }
        for(int i=0; i<n_samples; i++)
            output[i] = audio[(outpos+i)%len]*volume;
        outpos=(outpos+n_samples)%len;
    }
    void peek(sample* output, int n_samples, float volume=1)            /// Peek the n_samples that would be popped
    {
        if(!data_available(n_samples))
        {
            std::cout<<"\n\nAudio queue underflow\n\n";
            return;
        }
        for(int i=0; i<n_samples; i++)
            output[i] = audio[(outpos+i)%len]*volume;
    }
    void peekFreshData(sample* output, int n_samples, float volume=1)   /// Peek the freshest n_samples (for instantly reactive FFT)

    {
        if(!data_available(n_samples))
        {
            std::cout<<"\n\nAudio queue underflow\n\n";
            return;
        }
        for(int i=0; i<n_samples; i++)
            output[n_samples-i] = audio[(len+inpos-i)%len]*volume;
    }
};

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

void dftmag(sample* output, sample* input, int n)                       /// O(n^2) DFT. Not actually used.
{
    float* sinArr = new float[n];
    float* cosArr = new float[n];

    for(int i=0; i<n; i++)
    {
        sinArr[i] = sin(i*2*3.14159/n);
        cosArr[i] = cos(i*2*3.14159/n);
    }

    sample* outputcos = new sample[n];
    for(int i=0; i<n; i++)
    {
        output[i] = 0;
        outputcos[i] = 0;
    }

    for(int i=0; i<n; i++)
    {
        for(int j=0; j<n; j++)
        {
            output[i]+=(input[j]*sinArr[(i+1)*j%n]);
            outputcos[i]+=(input[j]*cosArr[(i+1)*j%n]);
        }
        output[i] = (sample)sqrt(output[i]*output[i]
                                 + outputcos[i]*outputcos[i]);
        if(output[i]<0)
        {
            std::cout<<"\n\nNEGATIVE SQRT RESULT\n\n";
            return;
        }
    }
    delete[] sinArr;
    delete[] cosArr;
    delete[] outputcos;
}

void fft(cmplx* output, cmplx* input, int n)                            /// Simplest FFT algorithm
{
    /*
    cout<<"\nFFT called on input ";
    for(int i=0; i<n; i++)
        cout<<input[i];
    */

    if(n==1)
    {
        output[0] = input[0];
        return;
    }

    cmplx* even = new cmplx[n/2];
    for(int i=0; i<n/2; i++)
        even[i] = input[2*i];
    cmplx* odd = new cmplx[n/2];
    for(int i=0; i<n/2; i++)
        odd[i] = input[2*i+1];

    cmplx* evenOut = new cmplx[n/2];
    fft(evenOut, even, n/2);
    cmplx* oddOut = new cmplx[n/2];
    fft(oddOut, odd, n/2);

    for(int i=0; i<n/2; i++)
    {
        cmplx t = exp(cmplx(0, -2*3.14159*i/n))*oddOut[i];
        output[i] = evenOut[i] + t;
        output[i+n/2] = evenOut[i] - t;
    }

    delete[] even;
    delete[] odd;
    delete[] evenOut;
    delete[] oddOut;

    return;
}

/**
----FindFrequencyContent()----
Takes pointer to an array of audio samples, performs FFT, and outputs magnitude of
complex coefficients.
i.e., it give amplitude but not phase of frequency components in given audio.
**/
void FindFrequencyContent(sample* output, sample* input, int n, float vScale = 0.005)
{
    cmplx* fftin = new cmplx[n];
    cmplx* fftout = new cmplx[n];
    for(int i=0; i<n; i++)                                              /// Convert input to complex
        fftin[i] = (cmplx)input[i];
    fft(fftout, fftin, n);                                              /// FFT
    for(int i=0; i<n; i++)                                              /// Convert output to real samples
    {
        double currentvalue = abs(fftout[i])*vScale;
        output[i] = (sample)(currentvalue>MAX_SAMPLE_VALUE ? MAX_SAMPLE_VALUE : currentvalue);
    }

    delete[] fftin;
    delete[] fftout;
}

void show_bargraph(int bars[], int n_bars, int height=50,               /// Histogram plotter
                   int hScale = 1, float vScale = 1, char symbol='|')
{
    char* Graph = new char[2*(n_bars+1)*(height+1)+1];                  /// String that will be printed
    int chnum = 0;                                                      /// Number of characters added to string Graph
    for(int i=height; i>=0; i--)                                        /// Iterating through rows (height is the number of rows)
    {
        for(int j=0; j<n_bars; j++)                                     /// Iterating through columns
            if(bars[j]*vScale>i)                                        /// Add symbols to string if (row, column) is below (bar value, column)
                for(int k=0; k<hScale; k++)
                    Graph[chnum++] = symbol;
            else                                                        /// Else add whitespaces
                for(int k=0; k<hScale; k++)
                    Graph[chnum++] = ' ';
        Graph[chnum++] = '\n';                                          /// Next row
    }

    for(int j=0; j<n_bars*hScale-1; j++)                                  /// Add extra line of symbols at the bottom
        Graph[chnum++] = symbol;

    Graph[chnum++] = '\0';                                              /// Null-terminate string

    std::cout<<Graph;                                                   /// Print to console

    delete[] Graph;
}

/**
----float mapLin2Log()----
Maps linear axis to log axis.
If graph A has a linearly scaled x-axis and graph B has a logarithmically scaled x-axis
then this function gives the x-coordinate in B corresponding to some x-coordinate in A.
**/
float mapLin2Log(float LinMin, float LinRange, float LogMin, float LogRange, float LinVal)
{
    return LogMin+(log(LinVal+1-LinMin)/log(LinRange+LinMin))*LogRange;
}

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

void startSemilogVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false,
                            int delayMicroseconds = 10, float graphScale = 0.0008)
{
    sample workingBuffer[FFTLEN];                                       /// Array to hold audio
    sample spectrum[FFTLEN];                                            /// Array to hold FFT coefficient magnitudes

    int numbars;                                                        /// Number of bars in the histogram. Will be set to console window width.
    int graphheight;                                                    /// Height of histogram in lines. Will be set to console window height.

    int bargraph[1000];                                                 /// The histogram

    int Freq0idx = (double)minfreq*(double)FFTLEN/(double)RATE;         /// Index in spectrum[] corresponding to minfreq
    int FreqLidx = (double)maxfreq*(double)FFTLEN/(double)RATE;         /// Index in spectrum[] corresponding to maxfreq

    CONSOLE_SCREEN_BUFFER_INFO csbi;                                    /// Console object to retrieve console window size for graph scaling

    for(int i=0; i<iterations; i++)
    {
        if(i%10==0)                                                     /// Every 10 iterations, get window size and update graph scaling
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            numbars = csbi.srWindow.Right - csbi.srWindow.Left;
            graphheight = csbi.srWindow.Bottom - csbi.srWindow.Top;
        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);            /// Get audio
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN);          /// Spectral analysis
        //dftmag(spectrum, workingBuffer, FFTLEN);

        for(int i=0; i<numbars; i++)                                    /// Initialize histogram to zeros
            bargraph[i]=0;

        /// Now mapping spectrum to histogram
        for(int i=Freq0idx; i<FreqLidx; i++)
        {
            int index = (int)mapLin2Log(Freq0idx, FreqLidx-Freq0idx, 0, numbars, i);
            /// For semilog scaling:
            /// The number of elements spectrum[i] that map to a certain bargraph[index] is
            /// roughly proportional to i.
            /// So, divide spectrum[i] by i before adding it to bargraph[index].
            bargraph[index]+=spectrum[i]/i;
        }
        //std::cout<<"\n";

        /// Now filling in the x-axis gaps left by the mapping.
        /// Using arithmetic mean for smoothing.
        for(int i=1; i<numbars-1; i++)
            if(bargraph[i]==0)
                bargraph[i]=(bargraph[i-1]+bargraph[i+1])/2;

        /// If adaptive find max value in bargraph[] and update graphScale to fit data on screen.
        if(adaptive)
        {
            int maxv = bargraph[0];
            for(int i=0; i<numbars; i++)
            {
                if(bargraph[i]>maxv)
                    maxv=bargraph[i];
            }
            graphScale = 1/(float)maxv;
        }

        system("cls");                                                  /// Clear screen in preparation for spectrum plotting. Bit ugly, computation wise.
        show_bargraph(bargraph, numbars, graphheight,
                      1, graphScale*graphheight, ':');

        SDL_Delay(delayMicroseconds);
    }
}

void startLinearVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false,
                           int delayMicroseconds = 10, float graphScale = 0.0008)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    int numbars;
    int graphheight;

    int bargraph[1000];

    int Freq0idx = (double)minfreq*(double)FFTLEN/(double)RATE;
    int FreqLidx = (double)maxfreq*(double)FFTLEN/(double)RATE;

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i=0; i<iterations; i++)
    {
        if(i%10==0)
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            numbars = csbi.srWindow.Right - csbi.srWindow.Left;
            graphheight = csbi.srWindow.Bottom - csbi.srWindow.Top;
        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN);
        //dftmag(spectrum, workingBuffer, FFTLEN);

        int bucketwidth = FFTLEN/numbars;

        for(int i=0; i<numbars; i++)
            bargraph[i]=0;

        for(int i=Freq0idx; i<FreqLidx; i++)
        {
            int index = (int)(numbars*(i-Freq0idx)/(FreqLidx-Freq0idx));    /// Linear mapping instead of logarithmic
            /// For linear scaling:
            /// The number of elements spectrum[i] that map to a certain bargraph[index] is
            /// fixed, and equal to bucketwidth.
            /// So, divide spectrum[i] by bucketwidth before adding it to bargraph[index].
            bargraph[index]+=spectrum[i]/bucketwidth;
        }
        //std::cout<<"\n";

        for(int i=1; i<numbars-1; i++)
            if(bargraph[i]==0)
                bargraph[i]=(bargraph[i-1]+bargraph[i+1])/2;

        if(adaptive)
        {
            int maxv = bargraph[0];
            for(int i=0; i<numbars; i++)
            {
                if(bargraph[i]>maxv)
                    maxv=bargraph[i];
            }
            graphScale = 1/(float)maxv;
        }

        system("cls");
        show_bargraph(bargraph, numbars, graphheight, 1, graphScale*graphheight, ':');
        SDL_Delay(delayMicroseconds);
    }
}

void startLoglogVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false,
                           int delayMicroseconds = 10, float graphScale = 0.0008)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    int numbars;
    int graphheight;

    int bargraph[1000];

    int Freq0idx = (double)minfreq*(double)FFTLEN/(double)RATE;
    int FreqLidx = (double)maxfreq*(double)FFTLEN/(double)RATE;

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i=0; i<iterations; i++)
    {
        if(i%10==0)
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            numbars = csbi.srWindow.Right - csbi.srWindow.Left;
            graphheight = csbi.srWindow.Bottom - csbi.srWindow.Top;
        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN);
        //dftmag(spectrum, workingBuffer, FFTLEN);

        for(int i=0; i<numbars; i++)
            bargraph[i]=0;

        for(int i=Freq0idx; i<FreqLidx; i++)
        {
            int index = (int)mapLin2Log(Freq0idx, FreqLidx-Freq0idx, 0, numbars, i);
            bargraph[index]+=spectrum[i]/i;
        }
        //std::cout<<"\n";

        /// Arithmetic-mean smoothing doesn't work well for log-log scaling.
        /// Gap filling is instead done by simply copying the bar on the right.
        for(int i=numbars-2; i>0; i--)
            if(bargraph[i]==0)
                bargraph[i]=bargraph[i+1];

        /// Log-scaling data (log base 1.01)
        for(int i=0; i<numbars; i++)
            bargraph[i]=log(bargraph[i])/log(1.01);

        if(adaptive)
        {
            int maxv = bargraph[0];
            for(int i=0; i<numbars; i++)
            {
                if(bargraph[i]>maxv)
                    maxv=bargraph[i];
            }
            graphScale = 1/(float)maxv;
        }

        system("cls");
        show_bargraph(bargraph, numbars, graphheight, 1, graphScale*graphheight, ':');
        SDL_Delay(delayMicroseconds);
    }
}

void startTuner(int iterations, bool adaptive = false, int delayMicroseconds = 10, float graphScale = 0.0008)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    int numbars = 0;
    int new_numbars = 0;
    int graphheight = 0;

    int bargraph[1000];
    float octave1index[1000];                                                   /// Will hold "fractional indices" in spectrum[] that map to each bar

    char pitchnames[1000] = "AA#BCC#DD#EFF#GG#";                                /// Will be updated with appropriate spacing as per window size.

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i=0; i<iterations; i++)
    {
        if(i%10==0)
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            new_numbars = csbi.srWindow.Right - csbi.srWindow.Left;
            graphheight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;         /// Minus 3 to leave room for pitch names scale

            /// If the window width has changed, then the following needs to be changed:
            /// 1. The frequency corresponding to each bar
            /// 2. The indices in spectrum[] that correspond to these frequencies
            /// 3. The pitch names string
            if(new_numbars!=numbars)
            {
                numbars = new_numbars;

                /// UPDATING FIRST-OCTAVE INDICES
                /// The entire x-axis of the histogram is to span one octave i.e. an interval of 2.
                /// Therefore, each bar index increment corresponds to an interval of 2^(1/numbars).
                /// The first frequency is A1 = 55Hz.
                /// So the ith frequency is 55Hz*2^(i/numbars).
                for(int i=0; i<numbars+1; i++)
                    octave1index[i] = 55*pow(2, (float)i/numbars)*FFTLEN/RATE;  /// "Fractional index" in spectrum[] corresponding to ith frequency.

                /// UPDATING PITCH NAMES STRING
                float bars_per_semitone = (float)(numbars)/(float)12;
                int chnum = 0;
                /// Adding pitch letter names
                pitchnames[chnum++]='A';
                while(chnum<round(bars_per_semitone*1.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='A';
                pitchnames[chnum++]='#';
                while(chnum<round(bars_per_semitone*2.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='B';
                while(chnum<round(bars_per_semitone*3.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='C';
                while(chnum<round(bars_per_semitone*4.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='C';
                pitchnames[chnum++]='#';
                while(chnum<round(bars_per_semitone*5.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='D';
                while(chnum<round(bars_per_semitone*6.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='D';
                pitchnames[chnum++]='#';
                while(chnum<round(bars_per_semitone*7.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='E';
                while(chnum<round(bars_per_semitone*8.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='F';
                while(chnum<round(bars_per_semitone*9.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='F';
                pitchnames[chnum++]='#';
                while(chnum<round(bars_per_semitone*10.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='G';
                while(chnum<round(bars_per_semitone*11.0)) pitchnames[chnum++]=' ';
                pitchnames[chnum++]='G';
                pitchnames[chnum++]='#';

                int newlinechar_pos = chnum;                                    /// Storing index of end of first line (letter names)
                pitchnames[chnum++]='\n';                                       /// And going to next line (row of pipes and dots)

                /// Adding row of pipes and dots
                for(int i=0; i<12; i++)
                {
                    pitchnames[chnum++]='|';
                    while((chnum-newlinechar_pos-1)<round(bars_per_semitone*(float)(i+1)))
                        pitchnames[chnum++]='.';
                }
                pitchnames[chnum++]='\n';
                pitchnames[chnum++]='\0';                                       /// Terminating String
            }
        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN);
        //dftmag(spectrum, workingBuffer, FFTLEN);

        for(int i=0; i<numbars; i++)
            bargraph[i]=0;

        /// PREPARING TUNER HISTOGRAM
        /// Iterating through log-scaled output indices and mapping them to linear input indices
        /// (instead of the other way round).
        /// So, an exponential mapping.
        for(int i=0; i<numbars-1; i++)
        {
            float index = octave1index[i];                                      /// "Fractional index" in spectrum[] corresponding to ith frequency.
            float nextindex = octave1index[i+1];                                /// "Fractional index" corresponding to (i+1)th frequency.
            /// OCTAVE WRAPPING
            /// To the frequency coefficient for any frequency F will be added:
            /// The frequency coefficients of all frequencies F*2^n for n=1..8
            /// (i.e., 8 octaves of the same-letter pitch)
            for(int j=0; j<8; j++)                                              /// Iterating through 8 octaves
            {
                //std::cout<<"\ni "<<i<<", index "<<index<<", next index "<<nextindex;
                /// Add everything in spectrum[] between current index and next index to current histogram bar.
                for(int k=round(index); k<round(nextindex); k++)                /// Fractional indices must be rounded for use
                    /// There are (nextindex-index) additions for a particular bar, so divide each addition by this.
                    bargraph[i]+=0.02*spectrum[k]/(nextindex-index);

                /// Frequency doubles with octave increment, so index in linearly spaced data also doubles.
                index*=2;
                nextindex*=2;
            }
        }
        //std::cout<<"\n";

        if(adaptive)
        {
            int maxv = bargraph[0];
            for(int i=0; i<numbars; i++)
            {
                if(bargraph[i]>maxv)
                    maxv=bargraph[i];
            }
            graphScale = 1/(float)maxv;
        }

        system("cls");
        std::cout<<pitchnames;
        show_bargraph(bargraph, numbars, graphheight, 1, graphScale*graphheight, '=');

        SDL_Delay(delayMicroseconds);
    }
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
    std::cout<<"\nVisualizer scaling options:"
        <<"\n1. Fixed semilog"
        <<"\n2. Fixed linear"
        <<"\n3. Fixed log-log"
        <<"\n4. Adaptive semilog"
        <<"\n5. Adaptive linear"
        <<"\n6. Adaptive log-log"
        <<"\n7. Octave-wrapped semilog (guitar tuner)"
        <<"\n8. Adaptive guitar tuner"
        <<"\nYour choice: ";
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
        case 1: startSemilogVisualizer(lim1, lim2, 1000); break;
        case 2: startLinearVisualizer(lim1, lim2, 1000); break;
        case 3: startLoglogVisualizer(lim1, lim2, 1000); break;
        case 4: startSemilogVisualizer(lim1, lim2, 1000, true); break;
        case 5: startLinearVisualizer(lim1, lim2, 1000, true); break;
        case 6: startLoglogVisualizer(lim1, lim2, 1000, true); break;
        case 7: startTuner(1000); break;
        case 8: startTuner(1000, true); break;
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
