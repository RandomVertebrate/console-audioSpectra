#include "audioDSP.h"

AudioQueue::AudioQueue(int QueueLength)                                     /// Constructor. Takes maximum length.
{
    len = QueueLength;
    audio = new sample[len];                                                /// Initializing audio data array.
    inpos = 0;                                                              /// Front and back both set to zero. Setting them 1 sample apart doesn't really make sense
    outpos = 0;                                                             /// because several samples will be pushed or popped at once.
}
AudioQueue::~AudioQueue()
{
    delete[] audio;
}
bool AudioQueue::data_available(int n_samples)                              /// Check if the queue has n_samples of data in it.
{
    if(inpos>=outpos)
        return (inpos-outpos)>=n_samples;
    else
        return (inpos+len-outpos)>=n_samples;
}
bool AudioQueue::space_available(int n_samples)                             /// Check if the queue has space for n_samples of new data.
{
    if(inpos>=outpos)
        return (outpos+len-inpos)>n_samples;
    else
        return (outpos-inpos)>n_samples;
}
void AudioQueue::push(sample* input, int n_samples, float volume)           /// Push n_samples of new data to the queue
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
void AudioQueue::pop(sample* output, int n_samples, float volume)           /// Pop n_samples of data from the queue
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
void AudioQueue::peek(sample* output, int n_samples, float volume)          /// Peek the n_samples that would be popped
{
    if(!data_available(n_samples))
    {
        std::cout<<"\n\nAudio queue underflow\n\n";
        return;
    }
    for(int i=0; i<n_samples; i++)
        output[i] = audio[(outpos+i)%len]*volume;
}
void AudioQueue::peekFreshData(sample* output, int n_samples,               /// Peek the freshest n_samples (for instantly reactive FFT)
                               float volume)
{
    if(!data_available(n_samples))
    {
        std::cout<<"\n\nAudio queue underflow\n\n";
        return;
    }
    for(int i=0; i<n_samples; i++)
        output[n_samples-i] = audio[(len+inpos-i)%len]*volume;
}

void dftmag(sample* output, sample* input, int n)                           /// O(n^2) DFT. Not actually used.
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

void fft(cmplx* output, cmplx* input, int n)                                /// Simplest FFT algorithm
{
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
void FindFrequencyContent(sample* output, sample* input, int n, float vScale)
{
    cmplx* fftin = new cmplx[n];
    cmplx* fftout = new cmplx[n];
    for(int i=0; i<n; i++)                                                  /// Convert input to complex
        fftin[i] = (cmplx)input[i];
    fft(fftout, fftin, n);                                                  /// FFT
    for(int i=0; i<n; i++)                                                  /// Convert output to real samples
    {
        double currentvalue = abs(fftout[i])*vScale;
        output[i] = (sample)(currentvalue>MAX_SAMPLE_VALUE ? MAX_SAMPLE_VALUE : currentvalue);
    }

    delete[] fftin;
    delete[] fftout;
}


