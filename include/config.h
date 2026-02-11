#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <complex.h> 
#include <getopt.h> 

/* Mathematical constants */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Macros */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* Limits */
#define MAX_SAMPLES 10000000
#define MAX_FILTERS 100
#define MAX_COEF 200

/* Debug level: 0=none, 3=complete */
#ifndef DEBUG
#define DEBUG 0 
#endif

/* Energy measurement (commented out by default) */
//#define ENERGY
#ifdef ENERGY
#include "pmlib.h"
#endif

#endif // CONFIG_H
