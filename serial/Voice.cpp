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

void sum_vec(vector<float> filtered_data)
{
    float sum = 0;
    for (size_t i = 0; i < filtered_data.size(); i++)
    {
        sum += filtered_data[i];
    }
    cout << "Summation of output frames : " << sum << endl;
    cout << "-------------------------------------"<<endl;
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
    cout << "-------------------------------------"<<endl;
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

void Voice::band_pass_filter(SF_INFO& fileInfo, double down, double up, double deltaFreq) {
    int sample_numbers = data.size();
    int sample_rate = fileInfo.samplerate;
    auto filterResponse = [=](double f) 
    {
        return (f * f) / (f * f + deltaFreq * deltaFreq);
    };

    for (size_t i = 0; i < sample_numbers; i++) 
    {
        double f = (static_cast<double>(i) * sample_rate) / sample_numbers; 
        if (f >= down && f <= up) 
            data[i] = data[i] * filterResponse(f);
        else 
            data[i] = 0; 
    }
   
}

void Voice::notch_filter(float f0, int n, SF_INFO& fileInfo) 
{
    size_t numSamples = data.size();
    int sampleRate = fileInfo.samplerate;
    vector<float> filteredData(numSamples, 0.0f);
    for (size_t i = 0; i < numSamples; ++i) 
    {
        float f = (sampleRate * i) / numSamples;
        float response = (1.0f / (pow((f / f0), 2 * n) + 1));
        filteredData[i] = data[i] * response;
    }
    data = filteredData;
}

void Voice::fir_filter() {
    vector<float> coefficients_xi(COEFFICIENT_SIZE, 0.1f);
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
    vector<float>feedforward (COEFFICIENT_SIZE, 0.1f);
    vector<float>feedback (COEFFICIENT_SIZE, 0.5f);
    size_t ff_order = feedforward.size();
    size_t fb_order = feedback.size();
    
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t k = 0; k < ff_order; ++k) {
            if (i >= k) {
                filtered_data[i] += feedforward[k] * data[i - k];
            }
        }
        for (size_t j = 1; j < fb_order; ++j) { 
            if (i >= j) {
                filtered_data[i] -= feedback[j] * filtered_data[i - j];
            }
        }
    }

    data = filtered_data;
}

void Voice::apply_filter(const std::string& filter_name, SF_INFO& fileInfo,...) {
    auto start_time = std::chrono::high_resolution_clock::now();
    va_list args;
    va_start(args, fileInfo);

    if (filter_name == "notch") {
        float removed_frequency = va_arg(args, double); // Use 'double' because va_arg promotes floats to doubles
        int n = va_arg(args, int);
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

    cout << "Filter '" << filter_name << "' applied in " << chrono::duration_cast<std::chrono::duration<float, std::milli>>(end_time - start_time).count() << " ms." << std::endl;
    start_time = chrono::high_resolution_clock::now();
    writeWavFile(OUTPUT_FILE, fileInfo);
    end_time = chrono::high_resolution_clock::now(); 
    cout << "Write Time: " << std::chrono::duration_cast<std::chrono::duration<float, milli>>(end_time - start_time).count() << " ms\n";
    sum_vec(data);
}

