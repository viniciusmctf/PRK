#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 64
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_64
#SBATCH -o transpose_mpi_64.out

# Run 25 iterations
date
srun -v -n 1536 ../../../MPI1/Transpose/transpose 25 49152
date
