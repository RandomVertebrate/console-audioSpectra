#include <iostream>
#include <math.h>
#include <complex>

#define RATE 44100                      /// Sample rate
#define CHUNK 64                        /// Buffer size
#define CHANNELS 1                      /// Mono audio
#define FORMAT AUDIO_S16SYS             /// Sample format: signed system-endian 16 bit integers
#define MAX_SAMPLE_VALUE 32767          /// Max sample value based on sample datatype

#define FFTLEN 65536                    /// Number of samples to perform FFT on. Must be power of 2.

typedef short sample;                   /// Datatype of samples. Also used to store frequency coefficients.
typedef std::complex<double> cmplx;     /// Complex number datatype for fft

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
    AudioQueue(int QueueLength = 10000);                                /// Constructor. Takes maximum length.
    ~AudioQueue();
    bool data_available(int n_samples = 1);                             /// Check if the queue has n_samples of data in it.
    bool space_available(int n_samples = 1);                            /// Check if the queue has space for n_samples of new data.
    void push(sample* input, int n_samples, float volume=1);            /// Push n_samples of new data to the queue
    void pop(sample* output, int n_samples, float volume=1);            /// Pop n_samples of data from the queue
    void peek(sample* output, int n_samples, float volume=1);           /// Peek the n_samples that would be popped
    void peekFreshData(sample* output, int n_samples, float volume=1);  /// Peek the freshest n_samples (for instantly reactive FFT)
};

void dftmag(sample* output, sample* input, int n);                      /// O(n^2) DFT. Not actually used.

void fft(cmplx* output, cmplx* input, int n);                           /// Simplest FFT algorithm

/**
----FindFrequencyContent()----
Takes pointer to an array of audio samples, performs FFT, and outputs magnitude of
complex coefficients.
i.e., it give amplitude but not phase of frequency components in given audio.
**/
void FindFrequencyContent(sample* output, sample* input, int n, float vScale = 0.005);


