#include "filter_kernels.h"

/**
 * @brief Streaming IFIR-style low-pass FIR evaluation with strided history access.
 *
 * This function implements the stretched prototype filter without explicitly
 * constructing the zero-inserted impulse response. It keeps a circular state
 * buffer and accesses past samples with stride @p s.
 *
 * @param[in]     x_in   Current input sample to be processed.
 * @param[in,out] zs     Circular state buffer (stage history/delay line).
 * @param[in,out] zs_ptr Write pointer into @p zs (updated in-place after processing).
 * @param[in]     b      Prototype FIR filter coefficients.
 * @param[in]     ncoef  Number of prototype taps.
 * @param[in]     s      Stride (stretch factor) for the IFIR implementation (2^f).
 *
 * @return Low-pass output sample for the current stage.
 */
float FIR(float x_in, float *zs, int *zs_ptr, float *b, int ncoef, int s) {
    int L_f = (ncoef - 1) * s + 1; 
    float x_out = 0.0;
    
    // Write the new input sample into the circular state
    zs[*zs_ptr] = x_in; 

#if DEBUG > 2
    printf("  zs[%d] = x_in (%e)\n", *zs_ptr, x_in);
#endif
    
    // Convolution / dot product over the prototype taps (strided history access)
    for (int j = 0; j < ncoef; j++) {
        // The current sample is stored at the current write position (*zs_ptr)
        // and we go back in time in steps of s.
        int j_idx = (*zs_ptr - j * s + L_f) % L_f;  // Circular buffer
        x_out += zs[j_idx] * b[j]; 

#if DEBUG > 2
    printf("    x_out (%e) += zs[%d] (%e) * b[%d] (%e)\n",
                     x_out, j_idx, zs[j_idx], j, b[j]); 
#endif
    }
    
    // Advance the pointer (circular behavior)
    *zs_ptr = (*zs_ptr + 1) % L_f; 

    return x_out;
}

/**
 * @brief Applies the aligned accumulation rule for one stage contribution.
 *
 * The function computes the global alignment shift for stage @p f and scatters
 * the complementary contribution (and, at the last stage, the remaining low-pass)
 * into the output buffer.
 *
 * @param[in,out] y                Destination output buffer (global or thread-local).
 * @param[in]     x_lp_out         Stage low-pass output sample.
 * @param[in]     x_delayed_input  Delayed stage-input sample used for the complementary split.
 * @param[in]     g                Vector of gains per band.
 * @param[in]     f                Current stage index (0 to nfilters-1).
 * @param[in]     nfilters         Total number of stages in the filter bank.
 * @param[in]     ncoef            Number of prototype taps (used to calculate phase delay D).
 * @param[in]     i                Global input sample index (current time step).
 * @param[in]     nsamples         Total signal length for bounds checking.
 */
void update_output(float *y, float x_lp_out, float x_delayed_input, float *g, 
                   int f, int nfilters, int ncoef, int i, int nsamples) {
  
    int D = (ncoef - 1) / 2; // Phase delay of the individual FIR filter
  
    // -- Calculate summation position (Alignment delay) --
    // The alignment delay is needed because lower stages have larger total delay.
    // Alignment stride calculation: (2^nfilters - 2^(f+1)) * D
    int s_align = ( (1 << nfilters) - (1 << (f+1)) ) * D; 
    int pos = i + s_align;

    if (pos >= nsamples) return; 
  
    // Complementary/High-Pass Band (B_f)
    // Term: (Delayed - LP_Out) * G. Gain index: nfilters - f
    y[pos] += (x_delayed_input - x_lp_out) * g[nfilters - f];

#if DEBUG > 1
    printf("  y[%d] (%e) += (x_delayed_input (%e) - x_lp_out (%e)) * g[%d] (%e)\n",
                pos, y[pos], x_delayed_input, x_lp_out, nfilters-f, g[nfilters-f]); 
#endif

    // Final Low-Pass Term (B_N) - Applied only in the LAST stage (f == nfilters - 1)
    // Term: LP_Out * G. Gain index: nfilters - f - 1 (which is index 0)
    if (f == nfilters - 1) { 
        float x_band_lp_final = x_lp_out * g[nfilters - f - 1]; 
        y[pos] += x_band_lp_final;
#if DEBUG > 1
        printf("  y[%d] (%e) += x_lp_out (%e) * g[%d] (%e)\n", 
                   pos, y[pos], x_lp_out, nfilters-f-1, g[nfilters-f-1]);
#endif
    }
}
