#ifndef FILTER_KERNELS_H
#define FILTER_KERNELS_H

#include "config.h"

float FIR(float x_in, float *zs, int *zs_ptr, float *b, int ncoef, int s);
void update_output(float *y, float x_lp_out, float x_delayed_input, float *g, int f, int nfilters, int ncoef, int i, int nsamples);

#endif // FILTER_KERNELS_H
