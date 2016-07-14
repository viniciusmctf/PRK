#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 32
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_32
#SBATCH -o transpose_mpi_32.out

# Run 25 iterations
date
srun -v -n 768 ../../../MPI1/Transpose/transpose 25 49152
date
