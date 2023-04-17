# Final project FHPC course 2022/2023

This is the final project for the Foundations of High Performance Computing course, part of the MSc in Data Science and Scientific Computing at University of Trieste (all related material can be found at https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/tree/main/Assignment).


## What you will find in this repository

- This markdown file: a brief overview of the content of this repository and the exercises we were assigned
- `report.pdf`: a detailed report of everything we did for this project
- `exercise1/`: a directory containing all files related to the first exercise
- `exercise2/`: a directory containing all files related to the second exercise

For an overview of the content of `exercise1/` and `exercise2/` (and to know how to reproduce the results) see the related `README.md` files.


## What were the exercises about


### Exercise 1

In this assignment we were asked to implement a parallel version of the famous Conway's Game of Life ("GOL") and to measure its scalability in different conditions. The parallelism had to be a **hybrid MPI/openMP**, i.e. we had to work with a distributed memory approach and a shared memory approach on the same code.

In practice, we implemented an MPI version of GOL in which the workload was equally distributed among the processes; then, each MPI process would parallelize its own work spawning a number of openMP processes. To study the scalability we compiled and ran the program on **ORFEO** (the cluster hosted at *Area Science Park* (Trieste)).

For details about this assignment see [this document][link1] in the original course repository. 


### Exercise 2

In this second assignment we were asked to compare the performance of three HPC math libraries: **MKL**, **openBLAS** and **BLIS** (the last one had to be downloaded and compiled by the student on his own working area on ORFEO).

In particular, we had to compare the performance of a level 3 BLAS function called *gemm* on matrix-matrix multiplications, both for increasing matrix size (at fixed number of CPUs) and for incresing number of CPUs (at fixed matrix size), both on **EPYC** and **THIN** nodes of ORFEO, both for single and double point precision floating point numbers and with different threads allocation policies (we chose to use *close cores* and *spread cores*).

For details about this assignment see [this document][link2] in the original course repository.




[link1]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/exercise1/Assignment_exercise1.pdf
[link2]: https://github.com/Foundations-of-HPC/Foundations_of_HPC_2022/blob/main/Assignment/README.MD#exercise-2--comparing-mkl-openblas-and-blis-on-matrix-matrix-multiplication
