#include <cstddef>
#include <complex>
#include "api.h"

#include <fft.hpp>

enum {
	in_left = 0,
	in_right = 1,
	in_hertz = 2
};

enum {
	out_left = 0,
	out_right = 1
};

SYMBOL_EXPORT void process(const Global_Parameters* global,
                           const float* const* input_ports,
                           float* const* output_ports,
                           std::size_t n_samples) {
	std::complex<double>* const tmp1 = new std::complex<double>[n_samples];
	std::complex<double>* const tmp2 = new std::complex<double>[n_samples];

	for (std::size_t sample = 0; sample < n_samples; ++sample)
		tmp1[sample] = std::complex<double>(input_ports[in_left][sample], input_ports[in_right][sample]);

	fft(tmp1, tmp2, n_samples);

	std::size_t spectrum_size = n_samples/2 + (n_samples&1);

	std::complex<double>* left = new std::complex<double>[spectrum_size];
	std::complex<double>* right = new std::complex<double>[spectrum_size];
	std::complex<double>* left_out = new std::complex<double>[spectrum_size]();
	std::complex<double>* right_out = new std::complex<double>[spectrum_size]();

	split_channels(tmp2, left, right, n_samples);

	const double freq_step = global->sample_rate/n_samples;
	std::size_t min_bin = static_cast<int>(15.0/freq_step);

	// Freq Shift

	// find freq step = 1/duration
	const int bin_shift = *input_ports[in_hertz]/freq_step;

	for (std::size_t bin = min_bin - std::min(bin_shift, 0); bin < spectrum_size - std::max(bin_shift, 0); ++bin) {
		left_out[bin+bin_shift] = left[bin];
		right_out[bin+bin_shift] = right[bin];
	}

	join_channels(left_out, right_out, tmp2, n_samples);

	delete[] left_out;
	delete[] right_out;
	delete[] left;
	delete[] right;

	ifft(tmp2, tmp1, n_samples);

	for (std::size_t sample = 0; sample < n_samples; ++sample) {
		output_ports[out_left][sample] = tmp1[sample].real();
		output_ports[out_right][sample] = tmp1[sample].imag();
	}

	delete[] tmp1;
	delete[] tmp2;
}
