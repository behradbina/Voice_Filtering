#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <sndfile.h>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>
#include <mutex>
#include <condition_variable>

using namespace std;

#define COEFFICIENT_SIZE 200
#define THREAD_NUMBER 4

const string OUTPUT_FILE = "output.wav";

struct FirThreadData {
    size_t start, end;                  // Range of indices to process
    vector<float>* data;     // Pointer to the input data
    vector<float>* filtered_data;  // Pointer to the output data
    const std::vector<float>* coefficients; // Pointer to FIR coefficients
    size_t filter_order;                // Filter order
};

struct NotchThreadData {
    vector<float>* data;
    vector<float>* filteredData;
    float f0;
    int n;
    int sampleRate;
    size_t startIdx;
    size_t endIdx;
};

struct BandPassThreadData 
{
    vector<float>* data;
    double down;
    double up;
    double delta;
    int sample_rate;
    size_t startIdx;
    size_t endIdx;
};

struct IIRThreadData {
    size_t start;
    size_t end;
    vector<float>* data;
    vector<float>* filtered_data;
    vector<float>* feedforward;
    size_t ff_order;
};

class Voice
{
private:
    string input_file;
    string output_file;
    vector<float> data;
    SF_INFO fileInfo;

    struct ThreadData {
        Voice* voice;
        size_t start;
        size_t end;
        SNDFILE* inFile;
    };

    static void* readChunk(void* arg);
    static void* writeChunk(void* arg);

public:
    Voice(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    ~Voice();
    void band_pass_filter(SF_INFO& fileInfo, double down, double up, double deltaFreq);
    void notch_filter(float removed_frequency, int n, SF_INFO& fileInfo);
    void fir_filter();
    void iir_filter();
    void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    void writeWavFile(const string& outputFile, SF_INFO& fileInfo);
    void apply_filter(const std::string& filter_name, SF_INFO& fileInfo,...); 
};

