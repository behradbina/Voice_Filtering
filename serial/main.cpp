#include "Voice.hpp"
//int argc, char* argv[]

int main() 
{
    string inputFile = "input.wav";
    float low = 300.0f, high= 3400.0f;
    float removed_frequency = 1000.0f;
    SF_INFO fileInfo;
    vector<float> audioData;

    auto timeStart = std::chrono::high_resolution_clock::now();
    Voice voice(inputFile, audioData, fileInfo);
    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Read Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
    
    //voice.apply_filter("band_pass", fileInfo, low, high);
    //voice.apply_filter("notch", fileInfo,removed_frequency);
    //voice.apply_filter("fir", fileInfo);
    voice.apply_filter("iir", fileInfo);

    return 0;
}