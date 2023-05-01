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
cd ../..
datafolder=$(pwd)
srun -n 1 make clean_images
srun -n 1 make parallel_gol data_folder=datafolder
cd datafolder


echo
echo -------------------------------------------------------------
echo -------------------------------------------------------------
echo


### setting variables for executables and csv file
node=EPYC
scal=openMP
mat_x_size=20000
mat_y_size=20000
n_gen=50
procs=2


echo CREATING/OVERWRITING CSV FILE...
echo
echo "#,,," > data.csv
echo "#node:,${node},," >> data.csv
echo "#scalability:,${scal},," >> data.csv
echo "#performance_measure:,time(s),," >> data.csv
echo "#,,," >> data.csv
echo "#playground_x_size:,${mat_x_size},," >> data.csv
echo "#playground_y_size:,${mat_y_size},," >> data.csv
echo "#generations:,${n_gen},," >> data.csv
echo "#,,," >> data.csv
echo "#,,," >> data.csv
echo "threads,ordered,static,static_in_place" >> data.csv


echo PERFORMING MEASURES...
echo
for n_threads in $(seq 1 1 64)
do
    for count in $(seq 1 1 10)
    do 
        ### generating random playgrounds
        export OMP_NUM_THREADS=64
        mpirun -np $procs --map-by socket parallel_gol.x -i -m $mat_x_size -k $mat_y_size

        ### running the evolution
        export OMP_NUM_THREADS=$n_threads
        echo -n "${n_threads}" >> data.csv
        mpirun -np $procs --map-by socket parallel_gol.x -r -e 0 -n $n_gen -s 0
        mpirun -np $procs --map-by socket parallel_gol.x -r -e 1 -n $n_gen -s 0
        mpirun -np $procs --map-by socket parallel_gol.x -r -e 2 -n $n_gen -s 0
        
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
make clean data=datafolder
Module purge
cd datafolder
