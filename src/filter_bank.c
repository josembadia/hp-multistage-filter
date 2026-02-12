#include <omp.h>
#include <string.h>
#include "filter_bank.h"
#include "filter_kernels.h"
#include "utils.h"

/**
 * @brief Sequential "sample-first" schedule (two nested loops).
 *
 * The outer loop iterates over input samples and the inner loop over stages.
 * Each input sample is propagated through the whole cascade before the next sample.
 * This is the classic "streaming" approach.
 *
 * @param[in]     x_input  Pointer to the input signal array.
 * @param[in]     nsamples Number of samples to process.
 * @param[in]     nfilters Number of filtering stages in the cascade.
 * @param[in]     b        Pointer to the prototype FIR filter coefficients.
 * @param[in]     ncoef    Number of coefficients in the prototype filter.
 * @param[in]     g        Vector of gains for each frequency band.
 * @param[in,out] y        Pointer to the output buffer where results are accumulated.
 */
void linear_filter_seq_sample_first(float *x_input, int nsamples, int nfilters, float *b, int ncoef, float *g, float *y)
{
    int D = (ncoef - 1) / 2;

    // Allocate and initialize per-stage streaming states (zs, zs_ptr)
    // Used as circular buffers
    float **zs = (float **)malloc(nfilters * sizeof(float *));
    int   *zs_ptr = (int *)calloc(nfilters, sizeof(int));

    // Precompute per-stage constants to keep the hot path minimal
    int *s_stage  = (int *)malloc(nfilters * sizeof(int));
    int *L_stage  = (int *)malloc(nfilters * sizeof(int));
    int *Df_stage = (int *)malloc(nfilters * sizeof(int));

    for (int f = 0; f < nfilters; f++) {
        int s   = 1 << f;
        int L_f = (ncoef - 1) * s + 1;
        int D_f = D * s;

        s_stage[f]  = s;
        L_stage[f]  = L_f;
        Df_stage[f] = D_f;

        zs[f] = (float *)calloc(L_f, sizeof(float));
    }

    // Process sample by sample
    for (int n = 0; n < nsamples; n++) {
        float x_stage_in = x_input[n]; 

        for (int f = 0; f < nfilters; f++) {
            int s   = s_stage[f];
            int L_f = L_stage[f];
            int D_f = Df_stage[f];

            // Access delayed input sample (for the split)
            int delay_idx = (zs_ptr[f] - D_f + L_f) % L_f;
            float x_delayed_input = zs[f][delay_idx];

            // Apply IFIR low-pass kernel
            float x_lp_out = FIR(x_stage_in, zs[f], &zs_ptr[f], b, ncoef, s);

            // Update output buffer at the correct aligned position
            update_output(y, x_lp_out, x_delayed_input, g, f, nfilters, ncoef, n, nsamples);

            // The output of this stage is the input of the next
            x_stage_in = x_lp_out;
        }
    }

    // Clean up
    for (int f = 0; f < nfilters; f++) free(zs[f]);
    free(zs); free(zs_ptr);
    free(s_stage); free(L_stage); free(Df_stage);
}

/**
 * @brief Sequential blocked evaluation (cache-friendly streaming schedule).
 *
 * Processes the input in blocks of @p block_size. For each block, all stages
 * are applied in order. This significantly improves temporal locality for
 * the prototype coefficients @p b and helps the filter state fit in cache.
 *
 * @param[in]     x_input    Pointer to the input signal array.
 * @param[in]     nsamples   Number of samples to process.
 * @param[in]     nfilters   Number of filtering stages in the cascade.
 * @param[in]     b          Pointer to the prototype FIR filter coefficients.
 * @param[in]     ncoef      Number of coefficients in the prototype filter.
 * @param[in]     g          Vector of gains for each frequency band.
 * @param[in,out] y          Pointer to the output buffer where results are accumulated.
 * @param[in]     block_size Size of the data block for cache optimization (must be power of 2).
 */
void linear_filter_seq_blocked(float *x_input, int nsamples, int nfilters, float *b, int ncoef, float *g, float *y, int block_size) {
    int D = (ncoef - 1) / 2; 
    
    // Per-stage streaming states
    float **zs = (float **)malloc(nfilters * sizeof(float *));
    int *zs_ptr = (int *)calloc(nfilters, sizeof(int)); 
    for (int f = 0; f < nfilters; f++) {
        int s = 1 << f; 
        int L_f = (ncoef - 1) * s + 1;
        zs[f] = (float *)calloc(L_f, sizeof(float)); 
    }

    // Ping-pong buffers for intermediate block samples
    float *buffer_A = (float *)malloc(block_size * sizeof(float));
    float *buffer_B = (float *)malloc(block_size * sizeof(float));

    // Loop over blocks of input samples
    for (int i_start = 0; i_start < nsamples; i_start += block_size) {
        int i_end = MIN(i_start + block_size, nsamples);
        int current_bsize = i_end - i_start;

        // Copy input to first buffer
        memcpy(buffer_A, &x_input[i_start], current_bsize * sizeof(float));

        // Cascade through stages for the current block
        for (int f = 0; f < nfilters; f++) {
            int s = 1 << f; 
            int L_f = (ncoef - 1) * s + 1;
            int D_f = D * s;

            float *in_ptr = buffer_A;
            float *out_ptr = buffer_B;

            for (int j = 0; j < current_bsize; j++) {
                int i_global = i_start + j;
                int delay_idx = (zs_ptr[f] - D_f + L_f) % L_f; 
                float x_delayed_input = zs[f][delay_idx];

                float x_lp_out = FIR(in_ptr[j], zs[f], &zs_ptr[f], b, ncoef, s);
                out_ptr[j] = x_lp_out;

                update_output(y, x_lp_out, x_delayed_input, g, f, nfilters, ncoef, i_global, nsamples);
            }

            // Swap ping-pong buffers for the next stage
            float *tmp = buffer_A;
            buffer_A = buffer_B;
            buffer_B = tmp;
        }
    }

    for (int f = 0; f < nfilters; f++) free(zs[f]);
    free(zs); free(zs_ptr);
    free(buffer_A); free(buffer_B);
}

