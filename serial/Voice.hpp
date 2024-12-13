#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <sndfile.h>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>

using namespace std;

#define COEFFICIENT_SIZE 200
const string OUTPUT_FILE = "output.wav";

class Voice
{
private:
    string input_file;
    vector<float> data;

public:
    Voice(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    ~Voice();
    void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    void writeWavFile(const string& outputFile, SF_INFO& fileInfo);
    void band_pass_filter(float down, float up);
    void notch_filter(float removed_frequency, int n, SF_INFO& fileInfo);
    void fir_filter();
    void iir_filter();
    void apply_filter(const std::string& filter_name, SF_INFO& fileInfo,...);
};

