#include <cmath>
#include <numeric>
#include "fft.hpp"

void split_channels(std::complex<double>* in,
                    std::complex<double>* left,
                    std::complex<double>* right,
                    std::size_t size) {
	left[0] = in[0].real();
	right[0] = in[0].imag();

	for (size_t i = 1; i < size/2; ++i) {
		left[i] = (in[i] + std::conj(in[size - i])) * 0.5;
		right[i] = std::complex<double>(0, 0.5) * (std::conj(in[size - i]) - in[i]);
	}

	if (size&1) {
		left[size/2] = std::complex<double>(0, in[size-1].imag());
		right[size/2] = std::complex<double>(0, in[size-1].real());
	}
}

void join_channels(std::complex<double>* left,
                   std::complex<double>* right,
                   std::complex<double>* out,
                   std::size_t size) {
	out[0] = std::complex<double>(left[0].real(), right[0].real());

	for (size_t i = 1; i < size/2; ++i) {
		out[i] = left[i]-right[i]*std::complex<double>(0, -1.0);
		out[size - i] = std::conj(left[i] + std::complex<double>(0, -1)*right[i]);
	}

	if (size&1)
		out[size-1] = std::complex<double>(right[size/2].imag(), left[size/2].imag());
}

static void dft(const std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	const std::complex<double> wmm = std::exp(std::complex<double>(0, -2.0*M_PI/size));
	std::complex<double> wm = 1; // wm = wmm^k
	for (std::size_t k = 0; k < size; ++k) {
		out[k] = 0.0;
		std::complex<double> w = 1; // w = wm^n = wmm^nk
		for (std::size_t n = 0; n < size; ++n) {
			out[k] += in[n]*w;
			w *= wm;
		}
		out[k] /= size;

		wm *= wmm;
	}
}

static void idft(const std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	const std::complex<double> wmm = std::exp(std::complex<double>(0, 2.0*M_PI/size));
	std::complex<double> wm = 1;
	for (std::size_t k = 0; k < size; ++k) {
		out[k] = 0.0;
		std::complex<double> w = 1;
		for (std::size_t n = 0; n < size; ++n) {
			out[k] += in[n]*w;
			w *= wm;
		}
		wm *= wmm;
	}
}

/**
 * bit_reverse reverses the first num_reverse bits in n
 */
static std::size_t bit_reverse(std::size_t n, std::size_t num_reverse) {
	std::size_t rtrn = 0;
	for (size_t bit = 0; bit < num_reverse; ++bit) {
		rtrn <<= 1;
		rtrn += ( (n&(1 << bit)) != 0 );
	}
	return rtrn;
}

/**
 * Requires size to be a power of 2
 */
static void bit_reverse_copy(const std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	std::size_t log_size = 0;
	while (1 << log_size ^ size) ++log_size;
	for (std::size_t i = 0; i < size; ++i) out[bit_reverse(i, log_size)] = in[i];
}

static void bit_reverse_fft(const std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	bit_reverse_copy(in, out, size);
	for (std::size_t m = 2; m <= size; m <<= 1) {
		const std::complex<double> wm = std::exp(std::complex<double>(0.0, -2.0*M_PI/m));
		for (std::size_t k = 0; k < size; k += m) {
			std::complex<double> w = 1;
			for (std::size_t j = 0; j < m/2; ++j) {
				const auto even = out[k+j];
				const auto odd = w*out[k+j+m/2];
				out[k+j] = even + odd;
				out[k+j+m/2] = even - odd;
				w *= wm;
			}
		}
	}

	for (std::size_t i = 0; i < size; ++i) out[i] /= static_cast<double>(size);
}

static void bit_reverse_ifft(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	bit_reverse_copy(in, out, size);
	for (std::size_t m = 2; m <= size; m <<= 1) {
		const std::complex<double> wm = std::exp(std::complex<double>(0.0, 2.0*M_PI/m));
		for (std::size_t k = 0; k < size; k += m) {
			std::complex<double> w = 1;
			for (std::size_t j = 0; j < m/2; ++j) {
				const auto even = out[k+j];
				const auto odd = w*out[k+j+m/2];
				out[k+j] = even + odd;
				out[k+j+m/2] = even - odd;
				w *= wm;
			}
		}
	}
}

