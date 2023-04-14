# Exercise 1

For a quick overview of the aim of this exercise see `README.md` in this directory's parent directory.

**NOTE**: we remind that by *configuration* we mean a given set of conditions, for instance: MKL library, single point
precision, EPYC node, fixed number of cores, spread cores configuration.


## What you will find in this repository

- This markdown file: an overview of the content of this folder and how to run it
- `gemm.c`: a code to call the gemm function and to measure its performance
- `Makefile`: a Makefile to compile `gemm.c` with different libraries and precisions
- `EPYC/`: a folder containing results gathered on ORFEO's EPYC nodes, organized in:
    - `cores_w_close_cores`
    - `cores_w_spread_cores`
    - `size_w_close_cores`
    - `size_w_spread_cores`
- `THIN/`: a folder containing results gathered on ORFEO's THIN nodes, organized in:
    - `cores_w_close_cores`
    - `cores_w_spread_cores`
    - `size_w_close_cores`
    - `size_w_spread_cores`

As you cas see, the structure of the two directories `EPYC/` and `THIN/` is exactly the same. The name of their subdirectories can be interpreted as `$(variable-condition)_w_$(threads-allocation-policy)`, for instance `cores_w_close_cores` in `THIN/` contains data gathered on THIN nodes with varying number of cores (at fixed size) with close-cores threads allocation policy.

Each subdirectory of `EPYC/` and `THIN/` contains the exact same files:

- `job.sh`
- `summary.out`
- blabla
