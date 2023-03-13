#!/bin/bash 
#SBATCH --partition=EPYC
#SBATCH --job-name=size_scalability
#SBATCH -N 1
#SBATCH -n 64
#SBATCH --time=1:00:00
#SBATCH --output=report.out


echo LOADING NEEDED MODULES...
echo
module load architecture/AMD
module load mkl
module load openBLAS/0.3.21-omp
export LD_LIBRARY_PATH=/u/dssc/ttarch00/myblis/lib:$LD_LIBRARY_PATH
echo
echo COMPILING PROGRAMS FOR OPENBLAS, MKL AND BLIS ON TARGET MACHINE...
echo
srun -n 1 make setup
srun -n 1 make float
srun -n 1 make double

echo
echo --------------------------------------------------------------------------------------------------------
echo --------------------------------------------------------------------------------------------------------
echo

echo GATHERING RESULTS...
echo
srun -n 1 ./setup.x    ### removing old datafiles and setting up new ones
for size in $(seq 5000 5000 20000)
do
	srun -n 1 --cpus-per-task=16 ./gemm_mkl_f.x $size $size $size
	###srun -n 1 --cpus-per-task=16 ./gemm_oblas_f.x $size $size $size
	srun -n 1 --cpus-per-task=16 ./gemm_blis_f.x $size $size $size
	###srun -n 1 --cpus-per-task=64 ./gemm_mkl_d.x $size $size $size
	###srun -n 1 --cpus-per-task=64 ./gemm_oblas_d.x $size $size $size
	###srun -n 1 --cpus-per-task=64 ./gemm_blis_d.x $size $size $size
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
srun -n 1 make clean
module purge
