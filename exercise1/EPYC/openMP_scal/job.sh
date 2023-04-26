#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="openMP_scal"
#SBATCH --partition=EPYC
#SBATCH -N 1
#SBATCH -n 128
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output="summary.out"




module load architecture/AMD
module load # METTERE OPENMPI


datafolder=$(pwd)


export OMP_PLACES=cores
export OMP_PROC_BIND=close   # PROVARE ANCHE SPREAD (MAGARI FUNZIONA MEGLIO PER FALSE SHARING)


cd ../..

make parallel_gol data=datafolder

cd datafolder


# INIZIALIZZARE/SOVRASCRIVERE CSV FILE


# FARE UN PO' DI STATISTICA


procs = #NUMERO DI SOCKET


for nthreads in $(seq 1 1 #NUMERO DI CORE SU OGNI SOCKET)
do
    for count in $(seq 1 1 10)
    do 
        export OMP_NUM_THREADS=#NUMERO DI CORE SU OGNI SOCKET
        mpirun -np procs --map-by socket 

        export OMP_NUM_THREADS=nthreads
        mpirun -np procs --map-by socket parallel_gol.x # AGGIUNGERE OPZIONI VARIE
    done
done


cd ../..

make clean data=datafolder
