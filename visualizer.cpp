#include "visualizer.h"

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

void startSemilogVisualizer(int minfreq, int maxfreq, AudioQueue &MainAudioQueue, int iterations, bool adaptive,
                            int delayMicroseconds, float graphScale)
{
    sample workingBuffer[FFTLEN];                                       /// Array to hold audio
    sample spectrum[FFTLEN];                                            /// Array to hold FFT coefficient magnitudes

    int numbars;                                                        /// Number of bars in the histogram. Will be set to console window width.
    int graphheight;                                                    /// Height of histogram in lines. Will be set to console window height.

    int bargraph[1000];                                                 /// The histogram

    int Freq0idx = freq2index(minfreq);                                 /// Index in spectrum[] corresponding to minfreq
    int FreqLidx = freq2index(maxfreq);                                 /// Index in spectrum[] corresponding to maxfreq

    CONSOLE_SCREEN_BUFFER_INFO csbi;                                    /// Console object to retrieve console window size for graph scaling

    for(int i_m=0; i_m<iterations; i_m++)
    {
        if(i_m%10==0)                                                   /// Every 10 iterations, get window size and update graph scaling
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

void startLinearVisualizer(int minfreq, int maxfreq,  AudioQueue &MainAudioQueue, int iterations, bool adaptive,
                           int delayMicroseconds, float graphScale)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    int numbars;
    int graphheight;

    int bargraph[1000];

    int Freq0idx = freq2index(minfreq);
    int FreqLidx = freq2index(maxfreq);

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i_m=0; i_m<iterations; i_m++)
    {
        if(i_m%10==0)
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

void startLoglogVisualizer(int minfreq, int maxfreq,  AudioQueue &MainAudioQueue, int iterations, bool adaptive,
                           int delayMicroseconds, float graphScale)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    int numbars;
    int graphheight;

    int bargraph[1000];

    int Freq0idx = freq2index(minfreq);
    int FreqLidx = freq2index(maxfreq);

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    for(int i_m=0; i_m<iterations; i_m++)
    {
        if(i_m%10==0)
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

void startTuner( AudioQueue &MainAudioQueue, int iterations, bool adaptive, int delayMicroseconds,
                           float graphScale )
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

    for(int i_m=0; i_m<iterations; i_m++)
    {
        if(i_m%10==0)
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

void startAutoTuner(AudioQueue &MainAudioQueue, int iterations, int delayMicroseconds, int span_semitones)
{
    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    char needle[1000];                                                  /// For tuner needle, e.g. "------------|------------"
    char notenames[1000];                                               /// For note names, e.g.   " A    A#   B    C    C#  "

    int window_width = 0;

    for(int i_m=0; i_m<iterations; i_m++)                               /// Main loop
    {
        if(i_m%10==0)                                                   /// Every 10 iterations, check if window width has changed
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

        int num_spikes = 5;                                            /// Number of fft spikes to consider for pitch deduction
        int SpikeLocs[100];                                             /// Array to store indices in spectrum[] of fft spikes
        float SpikeFreqs[100];                                          /// Array to store frequencies corresponding to spikes

        Find_n_Largest(SpikeLocs, spectrum, num_spikes, FFTLEN/2);      /// Find spikes

        for(int i=0; i<num_spikes; i++)                                 /// Find spike frequencies (assumed to be harmonics)
            SpikeFreqs[i] = index2freq(SpikeLocs[i]);

        float pitch = approx_hcf(SpikeFreqs, num_spikes, 5, 50);        /// Find pitch as approximate HCF of spike frequencies

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

void startChordSpeller(AudioQueue &MainAudioQueue, int iterations, int delayMicroseconds, int holdMicroseconds, int max_notes)
{
    system("cls");

    sample workingBuffer[FFTLEN];
    sample spectrum[FFTLEN];

    const float quartertone = pow(2.0, 1.0/24.0);

    const int num_spikes = 20;                                          /// Number of fft spikes to consider
    int SpikeLocs[100];                                                 /// Array to store indices in spectrum[] of fft spikes
    float SpikeFreqs[100];                                              /// Array to store frequencies corresponding to spikes

    float noteFreqs[100];                                               /// Array to store distinct peak frequencies
    int notes_found;                                                    /// Number of distinct peaks found

    for(int i_m=0; i_m<iterations; i_m++)
    {
        notes_found = 0;

        MainAudioQueue.peekFreshData(workingBuffer, FFTLEN);
        FindFrequencyContent(spectrum, workingBuffer, FFTLEN, 0.0001);

        Find_n_Largest(SpikeLocs, spectrum,
                       num_spikes, FFTLEN/2, false);                    /// Find spikes

        for(int i=0; i<num_spikes; i++)                                 /// Find spike frequencies
            SpikeFreqs[i] = index2freq(SpikeLocs[i]);

        noteFreqs[notes_found++] = SpikeFreqs[0];

        /// Find unique spike frequencies and store in noteFreqs[].
        /// SpikeFreqs[] is in decreasing order of spike intensity, so the tallest spikes will be added first.
        for(int i=1; i<num_spikes; i++)                                 /// For each frequency spike
        {
            bool uniq = true;                                           /// Assume unique by default
            for(int j=0; j<notes_found; j++)                            /// Look at each unique note already found,
            {
                float separation = (SpikeFreqs[i]>noteFreqs[j] ?        /// calculate the separation ratio (interval),
                                    SpikeFreqs[i]/noteFreqs[j] : noteFreqs[j]/SpikeFreqs[i]);
                if(separation<quartertone)                              /// and check that it is greater at least than a quarter tone
                {
                    uniq = false;                                       /// If separation less than a quarter tone, spike is non-unique
                    break;
                }
            }

            /// Add unique frequencies to noteFreqs until max_notes unique frequencies found OR all spikes checked
            if(notes_found>=max_notes)
                break;
            else if(uniq)
                noteFreqs[notes_found++] = SpikeFreqs[i];
        }

        /// Sort notes found in increasing order of frequency, so "chord root" appears first.
        for(int i=0; i<notes_found; i++)
            for(int j=0; j<notes_found-1; j++)
                if(noteFreqs[j] > noteFreqs[j+1])
                {
                    float tmp = noteFreqs[j];
                    noteFreqs[j] = noteFreqs[j+1];
                    noteFreqs[j+1] = tmp;
                }

        /// Now preparing and note name string
        char notenames[100];
        int chnum = 0;
        for(int i=0; i<notes_found; i++)
        {
            chnum += pitchName(notenames+chnum, pitchNumber(noteFreqs[i]));
            notenames[chnum++] = ' ';
        }
        notenames[chnum++] = '\0';

        /// Only display pitches if spectrum is peaky (chord has probably been played)
        double fft_max = spectrum[0];
        double fft_mean = spectrum[0];
        for(int i=1; i<FFTLEN; i++)
        {
            fft_mean += (double)spectrum[i]/(double)FFTLEN;
            if(spectrum[i]>fft_max)
                fft_max = spectrum[i];
        }
        double peakiness = fft_max/fft_mean;

        /// Display pitches, and delay
        if(peakiness>1100)
        {
            std::cout<<"\rPitches Identified: "<<notenames<<"             ";
            SDL_Delay(holdMicroseconds);
        }
        else
            SDL_Delay(delayMicroseconds);

    }
}
