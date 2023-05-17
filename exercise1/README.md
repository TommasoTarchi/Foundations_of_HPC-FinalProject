# Exercise 1

For a quick overview of the aim of this exercise see [`README.md`](../README.md) in this directory's parent directory. For a more detailed description see instead [this file][link1] in the original course repository.

**NOTE**: we will use the acronym GOL to refer to game of life.

**SECOND NOTE**: we will use *process* when speaking about an MPI process and *thread* when speaking about an openMP thread.


## What you will find in this directory

The current directory contains:
- This markdown file: an overview of the content of this folder and how to run it
- `src/`: a folder containing GOL source codes:
    - `serial_gol.c`: the serial version of GOL
    - `parallel_gol.c`: the main of the parallel version of GOL
    - `gol_lib.c`: a file containing the definition of the functions used for parallel GOL
    - `gol_lib.h`: a header file containing the signatures of these functions
    - `parallel_gol_unique.c`: a version of parallel GOL in which the evolution is directly performed inside the main (i.e. without using the functions defined in `gol_lib.c`)
- `images/`: a folder containing images of the system states produced by executables, in PGM format:
    - `snapshots/`: a folder containing dumps of the system taken with a variable frequency
    - other possible system states (e.g. initial conditions)
- `Makefile`: a makefile to compile all codes in `src/`
- `EPYC/`: a folder containing data collected on ORFEO's EPYC nodes:
    - `openMP_scal/`
    - `strong_MPI_scal/`
    - `weak_MPI_scal/`
- `THIN/`: a folder containing data collected on ORFEO's THIN nodes
    - `openMP_scal/`
    - `strong_MPI_scal/`
    - `weak_MPI_scal/`
- `analysis/`:
    - `analysis.ipynb`: a jupyter notebook used to carry out the analysis of the results
    - various plots obtained with this notebook

As you can see, `EPYC/` and `THIN/` have the very same content: `openMP_scal/`, `strong_MPI_scal/` and `weak_MPI_scal/` contain, respectively, data collected for **openMP scalability**, **strong MPI scalability** and **weak MPI scalability** (for details see the [readme file][link2] in the original course directory).

All `EPYC/` and `THIN/`'s subdirectories have themselves the very same content:
- `job.sh`: a bash script used to collect data on the cluster; the script is made to be run as a SLURM sbatch job on ORFEO
- `data.csv`: a CSV file containing data collected for all kinds of evolution
- `summary.out`: an output file produced while running the job, which can be inspected to check whether it was run correctly


## Source codes

For a description of the expected functionalities of these codes see again the original [exercise document][link1], while for a detailed description of these codes themselves see [Report.pdf](../Report.pdf).

**NOTE**: command-line arguments are passed to the following codes in the same way as described in the [exercise document][link1], except for the playground size, which can be passed using `-m` for the vertical size (y) and `-k` for the horizontal one (x).

### Serial GOL

[`serial_gol.c`](src/serial_gol.c) is the first version of GOL we wrote. It is a simple serial C code which can be used to perform **ordered**, **static** and **static in place** evolution; static in place is the same as static but without using an auxiliary grid, therefore saving half of the memory, and using two different bits of the char representing each cell to store the old and the new state. The evolution can start from any initialized playground of any size (i.e. any rectangular "table" of squared cells which can assume two states: *dead* or *alive*) and for any number of steps (*generations*). The kind of evolution to be performed is passed by command line. The code is also able to output a dump of the system every chosen by the user number of generations, and it always outputs the final state of the system under the name of `final_state.pgm`. All input and output files representing a state of the system are in the PGM format.

The code can also be used to initialise a random playground (with equal probability for dead and alive initial state for each cell) with any name assigned. In both intialisation and evolution, if a specific name for the playground is not passed the default `game_of_life.pgm` is used. **Notice** that default read or written images (e.g. `game_of_life.pgm` initialised if no name is passed) are placed in the `images/` directory, however, the program is perfectly capable of initialising and reading images from elsewhere; in any case, the name has to be passed with the **complete relative path**.

If compiled with `-DTIME` the code prints to standard output the time it took to perform the evolution in seconds. The time measured is the total time of evolution, not a single generation's one, and its measure excludes time spent to read the initial playgorund and time spent to write the final system's state; in other words it includes only the evolution and the system dumping. It is measured using the function `clock_gettime`. 

