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
export OMP_PLACES=cores
export OMP_PROC_BIND=close   # PROVARE ANCHE SPREAD (MAGARI FUNZIONA MEGLIO PER FALSE SHARING)

echo COMPILING EXECUTABLES...
echo
datafolder=$(pwd)
cd ../..
make parallel_gol data_folder=$datafolder
cd $datafolder


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


### setting variables for executables and csv file
node=EPYC
scal=openMP
mat_x_size=10000
mat_y_size=10000
n_gen=5
n_procs=2


echo CREATING/OVERWRITING CSV FILE...
echo
datafile=$datafolder/data.csv
echo "#,,," > ${datafile}
echo "#node:,${node},," >> $datafile
echo "#scalability:,${scal},," >> $datafile
echo "#performance_measure:,time(s),," >> $datafile
echo "#,,," >> $datafile
echo "#playground_x_size:,${mat_x_size},," >> $datafile
echo "#playground_y_size:,${mat_y_size},," >> $datafile
echo "#generations:,${n_gen},," >> $datafile
echo "#,,," >> $datafile
echo "#,,," >> $datafile
echo "threads,ordered,static,static_in_place" >> $datafile


echo PERFORMING MEASURES...
echo
for count in $(seq 1 1 5)
do

    ### generating random playground
    export OMP_NUM_THREADS=10
    mpirun -np 5 parallel_gol.x -i -m $mat_x_size -k $mat_y_size

    for n_threads in $(seq 1 1 64)
    do 

	### running the evolution
        export OMP_NUM_THREADS=$n_threads
        echo "${n_threads}" >> $datafile
        mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
        mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
        mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
 
        echo
        echo -----------
        echo
    done
done


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


echo RELEASING LOADED MODULES AND CLEANING FROM EXECUTABLES AND IMAGES...
cd ../..
make clean data_folder=$datafolder
Module purge
cd $datafolder
