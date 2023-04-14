# Exercise 2

For a quick overview of the aim of this exercise see `README.md` in this directory's parent directory.

**NOTE**: we remind that by ***configuration*** we mean a given set of conditions, for instance: "MKL library, single point
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

Each subdirectory of `EPYC/` and `THIN/` contains the exact same files:

- `job.sh`: a bash script used to compile `gemm.c` in the desired configuration, run it and gather results on performance; the script is made to be ran as a SLURM sbatch job on ORFEO
- `summary.out`: a file that is produced each time job.sh is run, and that can be used to check whether the job was ran successfully until the end
- `mkl_f.csv`: a CSV file containing results for MKL in single point precision
- `oblas_f.csv`: a CSV file containing results for openBLAS in single point precision
- `blis_f.csv`: a CSV file containing results for BLIS in single point precision
- `mkl_d.csv`: a CSV file containing results for MKL in double point precision
- `oblas_d.csv`: a CSV file containing results for openBLAS in double point precision
- `blis_d.csv`: a CSV file containing results for BLIS in double point precision


## Job files

All job files have a very similar structure.

The first block of istructions is ignored by bash and constitutes the resource request addressed to SLURM. For example let's look at `cores_w_close_cores/job.sh`:

````
#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="cores_scal"
#SBATCH --get-user-env
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
3. compile `gemm.c` in the current directory for all libraries and for both single and double point precision, passing the current directory as the one in which we want the CSV files to be saved

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

The following block (until the two separating lines) is exactly the same for all `job.sh` files; the only difference is that when the number of cores is fixed `export OMP_NUM_THREADS=$ncores` is put in the previous block of instructions (`OMP_NUM_THREADS` is an enviroment variables used to set the number of openMP threads). For instance:

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

In this block the first six subblocks are there to create (or ovewrite, if already present in the folder) the six CSV files to store data. The following for loop iterates over the numbers of cores (for fixed matrix size) or over the matrix sizes (for fixed number of cores), while the function of the inner for loop is to repeat the measurement five times in order to have a little bit of statistic. Inside the inner for loop each two lines are there to actually call the `gemm` function and to write the measures of preformance to the CSV files.

Note that all these lines (both for file overwriting and for measurements) are commented, and it's left to the user to uncomment the needed ones. For details see [the following section](#ref1).

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

### How to actually run jobs

PARLARE DI BLIS PATH E DI UNCOMMENTING LINES (E DI LIMITE DI TEMPO)
