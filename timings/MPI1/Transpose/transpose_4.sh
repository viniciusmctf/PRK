#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 4
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_4
#SBATCH -o transpose_mpi_4.out

# Run 25 iterations
date
srun -v -n 96 ../../../MPI1/Transpose/transpose 25 49152
date
