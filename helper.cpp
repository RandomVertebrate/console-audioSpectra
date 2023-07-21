#include "helper.h"

void show_bargraph(int bars[], int n_bars, int height,                  /// Histogram plotter
                   int hScale, float vScale, char symbol)
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
float approx_hcf(float inputs[], int num_inputs, int max_iter, int accuracy_threshold)
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
    int int_sum = 0;                                                    /// Sum of increasing multiples of integer parts used as ad-hoc measure of accuracy
    int n;                                                              /// Termination point of continued fraction (incremented in loop).
    /// Calculating integer parts...
    for(n=0; n<max_iter; n++)                                           /// Limit on iterations ensures simple integer ratio (or nothing)
    {
        IntegerParts[n] = (int)fracpart;
        int_sum += n*IntegerParts[n];
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
    /// the smaller input divided by the denominator (which is smaller than the numerator)
    /// Now returning the geometric mean of these two possibilities.
    return sqrt((inputs[0]/(float)numerator)*((inputs[1]/(float)denominator)));
}

/**
----Find_n_Largest()----
Finds indices of n_out largest samples in input array.

If ignore_clumped is true, then only the largest of any clump of consecutive spikes is
kept.
**/
void Find_n_Largest(int* output, sample* input, int n_out, int n_in, bool ignore_clumped)
{
    /// First, find the position of the smallest element in the input array
    sample min_pos = 0;
    for(int i=0; i<n_in; i++)
        if(input[i]<input[min_pos])
            min_pos = i;
    /// And set every element of the output to the index of this minimum element
    for(int i=0; i<n_out; i++)
        output[i] = min_pos;

    /**
    Now, iterate through inputs and for each input:
    1.  Check whether it is part of a clump
        - If part of a clump store output index of the other clump-mate
    2.  Check whether it is greater than ANY output
    3.  If input i is greater than some output j and input i is not clumped
        - Insert index i before output j (shifting outputs j, j+1, j+2... to the right)
        Else if input i is greater than output j and its clump-mate is to the right of j
        - Delete the other clump-mate (it is the smaller of the two clump-mates) and insert input index i at output location j
        Else if input i is greater than output j and its clump-mate is to the left of j
        - Do nothing (clump already represented)
    **/
    for(int i=0; i<n_in; i++)
    {
        /// Check if the current input element is part of a clump of peaks
        /// i.e., check if the index of the previous input element has been added to the output
        bool part_of_clump = false;
        int OutputClumpMate;
        if(output[i] == min_pos+1)                                      /// The (min_pos)th element is a non-spike,
            part_of_clump = false;                                      /// so the (min_pos+1)th element cannot be part of a clump
        else if(i>0)                                                    /// Only elements input[i>0] have a previous element to check
        {
            for(int j=0; j<n_out; j++)
            {
                if(output[j] == i-1 && j!=min_pos)                      /// If part of clump
                {
                    part_of_clump = true;                               /// Set clump flag and also
                    OutputClumpMate = output[j];                        /// store output index of clump-mate, i.e., previous input index
                }
            }
        }

        /// Check whether the current input is bigger than any output
        for(int j=0; j<n_out; j++)
            if(input[i]>input[output[j]]*1.01)
            {
                /// If it is, check if clumping must be accounted for
                if(!(part_of_clump && ignore_clumped))
                {
                    /// If not, just insert
                    for(int k=n_out-1; k>j; k--)
                        output[k] = output[k-1];
                    output[j] = i;
                    break;
                }
                /// Otherwise, check if clump-mate is to the right of the would-be position of the new addition
                else if(OutputClumpMate > j)
                {
                    /// And if so, shift everything between the would-be position and the clump-mate position to the right
                    /// and insert the new addition where it belongs
                    for(int k=OutputClumpMate; k>j; k--)
                        output[k] = output[k-1];
                    output[j] = i;
                    break;
                }
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
int pitchNumber(float freq, float *centsSharp)
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

