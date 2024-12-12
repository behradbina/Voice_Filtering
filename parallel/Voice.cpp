#include "Voice.hpp"


Voice::Voice(const string& _inputFile, vector<float>& _data, SF_INFO& fileInfo)
{
    data = _data;
    output_file = "output.wav";
    input_file = _inputFile;
    readWavFile(_inputFile, data, fileInfo);
}

Voice::~Voice()
{
}

// void Voice::readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
//     // Open the input file once
//     SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
//     if (!inFile) {
//         std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
//         exit(1);
//     }
//     // Resize the data buffer to accommodate all samples
//     data.resize(fileInfo.frames * fileInfo.channels);
//     size_t numThreads = 4; // Number of threads
//     pthread_t threads[numThreads];
//     ThreadData threadData[numThreads];
//     size_t chunkSize = data.size() / numThreads;
//     for (size_t i = 0; i < numThreads; ++i) {
//         threadData[i] = {this, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize, inFile};
//         pthread_create(&threads[i], nullptr, readChunk, &threadData[i]);
//     }
//     for (size_t i = 0; i < numThreads; ++i) {
//         pthread_join(threads[i], nullptr);
//     }
//     sf_close(inFile);
//     std::cout << "Successfully read " << fileInfo.frames << " frames from " << inputFile << std::endl;
// }

void* Voice::readChunk(void* arg) {
    ThreadData* threadData = static_cast<ThreadData*>(arg);
    Voice* voice = threadData->voice;
    size_t start = threadData->start;
    size_t end = threadData->end;
    SNDFILE* inFile = threadData->inFile;

    size_t chunkSize = end - start;

    // Locking for thread-safe access to the file
    static pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&fileLock);
    sf_seek(inFile, start, SEEK_SET); // Move file pointer to the correct position
    sf_count_t numFrames = sf_readf_float(inFile, voice->data.data() + start, chunkSize);
    pthread_mutex_unlock(&fileLock);

    if (numFrames != chunkSize) {
        std::cerr << "Error reading frames from file in chunk." << std::endl;
        exit(1);
    }

    return nullptr;
}

// void Voice::writeWavFile(const string& outputFile, SF_INFO& fileInfo) {
//     SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
//     if (!outFile) {
//         std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
//         exit(1);
//     }
//     size_t numThreads = 4; // Number of threads
//     pthread_t threads[numThreads];
//     ThreadData threadData[numThreads];
//     size_t chunkSize = data.size() / numThreads;
//     for (size_t i = 0; i < numThreads; ++i) {
//         threadData[i] = {this, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize};
//         threadData[i].output_file = output_file;
//         pthread_create(&threads[i], nullptr, writeChunk, &threadData[i]);
//     }
//     for (size_t i = 0; i < numThreads; ++i) {
//         pthread_join(threads[i], nullptr);
//     }
//     sf_close(outFile);
//     std::cout << "Successfully wrote " << fileInfo.frames << " frames to " << outputFile << std::endl;
// }
// void* Voice::writeChunk(void* arg) {
//     ThreadData* threadData = static_cast<ThreadData*>(arg);
//     Voice* voice = threadData->voice;
//     size_t start = threadData->start;
//     size_t end = threadData->end;
//     string out = threadData->output_file;
//     // Calculate the chunk size
//     size_t chunkSize = end - start;
//     // Open the file in append mode (ensure thread-safe writing)
//     SF_INFO fileInfo = voice->fileInfo; // Copy file information
//     SNDFILE* outFile = sf_open(out.c_str(), SFM_WRITE, &fileInfo);
//     if (!outFile) {
//         std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
//         pthread_exit(nullptr);
//     }
//     // Write only the specific chunk of frames
//     sf_count_t numFrames = sf_writef_float(outFile, voice->data.data() + start, chunkSize);
//     if (numFrames != chunkSize) {
//         std::cerr << "Error writing frames to file in chunk. Expected: " << chunkSize
//                   << ", but wrote: " << numFrames << std::endl;
//         sf_close(outFile);
//         pthread_exit(nullptr);
//     }
//     sf_close(outFile);
//     pthread_exit(nullptr);
// }

