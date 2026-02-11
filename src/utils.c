#include "utils.h"

/**
 * @brief Tests whether an integer is a power of two.
 *
 * @param[in] x Integer to test.
 * @return 1 if @p x is a power of two, 0 otherwise.
 */
int is_power_of_two(int x) {
  return (x > 0) && ((x & (x - 1)) == 0);
}

/**
 * @brief Allocates and zero-initializes a float vector.
 *
 * @param[out] v        Address of the vector pointer to allocate.
 * @param[in]  max_size Number of float elements.
 */
void allocate_vector(float **v, int max_size) {
  *v = (float *)calloc(max_size, sizeof(float));
  if (*v == NULL) {
    perror("Error allocating memory for the vector");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Allocates and zero-initializes a 2D float matrix (row-major pointers).
 *
 * @param[out] m        Address of the matrix pointer (array of row pointers).
 * @param[in]  max_rows Number of rows.
 * @param[in]  max_cols Number of columns per row.
 */
void allocate_matrix(float ***m, int max_rows, int max_cols) {
  *m = (float **)malloc(max_rows * sizeof(float *));
  if (*m == NULL) {
    perror("Error allocating memory for matrix rows");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < max_rows; i++) {
    (*m)[i] = (float *)calloc(max_cols, sizeof(float));
    if ((*m)[i] == NULL) {
      perror("Error allocating memory for matrix columns");
      // Clean up already allocated rows on failure
      for (int j = 0; j < i; j++) free((*m)[j]);
      free(*m);
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * @brief Frees a matrix previously allocated with allocate_matrix().
 *
 * @param[in,out] m        Matrix (array of row pointers).
 * @param[in]     max_rows Number of rows to free.
 */
void free_matrix(float ***m, int max_rows) {
  for (int i = 0; i < max_rows; i++) {
    free((*m)[i]);
  }
  free(*m);
  *m = NULL;
}

/**
 * @brief Resets the complete filtering state for a fresh run.
 *
 * @param[in,out] x         Input signal storage (intermediate stages 1..N).
 * @param[in]     nfilters  Number of stages.
 * @param[in,out] y         Output buffer to clear.
 * @param[in]     nsamples  Signal length.
 */
void reset_system_state(float **x, int nfilters, float *y, int nsamples) {
    /* Reset intermediate stage buffers (stages 1..N). 
       Note: stage 0 (x[0]) is the input signal and is usually kept. */
    for (int f = 1; f <= nfilters; f++) {
        memset(x[f], 0, (size_t)nsamples * sizeof(float));
    }

    /* Reset output buffer. */
    memset(y, 0, (size_t)nsamples * sizeof(float));
}

/**
 * @brief Initializes an input signal for benchmarking/validation.
 *
 * In this reference, it simply generates a unit impulse at index 0.
 *
 * @param[out] x        Input vector.
 * @param[in]  nsamples Number of samples.
 */
void init_samples(float *x, int nsamples) {
  memset(x, 0, nsamples * sizeof(float));
  x[0] = 1.0f; // Unit impulse at instant 0
}

/**
 * @brief Initializes the prototype FIR tap vector.
 *
 * The coefficients are hardcoded for a specific low-pass design (ncoef=19).
 *
 * @param[out] b      Tap vector.
 * @param[in]  ncoef  Number of prototype taps.
 */
void init_coef_b(float *b, int ncoef) {
  if (ncoef != 19) {
    fprintf(stderr, "Error: init_coef_b is designed for ncoef=19\n");
    exit(EXIT_FAILURE);
  }

  float coef_b[] = {
      0.003129, 0.000000, -0.013381, 0.000000, 0.035929,
      0.000000, -0.087175, 0.000000, 0.311578, 0.500000, 
      0.311578, 0.000000, -0.087175, 0.000000, 0.035929,
      0.000000, -0.013381, 0.000000, 0.003129
  };

  for (int i = 0; i < ncoef; i++) {
    b[i] = coef_b[i]; 
  }
}

/**
 * @brief Initializes the per-band gain vector.
 *
 * Alternates between +12dB and -12dB for testing purposes.
 *
 * @param[out] g    Gain vector (size nfilters + 1).
 * @param[in]  size Size of the gain vector.
 */
void init_g(float *g, int size) {
  float p12 = pow(10, (float) 12/20);  // 3.98107
  float p_12 = pow(10, (float) -12/20); // 0.25119

  for (int i = 0; i < size; i+=2) {
    g[i] = p12;
    if(i+1 < size) g[i+1] = p_12;
  }
}

/**
 * @brief Compares two vectors within absolute/relative tolerances.
 *
 * @param[in] v       Reference vector.
 * @param[in] w       Test vector.
 * @param[in] size    Vector length.
 * @param[in] tol_abs Absolute tolerance.
 * @param[in] tol_rel Relative tolerance.
 * @return 1 if equal within tolerance, 0 otherwise.
 */
int compare_vectors(const float *v, const float *w, int size,
                    float tol_abs, float tol_rel) {
    for (int i = 0; i < size; i++) {
        float a = v[i];
        float b = w[i];
        float diff = fabsf(a - b);
        float scale = fmaxf(fabsf(a), fabsf(b));

        if (diff > tol_abs && diff > tol_rel * scale) {
            printf("ERROR: v[%d] = %.10f, w[%d] = %.10f, dif = %e (abs), %e (rel)\n",
                   i, a, i, b, (double)diff, (double)(diff / (scale + 1e-30f)));
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Prints a vector (debug utility).
 *
 * @param[in] v    Vector to print.
 * @param[in] name Name label for printing.
 * @param[in] size Vector length.
 */
void show_vector(float *v, const char *name, int size) {
  printf("%s [0..%d]: ", name, size-1);
  for (int i = 0; i < size; i++) {
    printf("%.6f ", v[i]);
  }
  printf("\n");
}

/**
 * @brief Saves the frequency response to a text file (analysis utility).
 *
 * Computes a very basic DFT magnitude for visualization.
 *
 * @param[in] y        Output signal buffer.
 * @param[in] nsamples Number of samples.
 * @param[in] filename Output file path.
 */
void save_frequency_response(float *y, int nsamples, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;

    int N_fft = nsamples; 
    float fs = 44100.0; 

    for (int k = 0; k < N_fft / 2; k++) {
        float complex sum = 0;
        for (int n = 0; n < nsamples; n++) {
            float angle = -2.0 * M_PI * k * n / N_fft;
            sum += y[n] * (cos(angle) + I * sin(angle));
        }

        float magnitude = 20.0f * log10f(cabsf(sum) + 1e-9f); // Magnitude in dB
        float freq = (float)k * fs / N_fft;

        if (freq >= 20.0 && freq <= 20000.0) {
            fprintf(f, "%f %f\n", freq, magnitude);
        }
    }
    fclose(f);
}
