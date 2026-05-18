#ifndef FILTER_BANK_H
#define FILTER_BANK_H

#include "config.h"

/**
 * @brief Sequential "sample-first" schedule.
 */
void linear_filter_seq_sample_first(float *x_input, int nsamples, int nfilters, 
                                    float *b, int ncoef, float *g, float *y);

/**
 * @brief Sequential blocked evaluation (cache-friendly).
 */
void linear_filter_seq_blocked(float *x_input, int nsamples, int nfilters, 
                               float *b, int ncoef, float *g, float *y, int block_size);

/**
 * @brief Task-parallel pipeline implementation using OpenMP.
 */
void linear_filter_tasks_pipeline(float *x_input, int nsamples, int nfilters, 
                                          float *b, int ncoef, float *g, float *y, 
                                          int nthreads, int block_size);

/**
 * @brief Task-parallel pipeline implementation fusing load and stage 1 of the filter using OpenMP.
 */
void linear_filter_tasks_pipeline_fused_load(float *x_input, int nsamples, int nfilters, 
                                          float *b, int ncoef, float *g, float *y, 
                                          int nthreads, int block_size);

#endif // FILTER_BANK_H
