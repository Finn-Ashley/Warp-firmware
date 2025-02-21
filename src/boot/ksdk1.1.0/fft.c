/* Fast Fourier Transform
 * Cooley-Tukey algorithm with 2-radix DFT
 * Low memory embedded implementation from: https://github.com/brendanashworth/fft-small
 */

#include <complex.h>
#include <stdint.h>

#include "devADC.h"

#define PI 3.14159265358979323846

void fft_radix2(int* x, float complex* X, unsigned int N, unsigned int s) {
    unsigned int k;
    double complex t;

    // At the lowest level pass through (delta T=0 means no phase).
    if (N == 1) {
        X[0] = x[0];
        return;
    }

    // Cooley-Tukey: recursively split in two, then combine beneath.
    fft_radix2(x, X, N/2, 2*s);
    fft_radix2(x+s, X + N/2, N/2, 2*s);

    for (k = 0; k < N/2; k++) {
        t = X[k];
        X[k] = t + cexp(-2 * PI * I * k / N) * X[k + N/2];
        X[k + N/2] = t - cexp(-2 * PI * I * k / N) * X[k + N/2];
    }
}

void fft(int* x, float complex* X, unsigned int N) {
    fft_radix2(x, X, N, 1);
}

/*
 * Takes magnitude of FFT complex returns to find 'quantity' of each
 * frequency bin. Added by myself for program needs.
 */
void process_powers(float complex *fft_output, float *frequency_powers){
    for(uint8_t i = 0; i < NUMBER_OF_FREQS - IGNORED_FREQS; i++){
			
		frequency_powers[i] = (creal(fft_output[i + SHUNT])*creal(fft_output[i + SHUNT]) + cimag(fft_output[i + SHUNT])*cimag(fft_output[i + SHUNT]));
    }
}