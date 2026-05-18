#include "config.h"
#include "utils.h"
#include "filter_kernels.h"
#include "filter_bank.h"

/*
###########################################################################
#  Main execution flow: Argument parsing, initialization, benchmark       #
###########################################################################
*/

int main(int argc, char **argv) {
  int nthreads = 1;
  int nfilters = 9;
  int ncoef = 19;
  int nsamples = 100000;
  int block_size = 4096;
  int opt;

  // Command-line argument parsing
  while ((opt = getopt(argc, argv, "t:f:c:n:b:")) != -1) {
    switch (opt) {
      case 't': nthreads = atoi(optarg); break;
      case 'f': nfilters = atoi(optarg); break;
      case 'c': ncoef = atoi(optarg); break;
      case 'n': nsamples = atoi(optarg); break;
      case 'b': block_size = atoi(optarg); break;
      default:
        fprintf(stderr, "Usage: %s -t <threads> -f <filters> -c <ncoef> -n <nsamples> -b <block_size>\n", argv[0]);
        return 1;
    }
  }

  if (!is_power_of_two(block_size)) {
    fprintf(stderr, "Error: block_size (-b %d) must be a power of two.\n", block_size);
    return 1;
  }

  // Basic validation
  if (nfilters > MAX_FILTERS || nfilters < 1 || ncoef > MAX_COEF || ncoef < 1 || nsamples > MAX_SAMPLES || nsamples < 1 || ncoef % 2 != 1) { 
    fprintf(stderr, "Error: Input parameters out of range or ncoef is not odd.\n"); 
    return 1; 
  }
  
  float **x = NULL;
  float *b = NULL;
  float *g = NULL;  
  float *y_block = NULL;
  float *y_pipe = NULL;
  double start, end, tseq, tpar;

  // Total signal length for the test (including pipeline flush/alignment slack)
  int nsamples_pipe = nsamples + nfilters - 1; 

  // Memory allocation
  allocate_matrix(&x, nfilters + 1, nsamples_pipe);
  allocate_vector(&b, MAX_COEF);
  init_coef_b(b, ncoef);
  allocate_vector(&g, MAX_FILTERS + 1); 
  init_g(g, nfilters + 1);
  allocate_vector(&y_block, nsamples_pipe);
  allocate_vector(&y_pipe, nsamples_pipe);

  int ntimes = 3;

  init_samples(x[0], nsamples);
  start = omp_get_wtime(); 
  for (int times = 0; times < ntimes; times++) {
     linear_filter_seq_blocked(x[0], nsamples, nfilters, b, ncoef, g, y_block, block_size);
  }
  end = omp_get_wtime(); 
  tseq = (end - start)/ntimes;
  printf("Sequential blocked t: %d f: %d c: %d, n: %d b: %d time: %.6f s.\n", 
          1, nfilters, ncoef, nsamples, block_size, tseq);

  // 2) Parallel Pipeline Benchmark
  reset_system_state(x, nfilters, y_pipe, nsamples_pipe);
  init_samples(x[0], nsamples);

#ifdef ENERGY
     server_t server_INT;
     counter_t counter_INT;
     line_t lines_INT;
     int frequency_INT= 0;
     int aggregate_INT= 0;

     LINE_CLR_ALL(&lines_INT);
     pm_set_lines((char*)"0-2", &lines_INT);
     pm_set_server((char*)"127.0.0.1", 6526, &server_INT);
     pm_create_counter((char*)"Jetson-Orin-Nano", lines_INT, aggregate_INT, frequency_INT, server_INT, &counter_INT);
     
     pm_start_counter(&counter_INT);
#endif //ENERGY

  // Warming
//  linear_filter_tasks_pipeline(x[0], nsamples_pipe, nfilters, b, ncoef, g, y_pipe, nthreads, block_size);
  start = omp_get_wtime(); 
  for (int times = 0; times < ntimes; times++) {
//     linear_filter_tasks_pipeline_fused_load(x[0], nsamples_pipe, nfilters, b, ncoef, g, y_pipe, nthreads, block_size);
     linear_filter_tasks_pipeline(x[0], nsamples_pipe, nfilters, b, ncoef, g, y_pipe, nthreads, block_size);
  }
  end = omp_get_wtime(); 
  tpar = (end - start)/ntimes;

#ifdef ENERGY
    pm_stop_counter(&counter_INT);
    pm_get_counter_data(&counter_INT);

    pm_print_data_text("out.txt", counter_INT, lines_INT, -1);
    pm_finalize_counter(&counter_INT);
#endif //ENERGY

  printf("Parallel eq. t: %d f: %d c: %d, n: %d b: %d time: %.6f s.\n",
		  nthreads, nfilters, ncoef, nsamples, block_size, tpar);

  // Comparison of results
  printf("\nComparison of results: ");
  if (compare_vectors(y_block, y_pipe, nsamples_pipe, 1e-5, 1e-6)) {
    printf("Results are identical (within tolerance).\n");
  } else {
    printf("ERROR! Results are different.\n");
  }

  // Final cleanup
  free_matrix(&x, nfilters + 1);
  free(b);
  free(g);
  free(y_block);
  free(y_pipe);

  return 0;
}