/**
 * Finds the nearest power of 2 which is >= n
 */
static inline std::size_t next_pow_2(std::size_t n) {
	std::size_t log_n = ((n&(n-1)) != 0);
	while (n >>= 1) ++log_n;
	return 1 << log_n;
}

static void bluesteins_algorithm(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	const std::size_t padded_size = next_pow_2(2*size-1);

	std::complex<double>* a = new std::complex<double>[padded_size]();
	std::complex<double>* b = new std::complex<double>[padded_size]();
	std::complex<double>* c = new std::complex<double>[padded_size];

	for (std::size_t n = 0; n < size; ++n) {
		a[n] = in[n]*std::exp(std::complex<double>(0.0, -M_PI*n*n/size));
		b[n] = std::exp(std::complex<double>(0.0, M_PI*n*n/size));
	}

	for (std::size_t n = 1; n < size; ++n) b[padded_size-n] = b[n];

	bit_reverse_ifft(b, c, padded_size);
	bit_reverse_ifft(a, b, padded_size);

	for (std::size_t n = 0; n < padded_size; ++n) b[n] *= c[n];

	bit_reverse_fft(b, a, padded_size);

	for (int k = 0; static_cast<std::size_t>(k) < size; ++k)
		out[k] = std::exp(std::complex<double>(0.0, -M_PI*k*k/size)) * a[k] / static_cast<double>(size);

	delete[] a;
	delete[] b;
	delete[] c;
}

static void inverse_bluesteins_algorithm(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	const std::size_t padded_size = next_pow_2(2*size-1);

	std::complex<double>* a = new std::complex<double>[padded_size]();
	std::complex<double>* b = new std::complex<double>[padded_size]();
	std::complex<double>* c = new std::complex<double>[padded_size];

	for (std::size_t n = 0; n < size; ++n) {
		a[n] = in[n]*std::exp(std::complex<double>(0.0, M_PI*n*n/size));
		b[n] = std::exp(std::complex<double>(0.0, -M_PI*n*n/size));
	}

	for (std::size_t n = 1; n < size; ++n) b[padded_size-n] = b[n];

	bit_reverse_ifft(b, c, padded_size);
	bit_reverse_ifft(a, b, padded_size);

	for (std::size_t n = 0; n < padded_size; ++n) b[n] *= c[n];

	bit_reverse_fft(b, a, padded_size);

	for (int k = 0; static_cast<std::size_t>(k) < size; ++k)
		out[k] = std::exp(std::complex<double>(0.0, M_PI*k*k/size)) * a[k];

	delete[] a;
	delete[] b;
	delete[] c;
}

static void separate(const std::complex<double>* in, std::complex<double>* out, std::size_t radix, std::size_t size) {
	for (std::size_t i = 0; i < radix; ++i)
		for (std::size_t j = 0; j < size/radix; ++j)
			out[i*size/radix+j] = in[radix*j+i];
}

static void combine(const std::complex<double>* in, std::complex<double>* out, std::size_t radix, std::size_t size) {
	for (std::size_t i = 0; i < radix; ++i)
		for (std::size_t j = 0; j < size/radix; ++j)
			out[radix*j+i] = in[i*size/radix+j];
}

static void mixed_radix_fft(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	std::size_t radix = static_cast<std::size_t>(sqrt(size));
	while (size%radix) --radix;
	separate(in, out, radix, size);
	for (std::size_t n = 0; n < radix; ++n)
		fft(out + n*size/radix, in + n*size/radix, size/radix);

	combine(in, out, radix, size);
	for (std::size_t k = 0; k*radix < size; ++k) {
		for (std::size_t n = 0; n < radix; ++n)
			out[radix*k+n] *= std::exp(std::complex<double>(0, -2.0*M_PI*n*k/size));
		fft(out + radix*k, in + radix*k, radix);
	}
	separate(in, out, radix, size);
}

