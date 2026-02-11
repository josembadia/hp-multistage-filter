#!/usr/bin/env python3

import sys
import os

import subprocess

cpufreqs = [
    115200, 192000, 268800, 345600, 422400, 499200, 576000, 652800,
    729600, 806400, 883200, 960000, 1036800, 1113600, 1190400, 1267200,
    1344000, 1420800, 1497600, 1510400
]

def set_frequencies_and_run(nthreads, nfilters, ncoef, nsamples, block_size, cpufreqs):

#    print(f"nthreads: {nthreads} nfilters: {nfilters} ncoef: {ncoef} nsamples: {nsamples} block_size: {block_size}")

    for cpuf in cpufreqs:
        # Set CPU frequency
        subprocess.run(['sudo', 'cpufreq-set', '-c', '0', '-d', str(cpuf), '-u', str(cpuf)])
        subprocess.run(['sudo', 'cpufreq-set', '-c', '4', '-d', str(cpuf), '-u', str(cpuf)])

        # Convert CPU frequency to MHz
        cpu = cpuf / 1000

#        fichene = f"ene_linear_nth_seq_freq_{cpu:.0f}.txt"
        fichene = f"ene_linear_nth_{nthreads}_freq_{cpu:.0f}.txt"

        print(f"cpuf: {cpu:.2f} MHz")

        subprocess.run(["./linear", 
            "-t", str(nthreads), 
            "-f", str(nfilters), 
            "-c", str(ncoef), 
            "-n", str(nsamples), 
            "-b", str(block_size)])

        print("")

        # Move output files to results directory
        os.rename('out.txt', os.path.join('results', fichene))


if __name__ == "__main__":
    if len(sys.argv) != 6:
        print(f"Usage: {sys.argv[0]} <nthread> <nfilters> <ncoef> <nsamples> <block_size>")
        sys.exit(1)

    # Parse command-line arguments
    nthreads = int(sys.argv[1])
    nfilters = int(sys.argv[2])
    ncoef = int(sys.argv[3])
    nsamples = int(sys.argv[4])
    block_size = int(sys.argv[5])

    set_frequencies_and_run(nthreads, nfilters, ncoef, nsamples, block_size, cpufreqs)
