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
        string output_file;
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

