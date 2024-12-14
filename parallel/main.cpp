#include "Voice.hpp"

int main(int argc, char* argv[]) 
{
    if (argc != 2)
    {
        cerr << "Arguments number not matching" << endl;
    }
    string inputFile = argv[1];
    
    float low = 300.0f, high= 3400.0f, delta = 1.0f;
    float removed_frequency = 1000.0f;
    SF_INFO fileInfo;
    vector<float> audioData;

    auto timeStart = chrono::high_resolution_clock::now();
    Voice voice(inputFile, audioData, fileInfo);
    auto readtimeEnd = chrono::high_resolution_clock::now();
    cout << "Read Time: " << chrono::duration_cast<chrono::duration<float, milli>>(readtimeEnd - timeStart).count() << " ms\n";
    
    voice.apply_filter("band_pass", fileInfo, low, high, delta);
    //voice.apply_filter("notch", fileInfo,removed_frequency, 2);
    //voice.apply_filter("fir", fileInfo);
    //voice.apply_filter("iir", fileInfo);
    auto exetimeEnd = chrono::high_resolution_clock::now();
    cout << "Execution Time: " << chrono::duration_cast<chrono::duration<float, milli>>(exetimeEnd - timeStart).count() << " ms\n";
    return 0;
}
