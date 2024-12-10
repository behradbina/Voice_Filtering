#include "Voice.hpp"
//int argc, char* argv[]

int main() {
    
	
    string inputFile = "input.wav";
    string outputFile = "output.wav";
    float low = 300.0f, high= 3400.0f;
    //float removed_frequency;
    SF_INFO fileInfo;
    vector<float> audioData;

    auto timeStart = std::chrono::high_resolution_clock::now();
    Voice voice(inputFile, audioData, fileInfo);
    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Read Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
    timeStart = std::chrono::high_resolution_clock::now();
    voice.band_pass_filter(low, high);
    timeEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Bound pass filter time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
    //timeStart = std::chrono::high_resolution_clock::now();
    //voice.notch_filter(removed_frequency);
    timeEnd = std::chrono::high_resolution_clock::now();
    //std::cout << "Notch filter time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
    
    voice.writeWavFile(outputFile, fileInfo);
    //timeEnd = std::chrono::high_resolution_clock::now();
    //std::cout << "Write Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
	std::cout << "Done and the output is in output.bmp" << std::endl;

    return 0;
}