/**
 * @brief Task-parallel pipeline implementation using OpenMP.
 *
 * This version divides the workload into blocks and uses OpenMP tasks with 
 * 'depend' clauses to create a software pipeline. Stages of the filter bank 
 * for different blocks are executed in parallel while respecting data dependencies.
 *
 * @param[in]     x_input    Pointer to the input signal array.
 * @param[in]     nsamples   Number of samples to process.
 * @param[in]     nfilters   Number of filtering stages in the cascade.
 * @param[in]     b          Pointer to the prototype FIR filter coefficients.
 * @param[in]     ncoef      Number of coefficients in the prototype filter.
 * @param[in]     g          Vector of gains for each frequency band.
 * @param[in,out] y          Pointer to the output buffer where results are accumulated.
 * @param[in]     nthreads   Number of OpenMP threads to use for execution.
 * @param[in]     block_size Size of the data block for task granularity (must be power of 2).
 */
void linear_filter_tasks_pipeline(float *x_input, int nsamples, int nfilters, float *b, int ncoef, float *g, float *y, int nthreads, int block_size) {
    int D = (ncoef - 1) / 2;
    int nblocks = (nsamples + block_size - 1) / block_size;

    // Sliding window buffer size (must be power of two and large enough for the pipeline depth)
    int W = 1;
    while (W < block_size * nthreads * 2) W <<= 1;
    int mask = W - 1;

    // Windowed buffers for samples between stages
    float **x_win = (float **)malloc((nfilters + 1) * sizeof(float *));
    for (int f = 0; f <= nfilters; f++) {
        x_win[f] = (float *)calloc(W, sizeof(float));
    }

    // Per-stage streaming states
    float **zs = (float **)malloc(nfilters * sizeof(float *));
    int *zs_ptr = (int *)calloc(nfilters, sizeof(int));
    for (int f = 0; f < nfilters; f++) {
        int s = 1 << f;
        int L_f = (ncoef - 1) * s + 1;
        zs[f] = (float *)calloc(L_f, sizeof(float));
    }

    // Thread-local output buffers (to avoid races on y accumulation)
    float **y_local = (float **)malloc(nthreads * sizeof(float *));
    for (int t = 0; t < nthreads; t++) {
        y_local[t] = (float *)calloc(nsamples, sizeof(float));
    }

    #pragma omp parallel num_threads(nthreads) // parallel region
    {
        /* * A single thread orchestrates task generation to prevent redundant work. 
           * Once tasks are queued, this thread also joins the rest of the pool as a 
           * consumer, executing tasks in parallel as their dependencies are met.
        */   
        #pragma omp single
        {
            for (int b_idx = 0; b_idx < nblocks; b_idx++) {
                int i_start = b_idx * block_size;
                int i_end = MIN(i_start + block_size, nsamples);
                int current_bsize = i_end - i_start;
                int win_offset = i_start & mask;

                // Task load block: Fetch/Copy input block
                #pragma omp task depend(out: x_win[0][win_offset]) firstprivate(i_start, current_bsize, win_offset)
                {
                    memcpy(&x_win[0][win_offset], &x_input[i_start], current_bsize * sizeof(float));
                }

                // Task Pipeline for stages
                for (int f = 0; f < nfilters; f++) {
                    int s = 1 << f;
                    int L_f = (ncoef - 1) * s + 1;
                    int D_f = D * s;

                    #pragma omp task depend(in: x_win[f][win_offset]) \
                                     depend(out: x_win[f+1][win_offset]) \
                                     depend(inout: zs[f]) \
                                     firstprivate(f, s, L_f, D_f, i_start, i_end, win_offset, current_bsize)
                    {
                        int tid = omp_get_thread_num();
                        float *in_ptr = &x_win[f][win_offset];
                        float *out_ptr = &x_win[f+1][win_offset];

                        for (int j = 0; j < current_bsize; j++) {
                            int i_global = i_start + j;
                            int delay_idx = (zs_ptr[f] - D_f + L_f) % L_f;
                            float x_delayed_input = zs[f][delay_idx];

                            float x_lp_out = FIR(in_ptr[j], zs[f], &zs_ptr[f], b, ncoef, s);
                            out_ptr[j] = x_lp_out;

                            update_output(y_local[tid], x_lp_out, x_delayed_input, g, f, nfilters, ncoef, i_global, nsamples);
                        }
                    }
                }

                // Task Final: Local accumulation/reduction to global y
		// After applying every stage to each block
                #pragma omp task firstprivate(i_start, i_end, nthreads) \
                                 depend(in: x_win[nfilters][win_offset]) \
                                 depend(inout: y[i_start])
                {
                    for (int pos = i_start; pos < i_end; pos++) {
                        for (int tid = 0; tid < nthreads; tid++) {
                            y[pos] += y_local[tid][pos];
                            y_local[tid][pos] = 0.0f; 
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    for (int f = 0; f <= nfilters; f++) free(x_win[f]);
    free(x_win);
    for (int f = 0; f < nfilters; f++) free(zs[f]);
    free(zs); free(zs_ptr);
    for (int t = 0; t < nthreads; t++) free(y_local[t]);
    free(y_local);
}
