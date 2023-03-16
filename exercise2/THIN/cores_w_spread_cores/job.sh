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

export OMP_PLACES=cores
export OMP_PROC_BIND=$topo


### overwriting old datafiles and setting up new ones
###echo "node:        ${node}" > mkl_f.out
###echo "library:     MKL" >> mkl_f.out
###echo "precision:   float" >> mkl_f.out
###echo >> mkl_f.out
###echo "#cores     mat_size     time        GFLOPS" >> mkl_f.out

###echo "node:        ${node}" > oblas_f.out
###echo "library:     openBLAS" >> oblas_f.out
###echo "precision:   float" >> oblas_f.out
###echo >> oblas_f.out
###echo "#cores     mat_size     time        GFLOPS" >> oblas_f.out

echo "node:        ${node}" > blis_f.out
echo "library:     BLIS" >> blis_f.out
echo "precision:   float" >> blis_f.out
echo >> blis_f.out
echo "#cores     mat_size     time        GFLOPS" >> blis_f.out

echo "node:        ${node}" > mkl_d.out
echo "library:     MKL" >> mkl_d.out
echo "precision:   double" >> mkl_d.out
echo >> mkl_d.out
echo "#cores     mat_size     time        GFLOPS" >> mkl_d.out

echo "node:        ${node}" > oblas_d.out
echo "library:     openBLAS" >> oblas_d.out
echo "precision:   double" >> oblas_d.out
echo >> oblas_d.out
echo "#cores     mat_size     time        GFLOPS" >> oblas_d.out

echo "node:        ${node}" > blis_d.out
echo "library:  BLIS" >> blis_d.out
echo "precision:   double" >> blis_d.out
echo >> blis_d.out
echo "#cores     mat_size     time        GFLOPS" >> blis_d.out

for ncores in $(seq 2 2 128)
do
	export MKL_NUM_THREADS=$ncores
	export OPENBLAS_NUM_THREADS=$ncores
	export BLIS_NUM_THREADS=$ncores

	###echo -n "${ncores}       " >> mkl_f.out
	###./gemm_mkl_f.x $size $size $size
	###echo -n "${ncores}       "  >> oblas_f.out
	###./gemm_oblas_f.x $size $size $size
	echo -n "${ncores}       "  >> blis_f.out
	./gemm_blis_f.x $size $size $size
	###echo -n "${ncores}       " >> mkl_d.out
	###./gemm_mkl_d.x $size $size $size
	###echo -n "${ncores}       "  >> oblas_d.out
	###./gemm_oblas_d.x $size $size $size
	###echo -n "${ncores}       "  >> blis_d.out
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
