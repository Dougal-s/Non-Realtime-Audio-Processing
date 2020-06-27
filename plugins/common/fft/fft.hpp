#pragma once
#include <cstddef>
#include <complex>

// Separates the output of an fft with input l+ri into separate left and right channels
void split_channels(std::complex<double>* in,
                    std::complex<double>* left,
                    std::complex<double>* right,
                    std::size_t size);

// 
void join_channels(std::complex<double>* left,
                   std::complex<double>* right,
                   std::complex<double>* out,
                   std::size_t size);

void fft(std::complex<double>* in, std::complex<double>* out, std::size_t size);
void ifft(std::complex<double>* in, std::complex<double>* out, std::size_t size);