Here we avoid speaking in details about initialisation and evolution, since they will be treated extensively in the [related sections](#ref1): their serial version is just a simplified one of the parallel.

To read/write to/from PGM we used the functions already prepared for us, which can be found [here][link3].


### Parallel GOL

Starting from `serial_gol.c`, we used MPI to parallelize I/O, initialisation and evolution, and openMP to further parallelize initialisation and evolution, letting each process spawn a number of threads. To mix MPI and openMP we chose the **funneled approach**, in which MPI calls can be done from within openMP parallel regions, but only by the master thread. This allowed us to write the system's dumps and to communicate among mpi processes whithout having to get out of the parallel region, and therefore avoiding the parallel regions "management" overhead.

As we said previously, the code is presented in two forms: one [`parallel_gol.c`](src/parallel_gol.c) that uses functions defined in [`gol_lib.c`](src/gol_lib.c) for PGM header reading and evolution, and one [`parallel_gol_unique.c`](src/parallel_gol_unique.c) that has the very same functions for evolution "embedded". We present both versions because we cannot exclude the overhead caused by function calls to be not negligible on some systems. We did some tests on ORFEO and it seemed to be irrelevant, at least for small numbers of generations. In the following we will refer to `parallel_gol.c`, but, a part from the separation of evolution functions, the code is identical to `parallel_gol_unique.c`.

For time measurements we use the function `omp_get_wtime`, called by the master thread from within the parallel region. The use of `#pragma omp barrier` statements makes sure that the time measured is the actual one between the beginning and the end of the evolution, and not some kind of average among threads' times. If compliled with `-DTIME`, in addition to be printed to standard output, the measured time is also printed to a file called `data.csv`.

What follows is a brief exposition of the key points in the code. We removed some superfluous lines of code (like error checker stuff) and some comments to make everything more readable.


<a name="ref1">
</a>

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

To randomly initialise the grid we first need to set the seed. Since we want to have a different seed for each thread on each process, we use the function `time` to choose the 0-th process's seed and then broadcast it to all processes; then each process modifies the seed using its personal (and unique) id and finally each thread modifies it again according to its personal id, indicated by `my_thread_id`. Then, all cells of the grid are randomly chosen to be dead or alive (0 ascii code is dead and 1 ascii code is alive), and the work is splitted among threads using a parallel for loop with static scheduling.

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

At this point the initial playground can be written to the corresponding PGM file, whose name is stored in `fname`. To do that, we used the specific MPI I/O functions. First, the file is formatted by the 0-th process, which creates (or overwrites) the file and writes its header; notice that we use 1 to represent the maximum value of the PGM file's color. Then, after the barrier has syncronized all processes making sure that the file is formatted correctly, all processes write their own cells' states in parallel.

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

In running mode, the kind of evolution can be chosen by setting the `-e` option to `0` for ordered evolution, `1` for static evolution and `2` for static in place evolution.

Independently from the chosen kind of evolution, first of all the header of the initial playground's PGM file is read by the 0-th process using the function called `read_pgm_header` in `gol_lib.c`, obtained by modifying the function we were already given to read PGM files.

The content of the header (in particular the header size, the maximum color value and the size of the playground) is then broadcasted to all processes using `MPI_Bcast`, and the workload (i.e. the playground) is divided among processes in the same way it was in the [previous section](#ref1). The only difference is that this time each process's allocated grid has two more rows to store the bordering rows of neighbor processes. The parallel reading from PGM is carried out in a way similar to parallel writing: for each process an offset is computed summing the header size and all the previous processes' grid's sizes, and starting from that position cells' states are read and stored in the process's own grid. In case of static non in place evolution an auxiliary grid (called `my_grid_aux`) is allocated and a pointer `temp` to switch the two grids is defined.

The evolution is then carried out dividing equally the number of cells among threads, and letting each thread evolve its own "assigned" cells. After the parallel region is opened, quantities needed by threads are computed; for instance the number of cells that it has to evolve, the position of the first **edge cell** (i.e. on the edge of the grid) the thread will meet while evolving the cells and the first and last rows it will have to evolve completely from edge to edge:

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

The following part of the code, independetly from the chosen kind of evolution, can be summarized in the following steps executed at each generation:
1. (Optional) system's dump writing
2. Communication of bordering cells among neighbour processes
3. Proper evolution

Step 1. is identical for all evolutions and it is basically the same as parallel writing described in the [section](#ref1) about initialisation.

For what concerns steps 2. and 3., they change depending on the chosen kind of evolution:

#### - Ordered evolution

Ordered evolution is intrinsically serial, therefore evolution and communications have to be performed sequentially, that is iterating over processes and then over threads in ascending rank order. So, **for each process** (in sequential order):
- The two neighbour processes communicate their bordering rows to the "evolving" process (this is done by the master thread):

    ````
    if (n_procs > 1) {

        if (proc == 0) {
            if (my_id == n_procs-1)
                check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);

            if (my_id == 1)    
                check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

            if (my_id == 0) {
                check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
            }

        } else if (proc == n_procs-1) {
        
            if (my_id == n_procs-2)
                check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                        
            if (my_id == 0)
                check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

            if (my_id == n_procs-1) {
                check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
            }
                            
        } else if (n_procs > 2) {

            if (my_id == proc-1) {
                check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);

            } else if (my_id == proc+1) { 
                check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
        
            } else if (my_id == proc) {
                check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
            }

        }

                            
        /* case of single MPI process */
        } else {
            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

        }
    }
    ````
    
    (The way we defined tags and targets in communications is not relevant at this level, so it was skipped).
    
