#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="weak_MPI_scal"
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
alloc=close
export OMP_PLACES=cores
export OMP_PROC_BIND=$alloc

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
scal=weak_MPI
unit_mat_size=10000
n_gen=5
n_threads=64


echo CREATING/OVERWRITING CSV FILE...
echo
datafile=$datafolder/data.csv
echo "#,,,," > ${datafile}
echo "#node_part:,${node},,," >> $datafile
echo "#scalability:,${scal},,," >> $datafile
echo "#performance_measure:,time(s),,," >> $datafile
echo "#threads_affinity_policy:,${alloc},,," >> $datafile
echo "#generations:,${n_gen},,," >> $datafile
echo "#starting_mat_size,${unit_mat_size}x${unit_mat_size},,," >> $datafile
echo "#sockets:,4,,,"
echo "#threads_per_socket:,${n_threads},,," >> $datafile
echo "#,,,," >> $datafile
echo "#,,,," >> $datafile
echo "mat_size,processes,ordered,static,static_in_place" >> $datafile


echo PERFORMING MEASURES...
echo

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


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


echo RELEASING LOADED MODULES AND CLEANING FROM EXECUTABLES AND IMAGES...
cd ../..
make clean_all data_folder=$datafolder
module purge
cd $datafolder