static void mixed_radix_ifft(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	std::size_t radix = static_cast<std::size_t>(sqrt(size));
	while (size%radix) --radix;
	separate(in, out, radix, size);
	for (std::size_t n = 0; n < radix; ++n)
		ifft(out + n*size/radix, in + n*size/radix, size/radix);

	combine(in, out, radix, size);
	for (std::size_t k = 0; k*radix < size; ++k) {
		for (std::size_t n = 0; n < radix; ++n)
			out[radix*k+n] *= std::exp(std::complex<double>(0, 2.0*M_PI*n*k/size));
		ifft(out + radix*k, in + radix*k, radix);
	}
	separate(in, out, radix, size);
}

/**
 * return {x, y} where a*x + b*y = gcd(a, b)
 */
static std::pair<std::size_t, std::size_t> extended_euclid(std::size_t a, std::size_t b) {
	std::size_t c;
	std::pair<std::size_t, std::size_t> aa = {1, 0}, bb = {0, 1};
	while (true) {
		c = a/b;
		a %= b;
		aa.first -= c*aa.second;
		bb.first -= c*bb.second;
		if (!a) return {aa.second, bb.second};
		c = b/a;
		b %= a;
		aa.second -= c*aa.first;
		bb.second -= c*bb.first;
		if (!b) return {aa.first, bb.first};
	}
}

void fft(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	if (size < 16) return dft(in, out, size);
	if ((size & (size-1)) == 0) return bit_reverse_fft(in, out, size);

	std::size_t N1 = static_cast<std::size_t>(sqrt(size));
	while (size%N1) --N1;
	if (N1 == 1) return bluesteins_algorithm(in, out, size);
	while (std::gcd(N1, size/N1) != 1) N1 *= std::gcd(N1, size/N1);
	std::size_t N2 = size/N1;
	if (N2 == 1) return mixed_radix_fft(in, out, size);

	auto [i_N1, i_N2] = extended_euclid(N1, N2);
	i_N1 = std::min(i_N1, N2+i_N1);
	i_N2 = std::min(i_N2, N1+i_N2);

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		for (std::size_t n2 = 0; n2 < N2; ++n2)
			out[n1*N2+n2] = in[(n1*N2 + n2*N1)%size];

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		fft(out + n1*N2, in + n1*N2, N2);

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		for (std::size_t n2 = 0; n2 < N2; ++n2)
			out[n2*N1+n1] = in[n1*N2+n2];

	for (std::size_t k2 = 0; k2 < N2; ++k2)
		fft(out+k2*N1, in+k2*N1, N1);

	for (std::size_t k2 = 0; k2 < N2; ++k2)
		for (std::size_t k1 = 0; k1 < N1; ++k1)
			out[(k1*i_N2*N2 + k2*i_N1*N1)%size] = in[k2*N1+k1];
}

void ifft(std::complex<double>* in, std::complex<double>* out, std::size_t size) {
	if (size < 16) return idft(in, out, size);
	if ((size & (size-1)) == 0) return bit_reverse_ifft(in, out, size);

	std::size_t N1 = static_cast<std::size_t>(sqrt(size));
	while (size%N1) --N1;
	if (N1 == 1) return inverse_bluesteins_algorithm(in, out, size);
	while (std::gcd(N1, size/N1) != 1) N1 *= std::gcd(N1, size/N1);
	std::size_t N2 = size/N1;
	if (N2 == 1) return mixed_radix_ifft(in, out, size);

	auto [i_N1, i_N2] = extended_euclid(N1, N2);
	i_N1 = std::min(i_N1, N2+i_N1);
	i_N2 = std::min(i_N2, N1+i_N2);

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		for (std::size_t n2 = 0; n2 < N2; ++n2)
			out[n1*N2+n2] = in[(n1*N2 + n2*N1)%size];

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		ifft(out + n1*N2, in + n1*N2, N2);

	for (std::size_t n1 = 0; n1 < N1; ++n1)
		for (std::size_t n2 = 0; n2 < N2; ++n2)
			out[n2*N1+n1] = in[n1*N2+n2];

	for (std::size_t k2 = 0; k2 < N2; ++k2)
		ifft(out+k2*N1, in+k2*N1, N1);

	for (std::size_t k2 = 0; k2 < N2; ++k2)
		for (std::size_t k1 = 0; k1 < N1; ++k1)
			out[(k1*i_N2*N2 + k2*i_N1*N1)%size] = in[k2*N1+k1];
}
