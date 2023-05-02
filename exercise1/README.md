# Exercise 1

For a quick overview of the aim of this exercise see [`README.md`](../README.md) in this directory's parent directory. For a more detailed description see instead RIFERIMENTO AL PDF DI TORNATORE.

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


## Source codes

For a description of the requested purpose of these codes see RIFERIMENTO AL PDF DI TORNATORE, while for a detailed description of these codes themselves see RIFERIMENTO AL REPORT.

**NOTE**: command-line arguments are passed to the following codes in the same way that is described in RIFERIMENTO AL PDF DI TORNATORE CON PAGINA, exept for the playground size, which in these codes can be passed using `-m` for the vertical size (y) and `-k` for the horizontal one (x).

### `serial_gol.c`

This is the first version of GOL we wrote. It is a simple serial C code which can be used to perform static, ordered and ordered in place evolution (see RIFERIMENTO AL REPORT CON PAGINA for details), starting from any initialized playground of any size (i.e. any rectangular "table" of squared cells that can assume two states: *dead* or *alive*) and for any number of steps (*generations*). The kind of evolution to be perfomed is passed by command line as well. The code is also able to output a dump of the system every chosen by the user number of steps of the evolution. All input and output files representing a state of the system must be in the PGM format.

The code can also be used to initialise a random playground (with equal probability for dead and alive cell's initial status) with any name assigned.

If compiled with `-DTIME` the code prints to standard output the time it took to perform the initialisation or the evolution in seconds. In case of evolution the time measured is the total time, not a single step's one.

We avoid showing code snippets related to evolution here, since they will be treated extensively in the RIFERIMENTO ALLA SEZIONE SU GOL_LIB.C section: they are very similar in serial and parallel versions.

To read/write to/from PGM in this code we used the functions already prepared for us, which can be found here RIFERIMENTO.

In any case, if a specific name for the playground is not passed the defult `game_of_life.pgm` is used.

### `parallel_gol.c`

This is the final parallel version of GOL. It uses MPI to parallelize both I/O to PGM files and evolution, and openMP to further parallelize evolution.


## How to actually run jobs
