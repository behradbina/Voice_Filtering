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

void sum_vec(vector<float> filtered_data)
{
    float sum = 0;
    for (size_t i = 0; i < filtered_data.size(); i++)
    {
        sum += filtered_data[i];
    }
    cout << "sum : " << sum << endl;
}

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

    if (int(numFrames) != int(chunkSize)) {
        std::cerr << "Error reading frames from file in chunk." << std::endl;
        exit(1);
    }

    return nullptr;
}

void Voice::readWavFile_par(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    // Open the input file once
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }
    data.resize(fileInfo.frames * fileInfo.channels);
    size_t numThreads = 8;
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    size_t chunkSize = data.size() / numThreads;
    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {this, i * chunkSize, (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize, inFile};
        pthread_create(&threads[i], nullptr, readChunk, &threadData[i]);
    }
    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    sf_close(inFile);
    std::cout << "Successfully read " << fileInfo.frames << " frames from " << inputFile << std::endl;
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
    vector<float>& data = *(threadData->data);
    float down = threadData->down;
    float up = threadData->up;
    float delta_f = threadData->delta;
    int sample_rate = threadData->sample_rate;
    int sample_numbers = data.size();
    
    auto filterResponse = [=](double f) 
    {
        return (f * f) / (f * f + delta_f * delta_f);
    };

    for (size_t i = threadData->startIdx; i < threadData->endIdx; ++i) {
        double f = (static_cast<double>(i) * sample_rate) / sample_numbers; 
        
        if (f >= down && f <= up) 
            data[i] = data[i] * filterResponse(f);
        else 
            data[i] = 0; 
    }
    return nullptr;
}

