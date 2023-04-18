# Exercise 2

For a quick overview of the aim of this exercise see [`README.md`](../README.md) in this directory's parent directory.

**NOTE**: by ***configuration*** we mean a given set of conditions, for instance: "MKL library, single point
precision, EPYC node, fixed number of cores, spread-cores threads affinity policy" is a possible configuration.


## What you will find in this directory

The current directory contains:

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

We were already given a working code called `gemm.c` (actually, in the original repository of the course there is a mistake and the file is called `dgemm.c` - see [here][link1]) that could be used to call the function with single or double point precision from any of the three libraries, and to measure its performance in total time and *GFLOPS* (giga-floating point operations per second). The version you can find in this directory is a slightly modified one, in which it is possible to write the results to a CSV file (a different one depending on the library and the precision used).

We were also given a Makefile (see [here][link2]) to compile `gemm.c` with different libraries and precisions. Also in this case we slightly modified the code to write the results to file and to compile the executables in the right folder.

Each subdirectory of `EPYC/` and `THIN/` contains the same set of files:

- `job.sh`: a bash script used to compile `gemm.c` in the desired configuration, run it and gather results on performance; the script is made to be run as a SLURM sbatch job on ORFEO
- `summary.out`: a file that is produced each time `job.sh` is run, and that can be used to check whether the job was run successfully until the end
- `mkl_f.csv`: a CSV file containing results for MKL in single point precision
- `oblas_f.csv`: a CSV file containing results for openBLAS in single point precision
- `blis_f.csv`: a CSV file containing results for BLIS in single point precision
- `mkl_d.csv`: a CSV file containing results for MKL in double point precision
- `oblas_d.csv`: a CSV file containing results for openBLAS in double point precision
- `blis_d.csv`: a CSV file containing results for BLIS in double point precision


### Job files

All job files have a very similar structure.

The first block of istructions is ignored by bash and constitutes the resource request addressed to SLURM. For example let's look at `cores_w_close_cores/job.sh`:

````
#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="cores_scal"
#SBATCH --partition=EPYC
#SBATCH -N 1
#SBATCH -n 128
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output="summary.out"
````

Here, for instance, we are requesting an entire (all of its 128 cores) EPYC node (`-N 1`, `-n 128`, `--partition=EPYC`) for a maximum amount of time of two hours (`--time=02:00:00`). We are also asking SLURM to save the outputs of the job in a file called `summary.out`. This file can later be read to check that the program was actually run correctly with the right matrix size and for the requested amount of times.

In the second part (very similar for all `job.sh` files) we (in order):
1. load the needed modules: the architecture of the target machine (AMD for EPYC nodes and Intel for THIN nodes), MKL and openBLAS libraries
2. export the path for the BLIS library (**NOTE:** this has to be changed according to your own installation of the library)
3. compile `gemm.c` saving the executables in the current directory for all libraries and for both single and double point precision, passing the current directory as the one in which we want the CSV files to be saved

Again for example `cores_w_close_cores/job.sh` (the messages printed to screen via `echo` are there only to make `summary.out` readable):

````
echo LOADING NEEDED MODULES...
echo
module load architecture/AMD
module load mkl
module load openBLAS/0.3.21-omp
export LD_LIBRARY_PATH=/u/dssc/ttarch00/myblis/lib:$LD_LIBRARY_PATH
echo
echo COMPILING PROGRAMS FOR OPENBLAS, MKL AND BLIS ON TARGET MACHINE...
echo
datafolder=$(pwd)
cd ../..
make float data=$datafolder
make double data=$datafolder
cd $datafolder
````

Then we set other parameters, as size/number of cores if we are in a fixed size/number of cores condition and threads affinity policy, for example:

````
size=10000
node=EPYC
alloc=close

### setting threads allocation policy
export OMP_PLACES=cores
export OMP_PROC_BIND=$alloc
````

Where `OMP_PLACES` and `OMP_PROC_BIND` are enviroment variables used to set the threads affinity policy.

The following block (until the two separating dashed lines) is exactly the same for all `job.sh` files; the only difference is that when the number of cores is fixed `export OMP_NUM_THREADS=$ncores` is put in the previous block of instructions (`OMP_NUM_THREADS` is an enviroment variables used to set the number of openMP threads). For instance:

````
### overwriting old datafiles and setting up new ones
#echo "#,,," > mkl_f.csv
#echo "#node:,${node},," >> mkl_f.csv
#echo "#library:,MKL,," >> mkl_f.csv
#echo "#precision:,float,," >> mkl_f.csv
#echo "#allocation:,${alloc},," >> mkl_f.csv
#echo "#,,," >> mkl_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> mkl_f.csv

