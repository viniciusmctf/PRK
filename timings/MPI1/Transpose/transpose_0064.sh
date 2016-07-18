#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 64
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0064
#SBATCH -o transpose_mpi_0064.out

# Run 25 iterations
date
srun -n 1536 ../../../MPI1/Transpose/transpose 50 49152
date
