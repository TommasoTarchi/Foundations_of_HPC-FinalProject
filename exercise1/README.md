# Exercise 1

For a quick overview of the aim of this exercise see [`README.md`](../README.md) in this directory's parent directory. For a more detailed description see instead RIFERIMENTO AL PDF DI TORNATORE.

**NOTE**: we will use the acronym GOL to refer to game of life.

**SECOND NOTE**: we will use *process* when speaking about an MPI process and *thread* when speaking about an openMP thread.


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

**NOTE**: command-line arguments are passed to the following codes in the same way as described in RIFERIMENTO AL PDF DI TORNATORE CON PAGINA, exept for the playground size, which can be passed using `-m` for the vertical size (y) and `-k` for the horizontal one (x).

### Serial GOL

`serial_gol.c` is the first version of GOL we wrote. It is a simple serial C code which can be used to perform **ordered**, **static** and **static in place** evolution; static in place is the same as static but without using an auxiliary grid, therefore saving half of the memory. The evolution can start from any initialized playground of any size (i.e. any rectangular "table" of squared cells which can assume two states: *dead* or *alive*) and for any number of steps (*generations*). The kind of evolution to be perfomed is passed by command line. The code is also able to output a dump of the system every chosen by the user number of generations. All input and output files representing a state of the system are in the PGM format.

The code can also be used to initialise a random playground (with equal probability for dead and alive cell's initial status) with any name assigned. In both intialisation and evolution, if a specific name for the playground is not passed the default `game_of_life.pgm` is used.

If compiled with `-DTIME` the code prints to standard output the time it took to perform the evolution in seconds. The time measured is the total time of evolution, not a single generation's one, and its measure excludes time spent to read the initial playgorund and time spent to write the final system's state; in other words it includes only the evolution and the system dumping. It is measured using the function `clock_gettime`. 

Here we avoid speaking in details about initialisation and evolution, since they will be treated extensively in the RIFERIMENTO ALLA SEZIONE SU GOL_LIB.C section: they are similar in serial and parallel versions.

To read/write to/from PGM we used the functions already prepared for us, which can be found here RIFERIMENTO.

### Parallel GOL

Starting from `serial_gol.c`, we used MPI to parallelize both I/O and evolution, and openMP to further parallelize evolution, letting each procees spawn a number of threads. To mix MPI and openMP we chose the **funneled approach**, in which MPI calls can be done from within openMP parallel regions, but only by the master thread. This allowed us to write the system's dumps and to communicate among mpi processes whithout having to get out of the parallel region, and therefore avoiding the parallel regions "management" overhead.

As we said previously, the code is presented in two forms: one `parallel_gol.c` that uses functions defined in `gol_lib.c` for PGM header reading and evolution, and one `parallel_gol_unique.c` that has the very same functions for evolution "embedded". We present both versions because we cannot exclude the overheads caused at each generation by function calls to be not negligible on some systems. We did some tests on ORFEO and it seemed to be irrelevant (at least for small numbers of generations). In the following we will refer to `parallel_gol.c`, but, a part from the separation of evolution functions, the code is identical to `parallel_gol_unique.c`.

For time measurements we use the function `omp_get_wtime`, called by the master thread from within the parallel region. The use of `#pragma omp barrier` statements makes sure that the time measured is the actual one between the beginning and the end of the evolution, and not some kind of average among threads' times.

What follows is a brief exposition of the key points in the code. We removed some superfluous lines of code (like error checker stuff) and comments to make everything more readable.

### Initialisation

For each process a grid (i.e. a matrix, which is actually allocated as a vector) of BOOL variables is allocated, where BOOL is simply a renaming of the char type (the smallest standard type available in C). This grid's x size (i.e. the number of columns) is the same as the complete grid, while its y size (i.e. the number of rows) is chosen dividing the total grid's y size and assigning the remainder rows one to each process starting from the 0-th one. If, for instance, you have four processes and you pass to the program `-m 10 -k 25`, the processes's grids will be respectively 3x25, 3x25. 2x25 and 2x25. Here the related chunk of code (`my_id` indicates the id of the process):

````
const unsigned int n_cells = m*k;


/* distributing playground's rows to MPI processes */

unsigned int my_m = m/n_procs;   // number of rows assigned to the process
const int m_rmd = m%n_procs;
/* assigning remaining rows */
if (my_id < m_rmd)
    my_m++;
const int my_n_cells = my_m*k;   // number of cells assigned to the process

BOOL* my_grid = (BOOL*) malloc(my_n_cells*sizeof(BOOL));
````

To randomly initialise the grid we need first to set the seed. Since we want to have a different seed for each thread on each process, we use the function `time` to choose the 0-th process's seed and then broadcast it to all processes; then each process modifies the seed using its personal (and unique) id and finally each thread modifies it again according to its personal id, indicated by `my_thread_id`. Then, all cells of the grid are randomly chosen to be dead or alive (0 ascii code is dead and 1 ascii code is alive), and the work is splitted among threads using a parallel for loop with static scheduling.

````
/* setting the seed */

struct drand48_data rand_gen;

/* generating a unique seed for each process */
long int seed;
if (my_id == 0)   // in this way we are sure that the seed will be different for all processes
    seed = time(NULL);

check += MPI_Bcast(&seed, 1, MPI_LONG, 0, MPI_COMM_WORLD);
if (check != 0) {
    printf("\n--- AN ERROR OCCURRED WHILE BROADCASTING THE SEED ---\n\n");
    check = 0;
}
seed += my_id*n_threads;

#pragma omp parallel
 {
    /* setting a different seed for each thread */
    int my_thread_id = omp_get_thread_num();
    srand48_r(seed+my_thread_id, &rand_gen);

   #pragma omp for schedule(static)
    for (int i=0; i<my_n_cells; i++) {
        
        /* producing a random integer among 0 and 1 */
        double random_number;
        drand48_r(&rand_gen, &random_number);
        short int rand_bool = (short int) (random_number+0.5);
        
        /* converting random number to BOOL */
        my_grid[i] = (BOOL) rand_bool;
    }
 }
````

At this point the initial playground can be written to the corresponding PGM file, whose name is stored in `fname`. To do that, we used the specific MPI I/O functions. First, the file is formatted by the 0-th process, which creates (or overwrites) the file and writes its header; notice that we use 1 to represent the maximum value of the PGM file's color. Then, after the barrier has syncronized all processes, making sure that the file is formatted correctly, all processes write their own cells' states in parallel.

````
MPI_File f_handle;   // pointer to file
       
/* formatting the PGM file */

const int color_maxval = 1;

/* the third number added to header_size must be equal to the total length 
* in bytes of the header, spaces and new-line characters included and 
* length of m and k excluded */
const int header_size = m_length+k_length+7;

if (my_id == 0) {

    /* setting the header */
    char header[header_size];
    sprintf(header, "P5 %d %d\n%d\n", k, m, color_maxval);

    /* writing the header */
    int access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
    check += MPI_File_open(MPI_COMM_SELF, fname, access_mode, MPI_INFO_NULL, &f_handle);
    check += MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);
    check += MPI_File_close(&f_handle);
}


