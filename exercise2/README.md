# Exercise 1

For a quick overview of the aim of this exercise see `README.md` in this directory's parent directory.

**NOTE**: we remind that by *configuration* we mean a given set of conditions, for instance: MKL library, single point
precision, EPYC node, fixed number of cores, spread cores configuration.


## What you will find in this repository

- `gemm.c`: a code to call the gemm function and to measure its performance
- `Makefile`: a Makefile to compile `gemm.c` with different libraries and precisions
- `EPYC`: a folder containing results gathered on ORFEO's EPYC nodes, containing:
    - `cores_w_close_cores`
    - pippop
- `THIN`: a folder containing results gathered on ORFEO's THIN nodes
