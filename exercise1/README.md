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

**NOTE**: command-line arguments are passed to the following codes in the same way that is described in RIFERIMENTO AL PDF DI TORNATORE CON PAGINA, exept for the playground size, which can be passed using `-m` for the vertical size (y) and `-k` for the horizontal one (x).

### Serial GOL

`serial_gol.c` is the first version of GOL we wrote. It is a simple serial C code which can be used to perform ordered, static and static in place evolution; static in place is the same as static but without using an auxiliary grid, therefore saving half of the memory. The evolution can start from any initialized playground of any size (i.e. any rectangular "table" of squared cells that can assume two states: *dead* or *alive*) and for any number of steps (*generations*). The kind of evolution to be perfomed is passed by command line as well. The code is also able to output a dump of the system every chosen by the user number of generations. All input and output files representing a state of the system must be in the PGM format.

The code can also be used to initialise a random playground (with equal probability for dead and alive cell's initial status) with any name assigned. In both intialisation and evolution, if a specific name for the playground is not passed the default `game_of_life.pgm` is used.

If compiled with `-DTIME` the code prints to standard output the time it took to perform the evolution in seconds. The time measured is the total time, not a single step's one, and its measure excludes the time spent to read the initial playgorund and the time spent to write the final system's state; in other words it includes only the evolution and the system dumping. It is measured using the function `clock_gettime`. 

We avoid showing code snippets related to evolution here, since they will be treated extensively in the RIFERIMENTO ALLA SEZIONE SU GOL_LIB.C section: they are very similar in serial and parallel versions.

To read/write to/from PGM in this code we used a slightly modified version of the functions already prepared for us, which can be found here RIFERIMENTO.

### Parallel GOL

Starting from `serial_gol.c`, we used MPI to parallelize both I/O to PGM files and evolution, and openMP to further parallelize evolution. To mix MPI and openMP we chose the **funneled approach**, in which MPI calls can be done from within openMP parallel regions, but only by the master thread. This allowed us to write the system's dumps and to communicate among mpi processes whithout having to get out of the parallel region, and therefore avoiding the parallel region's "management" overhead.

As we said previously, the code is presented in two forms: one `parallel_gol.c` that uses functions defined in `gol_lib.c` for PGM header reading and evolution, and one `parallel_gol_unique.c` that has the very same functions for evolution "embedded". We present both versions because we cannot exclude the overheads caused at each generation by function calls to be not negligible on some systems. We did some tests on ORFEO and it seemed to be irrelevant (at least for small numbers of generations). In the following we will refer to `parallel_gol.c`, but, a part from the separation of evolution functions, the code is identical to `parallel_gol_unique.c`.

For time measurements we use the function `omp_get_wtime`, called by the master thread from within the parallel region. The use of `#pragma omp barrier` makes sure that the one measured is the actual time between the beginning and the end of evolution.

What follows is a brief sketch of the code:

1. 
