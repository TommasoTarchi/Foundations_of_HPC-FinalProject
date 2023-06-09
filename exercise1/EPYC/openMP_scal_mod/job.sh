#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="openMP_scal"
#SBATCH --partition=EPYC
#SBATCH -N 1
#SBATCH -n 128
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output="summary.out"



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
make parallel_gol_mod data_folder=$datafolder
cd $datafolder


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


### setting variables for executables and csv file
node=EPYC
scal=openMP
mat_x_size=25000
mat_y_size=25000
n_gen=5
n_procs=2


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


echo PERFORMING MEASURES...
echo

### generating random playground
export OMP_NUM_THREADS=64
mpirun -np 2 --map-by socket parallel_gol_mod.x -i -m $mat_x_size -k $mat_y_size

for n_threads in $(seq 1 1 64)
do 

    ### running the evolution
    export OMP_NUM_THREADS=$n_threads
    echo -n "${n_threads}" >> $datafile
    mpirun -np $n_procs --map-by socket parallel_gol_mod.x -r -e 0 -n $n_gen -s 0
    mpirun -np $n_procs --map-by socket parallel_gol_mod.x -r -e 1 -n $n_gen -s 0
    mpirun -np $n_procs --map-by socket parallel_gol_mod.x -r -e 2 -n $n_gen -s 0
    echo >> $datafile

    echo
    echo -----------
    echo
done


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


echo RELEASING LOADED MODULES AND CLEANING FROM EXECUTABLES AND IMAGES...
cd ../..
make clean_all data_folder=$datafolder
module purge
cd $datafolder