#echo "#,,," > oblas_f.csv
#echo "#node:,${node},," >> oblas_f.csv
#echo "#library:,openBLAS,," >> oblas_f.csv
#echo "#precision:,float,," >> oblas_f.csv
#echo "#allocation:,${alloc},," >> oblas_f.csv
#echo "#,,," >> oblas_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> oblas_f.csv

#echo "#,,," > blis_f.csv
#echo "#node:,${node},," >> blis_f.csv
#echo "#library:,BLIS,," >> blis_f.csv
#echo "#precision:,float,," >> blis_f.csv
#echo "#allocation:,${alloc},," >> blis_f.csv
#echo "#,,," >> blis_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> blis_f.csv

#echo "#,,," > mkl_d.csv
#echo "#node:,${node},," >> mkl_d.csv
#echo "#library:,MKL,," >> mkl_d.csv
#echo "#precision:,double,," >> mkl_d.csv
#echo "#allocation:,${alloc},," >> mkl_d.csv
#echo "#,,," >> mkl_d.csv
#echo "cores,mat_size,time(s),GFLOPS" >> mkl_d.csv

#echo "#,,," > oblas_d.csv
#echo "#node:,${node},," >> oblas_d.csv
#echo "#library:,openBLAS,," >> oblas_d.csv
#echo "#precision:,double,," >> oblas_d.csv
#echo "#allocation:,${alloc},," >> oblas_d.csv
#echo "#,,," >> oblas_d.csv
#echo "cores,mat_size,time(s),GFLOPS" >> oblas_d.csv

#echo "#,,," > blis_d.csv
#echo "#node:,${node},," >> blis_d.csv
#echo "#library:,BLIS,," >> blis_d.csv
#echo "#precision:,double,," >> blis_d.csv
#echo "#allocation:,${alloc},," >> blis_d.csv
#echo "#,,," >> blis_d.csv
#echo "cores,mat_size,time(s),GFLOPS" >> blis_d.csv

### performing measures
for ncores in $(seq 1 1 128)
do
	export OMP_NUM_THREADS=$ncores
	
	for count in $(seq 1 1 5)
	do
		#echo -n "${ncores}," >> mkl_f.csv
		#./gemm_mkl_f.x $size $size $size
		#echo -n "${ncores},"  >> oblas_f.csv
		#./gemm_oblas_f.x $size $size $size
		#echo -n "${ncores},"  >> blis_f.csv
		#./gemm_blis_f.x $size $size $size
		#echo -n "${ncores}," >> mkl_d.csv
		#./gemm_mkl_d.x $size $size $size
		#echo -n "${ncores},"  >> oblas_d.csv
		#./gemm_oblas_d.x $size $size $size
		#echo -n "${ncores},"  >> blis_d.csv
		#./gemm_blis_d.x $size $size $size
		echo
		echo -----------
		echo
	done
done
````

In this block the first six subblocks are there to create (or ovewrite, if already present in the folder) the six CSV files to store data. The following for loop iterates over the numbers of cores (for fixed matrix size) or over the matrix sizes (for fixed number of cores), while the function of the inner for loop is to repeat the measurement five times in order to have a little bit of statistic. Inside the inner for loop each two lines are there to actually call the executables and to write the preformance measure to the CSV files.

Note that all these lines (both for file overwriting and for measurements) are commented, and it's left to the user to uncomment the needed ones. To know how see [the following section](#ref1).

The last block of instructions simply cleans the current directory from executables and releases the previously loaded modules. It is the same for all job files:

````
echo REMOVING COMPILED PROGRAMS AND UNLOADING MODULES...
echo
cd ../..
make clean data=$datafolder
module purge
````


<a name="ref1">
</a>

## How to actually run jobs

**Note**: this job files are written to be run on facilities using SLURM as the resource manager, in particular the requested resources are compatible with ORFEO (cluster hosted at Area Science Park (Trieste)).

Let's suppose you have already cloned this repository and that you have already installed the BLIS library (if you do not know how to do that, you can find a [simple tutorial][link3] in the course material).

To reproduce on ORFEO some of the results here exposed, you can follow these steps:

