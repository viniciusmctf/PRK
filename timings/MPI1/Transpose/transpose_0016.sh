#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 16
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0016
#SBATCH -o transpose_mpi_0016.out

# Run 25 iterations
date
srun -v -n 384 ../../../MPI1/Transpose/transpose 25 49152
date
