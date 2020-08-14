#include <algorithm>
#include <cmath>

#include "api.h"

enum {
	in_left = 0,
	in_right = 1,
	in_peak = 2
};

enum {
	out_left = 0,
	out_right = 1
};

SYMBOL_EXPORT void process([[maybe_unused]] const Global_Parameters* global,
                           const float* const* input_ports,
                           float* const* output_ports,
                           size_t n_samples) {
	float threshold_ampl = exp10(*input_ports[in_peak]/20.f);

	float max = 0.0;
	for (std::size_t sample = 0; sample < n_samples; ++sample) {
		if (std::abs(input_ports[in_left][sample]) > max)
			max = std::abs(input_ports[in_left][sample]);
		if (std::abs(input_ports[in_right][sample]) > max)
			max = std::abs(input_ports[in_right][sample]);
	}
	max = max == 0 ? 1 : max; // set max = 1 if max is 0
	float ratio = threshold_ampl/max;
	for (size_t sample = 0; sample < n_samples; ++sample) {
		output_ports[out_left][sample] = ratio*input_ports[in_left][sample];
		output_ports[out_right][sample] = ratio*input_ports[in_right][sample];
	}
}
