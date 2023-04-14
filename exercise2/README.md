# Exercise 1

For a quick overview of the aim of this exercise see `README.md` in this directory's parent directory.

**NOTE**: we remind that by ***configuration*** we mean a given set of conditions, for instance: "MKL library, single point
precision, EPYC node, fixed number of cores, spread-cores threads affinity policy" is a possible configuration.


## What you will find in this repository

- This markdown file: an overview of the content of this folder and how to run it
- `gemm.c`: a code to call the *gemm* function and to measure its performance
- `Makefile`: a Makefile to compile `gemm.c` with different libraries and precisions
- `EPYC/`: a folder containing results gathered on ORFEO's EPYC nodes, organized in four subdirectories:
    - `cores_w_close_cores/`
    - `cores_w_spread_cores/`
    - `size_w_close_cores/`
    - `size_w_spread_cores/`
- `THIN/`: a folder containing results gathered on ORFEO's THIN nodes, organized in four subdirectories:
    - `cores_w_close_cores/`
    - `cores_w_spread_cores/`
    - `size_w_close_cores/`
    - `size_w_spread_cores/`

As you cas see, the structure of the two directories `EPYC/` and `THIN/` is exactly the same. The name of their subdirectories can be interpreted as `$(variable-condition)_w_$(threads-affinity-policy)/`, for instance `cores_w_close_cores/` in `THIN/` contains data gathered on THIN nodes with varying number of cores (at fixed size) with close-cores threads affinity policy.

Each subdirectory of `EPYC/` and `THIN/` contains the exact same files:

- `job.sh`: a bash script used to compile `gemm.c` in the desiderd configuration, run it and gather results on performance; the script is made to be ran as a SLURM sbatch job on ORFEO
- `summary.out`: a file that is produced each time job.sh is run, and that can be used to check whether the job was ran successfully until the end
- `mkl_f.csv`: a CSV file containing results for MKL in single point precision
- `oblas_f.csv`: a CSV file containing results for openBLAS in single point precision
- `blis_f.csv`: a CSV file containing results for BLIS in single point precision
- `mkl_d.csv`: a CSV file containing results for MKL in double point precision
- `oblas_d.csv`: a CSV file containing results for openBLAS in double point precision
- `blis_d.csv`: a CSV file containing results for BLIS in double point precision


### The job files

All job files have a very similar structure: ........ (parlare anche di summary.out)


## Running jobs in different configutations
