#include <cassert>
#include <cmath>
#include "dsp.h"

#define IzeroEPSILON 1E-21 /* Max error acceptable in Izero */

const int BAUD_RATE = 1200;
const int MARK_HZ = 1200;
const int SPACE_HZ = 2200;

std::vector<float> afsk(const std::vector<bool> &data)
{
    std::vector<float> wave;
    // start with 0.5sec silence
    for (int i = 0 ; i < AUDIO_SAMPLE_RATE/2 ; i++) {
        wave.push_back(0);
    }
    float phase = 0;
    float gain = 0.5;
    int samples_per_bit = AUDIO_SAMPLE_RATE / BAUD_RATE;
    for (auto b : data) {
        float freq = b ? MARK_HZ : SPACE_HZ;
        float phase_change_per_sample = (2*M_PI * freq) / AUDIO_SAMPLE_RATE;
        for (int i = 0 ; i < samples_per_bit ; i++) {
            wave.push_back(sin(phase) * gain);
            phase += phase_change_per_sample;
            if (phase > 2*M_PI) {
                phase -= 2*M_PI;
            }
        }
    }
    // end with 0.5sec silence
    for (int i = 0 ; i < AUDIO_SAMPLE_RATE/2 ; i++) {
        wave.push_back(0);
    }
    return wave;
}

static int compute_ntaps(double sampling_freq, double transition_width, double param)
{
    double a = param / 0.1102 + 8.7;
    int ntaps = (int)(a * sampling_freq / (22.0 * transition_width));
    if ((ntaps & 1) == 0) {
        ntaps++;
    }
    return ntaps;
}

static double Izero(double x)
{
    double sum, u, halfx, temp;
    int n;

    sum = u = n = 1;
    halfx = x / 2.0;
    do {
        temp = halfx / (double)n;
        n += 1;
        temp *= temp;
        u *= temp;
        sum += u;
    } while (u >= IzeroEPSILON * sum);
    return (sum);
}

static std::vector<float> kaiser(int ntaps, double beta)
{
    assert(beta >= 0);
    std::vector<float> taps(ntaps);

    double IBeta = 1.0 / Izero(beta);
    double inm1 = 1.0 / ((double)(ntaps - 1));
    double temp;

    taps[0] = IBeta;
    for (int i = 1; i < ntaps - 1; i++) {
        temp = 2 * i * inm1 - 1;
        taps[i] = Izero(beta * sqrt(1.0 - temp * temp)) * IBeta;
    }
    taps[ntaps - 1] = IBeta;
    return taps;
}

std::vector<float> lowpass(double gain, double sampling_freq, double cutoff_freq, double transition_width)
{
    double param = 7.0;
    int ntaps = compute_ntaps(sampling_freq, transition_width, param);
    std::vector<float> taps(ntaps);
    std::vector<float> w = kaiser(ntaps, param);

    int M = (ntaps - 1) / 2;
    double fwT0 = 2 * M_PI * cutoff_freq / sampling_freq;

    for (int n = -M; n <= M; n++) {
        if (n == 0) {
            taps[n + M] = fwT0 / M_PI * w[n + M];
        } else {
            // a little algebra gets this into the more familiar sin(x)/x form
            taps[n + M] = sin(n * fwT0) / (n * M_PI) * w[n + M];
        }
    }

    // find the factor to normalize the gain, fmax.
    // For low-pass, gain @ zero freq = 1.0
    double fmax = taps[0 + M];
    for (int n = 1; n <= M; n++) {
        fmax += 2 * taps[n + M];
    }

    gain /= fmax; // normalize

    for (int i = 0; i < ntaps; i++) {
        taps[i] *= gain;
    }

    return taps;
}

float fmmod(const float *input, size_t input_size, Ringbuffer_t &output, float sensitivity, float last_phase)
{
    float phase = last_phase;
    assert(input_size <= output.writeAvailable());
    for (size_t i=0; i<input_size; i++) {
        phase += input[i] * sensitivity;
        while (phase>M_PI) phase -= 2*M_PI;
        while (phase<=-M_PI) phase += 2*M_PI;
        output.insert(std::complex<float>(cos(phase), sin(phase)));
    }
    return phase;
}

void naive_interpolate(const std::vector<std::complex<float>> &input,
                      std::vector<std::complex<float>> &output,
                      int interpolation,
                      const std::vector<float> &taps)
{
    std::vector<float> new_taps = taps;
    int n = taps.size() % interpolation;
    if (n > 0) {
        n = interpolation - n;
        new_taps.resize(taps.size()+n);
    }

    // upsampling in tmp
    std::vector<std::complex<float>> tmp;
    for (int i = 0 ; i < (int)input.size() ; i++) {
        for (int j = 0 ; j < interpolation-1 ; j++) {
            tmp.push_back(std::complex<float>(0.0, 0.0));
        }
        tmp.push_back(input[i]);
    }
    // apply filter over tmp
    int taps_count = new_taps.size();
    int processed = tmp.size() - taps_count + 1;
    for (int i = 0 ; i < processed ; i++) {
        std::complex<float> sum(0.0, 0.0);
        for (int j = 0; j < taps_count; j++) {
            sum += tmp[i+j] * new_taps[taps_count-j-1];
        }
        output.push_back(sum);
    }
}

FIRInterpolator::FIRInterpolator(int interpolation, const std::vector<float> &taps)
{
    std::vector<float> new_taps = taps;
    int n = taps.size() % interpolation;
    if (n > 0) {
        n = interpolation - n;
        new_taps.resize(taps.size()+n);
    }
    int nfilters = interpolation;
    int nt = new_taps.size() / nfilters;
    xtaps.resize(nfilters);

    for (int i = 0; i < nfilters; i++) {
        xtaps[i].resize(nt);
    }
    for (int i = 0; i < (int) new_taps.size(); i++) {
        xtaps[i % nfilters][i / nfilters] = new_taps[i];
    }
}

// The algorithm is described at https://dspguru.com/dsp/faqs/multirate/interpolation/
int FIRInterpolator::interpolate(Ringbuffer_t &input, std::vector<std::complex<float>> &output)
{
    int input_size = input.readAvailable();
    // the count of the polyphase filters
    int fir_count = (int)xtaps.size();
    // the count of the taps in a polyphase filter
    int taps_count = (int)xtaps[0].size();

    for (int i = 0; i <= input_size-taps_count; i++) {
        for (int j = 0; j < fir_count; j++) {
            std::complex<float> sum(0.0, 0.0);
            for (int k = 0; k < taps_count; k++) {
                sum += input[i+k] * xtaps[j][taps_count-k-1];
            }
            output.push_back(sum);
        }
    }
    return input_size - taps_count + 1;
}

