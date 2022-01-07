#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdint>
#include <climits>
#include "ax25.h"
#include "dsp.h"

typedef enum {
    IQ_S8,
    IQ_F32,
    PCM_F32,
} OutputFormat;

static void usage()
{
    fprintf(stderr, "Usage: aprs -c <callsign> [-d <destination>] [-p <path>] [-o <output>] [-f <format>] <message>\n"
            "   -c callsign    - your callsign\n"
            "   -d destination - AX.25 destination address (default 'APRS')\n"
            "   -p path        - Digipeater path to use (default 'WIDE1-1,WIDE2-1')\n"
            "   -o output      - output file (default stdout)\n"
            "   -f format      - output format: f32(default), s8(HackRF), pcm\n"
            "   -v             - print debug info\n");
    exit(1);
}

static std::vector<int8_t> f32_to_s8(const std::vector<std::complex<float>> &input)
{
    std::vector<int8_t> result(input.size() * 2);
    for (int i = 0; i < (int) input.size(); i++) {
        result[i*2] = input[i].real() * SCHAR_MAX;
        result[i*2+1] = input[i].imag() * SCHAR_MAX;
    }
    return result;
}

// FM modulation + interpolation (x50)
// output sample rate: 48000 * 50 = 2400000
static void modulate(const std::vector<float> &waveform, FILE *fout, OutputFormat iq_sf)
{
    float max_deviation = 5000; // 5kHz deviation
    float sensitivity = 2 * M_PI * max_deviation / (float)AUDIO_SAMPLE_RATE;
    float factor = 50.0;
    float fractional_bw = 0.4;
    float halfband = 0.5;
    float trans_width = halfband - fractional_bw;
    float mid_transition_band = halfband - trans_width / 2.0;
    std::vector<float> taps = lowpass(factor, factor, mid_transition_band, trans_width);

    Ringbuffer_t mod_buf;
    FIRInterpolator interp(factor, taps);
    float last_phase = 0;
    int offset = 0;
    while (offset < (int)waveform.size()) {
        int input_size = std::min(BUFSIZE, (int)waveform.size() - offset);
        last_phase = fmmod(waveform.data()+offset, input_size, mod_buf, sensitivity, last_phase);

        std::vector<std::complex<float>> interp_buf;
        int processed = interp.interpolate(mod_buf, interp_buf);
        if (!processed) {
            break;
        }
        mod_buf.remove(processed);
        if (iq_sf == IQ_S8) {
            auto samples_s8 = f32_to_s8(interp_buf);
            fwrite(samples_s8.data(), sizeof(int8_t), samples_s8.size(), fout);
        } else {
            fwrite(interp_buf.data(), sizeof(std::complex<float>), interp_buf.size(), fout);
        }
        offset += input_size;
    }
}

extern "C" {
    int8_t* gen_iq_s8(const char *callsign, const char *user_path, const char *info, int32_t *total)
    {
        const char *dest = "APRS";
        char path[64];
        strncpy(path, user_path, 63);

        auto frame = ax25frame(callsign, dest, path, info, false);
        auto frame_nrzi = nrzi(frame);
        auto wave = afsk(frame_nrzi);

        // interpolation factor is 50 and each sample is 2 bytes
        *total = wave.size() * 50 * 2;
        int8_t *samples = (int8_t*) malloc(*total);
        if (!samples) {
            return 0;
        }
        FILE *f = fmemopen(samples, *total, "wb");
        modulate(wave, f, IQ_S8);
        fclose(f);
        return samples;
    }
}

int main(int argc, char *argv[])
{
    int opt;
    bool debug = false;
    char *output = NULL;
    const char *callsign = nullptr;
    const char *info = nullptr;
    const char *dest = "APRS";
    char path[64];
    strcpy(path, "WIDE1-1,WIDE2-1");
    OutputFormat iq_sf = IQ_F32;

    while ((opt = getopt(argc, argv, "c:d:p:o:f:v")) != -1) {
        switch (opt) {
            case 'c':
                callsign = strdup(optarg);
                break;
            case 'd':
                dest = strdup(optarg);
                break;
            case 'p':
                strncpy(path, optarg, sizeof(path)-1);
                break;
            case 'o':
                output = strdup(optarg);
                break;
            case 'f':
                if (strcmp(optarg, "s8") == 0) {
                    iq_sf = IQ_S8;
                } else if (strcmp(optarg, "f32") == 0) {
                    iq_sf = IQ_F32;
                } else if (strcmp(optarg, "pcm") == 0) {
                    iq_sf = PCM_F32;
                } else {
                    fprintf(stderr, "Incorrect sample format: %s\n", optarg);
                    return 1;
                }
                break;
            case 'v':
                debug = true;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "No message specified\n");
        return 1;
    }
    info = argv[optind];
    if (!callsign) {
        fprintf(stderr, "Missing callsign\n");
        return 1;
    }
    auto frame = ax25frame(callsign, dest, path, info, debug);
    auto frame_nrzi = nrzi(frame);
    auto wave = afsk(frame_nrzi);

    FILE *fout = stdout;
    if (output) {
        fout = fopen(output, "wb");
        if (fout == NULL) {
            fprintf(stderr, "Error creating output file '%s'\n", output);
            return 1;
        }
    }
    if (iq_sf == PCM_F32) {
        fwrite(wave.data(), sizeof(float), wave.size(), fout);
    } else {
        modulate(wave, fout, iq_sf);
    }
    fclose(fout);
    return 0;
}
