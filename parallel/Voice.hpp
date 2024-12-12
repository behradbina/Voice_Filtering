#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <sndfile.h>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace std;

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
    void iir_filter();
    void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    void writeWavFile(const string& outputFile, SF_INFO& fileInfo);
};

