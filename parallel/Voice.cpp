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
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        std::cerr << "Error reading frames from file." << std::endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    std::cout << "Successfully read " << numFrames << " frames from " << inputFile << std::endl;
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
void Voice::readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    size_t numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {this, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize};
        pthread_create(&threads[i], nullptr, readChunk, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    sf_close(inFile);
    std::cout << "Successfully read " << fileInfo.frames << " frames from " << inputFile << std::endl;
}

void* Voice::readChunk(void* arg) {
    ThreadData* threadData = static_cast<ThreadData*>(arg);
    Voice* voice = threadData->voice;
    size_t start = threadData->start;
    size_t end = threadData->end;

    sf_count_t numFrames = sf_readf_float(voice->data.data() + start, end - start);
    if (numFrames != end - start) {
        std::cerr << "Error reading frames from file in chunk." << std::endl;
        exit(1);
    }
    return nullptr;
}
void Voice::writeWavFile(const string& outputFile, SF_INFO& fileInfo) {
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    size_t numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {this, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize};
        pthread_create(&threads[i], nullptr, writeChunk, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    sf_close(outFile);
    std::cout << "Successfully wrote " << fileInfo.frames << " frames to " << outputFile << std::endl;
}

void* Voice::writeChunk(void* arg) {
    ThreadData* threadData = static_cast<ThreadData*>(arg);
    Voice* voice = threadData->voice;
    size_t start = threadData->start;
    size_t end = threadData->end;

    sf_count_t numFrames = sf_writef_float(voice->data.data() + start, end - start);
    if (numFrames != end - start) {
        std::cerr << "Error writing frames to file in chunk." << std::endl;
        exit(1);
    }
    return nullptr;
}