- Using an `omp for` loop with `ordered` attribute (meaning that the iteration will be performed sequentially over threads) each thread carries out the evolution of its own cells using the function `ordered_evo`, which can be found in [`gol_lib.c`](src/gol_lib.c).

Notice that in case of a single process, the communication is done halfway through the evolution (inside `ordered_evo`). That is because in this case the first row of the grid must be updated considering the "old" state of the last row, while the last one must be updated considering the "new" state of the first row.

#### - Static evolution

On the other hand, static and static in place evolutions can be done in parallel by all processes, in particular:
- Each process communicates its upper and lower rows to its neighbour processes and receives the bordering rows from them:

    ````
    /* communicating border cells' status to neighbor processes */ 

    if (my_id % 2 == 0) {

    check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
    check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
    check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
    check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

    } else {
   
    check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
    check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
    check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
    check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                          
    }
    ````

    Basically, even processes first send and then receive, while odd processes first receive and then send. This is, in our opinion, the best way to carry out the communications, since it minimizes the time the "slowest" process waits for data (especially if the number of processes is even).

- For each process (operating in parallel) the spawned threads update cells' state in parallel using the functions `static_evo` and `static_evo_in_place` of [`gol_lib.c`](src/gol_lib.c), depending on the chosen kind of evolution. After the state is updated for all cells, in case of simple static evolution pointers to the two grids (one storing the old and and one storing the new state) are swapped using `temp`:
    
    ````
    #pragma omp barrier
    #pragma omp master
     {                 
        temp = my_grid;
        my_grid = my_grid_aux;
        my_grid_aux = temp;
        temp = NULL;
     }
    ````
    
    while in case of static in place evolution we simply increment the "state signaling bit", which keeps track of whether the new state is stored in the first or in the second bit (starting from the end of the BOOL):
    
    ````
    #pragma omp barrier
    #pragma omp master
     {
        bit_control++;
     }
    ````
    
    
## Makefile
   
It is a very simple file that can be used to compile both the serial and the parallel versions of GOL, in a target directory whose path can be passed as a variable called `$data_folder`, which is actually renamed `$target_path` inside the file. The source codes will be looked for in the `src/` folder.

The file contains four rules:
- `serial_gol`, to compile `serial_gol.c`:

    ````
    serial_gol: $(target_path)/serial_gol.x

    $(target_path)/serial_gol.x: src/serial_gol.c
	    gcc -DTIME $^ -o $@
    ````

- `parallel_gol`, to compile `parallel_gol.c` or `parallel_gol_unique.c`:

    ````
    parallel_gol: $(target_path)/parallel_gol.x

    $(target_path)/parallel_gol.x: src/parallel_gol.c src/parallel_gol_unique.c src/gol_lib.c
	mpicc -fopenmp -DTIME src/parallel_gol.c src/gol_lib.c -c
	ar -rc libgol.a *.o
	mpicc -fopenmp -DTIME src/parallel_gol.c -L. -lgol -o $@
	#mpicc -fopenmp -DTIME src/parallel_gol_unique.c -o $@
    ````
    
    (to compile `parallel_gol_unique.c` you have to uncomment the bottom line and comment the previous three).

