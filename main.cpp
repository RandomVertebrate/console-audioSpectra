#include <iostream>
#include <math.h>
#include <complex>
#include <stdlib.h>
#include <random>
#include <windows.h>
#include <SDL2/SDL.h>

#define RATE 44100
#define CHUNK 64
#define CHANNELS 1
#define FORMAT AUDIO_S16SYS

#define FFTLEN 65536

typedef short sample;
typedef std::complex<double> cmplx;

float echoVolume;

class AudioQueue
{
    int len;
    sample *audio;
    int inpos;
    int outpos;
  public:
    AudioQueue(int QueueLength = 10000)
    {
        len = QueueLength;
        audio = new sample[len];
        inpos = 0;
        outpos = 0;
    }
    bool data_available(int n_samples = 1)
    {
        if(inpos>=outpos)
            return (inpos-outpos)>=n_samples;
        else
            return (inpos+len-outpos)>=n_samples;
    }
    bool space_available(int n_samples = 1)
    {
        if(inpos>=outpos)
            return (outpos+len-inpos)>n_samples;
        else
            return (outpos-inpos)>n_samples;
    }
    void push(sample* input, int n_samples, float volume=1)
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
    void pop(sample* output, int n_samples, float volume=1)
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
    void peek(sample* output, int n_samples, float volume=1)
    {
        if(!data_available(n_samples))
        {
            std::cout<<"\n\nAudio queue underflow\n\n";
            return;
        }
        for(int i=0; i<n_samples; i++)
            output[i] = audio[(outpos+i)%len]*volume;
    }
    void peekFreshData(sample* output, int n_samples, float volume=1)
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

AudioQueue MainAudioQueue(10000000);

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

void dftmag(sample* output, sample* input, int n)
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
        output[i] = (sample)sqrt(output[i]*output[i] + outputcos[i]*outputcos[i]);
        if(output[i]<0)
        {
            std::cout<<"\n\nNEGATIVE SQRT RESULT\n\n";
            return;
        }
    }
}

void show_bargraph(int bars[], int n_bars, int maxval=50, int hScale = 1, float vScale = 1, char symbol='|')
{
    for(int i=maxval; i>=0; i--)
    {
        for(int j=0; j<n_bars; j++)
            if(bars[j]*vScale>i)
                for(int k=0; k<hScale; k++)
                    std::cout<<symbol;
            else
                for(int k=0; k<hScale; k++)
                    std::cout<<" ";
        std::cout<<"\n";
    }

    for(int j=0; j<n_bars; j++)
        for(int k=0; k<hScale; k++)
            std::cout<<symbol;

}

void fft(cmplx* output, cmplx* input, int n)
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

    return;
}

void FindFrequencyContent(sample* output, sample* input, int n)
{
    cmplx* fftin = new cmplx[n];
    cmplx* fftout = new cmplx[n];
    for(int i=0; i<n; i++)
        fftin[i] = (cmplx)input[i];
    fft(fftout, fftin, n);
    for(int i=0; i<n; i++)
        output[i] = (sample)(abs(fftout[i])/200);
}

float maplog2lin(float logmin, float logrange, float linmin, float linrange, float logval)
{
    return linmin+(log(logval+1-logmin)/log(logrange+logmin))*linrange;
}

void startSemilogVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false, int delayMicroseconds = 10, float graphScale = 0.0008)
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
            int index = (int)maplog2lin(Freq0idx, FreqLidx-Freq0idx, 0, numbars, i);
            bargraph[index]+=spectrum[i]/i;
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

void startLinearVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false, int delayMicroseconds = 10, float graphScale = 0.0008)
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
            int index = (int)(numbars*(i-Freq0idx)/(FreqLidx-Freq0idx));
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

void startLoglogVisualizer(int minfreq, int maxfreq, int iterations, bool adaptive = false, int delayMicroseconds = 10, float graphScale = 0.0008)
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
            int index = (int)maplog2lin(Freq0idx, FreqLidx-Freq0idx, 0, numbars, i);
            bargraph[index]+=spectrum[i]/i;
        }
        //std::cout<<"\n";

        for(int i=numbars-2; i>0; i--)
            if(bargraph[i]==0)
                bargraph[i]=bargraph[i+1];

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

    int numbars;
    int graphheight;

    int bargraph[1000];

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i=0; i<iterations; i++)
    {
        if(i%10==0)
        {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            numbars = csbi.srWindow.Right - csbi.srWindow.Left;
            graphheight = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
        }

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN);
        //dftmag(spectrum, workingBuffer, FFTLEN);

        for(int i=0; i<numbars; i++)
            bargraph[i]=0;

        for(int i=0; i<numbars-1; i++)
        {
            int index = 55*pow(2, (float)i/numbars)*FFTLEN/RATE;
            int nextindex = 55*pow(2, (float)(i+1)/numbars)*FFTLEN/RATE;
            for(int j=0; j<8; j++)
            {
                //std::cout<<"\ni "<<i<<", index "<<index<<", next index "<<nextindex;
                for(int k=index; k<nextindex; k++)
                    bargraph[i]+=0.02*spectrum[k]/(1+nextindex-index);
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
        show_bargraph(bargraph, numbars, graphheight, 1, graphScale*graphheight, '=');

        int semitonespaces = round((float)(numbars-12)/(float)12);
        std::cout<<"\n";
        std::cout<<"A"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"A#"; for(int i=0; i<semitonespaces-1; i++) std::cout<<" ";
        std::cout<<"B"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"C"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"C#"; for(int i=0; i<semitonespaces-1; i++) std::cout<<" ";
        std::cout<<"D"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"D#"; for(int i=0; i<semitonespaces-1; i++) std::cout<<" ";
        std::cout<<"E"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"F"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"F#"; for(int i=0; i<semitonespaces-1; i++) std::cout<<" ";
        std::cout<<"G"; for(int i=0; i<semitonespaces; i++) std::cout<<" ";
        std::cout<<"G#";

        SDL_Delay(delayMicroseconds);
    }
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec RecAudiospec, PlayAudiospec;
    RecAudiospec.freq = RATE;
    RecAudiospec.format = AUDIO_S16SYS;
    RecAudiospec.samples = CHUNK;
    RecAudiospec.callback = RecCallback;
    PlayAudiospec = RecAudiospec;
    PlayAudiospec.callback = PlayCallback;

    SDL_AudioDeviceID PlayDevice = SDL_OpenAudioDevice(NULL, 0, &PlayAudiospec, NULL, 0);
    SDL_AudioDeviceID RecDevice = SDL_OpenAudioDevice(NULL, 1, &RecAudiospec, NULL, 0);

    if(PlayDevice <= 0)
        std::cerr<<"Playback device not opened: "<<SDL_GetError()<<"\n";
    if(RecDevice <= 0)
    {
        std::cerr<<"Recording device not opened: "<<SDL_GetError()<<"\n";
        return -1;
    }

    SDL_PauseAudioDevice(RecDevice, 0);
    SDL_Delay(1500);
    SDL_PauseAudioDevice(PlayDevice, 0);

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

    return 0;
}
