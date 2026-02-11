#ifndef UTILS_H
#define UTILS_H

#include "config.h"

/**
 * @brief Memory management utilities
 */
void allocate_vector(float **v, int max_size);
void allocate_matrix(float ***m, int max_rows, int max_cols);
void free_matrix(float ***m, int max_rows);

/**
 * @brief Initialization and state management
 */
void reset_system_state(float **x, int nfilters, float *y, int nsamples);
void init_samples(float *x, int nsamples);
void init_coef_b(float *b, int ncoef);
void init_g(float *g, int size);

/**
 * @brief Validation and Analysis
 */
int is_power_of_two(int x);
int compare_vectors(const float *v, const float *w, int size, float tol_abs, float tol_rel);
void show_vector(float *v, const char *name, int size);
void save_frequency_response(float *y, int nsamples, const char *filename);

#endif // UTILS_H
