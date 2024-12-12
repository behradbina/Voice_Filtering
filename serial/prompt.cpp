struct ThreadData {
    size_t start, end;                  // Range of indices to process
    const std::vector<float>* data;     // Pointer to the input data
    std::vector<float>* filtered_data;  // Pointer to the output data
    const std::vector<float>* coefficients; // Pointer to FIR coefficients
    size_t filter_order;                // Filter order
};

void* firWorker(void* arg) {
    ThreadData* threadData = static_cast<ThreadData*>(arg);
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
    ThreadData threadData[numThreads];

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


