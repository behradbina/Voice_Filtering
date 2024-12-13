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
const string OUTPUT_FILE = "output.wav";

struct FirThreadData {
    size_t start, end;                  // Range of indices to process
    const std::vector<float>* data;     // Pointer to the input data
    std::vector<float>* filtered_data;  // Pointer to the output data
    const std::vector<float>* coefficients; // Pointer to FIR coefficients
    size_t filter_order;                // Filter order
};

struct NotchThreadData {
    std::vector<float>* data;
    std::vector<float>* filteredData;
    float f0;
    int n;
    int sampleRate;
    size_t startIdx;
    size_t endIdx;
};

struct BandPassThreadData {
    std::vector<float>* data;
    float down;
    float up;
    size_t startIdx;
    size_t endIdx;
};

struct IirFilterData {
    const std::vector<float>* inputSignal;
    std::vector<float>* outputSignal;
    const std::vector<float>* feedforwardCoefficients;
    const std::vector<float>* feedbackCoefficients;
    size_t start;
    size_t end;
    size_t chunkIndex;
    std::vector<bool>* completedChunks;
    std::vector<std::mutex>* mutexes;
    std::vector<std::condition_variable>* cvs;
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
    void band_pass_filter(float down, float up);
    void notch_filter(float removed_frequency, int n, SF_INFO& fileInfo);
    void fir_filter();
    void fir_filter_serial();
    void iir_filter();
    void iir_filter_par();
    void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    void writeWavFile(const string& outputFile, SF_INFO& fileInfo);
    void apply_filter(const std::string& filter_name, SF_INFO& fileInfo,...); 
};

