#include "Voice.hpp"


Voice::Voice(const string& _inputFile, vector<float>& _data, SF_INFO& fileInfo)
{
    data = _data;
    input_file = _inputFile;
    readWavFile(_inputFile, data, fileInfo);
}

Voice::~Voice()
{
}

void Voice::readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    
    if (!inFile) {
        cerr << "Error opening input file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        cerr << "Error reading frames from file." << endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    cout << "Successfully read " << numFrames << " frames from " << inputFile << endl;
}

void Voice::writeWavFile(const string& outputFile, SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        std::cerr << "Error writing frames to file." << std::endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    std::cout << "Successfully wrote " << numFrames << " frames to " << outputFile << std::endl;
}

void Voice::band_pass_filter(float down, float up) {
    float delta_f = (up - down) / 2.0f;  // Bandwidth
    for (size_t i = 0; i < data.size(); ++i) {
        float f = i * (1.0f / data.size()); // Simulated frequency value
        float h_f = (f * f) / (f * f + delta_f * delta_f); // Filter formula

        // Amplify or attenuate the signal based on the filter response
        if (f >= down && f <= up) {
            data[i] *= h_f; // Apply band-pass
        } else {
            data[i] = 0.0f; // Attenuate frequencies outside the band
        }
    }
}

void Voice::notch_filter(float f0, int n, SF_INFO& fileInfo) 
{
    int numSamples = data.size();
    int sampleRate = fileInfo.samplerate;
    vector<float> filteredData(numSamples);
    for (int i = 0; i < numSamples; ++i) 
    {
        float f = (sampleRate * i) / numSamples;
        float response = (1.0 / (pow((f / f0), 2 * n) + 1));
        filteredData[i] = data[i] * response;
    }
    data = filteredData;
}

void Voice::fir_filter() {
    cout << "data.size() : " << data.size() << endl;
    vector<float> coefficients_xi(data.size(), 0.1f);
    cout << "coefficients.size() : " << coefficients_xi.size() << endl;
    vector<float> filtered_data(data.size(), 0.0f);
    size_t filter_order = coefficients_xi.size();

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t k = 0; k < filter_order; ++k) {
            if (i >= k) {
                filtered_data[i] += coefficients_xi[k] * data[i - k];
            }
        }
    }

    data = filtered_data;
}

void Voice::iir_filter() {
    vector<float> filtered_data(data.size(), 0.0f);
    vector<float>feedforward (data.size(), 0.1f);
    vector<float>feedback (data.size(), 0.5f);
    size_t ff_order = feedforward.size();
    size_t fb_order = feedback.size();
    
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t k = 0; k < ff_order; ++k) {
            if (i >= k) {
                filtered_data[i] += feedforward[k] * data[i - k];
            }
        }
        for (size_t j = 1; j < fb_order; ++j) { // j starts from 1 to avoid infinite recursion
            if (i >= j) {
                filtered_data[i] -= feedback[j] * filtered_data[i - j];
            }
        }
    }

    data = filtered_data;
}

