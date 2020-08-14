#include <cstddef>
#include <algorithm>
#include <complex>
#include "api.h"

#include <iostream>

#include <fft.hpp>

enum {
	in_left = 0,
	in_right = 1,
	mode = 2
};

enum {
	audio_out = 0
};

enum Mode {
	GEO_MEAN = 0,
	RMS = 1,
	ABS_SUM = 2,
	COMPONENTWISE_RMS = 3
};

SYMBOL_EXPORT void process([[maybe_unused]] const Global_Parameters* global,
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

	split_channels(tmp2, left, right, n_samples);

	// Monoify
	for (std::size_t i = 0; i < spectrum_size; ++i)
		switch(static_cast<Mode>(*input_ports[mode])) {
			case Mode::GEO_MEAN:
				tmp1[i] = sqrt(left[i]*right[i]);
				break;
			case Mode::RMS:
				tmp1[i] = sqrt((left[i]*left[i] + right[i]*right[i])/2.0);
				break;
			case Mode::ABS_SUM:
				tmp1[i] = std::complex<double>(
					std::copysign(1.0, left[i].real()+right[i].real())*(std::abs(left[i].real()) + std::abs(right[i].real())),
					std::copysign(1.0, left[i].imag()+right[i].imag())*(std::abs(left[i].imag()) + std::abs(right[i].imag()))
				)/2.0;
				break;
			case Mode::COMPONENTWISE_RMS:
				tmp1[i] = std::complex<double>(
					std::copysign(1.0, left[i].real()+right[i].real())*std::hypot(left[i].real(), right[i].real()),
					std::copysign(1.0, left[i].imag()+right[i].imag())*std::hypot(left[i].imag(), right[i].imag())
				)/sqrt(2.0);
				break;
		}

	join_channels(tmp1, tmp1, tmp2, n_samples);

	delete[] left;
	delete[] right;

	ifft(tmp2, tmp1, n_samples);

	for (std::size_t sample = 0; sample < n_samples; ++sample)
		output_ports[audio_out][sample] = tmp1[sample].real();

	delete[] tmp1;
	delete[] tmp2;

	// Lower volume if peaking
	float max = 1.0;
	for (std::size_t sample = 0; sample < n_samples; ++sample)
		if (std::abs(output_ports[audio_out][sample]) > max)
			max = std::abs(output_ports[audio_out][sample]);
	const float ratio = 1.0/max;
	for (std::size_t sample = 0; sample < n_samples; ++sample)
		output_ports[audio_out][sample] *= ratio;
}
