# High-Performance and Energy-Aware Multistage Linear-Phase Filter on Edge SoC Architectures

Official C/OpenMP implementation of the high-performance multistage linear-phase equalizer presented in:

> **J. M. Badia, J. A. Belloch, and V. Välimäki**, *"Efficient Sequential and Parallel Implementation of the Multistage Linear-Phase Filter"*, Proceedings of the 29th International Conference on Digital Audio Effects (DAFx26), Cambridge, MA, USA, 2026.

## 1. Research Overview

This project provides a computationally efficient and energy-aware implementation of the octave-band graphic equalizer originally proposed by Bruschi et al. (2022). The software is specifically optimized for **Edge SoC (System-on-Chip) architectures**, such as the NVIDIA Jetson Orin Nano, balancing real-time audio constraints with strict power-consumption limits.



Key optimization strategies included:
* **Cache-Friendly Blocking**: Optimized sequential processing to maximize temporal locality.
* **Task-Parallel Pipelining**: An advanced OpenMP task-based model with data dependencies (`depend` clauses) for asynchronous multicore execution.

## 2. Experimental Environment

The performance and energy characterization reported in the paper were obtained using:
* **Platform**: NVIDIA Jetson Orin Nano Developer Kit (6-core Arm Cortex-A78AE).
* **OS**: Ubuntu 20.04.5 LTS (JetPack 5.1.1 / L4T 35.3.1).
* **Compiler**: GCC 9.4.0 (Flags: `-O3 -march=native`).
* **Parallelism**: OpenMP 4.5 Task-based programming.
* **Energy Monitoring**: `pmlib` framework sampling at 10 Hz.

## 3. Directory Structure

* `src/`: Implementation files (`main.c`, `filter_bank.c`, `filter_kernels.c`, `utils.c`).
* `include/`: API definitions and global configuration (`config.h`).
* `Makefile`: Multi-target build system.
* `LICENSE`: Full GNU GPL v3.0 legal text.
* `AUTHORS.rst`: List of contributors, contact information, and detailed copyright.

## 4. Build and Execution

To compile the project with high-level optimizations:
```bash
make