- `clean_exe`, to clean the `target_path` and the current directory from executables, libraries and object files:

    ````
    clean_exe:
	    rm $(target_path)/*.x *.o *.a
    ````

- `clean_images`, to clean `images/` from all PGM files:

    ````
    clean_images:
	    rm images/snapshots/*.pgm
	    rm images/*.pgm
    ````

The first two rules can be called together using `gol_all` and the last two using `clean_all`.


## Job files

All job files have a similar structure.

The first block of istructions is ignored by bash and constitutes the resource request addressed to SLURM. For example let's look at `EPYC/openMP_scal/job.sh`:

````
#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="openMP_scal"
#SBATCH --partition=EPYC
#SBATCH -N 1
#SBATCH -n 128
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output="summary.out"
````

Here, for instance, we are requesting an entire (all of its 128 cores) EPYC node (-N 1, -n 128, --partition=EPYC) for a maximum amount of time of two hours (--time=02:00:00). We are also asking SLURM to save the outputs of the job in a file called `summary.out`. This file can later be read to check that the program was actually run correctly, using the correct amount of cores and without raising any error.

This part is different in `strong_MPI_scal/job.sh` and `weak_MPI_scal/job.sh` only in the requested resources: in that case we ask for two entire nodes. Also, when running on THIN nodes you have to change resource requests remembering that each node has 24 cores.

The following part is identical for all job files, a part from the architecture which is Intel for THIN nodes. In order we:
1. load the needed modules (architecture and openMPI)
2. set the threads affinity policy, using the environment variables `OMP_PLACES` and `OMP_PROC_BIND`
3. compile `parallel_gol.c` in the current directory (referred as `$datafolder`)

````
echo LOADING MODULES...
echo
module load architecture/AMD
module load openMPI/4.1.4/gnu/12.2.1

echo SETTING THREADS AFFINITY POLICY...
echo
alloc=close
export OMP_PLACES=cores
export OMP_PROC_BIND=$alloc

echo COMPILING EXECUTABLES...
echo
datafolder=$(pwd)
cd ../..
make parallel_gol data_folder=$datafolder
cd $datafolder
````

Messages printed to screen via `echo` are there only to make `summary.out` more readable.

In the following we will show examples from EPYC, but they are similar for THIN: basically, the only thing that changes is the maximum number of cores per socket from 64 to 12.

Then we set other variables needed (some to run the computation, others to write a sort of "header" of the data files), and we create/overwrite the data file called `data.csv` (but here addressed as `$datafolder`). For exemple here is again `openMP_scal/job.sh`:

````
### setting variables for executables and csv file
node=EPYC
scal=openMP
mat_x_size=25000
mat_y_size=25000
n_gen=5
n_procs=2
````

As you can see, here we fix the matrix size but not the number of threads, while in `strong_MPI_scal/job.sh` it is the other way around (and the number of threads is fixed in such a way that sockets are saturated):

````
node=EPYC
scal=strong_MPI
n_gen=5
n_threads=64
````

while in `weak_MPI_scal/job.sh` we fix the number of threads and we define a "unit" matrix size used to obtain at each iteration a workload proportional to the number of sockets involved:

````
node=EPYC
scal=weak_MPI
unit_mat_size=10000
n_gen=5
n_threads=64
````

Now we create/overwrite the file to store data, called `data.csv` but referred here using the variable `$datafile`. We initialise it by writing few lines summing up the conditions in which measures are performed, for example in `openMP_scal/job.sh`:

````
echo CREATING/OVERWRITING CSV FILE...
echo
datafile=$datafolder/data.csv
echo "#,,," > ${datafile}
echo "#node:,${node},," >> $datafile
echo "#scalability:,${scal},," >> $datafile
echo "#performance_measure:,time(s),," >> $datafile
echo "#threads_affinity_policy:,${alloc},," >> $datafile
echo "#playground_x_size:,${mat_x_size},," >> $datafile
echo "#playground_y_size:,${mat_y_size},," >> $datafile
echo "#generations:,${n_gen},," >> $datafile
echo "#sockets:,2,," >> $datafile
echo "#,,," >> $datafile
echo "#,,," >> $datafile
echo "threads_per_socket,ordered,static,static_in_place" >> $datafile
````

At this point we can execute the proper computation.

For openMP scalability we have:

````
### generating random playground
export OMP_NUM_THREADS=64
mpirun -np 2 --map-by socket parallel_gol.x -i -m $mat_x_size -k $mat_y_size

for n_threads in $(seq 1 1 64)
do 

    ### running the evolution
    export OMP_NUM_THREADS=$n_threads
    echo -n "${n_threads}" >> $datafile
    mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
    mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
    mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
    echo >> $datafile

    echo
    echo -----------
    echo
done
````

where we first generate a random playgorund using all available resources, and then perform the measurement incrementing the number of cores per socket from 1 up to saturation.

For strong MPI scalability, instead, we have:

````
export OMP_NUM_THREADS=$n_threads

for mat_size in $(seq 10000 10000 30000)
do
    for count in $(seq 1 1 5)
    do

        ### generating random playground
        mpirun -np 4 -N 2 --map-by socket parallel_gol.x -i -m $mat_size -k $mat_size

        for n_procs in $(seq 1 1 4)
        do 

            ### running the evolution
            echo -n "${mat_size}x${mat_size}," >> $datafile
            echo -n "${n_procs}" >> $datafile
            mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
            mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
            mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
            echo >> $datafile

            echo
            echo -----------
            echo
        done
    done
done
````

In this case, given the small amount of computation we have to perform, we have time to do some statistics. In fact, each time measure with number of sockets from 1 to 4 is executed for three different matrix sizes and for five times, in order to have some statistics. Notice that each different measure is taken using a different random matrix, so that the average over all measures is more representative.

For weak MPI scalability we have:

````
export OMP_NUM_THREADS=$n_threads

for count in $(seq 1 1 5)
do 
    for n_procs in $(seq 1 1 4)
    do 
        ### generating random playground
        mat_x_size=$((unit_mat_size*n_procs))
        mpirun -np 4 -N 2 --map-by socket parallel_gol.x -i -m $mat_x_size -k $unit_mat_size

        ### running the evolution
        echo -n "${mat_x_size}x${unit_mat_size}," >> $datafile
        echo -n "${n_procs}" >> $datafile
        mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
        mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
        mpirun -np $n_procs -N 2 --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
        echo >> $datafile
        
        echo
        echo -----------
        echo
    done
done
````

where again we did some statistics on each measure generating a new matrix at each iteration.

Finally, we unload all modules and clean from executables and PGM files:

````
echo RELEASING LOADED MODULES AND CLEANING FROM EXECUTABLES AND IMAGES...
cd ../..
make clean_all data_folder=$datafolder
module purge
cd $datafolder
````


## How to actually run jobs

**Note**: this job files are written to be run on facilities using SLURM as the resource manager, in particular the requested resources are compatible with ORFEO (cluster hosted at Area Science Park (Trieste)).

Assuming you have already cloned the repository, to reproduce on ORFEO some of the results here exposed, you can follow these simple steps:
1. Navigate to the directory corresponding to the nodes partition you are interested to test on (either `EPYC/` or `THIN/`)
2. Navigate to the directory corresponding to the kind of scalability you want to test (either `openMP_scal/`, `strong_MPI_scal/` or `weak_MPI_scal/`)
3. (Optional) change the commented lines in `parallel_gol` rule in `job.sh` if you want to compile parallel GOL using `parallel_gol_unique.c`
3. Call `sbatch job.sh` from inside the directory

### Drawing graphs

To better analyse the results it is useful to put data into a chart. To do that, after having collected data, it is sufficient to run `analysis.ipynb` on any machine. The code will produce a series of graphs each one displaying the speedup (or the inverse speedup) for a certain scalability and a certain kind of evolution.

**Note**: `analysis.ipynb` is written to work with this specific organization of the directory; if you decide to rearrange it in a different way, you will have to change `analysis.ipynb` as well.

### Changing parameters

Of course, there are a lot of parameters that you can change in the job files, for example: number of generations, thread affinity policy, matrix size, number of iterations for statistics, etc...

Obviously the time needed for execution can increase a lot changing even just one parameter. The combinations of parameters presented here are assured to run in a time smaller than two hours on ORFEO.

### Running on other clusters

To run these jobs on a cluster different from ORFEO (given that it has SLURM as the resource manager), you will have to make a couple of changes to `job.sh`. In particular you will have to change the resource requests (i.e. the first block of instructions) according to the partitions, the number of cores per node available and the maximum time allowed.

Also the module loading/unloading parts will probably need to be changed, depending on the module system on that cluster.


## Results

Here we just briefly expose the data we got. For their analysis we invite you to read [Report.pdf](../Report.pdf) in this directory's parent directory.

To make it easier to consult data, here you can find a table with direct access to CSV files:

| node | scalability | file |
| ---- | ----------- | ---- |
| EPYC | openMP      | [1](EPYC/openMP_scal/data.csv) |
| EPYC | strong MPI  | [2](EPYC/strong_MPI_scal/data.csv) |
| EPYC | weak MPI    | [3](EPYC/weak_MPI_scal/data.csv) |
| THIN | openMP      | [4](THIN/openMP_scal/data.csv) |
| THIN | strong MPI  | [5](THIN/strong_MPI_scal/data.csv) |
| THIN | weak MPI    | [6](THIN/weak_MPI_scal/data.csv) |




[link1]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise1/Assignment_exercise1.pdf
[link2]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise1/README.md
[link3]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise1/read_write_pgm_image.c
