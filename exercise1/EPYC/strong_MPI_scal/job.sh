#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="strong_MPI_scal"
#SBATCH --partition=EPYC
#SBATCH -N 2
#SBATCH -n 256
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
scal=strong_MPI
n_gen=10
n_threads=64


echo CREATING/OVERWRITING CSV FILE...
echo
datafile=$datafolder/data.csv
echo "#,,,," > ${datafile}
echo "#node:,${node},,," >> $datafile
echo "#scalability:,${scal},,," >> $datafile
echo "#performance_measure:,time(s),,," >> $datafile
echo "#,,,," >> $datafile
echo "#generations:,${n_gen},,," >> $datafile
echo "#threads_per_socket:,${n_threads},,," >> $datafile
echo "#,,,," >> $datafile
echo "#,,,," >> $datafile
echo "mat_size,processes,ordered,static,static_in_place" >> $datafile


echo PERFORMING MEASURES...
echo
for mat_size in $(seq 10000 10000 50000)
do
    for count in $(seq 1 1 5)
    do

        ### generating random playground
        export OMP_NUM_THREADS=20
        mpirun -np 5 --map-by socket parallel_gol.x -i -m $mat_size -k $mat_size

        for n_procs in $(seq 1 1 4)
        do 

            ### running the evolution
            export OMP_NUM_THREADS=$n_threads
            echo "${mat_size}x${mat_size}" >> $datafile
            echo -n "${n_procs}" >> $datafile
            mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
            mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
            mpirun -np $n_procs --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
            
            echo
            echo -----------
            echo
        done
    done
done


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


echo RELEASING LOADED MODULES AND CLEANING FROM EXECUTABLES AND IMAGES...
cd ../..
make clean data=$datafolder
Module purge
cd $datafolder