void Voice::band_pass_filter(SF_INFO& fileInfo, double down, double up, double deltaFreq) {
    size_t numThreads = THREAD_NUMBER;
    pthread_t threads[numThreads];
    BandPassThreadData threadData[numThreads];
    int sample_rate = fileInfo.samplerate;
    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        threadData[i] = {&data, down, up, deltaFreq, sample_rate, 
                        i * chunkSize,
                        (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize};
        pthread_create(&threads[i], nullptr, bandPassWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
}

void* notchFilterWorker(void* arg) {
    NotchThreadData* threadData = static_cast<NotchThreadData*>(arg);
    vector<float>& data_ = *(threadData->data);
    vector<float>& filteredData = *(threadData->filteredData);
    float f0 = threadData->f0;
    int n = threadData->n;
    int sampleRate = threadData->sampleRate;
    for (size_t i = threadData->startIdx; i < threadData->endIdx; ++i) {
        float f = (sampleRate * i) / data_.size(); // Simulated frequency
        float response = 1.0f / (pow((f / f0), 2 * n) + 1); // Filter formula
        filteredData[i] = data_[i] * response;
    }
    return nullptr;
}

void Voice::notch_filter(float f0, int n, SF_INFO& fileInfo) {
    sum_vec(data);
    int numSamples = data.size();
    int sampleRate = fileInfo.samplerate;
    size_t end_index;
    size_t numThreads = THREAD_NUMBER;
    pthread_t threads[numThreads];
    NotchThreadData threadData[numThreads];
    vector<float> filteredData(numSamples, 0.0f);

    size_t chunkSize = numSamples / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        
        if (i==numThreads-1)
        {
            end_index = numSamples;
        }
        else
        {
            end_index = (i + 1) * chunkSize;
        }
        
        threadData[i] = {&data, &filteredData, f0, n, sampleRate, i * chunkSize, end_index};
        pthread_create(&threads[i], nullptr, notchFilterWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    data = filteredData; // Replace original data with filtered data
}

void* firWorker(void* arg) {
    FirThreadData* threadData = static_cast<FirThreadData*>(arg);
    vector<float>& data_ = *(threadData->data);
    vector<float>& filtered_data = *(threadData->filtered_data);
    const auto& coefficients = *threadData->coefficients;
    size_t filter_order = threadData->filter_order;
    size_t start = threadData->start;
    size_t end = threadData->end;

    for (size_t i = start; i < end; ++i) {
        for (size_t k = 0; k < filter_order; ++k) {
            if (i >= k) {
                filtered_data[i] += coefficients[k] * data_[i - k];
            }
        }
    }

    
    return nullptr;
}

void Voice::apply_filter(const std::string& filter_name, SF_INFO& fileInfo,...) {
    auto start_time = std::chrono::high_resolution_clock::now();
    va_list args;
    va_start(args, fileInfo);

    if (filter_name == "notch") {
        float removed_frequency = va_arg(args, double); // Use 'double' because va_arg promotes floats to doubles
        int n = va_arg(args, int);
        cout << "n :" << n << "removed_frequency : " << removed_frequency << endl;
        notch_filter(removed_frequency, n, fileInfo);
    } 
    else if (filter_name == "band_pass") {
        float down = va_arg(args, double);
        float up = va_arg(args, double);
        float delta = va_arg(args, double);
        band_pass_filter(fileInfo, down, up, delta);
    } 
    else if (filter_name == "fir") {
        fir_filter();
    } 
    else if (filter_name == "iir") {
        iir_filter();
    } 
    else {
        std::cerr << "Error: Unknown filter name: " << filter_name << std::endl;
        va_end(args);
        return;
    }

    va_end(args);

    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Filter '" << filter_name << "' applied in " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end_time - start_time).count() << " ms." << std::endl;
    start_time = std::chrono::high_resolution_clock::now();
    writeWavFile(OUTPUT_FILE, fileInfo);
    end_time = std::chrono::high_resolution_clock::now(); 
    std::cout << "Write Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end_time - start_time).count() << " ms\n";
    sum_vec(data);
}

void Voice::fir_filter() {
    std::vector<float> coefficients_xi(COEFFICIENT_SIZE, 0.1f);
    std::vector<float> filtered_data(data.size(), 0.0f);
    size_t numThreads = THREAD_NUMBER; 
    pthread_t threads[numThreads];
    FirThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize;
        threadData[i] = {start, end, &data, &filtered_data, &coefficients_xi, COEFFICIENT_SIZE};

        pthread_create(&threads[i], nullptr, firWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    data = filtered_data;
}

void* iirFeedforwardWorker(void* arg) {
    IIRThreadData* threadData = static_cast<IIRThreadData*>(arg);
    vector<float>& data_ = *(threadData->data);
    vector<float>& filtered_data = *(threadData->filtered_data);
    const auto& feedforward = *threadData->feedforward;
    size_t ff_order = threadData->ff_order;
    size_t start = threadData->start;
    size_t end = threadData->end;

    for (size_t i = start; i < end; ++i) {
        for (size_t k = 0; k < ff_order; ++k) {
            if (i >= k) {
                filtered_data[i] += feedforward[k] * data_[i - k];
            }
        }
    }

    return nullptr;
}

void Voice::iir_filter() {
    vector<float> filtered_data(data.size(), 0.0f);
    vector<float> feedforward(COEFFICIENT_SIZE, 0.1f);
    vector<float> feedback(COEFFICIENT_SIZE, 0.5f);
    size_t ff_order = feedforward.size();
    size_t fb_order = feedback.size();

    size_t numThreads = THREAD_NUMBER; 
    pthread_t threads[numThreads];
    IIRThreadData threadData[numThreads];

    size_t chunkSize = data.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize;

        threadData[i] = {start, end, &data, &filtered_data, &feedforward, ff_order};

        pthread_create(&threads[i], nullptr, iirFeedforwardWorker, &threadData[i]);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Single-threaded feedback part
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 1; j < fb_order; ++j) {
            if (i >= j) {
                filtered_data[i] -= feedback[j] * filtered_data[i - j];
            }
        }
    }

    // Update the original data
    data = filtered_data;
}
