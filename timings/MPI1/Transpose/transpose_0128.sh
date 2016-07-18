#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 128
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0128
#SBATCH -o transpose_mpi_0128.out

# Run 25 iterations
date
srun -n 3072 ../../../MPI1/Transpose/transpose 50 49152
date