1. change the BLIS library path in `Makefile` (i.e. change the variable `BLISROOT`'s value) to the one in which you installed the BLIS library
2. navigate to the folder corresponding to the nodes partition you are interested to test on (either `EPYC/` or `THIN/`)
3. navigate to the folder corresponding to the parameter you want to vary (either the matrix size or the number of cores) and the threads affinity policy you want to use (either close or spread cores)
4. modify `job.sh` uncommenting the lines corresponding to the library and the precision you want to use (i.e. the four lines before the loop needed to overwrite the CSV file and the two lines inside the inner loop needed to run the executable and save results into the CSV)
5. call `sbatch job.sh` from inside the directory contaning `job.sh`

To clarify point 4., let's see an example in which we want to gather data for openBLAS library with double point precision:

````
### overwriting old datafiles and setting up new ones
#echo "#,,," > mkl_f.csv
#echo "#node:,${node},," >> mkl_f.csv
#echo "#library:,MKL,," >> mkl_f.csv
#echo "#precision:,float,," >> mkl_f.csv
#echo "#allocation:,${alloc},," >> mkl_f.csv
#echo "#,,," >> mkl_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> mkl_f.csv

#echo "#,,," > oblas_f.csv
#echo "#node:,${node},," >> oblas_f.csv
#echo "#library:,openBLAS,," >> oblas_f.csv
#echo "#precision:,float,," >> oblas_f.csv
#echo "#allocation:,${alloc},," >> oblas_f.csv
#echo "#,,," >> oblas_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> oblas_f.csv

#echo "#,,," > blis_f.csv
#echo "#node:,${node},," >> blis_f.csv
#echo "#library:,BLIS,," >> blis_f.csv
#echo "#precision:,float,," >> blis_f.csv
#echo "#allocation:,${alloc},," >> blis_f.csv
#echo "#,,," >> blis_f.csv
#echo "cores,mat_size,time(s),GFLOPS" >> blis_f.csv

#echo "#,,," > mkl_d.csv
#echo "#node:,${node},," >> mkl_d.csv
#echo "#library:,MKL,," >> mkl_d.csv
#echo "#precision:,double,," >> mkl_d.csv
#echo "#allocation:,${alloc},," >> mkl_d.csv
#echo "#,,," >> mkl_d.csv
#echo "cores,mat_size,time(s),GFLOPS" >> mkl_d.csv

echo "#,,," > oblas_d.csv
echo "#node:,${node},," >> oblas_d.csv
echo "#library:,openBLAS,," >> oblas_d.csv
echo "#precision:,double,," >> oblas_d.csv
echo "#allocation:,${alloc},," >> oblas_d.csv
echo "#,,," >> oblas_d.csv
echo "cores,mat_size,time(s),GFLOPS" >> oblas_d.csv

#echo "#,,," > blis_d.csv
#echo "#node:,${node},," >> blis_d.csv
#echo "#library:,BLIS,," >> blis_d.csv
#echo "#precision:,double,," >> blis_d.csv
#echo "#allocation:,${alloc},," >> blis_d.csv
#echo "#,,," >> blis_d.csv
#echo "cores,mat_size,time(s),GFLOPS" >> blis_d.csv

### performing measures
for ncores in $(seq 1 1 128)
do
	export OMP_NUM_THREADS=$ncores
	
	for count in $(seq 1 1 5)
	do
		#echo -n "${ncores}," >> mkl_f.csv
		#./gemm_mkl_f.x $size $size $size
		#echo -n "${ncores},"  >> oblas_f.csv
		#./gemm_oblas_f.x $size $size $size
		#echo -n "${ncores},"  >> blis_f.csv
		#./gemm_blis_f.x $size $size $size
		#echo -n "${ncores}," >> mkl_d.csv
		#./gemm_mkl_d.x $size $size $size
		echo -n "${ncores},"  >> oblas_d.csv
		./gemm_oblas_d.x $size $size $size
		#echo -n "${ncores},"  >> blis_d.csv
		#./gemm_blis_d.x $size $size $size
		echo
		echo -----------
		echo
	done
done
````

### Running more configurations at the same time

Of course we could think of uncommenting lines for more than one configuration at the same time, but that is feasible only if we have a time limit large enough to do all the computation (to change the time limit you can just change the related command in the first block of instructions). In general, the time needed to run one of the configurations depends on several factors: which is the varying parameter, which nodes partition you are running one, what precision you are using, etc...

With a time limit of at least two hours, we can guarantee jobs to be completed **only** in the case in which only one configuration at a time is run.


### Running on other clusters

To run these jobs on a cluster different from ORFEO (**given that it has SLURM as the resource manager**), you will have to make a couple of changes to `job.sh`. In particular you will have to change the resource requests (i.e. the first block of instructions) according to the partitions and the number of cores per node available and the maximum time allowed.

Also the module loading/unloading parts will probably need to be changed, depending on the modules organization on the cluster.


## Results

Here we just briefly expose the data we got. For their analysis we invite you to read `report.pdf` (**INSERIRE RIFERIMENTO AL REPORT**) in this directory's parent directory.

To make it easier to consult data, here you can find a table with direct access to all CSV files (if a cell is empty, then its content is the same as the last non empty cell above in the same column - we believe the table to be more readable in this way):

| node | varying parameter | precision | threads affinity | library | file |
| ---- | ----------------- | ---------------- | --------- | ------- | ---- |
| EPYC | number of cores | float | close cores | MKL | [1](EPYC/cores_w_close_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [2](EPYC/cores_w_close_cores/oblas_f.csv) |
| | | |                                        | BLIS | [3](EPYC/cores_w_close_cores/blis_f.csv) |
| | |                           | spread cores | MKL |  [4](EPYC/cores_w_spread_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [5](EPYC/cores_w_spread_cores/oblas_f.csv) |
| | | |                                        | BLIS | [6](EPYC/cores_w_spread_cores/blis_f.csv) |
| |                     | double | close cores | MKL | [7](EPYC/cores_w_close_cores/mkl_d.csv) | 
| | | |                                        | openBLAS | [8](EPYC/cores_w_close_cores/oblas_d.csv) |
| | | |                                        | BLIS | [9](EPYC/cores_w_close_cores/blis_d.csv) |
| | |                           | spread cores | MKL | [10](EPYC/cores_w_spread_cores/mkl_d.csv) |
| | | |                                        | openBLAS| [11](EPYC/cores_w_spread_cores/oblas_d.csv) |
| | | |                                        | BLIS | [12](EPYC/cores_w_spread_cores/blis_d.csv) |
|          | matrix size | float | close cores | MKL | [13](EPYC/size_w_close_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [14](EPYC/size_w_close_cores/oblas_f.csv) |
| | | |                                        | BLIS | [15](EPYC/size_w_close_cores/blis_f.csv) |
| | |                           | spread cores | MKL | [16](EPYC/size_w_spread_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [17](EPYC/size_w_spread_cores/oblas_f.csv) |
| | | |                                        | BLIS | [18](EPYC/size_w_spread_cores/blis_f.csv) |
| |                     | double | close cores | MKL | [19](EPYC/size_w_close_cores/mkl_d.csv) |
| | | |                                        | openBLAS | [20](EPYC/size_w_close_cores/oblas_d.csv) |
| | | |                                        | BLIS | [21](EPYC/size_w_close_cores/blis_d.csv) |
| | |                           | spread cores | MKL | [22](EPYC/size_w_spread_cores/mkl_d.csv) |
| | | |                                        | openBLAS | [23](EPYC/size_w_spread_cores/oblas_d.csv) |
| | | |                                        | BLIS | [24](EPYC/size_w_spread_cores/blis_d.csv) |
| THIN | number of cores | float | close cores | MKL | [1](THIN/cores_w_close_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [2](THIN/cores_w_close_cores/oblas_f.csv) |
| | | |                                        | BLIS | [3](THIN/cores_w_close_cores/blis_f.csv) |
| | |                           | spread cores | MKL |  [4](THIN/cores_w_spread_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [5](THIN/cores_w_spread_cores/oblas_f.csv) |
| | | |                                        | BLIS | [6](THIN/cores_w_spread_cores/blis_f.csv) |
| |                     | double | close cores | MKL | [7](THIN/cores_w_close_cores/mkl_d.csv) | 
| | | |                                        | openBLAS | [8](THIN/cores_w_close_cores/oblas_d.csv) |
| | | |                                        | BLIS | [9](THIN/cores_w_close_cores/blis_d.csv) |
| | |                           | spread cores | MKL | [10](THIN/cores_w_spread_cores/mkl_d.csv) |
| | | |                                        | openBLAS| [11](THIN/cores_w_spread_cores/oblas_d.csv) |
| | | |                                        | BLIS | [12](THIN/cores_w_spread_cores/blis_d.csv) |
|          | matrix size | float | close cores | MKL | [13](THIN/size_w_close_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [14](THIN/size_w_close_cores/oblas_f.csv) |
| | | |                                        | BLIS | [15](THIN/size_w_close_cores/blis_f.csv) |
| | |                           | spread cores | MKL | [16](THIN/size_w_spread_cores/mkl_f.csv) |
| | | |                                        | openBLAS | [17](THIN/size_w_spread_cores/oblas_f.csv) |
| | | |                                        | BLIS | [18](THIN/size_w_spread_cores/blis_f.csv) |
| |                     | double | close cores | MKL | [19](THIN/size_w_close_cores/mkl_d.csv) |
| | | |                                        | openBLAS | [20](THIN/size_w_close_cores/oblas_d.csv) |
| | | |                                        | BLIS | [21](THIN/size_w_close_cores/blis_d.csv) |
| | |                           | spread cores | MKL | [22](THIN/size_w_spread_cores/mkl_d.csv) |
| | | |                                        | openBLAS | [23](THIN/size_w_spread_cores/oblas_d.csv) |
| | | |                                        | BLIS | [24](THIN/size_w_spread_cores/blis_d.csv) |





[link1]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise2/dgemm.c
[link2]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise2/Makefile
[link3]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise2/README.md
