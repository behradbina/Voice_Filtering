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
    vector<float> data;
    SF_INFO fileInfo;

    struct ThreadData {
        Voice* voice;
        size_t start;
        size_t end;
    };

    static void* readChunk(void* arg);
    static void* writeChunk(void* arg);

public:
    Voice(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    ~Voice();
    void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo);
    void writeWavFile(const string& outputFile, SF_INFO& fileInfo);
};

