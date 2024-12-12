#include <pthread.h>
#include <vector>
#include <cmath>
#include <iostream>

struct BandPassThreadData {
    std::vector<float>* data;
    float down;
    float up;
    size_t startIdx;
    size_t endIdx;
};

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
struct NotchThreadData {
    std::vector<float>* data;
    std::vector<float>* filteredData;
    float f0;
    int n;
    int sampleRate;
    size_t startIdx;
    size_t endIdx;
};

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

