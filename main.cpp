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

    for(int j=0; j<n_bars*hScale-1; j++)                                /// Add extra line of symbols at the bottom
        Graph[chnum++] = symbol;

    Graph[chnum++] = '\0';                                              /// Null-terminate string

    std::cout<<Graph;                                                   /// Print to console

    delete[] Graph;
}

float index2freq(int index)
{
    return 2*(float)index*(float)RATE/(float)FFTLEN;
}

float freq2index(float freq)
{
    return 0.5*freq*(float)FFTLEN/(float)RATE;
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
----float approx_hcf()----
Finds approximate HCF of numbers,
e.g. finds a fundamental frequency given an arbitrary subset of its harmonics. Must be
approximate because input will be real measured data, so not exact. Returns 0 if no HCF
is found (input is float, so 1 is not always a factor).
**/
float approx_hcf(float inputs[], int num_inputs, int max_iter = 5, int accuracy_threshold = 10)
{
    /// HCF of one number is itself
    if(num_inputs<=1)
        return inputs[0];

    /// Recursive call: if more than 2 inputs, return hcf(first input, hcf(other inputs))
    if(num_inputs>2)
    {
        float newinputs[2];
        newinputs[0] = inputs[0];
        newinputs[1] = approx_hcf(inputs+1, num_inputs-1);
        float ans = approx_hcf(newinputs, 2);
        return ans;
    }

    /// BASE CASE: num_inputs = 2

    /// Now using continued fractions to find a simple-integer approximation to the ratio inputs[0]/inputs[1]
    /// First, make sure the first input is bigger than the second, so that numerator > denominator
    if(inputs[0] < inputs[1])
    {
        float tmp = inputs[0];
        inputs[0] = inputs[1];
        inputs[1] = tmp;
    }
    /// Now setting up for continued fractions (iteration zero)
    float Ratio = inputs[0]/inputs[1];                                  /// Actual ratio. Greater than 1.
    int* IntegerParts = new int[max_iter];                              /// Array for continued fraction integer parts
    float fracpart = Ratio;                                             /// Fractional part (equals Ratio for iteration zero)
    bool accuracy_threshold_reached = false;                            /// Termination flag
    int int_sum = 0;                                                    /// Sum of integer parts used as measure of accuracy for termination
    int n;                                                              /// Termination point of continued fraction (incremented in loop).
    /// Calculating integer parts...
    for(n=0; n<max_iter; n++)                                           /// Limit on iterations ensures simple integer ratio (or nothing)
    {
        IntegerParts[n] = (int)fracpart;
        int_sum += IntegerParts[n];
        fracpart = 1.0/(fracpart - IntegerParts[n]);

        if(int_sum > accuracy_threshold)
        {
            accuracy_threshold_reached = true;
            break;
        }
    }

    /// Now finding the simple integer ratio that corresponds to the integer parts found
    int numerator = 1;
    int denominator = IntegerParts[n-1];
    for(int i=n-1; i>0; i--)
    {
        int tmp = denominator;
        denominator = denominator*IntegerParts[i-1] + numerator;
        numerator = tmp;
    }

    delete[] IntegerParts;

    /// If simple integer ratio not found return 0
    if(!accuracy_threshold_reached)
        return 0;

    /// If simple integer ratio successfully found, then the HCF is:
    /// the bigger input divided by the numerator (which is bigger than the denominator)
    /// or
    /// the smaller input divided by the denominator(which is smaller than the denominator)
    /// Now returning the geometric mean of these two possibilities.
    return sqrt((inputs[0]/(float)numerator)*((inputs[1]/(float)denominator)));
}

/**
----Find_n_Largest()----
Finds indices of n_out largest samples in input array.
**/
void Find_n_Largest(int* output, sample* input, int n_out, int n_in, float MinRatio = 1)
{
    /// Initialize output to 0 (i.e., index of first input)
    for(int i=0; i<n_out; i++)
        output[i] = 0;

    for(int i=0; i<n_in; i=(float)i*MinRatio+1)                         /// Iterate through input
    {
        for(int j=0; j<n_out; j++)                                      /// For each input, iterate through outputs
            if(input[i]>input[output[j]]*1.01)                          /// and check if it is bigger than any output
                {
                    for(int k=n_out-1; k>0; k--)                        /// If bigger than the jth output,
                        output[k] = output[k-1];                        /// Shift all outputs to the right from j onwards
                    output[j] = i;                                      /// and insert the index of the input that was bigger
                    break;
                }
    }
}

/**
----int pitchNumber()----
Given a frequency freq, this function finds and returns the closest pitch number,
which is an integer between 1 and 12 where 1 refers to A and 12 to G#.
The number of cents between the input frequency and the 'correct' pitch is written
to centsSharp
**/
int pitchNumber(float freq, float *centsSharp = nullptr)
{
    const double semitone = pow(2.0, 1.0/12.0);
    const double cent = pow(2.0, 1.0/1200.0);

    /// First, octave shift input frequency to within 440Hz and 880Hz (A4 and A5)
    while(freq<440)
        freq*=2;
    while(freq>880)
        freq/=2;

    /// "Log base semitone" of ratio of frequency to A4
    /// i.e., Number of semitones between A4 and freq
    int pitch_num = round(log(freq/440.0)/log(semitone));

    /// Enforcing min and max values of pitch_num
    if(pitch_num>11)
        pitch_num = 11;
    if(pitch_num<0)
        pitch_num = 0;

    /// "Log base cent" of ratio of freq to 'correct' (i.e. A440 12TET) pitch
    if(centsSharp != nullptr)
        *centsSharp = log(freq/(440.0*pow(semitone, (double)(pitch_num))))/log(cent);

    return pitch_num+1;                                                 /// Plus 1 so that 1 corresponds to A (0 = error).
}

/**
----int pitchName()----
Given a pitch number from 1 to 12 where 1 refers to A and 12 to G#
This function writes the pitch name to char* name and returns the length of the name.
**/
int pitchName(char* name, int pitch_num)
{
    switch(pitch_num)
    {
        case 1 : name[0] = 'A'; return 1;
        case 2 : name[0] = 'A'; name[1] = '#'; return 2;
        case 3 : name[0] = 'B'; return 1;
        case 4 : name[0] = 'C'; return 1;
        case 5 : name[0] = 'C'; name[1] = '#'; return 2;
        case 6 : name[0] = 'D'; return 1;
        case 7 : name[0] = 'D'; name[1] = '#'; return 2;
        case 8 : name[0] = 'E'; return 1;
        case 9 : name[0] = 'F'; return 1;
        case 10: name[0] = 'F'; name[1] = '#'; return 2;
        case 11: name[0] = 'G'; return 1;
        case 12: name[0] = 'G'; name[1] = '#'; return 2;
        default: std::cout<<"\n\nInvalid pitch number "<<pitch_num<<" received\n\n"; return 0;
    }
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

    int Freq0idx = freq2index(minfreq);                                 /// Index in spectrum[] corresponding to minfreq
    int FreqLidx = freq2index(maxfreq);                                 /// Index in spectrum[] corresponding to maxfreq

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

    int Freq0idx = freq2index(minfreq);
    int FreqLidx = freq2index(maxfreq);

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

    int Freq0idx = freq2index(minfreq);
    int FreqLidx = freq2index(maxfreq);

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
                    octave1index[i] = freq2index(55.0*pow(2,(float)i/numbars)); /// "Fractional index" in spectrum[] corresponding to ith frequency.

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

void startAutoTuner(int iterations, int delayMicroseconds = 10, int span_semitones = 5)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    char needle[1000];                                                  /// For tuner needle, e.g. "------------|------------"
    char notenames[1000];                                               /// For note names, e.g.   " A    A#   B    C    C#  "

    int window_width = 0;

    for(int i=0; i<iterations; i++)                                     /// Main loop
    {
        if(i%10==0)                                                     /// Every 10 iterations, check if window width has changed
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            int new_window_width = csbi.srWindow.Right - csbi.srWindow.Left;

            /// If window width has changed, update window width and needle
            if(new_window_width != window_width)
            {
                window_width = new_window_width;

                /**
                Sample needle:

                -----------------------------|-----------------------------
                                             |

                **/

                /// Preparing needle
                int chnum = 0;
                /// Add dashes in first half of first line
                while(chnum<window_width/2)
                    needle[chnum++] = '-';
                /// Add pipe
                needle[chnum++] = '|';
                /// Add dashes in second half of first line
                while(chnum<window_width)
                    needle[chnum++] = '-';
                /// Go to second line
                needle[chnum++] = '\n';
                /// Add whitespaces in first half of second line
                while(chnum<3*window_width/2+1)
                    needle[chnum++] = ' ';
                /// Add pipe
                needle[chnum++] = '|';
                /// whitespaces in second half of second line
                while(chnum<2*window_width+1)
                    needle[chnum++] = ' ';
                /// Go to next line for printing notenames
                needle[chnum++] = '\n';
                /// Terminate string
                needle[chnum++] = '\0';
                /// Clear screen to get rid of previously printed needle
                system("cls");
                /// Print needle
                std::cout<<needle;
            }

        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN, 0.00005);

        int num_spikes = 50;                                            /// Number of fft spikes to consider for pitch deduction
        int SpikeLocs[100];                                             /// Array to store indices in spectrum[] of fft spikes
        float SpikeFreqs[100];                                          /// Array to store frequencies corresponding to spikes

        Find_n_Largest(SpikeLocs, spectrum, num_spikes, FFTLEN/2);      /// Find spikes

        for(int i=0; i<num_spikes; i++)                                 /// Find spike frequencies (assumed to be harmonics)
            SpikeFreqs[i] = index2freq(SpikeLocs[i]);

        float pitch = approx_hcf(SpikeFreqs, num_spikes, 10, 4);        /// Find pitch as approximate HCF of spike frequencies

        if(pitch)                                                       /// If pitch found, update notenames and print
        {
            for(int i=0; i<window_width; i++)                           /// First initialize notenames to all whitespace
                notenames[i] = ' ';

            /// Find pitch number (1 = A, 2 = A# etc.) and how many cents sharp or flat (centsOff<0 means flat)
            float centsOff;
            int pitch_num = pitchNumber(pitch, &centsOff);

            /// Find appropriate location for pitch letter name based on centsOff
            /// (centsOff = 0 means "In Tune", location exactly in the  middle of the window)
            int loc_pitch = window_width/2 - centsOff*window_width/(span_semitones*100);

            /// Write letter name corresponding to current pitch to appropriate location
            pitchName(notenames+loc_pitch, pitch_num);

            /// Write letter names of as many lower pitches as will fit on screen
            int loc_prev_pitch = loc_pitch - window_width/span_semitones;
            for(int i=0; loc_prev_pitch>0; i++)
            {
                pitchName(notenames+loc_prev_pitch, (22-i+pitch_num)%12+1);
                loc_prev_pitch -= window_width/span_semitones;
            }

            /// Write letter names of as many higher pitches as will fit on screen
            int loc_next_pitch = loc_pitch + window_width/span_semitones;
            for(int i=0; loc_next_pitch<window_width; i++)
            {
                pitchName(notenames+loc_next_pitch, (pitch_num+i)%12+1);
                loc_next_pitch += window_width/span_semitones;
            }

            notenames[window_width] = '\0';                             /// Terminate string

            std::cout<<'\r'<<notenames;                                 /// Go to beginning of line and print (overwrite) notenames
        }

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
    std::cout<<"\nVisualizer Options\n------------------\n"
        <<"\nScaled Spectrum:\n"
        <<"\n1. Fixed semilog"
        <<"\n2. Fixed linear"
        <<"\n3. Fixed log-log"
        <<"\n4. Adaptive semilog"
        <<"\n5. Adaptive linear"
        <<"\n6. Adaptive log-log"
        <<"\n7. Octave-wrapped semilog (guitar tuner)"
        <<"\n8. Adaptive guitar tuner"
        <<"\n\nOther:\n"
        <<"\n9. Pitch recognition (auto tuner)"
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
        case 1: startSemilogVisualizer(lim1, lim2, 1000); break;
        case 2: startLinearVisualizer(lim1, lim2, 1000); break;
        case 3: startLoglogVisualizer(lim1, lim2, 1000); break;
        case 4: startSemilogVisualizer(lim1, lim2, 1000, true); break;
        case 5: startLinearVisualizer(lim1, lim2, 1000, true); break;
        case 6: startLoglogVisualizer(lim1, lim2, 1000, true); break;
        case 7: startTuner(1000); break;
        case 8: startTuner(1000, true); break;
        case 9: startAutoTuner(1000); break;
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
