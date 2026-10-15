// SP500 log-mel example: read a 16 kHz mono 16-bit WAV file, compute the
// log-mel spectrogram on the DSP, save it as a PNG image.
//
//   ./log_mel_demo <input wav> [output png]

#include <sp500/log_mel.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

static uint16_t read_u16_le(const char* data) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}

static uint32_t read_u32_le(const char* data) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

static bool read_exact(std::ifstream& file, void* data, uint32_t size) {
    file.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
    return file.gcount() == static_cast<std::streamsize>(size);
}

// Read a 16-bit PCM mono WAV file into float samples in -1..1.
// Returns the sample rate, or 0 on failure.
static int read_wav(const char* path, std::vector<float>& samples) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return 0;
    }
    file.seekg(0, std::ios::end);
    const std::streamoff file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (file_size < 12) {
        return 0;
    }

    char riff[12];  // "RIFF" <size> "WAVE"
    if (!read_exact(file, riff, sizeof(riff))) {
        return 0;
    }
    if (std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(riff + 8, "WAVE", 4) != 0) {
        return 0;
    }

    const uint64_t riff_size = read_u32_le(riff + 4);
    const uint64_t riff_end = 8 + riff_size;
    if (riff_size < 4 || riff_end > static_cast<uint64_t>(file_size)) {
        return 0;
    }

    int sample_rate = 0;
    int channels = 0;
    int bits = 0;
    uint64_t offset = 12;
    while (offset < riff_end) {
        if (riff_end - offset < 8) {
            return 0;
        }

        char chunk[8];  // <chunk id> <chunk size>
        if (!read_exact(file, chunk, sizeof(chunk))) {
            return 0;
        }
        offset += sizeof(chunk);

        const uint32_t size = read_u32_le(chunk + 4);
        const uint64_t padded_size = static_cast<uint64_t>(size) + (size & 1U);
        if (padded_size > riff_end - offset) {
            return 0;
        }

        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            if (size < 16) {
                return 0;
            }
            char fmt[16];
            if (!read_exact(file, fmt, sizeof(fmt))) {
                return 0;
            }
            const uint16_t format = read_u16_le(fmt);
            const uint16_t ch = read_u16_le(fmt + 2);
            const uint32_t rate = read_u32_le(fmt + 4);
            const uint16_t bps = read_u16_le(fmt + 14);
            if (format != 1) {  // 1 = PCM
                return 0;
            }
            if (rate == 0 ||
                rate > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
                return 0;
            }
            channels = ch;
            sample_rate = static_cast<int>(rate);
            bits = bps;
            file.seekg(static_cast<std::streamoff>(padded_size - sizeof(fmt)),
                       std::ios::cur);
            if (!file) {
                return 0;
            }
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            if (channels != 1 || bits != 16 || sample_rate == 0 || size == 0 ||
                (size & 1U) != 0) {
                return 0;
            }

            const size_t sample_count = size / sizeof(int16_t);
            std::vector<int16_t> pcm;
            if (sample_count > static_cast<size_t>(std::numeric_limits<int32_t>::max()) ||
                sample_count > std::numeric_limits<uint32_t>::max() / sizeof(float) ||
                sample_count > pcm.max_size() || sample_count > samples.max_size()) {
                return 0;
            }

            try {
                pcm.resize(sample_count);
                samples.resize(sample_count);
            } catch (const std::bad_alloc&) {
                samples.clear();
                return 0;
            }
            if (!read_exact(file, pcm.data(), size)) {
                samples.clear();
                return 0;
            }
            for (size_t i = 0; i < pcm.size(); ++i) {
                samples[i] = pcm[i] / 32768.0f;
            }
            return sample_rate;
        } else {
            file.seekg(static_cast<std::streamoff>(padded_size), std::ios::cur);
            if (!file) {
                return 0;
            }
        }
        offset += padded_size;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <input wav> [output png]\n", argv[0]);
        return 1;
    }
    const char* input_path = argv[1];
    const char* output_path = argc > 2 ? argv[2] : "log_mel_out.png";

    // 1. Read the WAV file. The example expects 16 kHz mono 16-bit PCM.
    std::vector<float> samples;
    int sample_rate = read_wav(input_path, samples);
    if (sample_rate == 0) {
        std::fprintf(stderr, "cannot read %s (need 16-bit PCM mono WAV)\n", input_path);
        return 1;
    }
    if (sample_rate != 16000) {
        std::fprintf(stderr, "sample rate must be 16000, got %d\n", sample_rate);
        return 1;
    }
    std::printf("%zu samples, %.2f s\n", samples.size(), samples.size() / 16000.0);

    // 2. Connect to the DSP. Fails with SP500_ERR_TIMEOUT if no firmware is loaded.
    sp500_status rc = sp500_dsp_open(0);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_dsp_open: %s\n", sp500_status_string(rc));
        return 1;
    }

    // 3. Set the log-mel parameters. Every field is written here for clarity;
    //    values other than n_samples are the Whisper defaults set by params_init.
    sp500_log_mel_params params;
    sp500_log_mel_params_init(&params);
    params.n_samples = static_cast<int32_t>(samples.size());  // input length
    params.sample_rate = 16000;                               // Hz
    params.n_fft = 400;                                       // FFT size, 25 ms
    params.hop_length = 160;                                  // frame step, 10 ms
    params.n_mels = 80;                                       // mel bands
    params.min_frequency = 0.0f;                              // mel bank low edge
    params.max_frequency = 8000.0f;                           // mel bank high edge
    params.normalize = 1;                                     // Whisper normalisation
    params.mel_filters = nullptr;  // use the built-in slaney bank
    params.mel_filters_count = 0;

    // 4. Prepare the input and output buffers. Output is n_frames x n_mels float.
    const int32_t n_frames =
        sp500_log_mel_num_frames(params.n_samples, params.hop_length);
    std::vector<float> features(static_cast<size_t>(n_frames) * params.n_mels);
    sp500_buffer audio =
        sp500_buffer_wrap(samples.data(), samples.size() * sizeof(float));
    sp500_buffer out =
        sp500_buffer_wrap(features.data(), features.size() * sizeof(float));

    // 5. Run log-mel on the DSP. Long clips take longer than the 3 s default deadline.
    sp500_set_dsp_timeout_ms(20000);
    sp500_log_mel_result result;
    rc = sp500_log_mel(&audio, &params, &out, &result);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_log_mel: %s\n", sp500_status_string(rc));
        return 1;
    }
    std::printf("%d frames x %d mels, dsp %.3f ms\n", result.n_frames, params.n_mels,
                result.host_seconds * 1000.0);

    // 6. Draw the spectrogram: time on the x axis, mel band on the y axis with
    //    low frequencies at the bottom, values mapped to a color scale.
    cv::Mat spec(result.n_frames, params.n_mels, CV_32F, features.data());  // [t][m]
    cv::Mat image;
    cv::transpose(spec, image);  // [m][t]
    cv::flip(image, image, 0);   // low mel at the bottom
    cv::normalize(image, image, 0, 255, cv::NORM_MINMAX);
    image.convertTo(image, CV_8U);
    cv::applyColorMap(image, image, cv::COLORMAP_VIRIDIS);
    cv::resize(image, image, cv::Size(image.cols, params.n_mels * 4), 0, 0,
               cv::INTER_NEAREST);

    // 7. Save the result.
    if (!cv::imwrite(output_path, image)) {
        std::fprintf(stderr, "cannot write %s\n", output_path);
        return 1;
    }
    std::printf("saved %s (%dx%d)\n", output_path, image.cols, image.rows);
    return 0;
}
