#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0001
#SBATCH -o transpose_mpi_0001.out

# Run 25 iterations
date
srun -v -n 24 ../../../MPI1/Transpose/transpose 25 49152
date
