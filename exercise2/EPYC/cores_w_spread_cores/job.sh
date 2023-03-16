#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="cores_scal"
#SBATCH --get-user-env
#SBATCH --partition=EPYC
#SBATCH -N 1
#SBATCH -n 128
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output="report.out"


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

echo
echo --------------------------------------------------------------------------------------------------------
echo --------------------------------------------------------------------------------------------------------
echo

echo GATHERING RESULTS...
echo
size=10000
node=EPYC
topo=spread

### setting cores' topology
export OMP_PLACES=cores
export OMP_PROC_BIND=$topo

### overwriting old datafiles and setting up new ones
echo "#node:        ${node}" > mkl_f.csv
echo "#library:     MKL" >> mkl_f.csv
echo "#precision:   float" >> mkl_f.csv
echo >> mkl_f.csv
echo "n_cores     mat_size     time        GFLOPS" >> mkl_f.csv

echo "#node:        ${node}" > oblas_f.csv
echo "#library:     openBLAS" >> oblas_f.csv
echo "#precision:   float" >> oblas_f.csv
echo >> oblas_f.csv
echo "cores     mat_size     time        GFLOPS" >> oblas_f.csv

echo "#node:        ${node}" > blis_f.csv
echo "#library:     BLIS" >> blis_f.csv
echo "#precision:   float" >> blis_f.csv
echo >> blis_f.csv
echo "cores     mat_size     time        GFLOPS" >> blis_f.csv

echo "#node:        ${node}" > mkl_d.csv
echo "#library:     MKL" >> mkl_d.csv
echo "#precision:   double" >> mkl_d.csv
echo >> mkl_d.csv
echo "cores     mat_size     time        GFLOPS" >> mkl_d.csv

echo "#node:        ${node}" > oblas_d.csv
echo "#library:     openBLAS" >> oblas_d.csv
echo "#precision:   double" >> oblas_d.csv
echo >> oblas_d.csv
echo "cores     mat_size     time        GFLOPS" >> oblas_d.csv

echo "#node:        ${node}" > blis_d.csv
echo "#library:     BLIS" >> blis_d.csv
echo "#precision:   double" >> blis_d.csv
echo >> blis_d.csv
echo "cores     mat_size     time        GFLOPS" >> blis_d.csv

### performing measures
for ncores in $(seq 2 2 128)
do
	export OMP_NUM_THREADS=$ncores

	###echo -n "${ncores}       " >> mkl_f.csv
	###./gemm_mkl_f.x $size $size $size
	###echo -n "${ncores}       "  >> oblas_f.csv
	###./gemm_oblas_f.x $size $size $size
	echo -n "${ncores}       "  >> blis_f.csv
	./gemm_blis_f.x $size $size $size
	###echo -n "${ncores}       " >> mkl_d.csv
	###./gemm_mkl_d.x $size $size $size
	###echo -n "${ncores}       "  >> oblas_d.csv
	###./gemm_oblas_d.x $size $size $size
	###echo -n "${ncores}       "  >> blis_d.csv
	###./gemm_blis_d.x $size $size $size
	echo
	echo -----------
	echo
done

echo
echo -------------------------------------------------------------------------------------------------------
echo -------------------------------------------------------------------------------------------------------
echo

echo REMOVING COMPILED PROGRAMS AND UNLOADING MODULES...
echo
cd ../..
make clean data=$datafolder
module purge
