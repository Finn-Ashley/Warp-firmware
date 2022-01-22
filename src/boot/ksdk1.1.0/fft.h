/* Fast Fourier Transform
 * Cooley-Tukey algorithm with 2-radix DFT
 */

#include <stdint.h>
#include <complex.h>

void fft_radix2(int* x, float complex* X, unsigned int N, unsigned int s);
void fft(int* x, float complex* X, uint16_t N);
void process_powers(float complex *fft_output, float *frequency_powers);