// void Voice::fir_filter() {
//     vector<float> coefficients_xi(data.size(), 0.1f);
//     vector<float> filtered_data(data.size(), 0.0f);
//     size_t filter_order = coefficients_xi.size();

//     for (size_t i = 0; i < data.size(); ++i) {
//         for (size_t k = 0; k < filter_order; ++k) {
//             if (i >= k) {
//                 filtered_data[i] += coefficients_xi[k] * data[i - k];
//             }
//         }
//     }

//     data = filtered_data;
// }

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

void* bandPassWorker(void* arg) {
    BandPassThreadData* threadData = static_cast<BandPassThreadData*>(arg);
    std::vector<float>& data = *(threadData->data);
    float down = threadData->down;
    float up = threadData->up;
    float delta_f = (up - down) / 2.0f; // Bandwidth

    for (size_t i = threadData->startIdx; i < threadData->endIdx; ++i) {
        float f = i * (1.0f / data.size()); // Simulated frequency value
        float h_f = (f * f) / (f * f + delta_f * delta_f); // Filter formula
        if (f >= down && f <= up) {
            data[i] *= h_f; // Apply band-pass
        } else {
            data[i] = 0.0f; // Attenuate frequencies outside the band
        }
    }
    return nullptr;
}

void Voice::band_pass_filter(float down, float up) {
    size_t numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    BandPassThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {&data, down, up, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize};
        pthread_create(&threads[i], nullptr, bandPassWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
}

void* notchFilterWorker(void* arg) {
    NotchThreadData* threadData = static_cast<NotchThreadData*>(arg);
    std::vector<float>& data = *(threadData->data);
    std::vector<float>& filteredData = *(threadData->filteredData);
    float f0 = threadData->f0;
    int n = threadData->n;
    int sampleRate = threadData->sampleRate;

    for (size_t i = threadData->startIdx; i < threadData->endIdx; ++i) {
        float f = (sampleRate * i) / data.size(); // Simulated frequency
        float response = 1.0f / (pow((f / f0), 2 * n) + 1); // Filter formula
        filteredData[i] = data[i] * response;
    }
    return nullptr;
}

void Voice::notch_filter(float f0, int n, SF_INFO& fileInfo) {
    int numSamples = data.size();
    int sampleRate = fileInfo.samplerate;
    size_t numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    NotchThreadData threadData[numThreads];
    std::vector<float> filteredData(numSamples);

    size_t chunkSize = numSamples / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {&data, &filteredData, f0, n, sampleRate, i * chunkSize, (i == numThreads - 1) ? numSamples : (i + 1) * chunkSize};
        pthread_create(&threads[i], nullptr, notchFilterWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    data = filteredData; // Replace original data with filtered data
}

void* firWorker(void* arg) {
    FirThreadData* threadData = static_cast<FirThreadData*>(arg);
    const auto& data = *threadData->data;
    auto& filtered_data = *threadData->filtered_data;
    const auto& coefficients = *threadData->coefficients;
    size_t filter_order = threadData->filter_order;
    size_t start = threadData->start;
    size_t end = threadData->end;

    for (size_t i = start; i < end; ++i) {
        for (size_t k = 0; k < filter_order; ++k) {
            if (i >= k) {
                filtered_data[i] += coefficients[k] * data[i - k];
            }
        }
    }

    return nullptr;
}

void Voice::fir_filter() {
    std::vector<float> coefficients_xi(data.size(), 0.1f); // Example coefficients
    std::vector<float> filtered_data(data.size(), 0.0f);
    size_t filter_order = coefficients_xi.size();
    size_t numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    FirThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize;

        // Expand the start index for overlap (boundary condition)
        size_t expandedStart = (start > filter_order) ? start - filter_order : 0;

        threadData[i] = {expandedStart, end, &data, &filtered_data, &coefficients_xi, filter_order};

        pthread_create(&threads[i], nullptr, firWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Assign the computed filtered data back to the class member
    data = filtered_data;
}

