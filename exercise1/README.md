# Exercise 1

For a quick overview of the aim of this exercise see [`README.md`](../README.md) in this directory's parent directory.

**NOTE**: we will use the acronym GOL to refer to game of life.


## What you will find in this directory

The current directory contains:
- This markdown file: an overview of the content of this folder and how to run it
- `src/`: a folder containing GOL source codes:
    - `serial_gol.c`: the serial version of GOL
    - `parallel_gol.c`: the main of the parallelized version of GOL
    - `gol_lib.c`: a file containing the definition of the functions used for parallel GOL
    - `gol_lib.h`: a header file containing the signatures of these functions
    - `parallel_gol_unique.c`: a version of parallelized GOL in which the evolution is directly performed inside the main (i.e. without using the functions defined in `gol_lib.c`)
- `images/`: a folder containing images of the system states produced by executables, in PGM format:
    - `snapshots/`: a folder containing dumps of the system taken with a variable frequency
    - other possible system states (e.g. initial conditions)
- `Makefile`: a makefile to compile all codes in `src/`
- `EPYC/`: a folder containing results gathered on ORFEO's EPYC nodes:
    - `openMP_scal/`
    - `strong_MPI_scal/`
    - `weak_MPI_scal/`
- `THIN/`: a folder containing results gathered on ORFEO's THIN nodes
    - `openMP_scal/`
    - `strong_MPI_scal/`
    - `weak_MPI_scal/`
- `analysis/`:
    - `analysis.ipynb`: a jupyter notebook used to carry out the analysis of the results

As you can see, `EPYC/` and `THIN/` have the very same structure: `openMP_scal/`, `strong_MPI_scal/` and `weak_MPI_scal/` contain, respectively, data collected for **openMP scalability**, **strong MPI scalability** and **weak MPI scalability** (for details see RIFERIMENTO AL PDF DI TORNATORE CON PAGINA).

All `EPYC/` and `THIN/`'s subdirectories have themselves the very same structure:
- `job.sh`: a bash script used to collect data on the cluster; the script is made to be run as a SLURM sbatch job on ORFEO
- `data.csv`: a CSV file containing the collected data for all kinds of evolution
- `summary.out`: an output file produced while running the job, which can be inspected to check whether it was run correctly