/* needed to make sure that all processes are actually 
* wrtiting on an already formatted PGM file */
MPI_Barrier(MPI_COMM_WORLD);


/* opening file in parallel */
int access_mode = MPI_MODE_WRONLY;
check += MPI_File_open(MPI_COMM_WORLD, fname, access_mode, MPI_INFO_NULL, &f_handle);

/* computing offsets */
int offset = header_size;
if (my_id < m_rmd) {
    offset += my_id*my_n_cells;
} else {
    offset += m_rmd*k + my_id*my_n_cells;
}

/* writing in parallel */
check += MPI_File_write_at_all(f_handle, offset, my_grid, my_m*k, MPI_CHAR, &status);
check += MPI_Barrier(MPI_COMM_WORLD);
check += MPI_File_close(&f_handle);
````

### Evolution

Independently from the chosen kind of evolution, in the first part of this code block the header of the initial playground's PGM file is read by the 0-th process using the function, obtained by modifying the function we were already given to read PGM files, called `read_pgm_header` in `gol_lib.c` (AGGIUNGERE RIFERIMENTO AL FILE).

The content of the header (in particular the header size, the maximum color value and the size of the playground) is then broadcasted to all processes using `MPI_Bcast`, and the workload (i.e. the playground) is divided among processes in the same way it was in RIFERIMENTO A SEZIONE PRECEDENTE. The only difference is that this time each process's allocated grid has two more rows to store the bordering rows of neighbor processes. The parallel reading from PGM is carried out in a way similar to parallel writing: for each process an offset is computed summing the header size and all the previous processes' grid's sizes, and starting from that position cells' states are read and stored in the process's own grid. In case of static non in place evolution an auxiliary grid (called `my_grid_aux`) is allocated and a pointer `temp` to switch he grids is defined.

The evolution is then carried out dividing equally the number of cells among threads, and letting each thread evolve its own "assigned" cells. After the parallel region is opened, useful quantities for threads are computed, for instance the number of cells that it has to evolve, the position of the first **edge cell** (meaning that it is on the edge of the grid) the thread will meet while evolving the cells and the first and last rows it will have to evolve completely from edge to edge:

````
/* computing number of cells for each thread */
int my_thread_n_cells = my_n_cells / n_threads;
const int thread_n_cells_rmd = my_n_cells % n_threads;
if (my_thread_id < thread_n_cells_rmd)
    my_thread_n_cells++;

/* computing beginning and end of threads' portion of grid */
int my_thread_start = x_size + my_thread_id * my_thread_n_cells;
if (my_thread_id >= thread_n_cells_rmd)
    my_thread_start += thread_n_cells_rmd;
const int my_thread_stop = my_thread_start + my_thread_n_cells;

/* computing first and last (excluded) 'complete' rows */
const int first_row = my_thread_start / x_size + (my_thread_start % x_size != 0);
const int last_row = my_thread_stop / x_size;
            
/* computing first and last nearest edge's positions */ 
int first_edge;
if (my_thread_start % x_size == 0) {   // first edge position
    first_edge = my_thread_start - 1;
} else {
    first_edge = first_row * x_size - 1;
}
````

At this point the code to be executed changes depending on the chosen kind of evolution. In any case it follows roughly these steps:
1. (Optional) system's dump writing
2. Communication of bordering cells among neighbor processes
3. Proper evolution
