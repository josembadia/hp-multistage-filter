# High-Performance and Energy-Aware Multistage Linear-Phase Filter on Edge SoC Architectures

Reference C/OpenMP implementation of the high-performance multistage linear-phase equalizer presented in:

> **J. M. Badia, J. A. Belloch, and V. Välimäki**,  
> *"Efficient Sequential and Parallel Implementation of the Multistage Linear-Phase Filter"*,  
> Proceedings of the 29th International Conference on Digital Audio Effects (DAFx26), Cambridge, MA, USA, 2026.

## 1. Research Overview

This repository provides a computationally efficient and energy-aware implementation of a multistage linear-phase graphic equalizer.

The implementation is designed for **Edge SoC (System-on-Chip) architectures**, such as the NVIDIA Jetson Orin Nano, where real-time audio processing must be balanced with strict power-consumption constraints.

The main optimization strategies include:

* **Cache-friendly blocking**: sequential processing organized in blocks to improve temporal locality and reduce memory traffic.
* **Task-parallel pipelining**: OpenMP task-based parallelization with data dependencies (`depend` clauses) for asynchronous multicore execution.
* **Energy-aware evaluation**: support for execution-time and energy measurements on constrained embedded platforms.

The program benchmarks both the blocked sequential implementation and the task-parallel pipeline implementation, and then checks that both versions produce equivalent results within a numerical tolerance.

The repository ships with predefined prototype coefficients and gains so that the reference experiments can be reproduced and compared consistently. These defaults are not a structural limitation of the implementation: the number of filter stages is configurable from the command line, and the prototype taps and gain values can be changed in the corresponding initialization routines.

The code is also prepared to measure energy consumption using the power-performance analysis framework introduced in:

> **S. Barrachina et al.**,  
> *"An Integrated Framework for Power-Performance Analysis of Parallel Scientific Workloads"*,  
> Proceedings of the Third International Conference on Smart Grids, Green Communications and IT Energy-aware Technologies (ENERGY), 2013.

Energy measurements are performed through the `libpmlib` library, which must be installed and configured on the target platform.

## 2. Experimental Environment

The performance and energy characterization reported in the paper were obtained using the following platform:

* **Platform**: NVIDIA Jetson Orin Nano Developer Kit
* **CPU**: 6-core Arm Cortex-A78AE
* **OS**: Ubuntu 20.04.5 LTS
* **JetPack / L4T**: JetPack 5.1.1 / L4T 35.3.1
* **Compiler**: GCC 9.4.0
* **Compilation flags**: `-O3 -fopenmp -march=native`
* **Parallel programming model**: OpenMP task-based programming
* **Energy monitoring**: `libpmlib`, using the framework by Barrachina et al. for power-performance analysis

## 3. Repository Structure

```text
.
├── AUTHORS.rst          # Contributors, affiliations, and copyright notice
├── LICENSE              # GNU GPL v3.0 license text
├── LICENSE.rst          # Short license notice
├── Makefile             # Build system
├── README.md            # Project documentation
├── include/             # Header files and global configuration
│   ├── config.h
│   ├── filter_bank.h
│   ├── filter_kernels.h
│   ├── kernels.h
│   └── utils.h
├── src/                 # C/OpenMP source files
│   ├── main.c
│   ├── filter_bank.c
│   ├── filter_kernels.c
│   └── utils.c
├── scripts/             # Auxiliary scripts for frequency/energy experiments
└── results/             # Experimental timing and energy results
```

## 4. Requirements

To build and run the project, the following tools are required:

* `gcc`
* `make`
* OpenMP support, enabled through `-fopenmp`
* standard math library, linked through `-lm`

The provided `Makefile` also links against `libpmlib`, which is used for energy measurements:

```make
PMLIB_DIR = ${HOME}/pmlib
INCLUDES = -I$(INC_DIR) -I${PMLIB_DIR}/client
LIBS = ${PMLIB_DIR}/client/libpmlib.so
```

Therefore, the default build expects the `libpmlib` client library to be available at:

```text
$HOME/pmlib/client/libpmlib.so
```

`libpmlib` is part of the power-performance analysis framework introduced in:

> S. Barrachina et al.,  
> *"An Integrated Framework for Power-Performance Analysis of Parallel Scientific Workloads"*,  
> Proc. Third Int. Conf. Smart Grids, Green Communications and IT Energy-aware Technologies (ENERGY), 2013.

To use the energy-measurement functionality, `libpmlib` must be installed and configured on the target system. The corresponding measurement server must also be available according to the configuration used in the source code.

In the current implementation, energy measurement calls are conditionally compiled through the `ENERGY` macro in `include/config.h`:

```c
/* Energy measurement (commented out by default) */
//#define ENERGY
```

When `ENERGY` is enabled, the program initializes a `libpmlib` counter, starts the measurement before the parallel task-pipeline execution, stops it afterwards, retrieves the collected data, and writes the measurement output to:

```text
out.txt
```

The current source configuration uses a local `pmlib` server:

```c
pm_set_server((char*)"127.0.0.1", 6526, &server_INT);
```

and the platform label:

```c
Jetson-Orin-Nano
```

If `libpmlib` is not installed, compilation may fail at the linking stage because the default `Makefile` links against `libpmlib.so`. In that case, either install and configure `libpmlib`, or adapt the `Makefile` by removing the `pmlib` include and library references when energy measurements are not required.

## 5. Build Instructions

To compile the project, run:

```bash
make
```

This generates the executable:

```text
linear
```

To remove object files and the executable, run:

```bash
make clean
```

## 6. Running the Program

The executable is run from the command line as follows:

```bash
./linear -t <threads> -f <filters> -c <ncoef> -n <nsamples> -b <block_size>
```

The arguments are:

| Argument | Meaning | Default | Constraint |
|---|---|---:|---|
| `-t <threads>` | Number of OpenMP threads used by the task-parallel pipeline version | `1` | Positive integer |
| `-f <filters>` | Number of filter stages | `9` | `1 <= filters <= MAX_FILTERS` |
| `-c <ncoef>` | Number of FIR coefficients / prototype taps | `19` | Must be odd; the distributed initializer provides the 19-tap reference filter |
| `-n <nsamples>` | Number of input samples | `100000` | `1 <= nsamples <= MAX_SAMPLES` |
| `-b <block_size>` | Processing block size | `2048` | Must be a power of two |

The limits are defined in `include/config.h`:

```c
#define MAX_SAMPLES 10000000
#define MAX_FILTERS 100
#define MAX_COEF 200
```

The distributed version is configured for the reference case used in the experiments: `init_coef_b()` initializes a predefined 19-tap prototype filter, and `init_g()` initializes an alternating `+12 dB / -12 dB` gain pattern for validation. This makes it easy to compare the output against known reference results.

The implementation itself is more general. To use a different prototype filter, replace the coefficients defined in `init_coef_b()` in `src/utils.c` and run the program with the matching value of `-c`. The value of `ncoef` must remain odd because the implementation assumes a linear-phase FIR prototype with integer group delay `(ncoef - 1) / 2`. If more taps are required, increase `MAX_COEF` in `include/config.h` accordingly.

The input signal is generated internally for benchmarking and validation. In the current version, `init_samples()` initializes a unit impulse at sample index 0; no external audio or data file is required.

### Example

```bash
./linear -t 6 -f 9 -c 19 -n 100000 -b 2048
```

This runs the benchmark with:

* 6 OpenMP threads
* 9 filter stages
* 19 FIR coefficients
* 100000 input samples
* block size of 2048 samples

The program reports the average execution time of the sequential blocked version and the parallel task-pipeline version, then compares both outputs.

Example output format:

```text
Sequential blocked t: 1 f: 9 c: 19, n: 100000 b: 2048 time: ... s.
Parallel eq. t: 6 f: 9 c: 19, n: 100000 b: 2048 time: ... s.

Comparison of results: Results are identical (within tolerance).
```

## 7. Configuring the Filter Design

The default configuration is intended to reproduce the reference experiments. However, the code can be adapted in a straightforward way to evaluate other multistage linear-phase filter configurations.

The main parameters are:

* **Number of stages**: selected at run time with `-f <filters>`. The value must not exceed `MAX_FILTERS` in `include/config.h`.
* **Prototype FIR taps**: initialized in `init_coef_b()` in `src/utils.c`. Replace the `coef_b[]` array with the desired linear-phase prototype coefficients and run with the corresponding `-c <ncoef>` value.
* **Per-band gains**: initialized in `init_g()` in `src/utils.c`. The gain vector has `nfilters + 1` entries, corresponding to the bands generated by the multistage structure.
* **Maximum sizes**: controlled by `MAX_FILTERS`, `MAX_COEF`, and `MAX_SAMPLES` in `include/config.h`. Increase these values if the selected configuration requires more stages, more taps, or longer signals.

For example, to test a different 31-tap prototype filter, define 31 coefficients in `init_coef_b()`, update the validation check inside that function to accept `ncoef == 31`, ensure that `MAX_COEF >= 31`, and run:

```bash
./linear -t 6 -f 9 -c 31 -n 100000 -b 2048
```

Similarly, to evaluate a different number of filter stages, pass the desired value with `-f` and make sure that `init_g()` provides `nfilters + 1` gain values suitable for the experiment.

## 8. Benchmarking and Energy Experiments

A typical performance run is:

```bash
make clean
make
./linear -t 6 -f 9 -c 19 -n 100000 -b 2048
```

When reporting performance results, include at least:

* target platform and processor
* operating system and JetPack/L4T version, when applicable
* compiler version
* compilation flags
* number of OpenMP threads
* number of filters
* number of coefficients
* number of samples
* block size

For energy-aware experiments, the code is prepared to use `libpmlib`, the client library associated with the power-performance analysis framework described by Barrachina et al. This allows the execution of the parallel workload to be measured together with power/energy data gathered by the external monitoring infrastructure.

To enable energy measurements:

1. Install and configure the `pmlib` framework on the target platform.
2. Ensure that the client library is available at the path expected by the `Makefile`:

```text
$HOME/pmlib/client/libpmlib.so
```

3. Ensure that the corresponding `pmlib` server is running and reachable. The current code expects:

```text
127.0.0.1:6526
```

4. Enable the `ENERGY` macro in `include/config.h`:

```c
#define ENERGY
```

5. Recompile and run the benchmark:

```bash
make clean
make
./linear -t 6 -f 9 -c 19 -n 100000 -b 2048
```

When `ENERGY` is enabled, the program measures the parallel task-pipeline section and writes the collected `pmlib` data to:

```text
out.txt
```

The `scripts/` directory contains auxiliary scripts used for CPU-frequency and energy experiments. For example:

```bash
python3 scripts/test_freq.py <nthreads> <nfilters> <ncoef> <nsamples> <block_size>
```

This script iterates over several CPU frequencies, runs `./linear`, and stores the generated energy output files in `results/`.

These scripts assume a Linux system with CPU-frequency control tools, `sudo` permissions, and the `libpmlib`-based energy-measurement setup used in the original experiments.

When reporting energy results, include:

* target platform
* monitored power lines or sensors
* `pmlib` configuration
* CPU frequency
* number of OpenMP threads
* input size
* block size
* number of filter stages
* number of FIR coefficients
* whether measurements correspond to the sequential version, the parallel version, or a specific code region

## 9. License

This project is distributed under the terms of the GNU General Public License v3.0. See the `LICENSE` and `LICENSE.rst` files for details.

## 10. Authors

* **Jose M. Badia** — Universitat Jaume I
* **Jose A. Belloch** — Universidad Carlos III de Madrid
* **Vesa Välimäki** — Aalto University

See `AUTHORS.rst` for contact information and copyright details.

## 11. Citation

If you use this software in academic work, please cite:

```bibtex
@inproceedings{badia2026multistage,
  author    = {Badia, J. M. and Belloch, J. A. and Välimäki, V.},
  title     = {Efficient Sequential and Parallel Implementation of the Multistage Linear-Phase Filter},
  booktitle = {Proceedings of the 29th International Conference on Digital Audio Effects},
  address   = {Cambridge, MA, USA},
  year      = {2026}
}
```

If you use the energy-measurement support based on `libpmlib`, please also cite the framework introduced in:

```bibtex
@inproceedings{barrachina2013integrated,
  author    = {Barrachina, S. and others},
  title     = {An Integrated Framework for Power-Performance Analysis of Parallel Scientific Workloads},
  booktitle = {Proceedings of the Third International Conference on Smart Grids, Green Communications and IT Energy-aware Technologies},
  year      = {2013}
}
```
